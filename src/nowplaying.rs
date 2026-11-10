//! The "now playing" popup: a headphone that bobs to the kick, the track,
//! its artist and licence, and a link to it. It slides in when a track
//! starts, stays while the pointer is on it, and slides away.

use crate::anim::{clamp01, ease_out_expo};
use crate::audio::{Sfx, Track};
use crate::credits;
use crate::gfx::text::{self, Align, Face};
use crate::gfx::{pal, Gfx};
use crate::i18n::tr;
use crate::ui::{self, Ui};
use macroquad::prelude::*;
use std::f32::consts::PI;

const IN: f32 = 0.5;
const HOLD: f32 = 7.0;
const OUT: f32 = 0.45;

pub struct NowPlaying {
    seq: u32,
    since: f32,
    track: Option<&'static Track>,
    /// the music stopped at this time: leave early
    stopped: Option<f32>,
}

impl NowPlaying {
    pub fn new() -> NowPlaying {
        NowPlaying { seq: 0, since: -100.0, track: None, stopped: None }
    }

    /// Feed it the audio's now-playing each frame.
    pub fn update(&mut self, np: Option<(u32, &'static Track)>, time: f32) {
        match np {
            Some((n, t)) if n != self.seq => {
                self.seq = n;
                self.track = Some(t);
                self.since = time;
                self.stopped = None;
            }
            Some(_) => self.stopped = None,
            None => {
                if self.stopped.is_none() {
                    self.stopped = Some(time);
                }
            }
        }
    }

    /// Bring it back for a while (the sound badge was pointed at).
    pub fn poke(&mut self, time: f32) {
        if self.track.is_some() && self.stopped.is_none() {
            let age = time - self.since;
            if age > IN + HOLD {
                // slide in again, or hold if it's still on its way out
                self.since = if age < IN + HOLD + OUT { time - IN - (IN + HOLD + OUT - age) * IN / OUT } else { time };
            } else if age > IN {
                self.since = time - IN;
            }
        }
    }

    /// Draw it with its top-left corner at `corner`.
    pub fn draw(&mut self, ui: &mut Ui, g: &Gfx, corner: Vec2, beat: f32) {
        let Some(tr_) = self.track else { return };
        let t = ui.time;
        let age = t - self.since;
        let mut k = if age < IN {
            ease_out_expo(clamp01(age / IN))
        } else if age < IN + HOLD {
            1.0
        } else {
            1.0 - clamp01((age - IN - HOLD) / OUT).powi(2)
        };
        if let Some(s) = self.stopped {
            k *= 1.0 - clamp01((t - s) / OUT);
        }
        if k <= 0.0 {
            return;
        }
        let w = 330.0;
        let h = 84.0;
        // slides in from the left edge of the screen
        let off = (1.0 - k) * (w + corner.x + 20.0);
        let r = Rect::new(corner.x - off, corner.y, w, h);
        if r.contains(ui.mouse) {
            ui.captured = true;
            if age > IN {
                self.since = t - IN;
            }
        }
        let pts = ui::chamfer(r, 12.0);
        ui::fill_convex(&pts, Color::new(0.015, 0.01, 0.04, 0.9 * k));
        ui::polyline(&pts, 1.3, pal::alpha(pal::scale(pal::MAGENTA, 0.8 + 0.4 * beat), k), true);
        ui::brackets(Rect::new(r.x - 4.0, r.y - 4.0, r.w + 8.0, r.h + 8.0), 8.0, 1.4, pal::alpha(pal::MAGENTA, k));
        // the headphone
        let c = vec2(r.x + 36.0, r.y + 44.0);
        let s = 1.0 + 0.14 * beat;
        let band: Vec<Vec2> = (0..=16).map(|i| c + vec2(-(i as f32 / 16.0 * PI).cos(), -(i as f32 / 16.0 * PI).sin()) * 15.0 * s).collect();
        let hc = pal::mix(pal::CYAN, pal::WHITE, 0.3 * beat);
        ui::polyline(&band, 2.6, pal::alpha(hc, k), false);
        for side in [-1.0, 1.0] {
            let cup = Rect::new(c.x + side * 15.0 * s - 4.5 * s, c.y - 2.0 * s, 9.0 * s, 15.0 * s);
            ui::fill_convex(&ui::chamfer(cup, 3.0), pal::alpha(hc, k));
        }
        // sound waves off the right cup
        for i in 0..2 {
            let ph = ((t * 1.6 + i as f32 * 0.5) % 1.0) as f32;
            let rad = 8.0 + ph * 12.0;
            let wc = c + vec2(15.0 * s + 4.0, 5.5 * s);
            let arc: Vec<Vec2> = (0..=8).map(|j| wc + Vec2::from_angle(-0.7 + j as f32 / 8.0 * 1.4) * rad).collect();
            ui::polyline(&arc, 1.2, pal::alpha(pal::MAGENTA, k * (1.0 - ph) * 0.8), false);
        }
        // the words
        let tx = r.x + 80.0;
        let tw = r.w - 80.0 - 16.0;
        let reveal = clamp01((age - 0.15) * 2.5);
        text::draw_spaced_fit(g, Face::Semi, tr("NOW PLAYING"), vec2(tx, r.y + 19.0), 9.5, pal::alpha(pal::MAGENTA, k), Align::Left, 2.5, tw);
        let title = text::decode(tr_.title, reveal, t, 77);
        text::draw_spaced_fit(g, Face::Semi, &title, vec2(tx, r.y + 39.0), 16.0, pal::alpha(pal::WHITE, k), Align::Left, 1.0, tw);
        let by = format!("{}  \u{b7}  {}", tr_.artist, tr_.licence);
        text::draw_fit(g, Face::Mono, &text::decode(&by, reveal, t, 78), vec2(tx, r.y + 56.0), 11.5, pal::alpha(pal::scale(pal::CYAN, 0.85), k), Align::Left, tw);
        // the link
        let link = text::ellipsize(g, Face::Mono, tr_.url.split_once("://").map(|p| p.1).unwrap_or(tr_.url), 10.5, 0.0, tw - 14.0);
        let lw = text::measure(g, Face::Mono, &link, 10.5).x;
        let ly = r.y + 73.0;
        let hit = Rect::new(tx - 4.0, ly - 12.0, lw + 20.0, 16.0);
        let over = k > 0.9 && hit.contains(ui.mouse);
        let hv = ui.hover("np-link", over);
        if over {
            ui.cursor_hand = true;
        }
        if ui.click(over, Sfx::Click) {
            credits::open_url(tr_.url);
        }
        let lc = pal::mix(pal::scale(pal::VIOLET, 1.25), pal::WHITE, hv);
        text::draw(g, Face::Mono, &link, vec2(tx, ly), 10.5, pal::alpha(lc, k), Align::Left);
        ui::line(vec2(tx, ly + 2.5), vec2(tx + lw * (0.3 + 0.7 * hv), ly + 2.5), 1.0, pal::alpha(lc, k * (0.4 + 0.6 * hv)));
        let ax = tx + lw + 5.0;
        ui::line(vec2(ax, ly - 1.0), vec2(ax + 6.0, ly - 7.0), 1.2, pal::alpha(lc, k));
        ui::line(vec2(ax + 2.0, ly - 7.0), vec2(ax + 6.0, ly - 7.0), 1.2, pal::alpha(lc, k));
        ui::line(vec2(ax + 6.0, ly - 7.0), vec2(ax + 6.0, ly - 3.0), 1.2, pal::alpha(lc, k));
    }
}
