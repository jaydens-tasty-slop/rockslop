//! The waiver before a first flash, over the payload screen when FLASH is
//! pressed: three clauses (it can brick the player, the warranty is void,
//! it never goes back to the seller), each redacted until the one before
//! is ticked and tickable once it has decoded in; then HOLD TO SIGN, held
//! for a few seconds while the room shakes and reddens; then a NO BACKSIES
//! stamp, and the panel switches off like an old CRT (flat to a line, the
//! line to a dot, the dot flares) as the flash starts. Space or Enter ticks and holds, Escape
//! backs out. The terms themselves are rockslop::consent's.

use crate::anim::{clamp01, ease_out_cubic, ease_out_expo};
use crate::audio::{Rise, Sfx};
use crate::gfx::text::{self, Align, Face};
use crate::gfx::{pal, Gfx};
use crate::i18n::{tf, tr};
use crate::ui::{self, Tone, Ui};
use macroquad::prelude::*;
use rockslop::consent::{CLAUSES, STATEMENT, VIBES};

/// Seconds HOLD TO SIGN must be held.
const HOLD: f32 = 2.4;
/// The stamp's fall, and how long it shows before the flash starts.
const LAND: f32 = 0.16;
const STAMPED: f32 = 2.6;
/// The switch-off: flat to a line, the line to a dot, the dot's flare.
const FLAT: f32 = 0.26;
const DOT: f32 = 0.44;
const FLARE: f32 = 0.95;
/// A clause's first body line, below its top: clear of the I UNDERSTAND
/// chip's brackets.
const BODY: f32 = 52.0;
/// A clause's lines decode one after another, this far apart.
const LINE_GAP: f32 = 0.09;

/// What happened this frame, for the app.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum Out {
    /// held all the way: record it
    Signed,
    /// the stamp hit the page
    Landed,
    /// the stamp has been seen: flash (it switches off over the start)
    Done,
    /// the switch-off's last dot flares
    Flare,
    Cancelled,
}

pub struct Waiver {
    t0: f32,
    kind: &'static str,
    demo: bool,
    auto: bool,
    /// when each clause began to decode in (until then it is redacted)
    open_at: [Option<f32>; 3],
    /// its text is all in: it can be ticked
    armed: [Option<f32>; 3],
    ticked: [Option<f32>; 3],
    /// how far the hold has got, 0..1; it drains when let go
    pub hold: f32,
    by_mouse: bool,
    by_key: bool,
    /// held this frame
    holding: bool,
    signed: Option<f32>,
    /// the stamp's date line
    date: String,
    /// cancelled at: it fades
    closed: Option<f32>,
    /// signed and seen at: it switches off
    outro: Option<f32>,
    sfx: Vec<Sfx>,
    out: Vec<Out>,
}

/// 0..1 noise from an integer.
fn noise(i: u32) -> f32 {
    ((i as f32 * 12.9898).sin() * 43758.547).fract().abs()
}

impl Waiver {
    pub fn open(time: f32, kind: &'static str, demo: bool, auto: bool) -> Waiver {
        Waiver {
            t0: time,
            kind,
            demo,
            auto,
            open_at: [Some(time + 0.6), None, None],
            armed: [None; 3],
            ticked: [None; 3],
            hold: 0.0,
            by_mouse: false,
            by_key: false,
            holding: false,
            signed: None,
            date: String::new(),
            closed: None,
            outro: None,
            sfx: vec![Sfx::Transition],
            out: Vec::new(),
        }
    }

    /// The close animation has finished.
    pub fn gone(&self, time: f32) -> bool {
        self.closed.is_some_and(|c| time - c > 0.4) || self.outro.is_some_and(|o| time - o > FLARE)
    }

    /// How much it is up (0..1): the room darkens and reddens with it.
    pub fn presence(&self, time: f32) -> f32 {
        let a = ease_out_cubic(clamp01((time - self.t0) / 0.5));
        let fade = match (self.closed, self.outro) {
            (Some(c), _) => clamp01((time - c) / 0.4),
            (_, Some(o)) => clamp01((time - o) / DOT),
            _ => 0.0,
        };
        a * (1.0 - fade)
    }

    /// How hard the room shakes and races (0..1): with the hold; easing
    /// off once the stamp is down; up again as it switches off.
    pub fn tension(&self, time: f32) -> f32 {
        match (self.signed, self.outro) {
            (_, Some(o)) => 0.3 + 0.7 * clamp01((time - o) / DOT),
            (Some(s), _) => 1.0 - 0.7 * clamp01((time - s - LAND) / 0.6),
            _ => self.hold,
        }
    }

    /// The riser's part: it climbs with the hold to the stamp's landing,
    /// winds down if let go, and stops dead on the impact.
    pub fn riser(&self, time: f32) -> (f32, Rise) {
        match self.signed {
            Some(s) if time - s < LAND => (1.0, Rise::Hold),
            Some(_) => (0.0, Rise::Cut),
            None if self.closed.is_some() => (self.hold, Rise::Let),
            None => (self.hold, if self.holding { Rise::Hold } else { Rise::Let }),
        }
    }

    fn ready(&self) -> bool {
        self.ticked.iter().all(Option::is_some) && self.signed.is_none() && self.closed.is_none()
    }

    fn live(&self) -> bool {
        self.signed.is_none() && self.closed.is_none()
    }

    /// Back out, flashing nothing (also when the player goes away).
    pub fn cancel(&mut self, time: f32) {
        if self.live() {
            self.closed = Some(time);
            self.out.push(Out::Cancelled);
        }
    }

    fn tick(&mut self, k: usize, time: f32) {
        self.ticked[k] = if self.ticked[k].is_some() { None } else { Some(time) };
        if self.ticked[k].is_some() && k + 1 < 3 && self.open_at[k + 1].is_none() {
            self.open_at[k + 1] = Some(time + 0.25);
        }
    }

    /// Escape backs out; Space or Enter ticks the next clause, and once all
    /// three are, a fresh press held signs.
    pub fn keys(&mut self, time: f32) {
        if !self.live() {
            return;
        }
        if is_key_pressed(KeyCode::Escape) {
            self.cancel(time);
            return;
        }
        let keys = [KeyCode::Space, KeyCode::Enter, KeyCode::KpEnter];
        let press = keys.iter().any(|k| is_key_pressed(*k));
        if self.ready() {
            self.by_key |= press;
        } else if press {
            if let Some(k) = (0..3).find(|&k| self.ticked[k].is_none()) {
                if self.armed[k].is_some() {
                    self.tick(k, time);
                    self.sfx.push(Sfx::Click);
                }
            }
        }
        if !keys.iter().any(|k| is_key_down(*k)) {
            self.by_key = false;
        }
    }

    /// The --auto click-through: each clause ticked a moment after it can
    /// be, then the hold.
    fn autopilot(&mut self, time: f32) {
        if !self.live() {
            return;
        }
        if let Some(k) = (0..3).find(|&k| self.ticked[k].is_none()) {
            if self.armed[k].is_some_and(|a| time - a > 0.7) {
                self.tick(k, time);
                self.sfx.push(Sfx::Click);
            }
        }
    }

    pub fn draw(&mut self, ui: &mut Ui, g: &Gfx) -> Vec<Out> {
        let t = ui.time;
        let dt = ui.dt;
        let st = t - self.t0;
        let pres = self.presence(t);
        let out = self.closed.map(|c| clamp01((t - c) / 0.4)).unwrap_or(0.0);
        let sz = g.size();
        if self.auto {
            self.autopilot(t);
        }
        ui.captured = true;
        // the room behind goes dark, and red at the edges, like an alarm
        draw_rectangle(0.0, 0.0, sz.x, sz.y, Color::new(0.02, 0.0, 0.008, 0.7 * pres));
        let alarm = (0.3 + 0.15 * (t * 3.4).sin() + 0.55 * self.hold) * pres;
        for k in 0..12 {
            let i = k as f32 * 3.0;
            let f = 1.0 - k as f32 / 12.0;
            draw_rectangle_lines(i, i, sz.x - 2.0 * i, sz.y - 2.0 * i, 3.0, pal::alpha(pal::RED, alarm * f * f * 0.45));
        }

        // layout: the biggest text that fits the window
        let pw = (sz.x - 80.0).min(880.0);
        let iw = pw - 64.0;
        let body_dx = 74.0;
        let bw = iw - body_dx;
        let kind = self.kind;
        let intro = tf("rockslop is about to write Rockbox to your {0}. Read each of these and tick it, then hold to sign.", &[&kind]);
        let mut pick = None;
        for (size, lh) in [(12.5, 18.0), (12.0, 16.5), (11.0, 15.0), (10.0, 13.5)] {
            let cards: Vec<Vec<String>> = CLAUSES.iter().map(|&(_, b)| ui::wrap(g, Face::Mono, tr(b), size, bw)).collect();
            let intro_l = ui::wrap(g, Face::Mono, &intro, size, iw);
            let vibe_l = ui::wrap(g, Face::Mono, tr(VIBES), size, iw - 18.0);
            let stmt_l = ui::wrap(g, Face::Mono, tr(STATEMENT), size + 0.5, iw - 18.0);
            let cards_h: f32 = cards.iter().map(|c| BODY + c.len() as f32 * lh).sum();
            let h = 310.0 + (intro_l.len() + vibe_l.len()) as f32 * lh + cards_h + stmt_l.len() as f32 * (lh + 1.0);
            let fits = h <= sz.y - 44.0;
            pick = Some((size, lh, cards, intro_l, vibe_l, stmt_l, h));
            if fits {
                break;
            }
        }
        let (size, lh, cards, intro_l, vibe_l, stmt_l, ph) = pick.unwrap();
        let ph = ph.min(sz.y - 20.0);
        // held, it shakes harder and harder; the stamp jolts it once
        let jolt = self.signed.map(|s| (1.0 - clamp01((t - s - LAND) / 0.35)) * (t - s >= LAND) as u8 as f32).unwrap_or(0.0);
        let tick = (t * 40.0) as u32;
        let amp = self.hold * self.hold * 3.5 * self.live() as u8 as f32 + jolt * 7.0;
        let jig = vec2(noise(tick) - 0.5, noise(tick ^ 0x55) - 0.5) * 2.0 * amp;
        let p = Rect::new((sz.x - pw) / 2.0 + jig.x, (sz.y - ph) / 2.0 + jig.y + 24.0 * out, pw, ph);
        // switching off, the whole panel is squashed about its centre
        let o = self.outro.map(|o| t - o);
        let warp = o.map(|o| crt(o, p.center())).unwrap_or(Mat4::IDENTITY);
        g.set_2d_warped(warp);
        ui::panel(p, pal::RED, ease_out_expo(clamp01(st / 0.5)) * (1.0 - out));
        let a = ease_out_cubic(clamp01((st - 0.15) / 0.4)) * (1.0 - out);
        if a < 0.02 {
            g.set_2d();
            return std::mem::take(&mut self.out);
        }
        ui.sfx.append(&mut self.sfx);
        let x = p.x + 32.0;
        let speed = 26.0 + 160.0 * self.hold;
        ui::hazard(Rect::new(p.x + 16.0, p.y + 13.0, pw - 32.0, 11.0), t * speed, a);
        ui::hazard(Rect::new(p.x + 16.0, p.y + ph - 24.0, pw - 32.0, 11.0), -t * speed, a);

        // title
        ui::header(g, vec2(x + 10.0, p.y + 46.0), iw - 10.0, tr("BEFORE THE FIRST FLASH"), pal::AMBER, a, t);
        let ty = p.y + 86.0;
        text::glitch_reveal(g, tr("POINT OF NO RETURN"), st * 1.6 - 0.2, 31, vec2(x, ty), 30.0, pal::alpha(pal::RED, a), Align::Left, 0.07 + self.hold * 0.6, t, 5.0, iw - 64.0);
        ui::warning(vec2(x + iw - 22.0, ty - 12.0), 20.0, pal::alpha(pal::AMBER, a * (0.65 + 0.35 * (t * 5.0).sin().abs())));
        let mut y = ty + 28.0;
        for l in &intro_l {
            text::draw(g, Face::Mono, l, vec2(x, y), size, pal::alpha(pal::scale(pal::ICE, 0.8), a), Align::Left);
            y += lh;
        }
        // how it was made, marked like the statement below
        y += 6.0;
        draw_rectangle(x, y - size - 1.0, 3.0, vibe_l.len() as f32 * lh, pal::alpha(pal::AMBER, a));
        for l in &vibe_l {
            text::draw(g, Face::Mono, l, vec2(x + 16.0, y), size, pal::alpha(pal::AMBER, a), Align::Left);
            y += lh;
        }
        y += 14.0;

        // the clauses
        let cards_top = y;
        let tick_label = tr("I UNDERSTAND");
        let tlw = text::spaced_width(g, Face::Semi, tick_label, 12.0, 2.0).min(iw * 0.3);
        let chip_w = tlw + 66.0;
        for (k, lines) in cards.iter().enumerate() {
            let ch = BODY + lines.len() as f32 * lh;
            let r = Rect::new(x, y, iw, ch);
            let open = self.open_at[k].map(|o| t - o).filter(|o| *o >= 0.0);
            let ticked = self.ticked[k];
            let col = if ticked.is_some() {
                pal::ACID
            } else if open.is_some() {
                pal::RED
            } else {
                pal::DIM
            };
            let pts = ui::chamfer(r, 10.0);
            ui::fill_convex(&pts, Color::new(0.03, 0.0, 0.015, 0.6 * a));
            if let Some(tk) = ticked {
                ui::fill_convex(&pts, pal::alpha(pal::ACID, 0.16 * (1.0 - clamp01((t - tk) / 0.5)) * a));
            }
            ui::polyline(&pts, 1.2, pal::alpha(pal::scale(col, if open.is_some() { 0.75 } else { 0.4 }), a), true);
            text::draw_spaced(g, Face::Display, &format!("{:02}", k + 1), vec2(x + 16.0, y + 36.0), 26.0, pal::alpha(col, a * if open.is_some() { 1.0 } else { 0.5 }), Align::Left, 2.0);
            // heading, then the body a line at a time; still redacted, each
            // is a bar as long as its text, which burns away as it decodes
            let head = CLAUSES[k].0;
            let hx = x + body_dx;
            let hw = iw - body_dx - chip_w - 44.0;
            let hr = open.map(|o| clamp01(o * 2.5)).unwrap_or(0.0);
            let (hs, hsp) = text::fit(g, Face::Semi, tr(head), 15.0, 2.5, hw);
            let head_w = text::spaced_width(g, Face::Semi, tr(head), hs, hsp).min(hw);
            redact(Rect::new(hx, y + 12.0, head_w, 14.0), hr, a);
            if hr > 0.0 {
                let hc = if ticked.is_some() { pal::mix(pal::ACID, pal::ICE, 0.4) } else { pal::ICE };
                text::draw_reveal(g, Face::Semi, tr(head), hr, t, 900 + k as u32, vec2(hx, y + 25.0), 15.0, pal::alpha(hc, a), Align::Left, 2.5, hw);
            }
            let mut done = open.is_some();
            for (i, l) in lines.iter().enumerate() {
                let ly = y + BODY + i as f32 * lh;
                let rv = open.map(|o| clamp01((o - 0.3 - i as f32 * LINE_GAP) * 2.2)).unwrap_or(0.0);
                done &= rv >= 1.0;
                let lw = text::measure(g, Face::Mono, l, size).x;
                redact(Rect::new(hx, ly - size * 0.85, lw, size * 1.05), rv, a);
                if rv > 0.0 {
                    let shown = text::decode(l, rv, t, 1000 + (k * 64 + i) as u32);
                    text::draw(g, Face::Mono, &shown, vec2(hx, ly), size, pal::alpha(pal::scale(pal::ICE, 0.82), a), Align::Left);
                }
            }
            if done && self.armed[k].is_none() {
                self.armed[k] = Some(t);
            }
            // I UNDERSTAND, at the heading's right. Locked, a faint outline;
            // tickable, a red chip that throbs, with brackets, an arrow
            // nudging at it and a ping from its box; ticked, green.
            let armed = self.armed[k].is_some() && self.live();
            let tr_ = Rect::new(x + iw - chip_w - 12.0, y + 5.0, chip_w, 28.0);
            let over = armed && tr_.contains(ui.mouse);
            if over {
                ui.cursor_hand = true;
            }
            let hov = ui.hover(&format!("waiver-tick{k}"), over);
            if ui.click(over, Sfx::Click) {
                self.tick(k, t);
            }
            let since = self.armed[k].map(|w| t - w).unwrap_or(0.0);
            let ca = a * (0.3 + 0.7 * clamp01(since / 0.3));
            let waiting = armed && ticked.is_none();
            let pulse = if waiting { 0.5 + 0.5 * (t * 4.5).sin() } else { 0.0 };
            let bc = if ticked.is_some() {
                pal::ACID
            } else if armed {
                pal::mix(pal::RED, pal::WHITE, 0.3 * hov + 0.25 * pulse)
            } else {
                pal::DIM
            };
            let chip = ui::chamfer(tr_, 7.0);
            if armed || ticked.is_some() {
                ui::fill_convex(&chip, pal::alpha(bc, (0.1 + 0.14 * pulse + 0.16 * hov) * ca));
                ui::polyline(&chip, 1.4 + 0.6 * pulse, pal::alpha(bc, ca), true);
            } else {
                ui::polyline(&chip, 1.0, pal::alpha(pal::DIM, 0.3 * a), true);
            }
            let bx = Rect::new(tr_.x + 9.0, tr_.y + 5.0, 18.0, 18.0);
            if waiting {
                // (all of it clear of the body's first line)
                let gap = 3.0 + 2.0 * pulse + 1.5 * hov;
                ui::brackets(Rect::new(tr_.x - gap, tr_.y - gap, tr_.w + 2.0 * gap, tr_.h + 2.0 * gap), 7.0, 1.5, pal::alpha(pal::RED, ca));
                let ax = tr_.x - 12.0 - 7.0 * (t * 5.0).sin().abs();
                let ay = tr_.y + tr_.h / 2.0;
                draw_triangle(vec2(ax - 8.0, ay - 6.5), vec2(ax, ay), vec2(ax - 8.0, ay + 6.5), pal::alpha(pal::RED, ca));
                let ph = (since / 1.3).fract();
                ui::ring(bx.center(), 10.0 + 12.0 * ph, 1.6, pal::alpha(pal::RED, (1.0 - ph) * 0.85 * ca), 28);
            }
            ui::polyline(&ui::chamfer(bx, 4.0), 2.0, pal::alpha(bc, if armed || ticked.is_some() { ca } else { 0.4 * a }), true);
            if ticked.is_some() {
                ui::fill_convex(&ui::chamfer(bx, 4.0), pal::alpha(pal::ACID, 0.3 * a));
                let c = vec2(bx.x + 9.0, bx.y + 10.0);
                ui::polyline(&[c + vec2(-5.5, -1.0), c + vec2(-1.5, 3.5), c + vec2(6.5, -6.5)], 2.4, pal::alpha(pal::ACID, a), false);
            }
            let lc = if ticked.is_some() {
                pal::ACID
            } else if armed {
                pal::mix(pal::ICE, pal::WHITE, 0.5 + 0.5 * hov)
            } else {
                pal::scale(pal::DIM, 0.8)
            };
            let la = if armed || ticked.is_some() { ca } else { 0.5 * a };
            text::draw_spaced_fit(g, Face::Semi, tick_label, vec2(bx.x + 30.0, tr_.y + 19.0), 12.0, pal::alpha(lc, la), Align::Left, 2.0, tlw);
            y += ch + 10.0;
        }
        let cards_bottom = y - 10.0;
        y += 8.0;

        // the statement signing makes, lit once all three are ticked
        let ready = self.ready();
        let all = self.ticked.iter().all(Option::is_some);
        let sc = if all { pal::ICE } else { pal::scale(pal::DIM, 0.8) };
        let sh = stmt_l.len() as f32 * (lh + 1.0);
        draw_rectangle(x, y + 2.0, 3.0, sh - 2.0, pal::alpha(if all { pal::RED } else { pal::DIM }, a));
        for (i, l) in stmt_l.iter().enumerate() {
            text::draw(g, Face::Mono, l, vec2(x + 16.0, y + size + 2.0 + i as f32 * (lh + 1.0)), size + 0.5, pal::alpha(sc, a), Align::Left);
        }
        y += sh + 16.0;

        // HOLD TO SIGN, and NOT NOW
        let cancel_label = tr("NOT NOW");
        let cw = ui::button_width(g, cancel_label, 42.0).clamp(150.0, iw * 0.34);
        let hr = Rect::new(x, y, iw - cw - 24.0, 58.0);
        let over = ready && hr.contains(ui.mouse);
        if over {
            ui.cursor_hand = true;
            if ui.pressed {
                self.by_mouse = true;
            }
        }
        if !ui.down || !over {
            self.by_mouse = false;
        }
        let auto_hold = self.auto && self.armed[2].is_some() && self.ticked[2].is_some_and(|k| t - k > 0.9);
        let holding = ready && (self.by_mouse || self.by_key || auto_hold);
        self.holding = holding;
        if self.live() {
            if holding {
                // (the riser is its sound: see `riser`)
                self.hold = (self.hold + dt / HOLD).min(1.0);
                if self.hold >= 1.0 {
                    self.signed = Some(t);
                    self.by_mouse = false;
                    self.by_key = false;
                    let d = rockslop::device::backup::stamp_now();
                    self.date = format!("{}-{}-{}", &d[0..4], &d[4..6], &d[6..8]);
                    self.out.push(Out::Signed);
                }
            } else {
                self.hold = (self.hold - dt * 1.4).max(0.0);
            }
        }
        let hov = ui.hover("waiver-sign", over);
        sign_button(g, hr, self.hold, ready, holding, self.signed.is_some(), hov, a, t);
        if self.live() && ui::button(ui, g, Rect::new(x + iw - cw, y + 8.0, cw, 42.0), cancel_label, Tone::Secondary, a, true) {
            self.cancel(t);
        }
        let fy = y + 58.0 + 24.0;
        let note = if !all && self.live() {
            Some((tr("tick all three boxes first"), pal::AMBER))
        } else if !self.demo {
            Some((tr("Signing is final. rockslop won't ask again on this computer."), pal::DIM))
        } else {
            None
        };
        if let Some((note, nc)) = note {
            text::draw_fit(g, Face::Mono, note, vec2(x, fy), 11.0, pal::alpha(nc, a), Align::Left, iw);
        }

        // the stamp
        if let Some(s0) = self.signed {
            let s = t - s0;
            if s >= LAND && s - dt < LAND {
                self.out.push(Out::Landed);
            }
            if s > STAMPED && self.outro.is_none() {
                self.outro = Some(t);
                self.out.push(Out::Done);
            }
            let centre = vec2(p.x + pw / 2.0, (cards_top + cards_bottom) / 2.0);
            self.stamp(g, warp, centre, pw * 0.7, s, a);
        }
        if let Some(o) = o {
            // the picture goes white as it flattens: at the end, a line
            let w = ease_in(clamp01(o / FLAT));
            ui::fill_convex(&ui::chamfer(p, 14.0), pal::alpha(pal::mix(pal::RED, pal::WHITE, w), (0.2 + 0.8 * w) * w));
        }
        g.set_2d();
        if let Some(o) = o {
            self.switch_off(o, p.center(), sz, dt);
        }
        std::mem::take(&mut self.out)
    }

    /// The end of the switch-off, unwarped: a dot where the panel's line
    /// shrank to, then a flare and a shockwave across the screen.
    fn switch_off(&mut self, o: f32, c: Vec2, sz: Vec2, dt: f32) {
        if o >= DOT && o - dt < DOT {
            self.out.push(Out::Flare);
        }
        if (FLAT..DOT).contains(&o) {
            // the line's glow, which the warp would flatten away
            let k = (o - FLAT) / (DOT - FLAT);
            let half = 0.5 * (1.0 - ease_in(k)) * 900.0;
            draw_rectangle(c.x - half, c.y - 2.0, 2.0 * half, 4.0, pal::alpha(pal::WHITE, 0.9));
            draw_rectangle(c.x - half * 1.1, c.y - 6.0, 2.2 * half, 12.0, pal::alpha(pal::RED, 0.35));
        }
        if o >= DOT {
            let f = clamp01((o - DOT) / (FLARE - DOT));
            let fade = 1.0 - f;
            let reach = sz.length() * 0.62;
            // the dot, burning out
            draw_circle(c.x, c.y, 3.0 + 9.0 * fade * fade, pal::alpha(pal::WHITE, fade));
            // a lens streak
            let streak = 40.0 + 520.0 * ease_out_expo(f);
            draw_rectangle(c.x - streak, c.y - 1.0, 2.0 * streak, 2.0, pal::alpha(pal::WHITE, 0.8 * fade));
            // the shockwave, red then magenta behind it
            ui::ring(c, reach * ease_out_expo(f), 3.0 + 6.0 * fade, pal::alpha(pal::RED, 0.85 * fade), 96);
            ui::ring(c, reach * ease_out_expo((f - 0.08).max(0.0)), 2.0, pal::alpha(pal::MAGENTA, 0.6 * fade), 96);
        }
    }

    /// NO BACKSIES in red ink, struck at an angle: it falls from above the
    /// page, lands, and stays, worn where the ink didn't take.
    fn stamp(&self, g: &Gfx, warp: Mat4, centre: Vec2, max_w: f32, s: f32, a: f32) {
        let k = clamp01(s / LAND);
        let grow = 1.0 + 1.4 * (1.0 - k) * (1.0 - k);
        let sa = clamp01(s / 0.05) * a;
        // it twists the last few degrees as it lands
        g.set_2d_warped(warp * turn(centre, -7.0 - 6.0 * (1.0 - k)));
        let label = tr("NO BACKSIES");
        let (size, sp) = text::fit(g, Face::Display, label, 62.0, 6.0, max_w - 70.0);
        let (size, sp) = (size * grow, sp * grow);
        let tw = text::spaced_width(g, Face::Display, label, size, sp);
        let sub = format!("{}  //  {}  //  {}", tr("SIGNED"), self.date, self.kind);
        let ss = 13.0 * grow;
        let bw = tw + 64.0 * grow;
        let bh = size * 0.78 + ss + 48.0 * grow;
        let r = Rect::new(centre.x - bw / 2.0, centre.y - bh / 2.0, bw, bh);
        let ink = pal::alpha(pal::RED, 0.92 * sa);
        ui::fill_convex(&ui::chamfer(r, 12.0 * grow), Color::new(0.06, 0.0, 0.015, 0.93 * sa));
        ui::polyline(&ui::chamfer(r, 12.0 * grow), 4.5 * grow, ink, true);
        let inset = 8.0 * grow;
        ui::polyline(&ui::chamfer(Rect::new(r.x + inset, r.y + inset, r.w - 2.0 * inset, r.h - 2.0 * inset), 8.0 * grow), 1.5 * grow, ink, true);
        let base = r.y + 22.0 * grow + size * 0.78;
        text::draw_spaced(g, Face::Display, label, vec2(centre.x, base), size, ink, Align::Center, sp);
        // where the ink didn't take (the date line mostly did)
        for i in 0..120u32 {
            let px = r.x + 4.0 + noise(i * 3 + 1) * (r.w - 8.0);
            let py = r.y + 4.0 + noise(i * 3 + 2) * (base - r.y + 2.0);
            let w = 1.0 + noise(i * 3 + 3) * 4.0 * grow;
            draw_rectangle(px, py, w, 1.0 + (i % 3) as f32, Color::new(0.04, 0.0, 0.01, 0.8 * sa));
        }
        text::draw_spaced(g, Face::Mono, &sub, vec2(centre.x, base + ss + 12.0 * grow), ss, pal::alpha(pal::mix(pal::RED, pal::WHITE, 0.2), sa), Align::Center, 1.5 * grow);
        g.set_2d_warped(warp);
    }
}

fn ease_in(t: f32) -> f32 {
    t * t * t
}

/// Turned `deg` about `about`.
fn turn(about: Vec2, deg: f32) -> Mat4 {
    Mat4::from_translation(about.extend(0.0)) * Mat4::from_rotation_z(deg.to_radians()) * Mat4::from_translation(-about.extend(0.0))
}

/// An old CRT switching off, `o` seconds in, about `c`: the picture
/// flattens to a line (a touch wider as it goes), then the line shrinks to
/// a dot.
fn crt(o: f32, c: Vec2) -> Mat4 {
    let flat = ease_in(clamp01(o / FLAT));
    let sy = 1.0 - 0.996 * flat;
    let sx = (1.0 + 0.08 * flat) * (1.0 - ease_in(clamp01((o - FLAT) / (DOT - FLAT))));
    Mat4::from_translation(c.extend(0.0)) * Mat4::from_scale(vec3(sx.max(0.0), sy, 1.0)) * Mat4::from_translation(-c.extend(0.0))
}

/// A redaction bar over `r`, burnt away from the left as `reveal` goes 0..1.
fn redact(r: Rect, reveal: f32, a: f32) {
    if reveal >= 1.0 {
        return;
    }
    let x = r.x + r.w * reveal;
    let w = r.w - (x - r.x);
    draw_rectangle(x, r.y, w, r.h, Color::new(0.16, 0.18, 0.24, 0.85 * a));
    if reveal > 0.0 {
        // the burning edge
        draw_rectangle(x - 1.0, r.y - 1.0, 2.0, r.h + 2.0, pal::alpha(pal::AMBER, 0.9 * a));
    }
}

/// HOLD TO SIGN: fills from the left while held, with a bright leading
/// edge; dim until the clauses are all ticked.
#[allow(clippy::too_many_arguments)]
fn sign_button(g: &Gfx, r: Rect, hold: f32, ready: bool, holding: bool, signed: bool, hov: f32, a: f32, t: f32) {
    let on = ready || signed;
    let base = if on { pal::RED } else { pal::DIM };
    let pulse = if ready && !holding { 0.5 + 0.5 * (t * 3.2).sin() } else { 0.0 };
    let pts = ui::chamfer(r, 12.0);
    ui::fill_convex(&pts, pal::alpha(base, (0.06 + 0.1 * hov + 0.06 * pulse) * a));
    if hold > 0.0 {
        let fw = r.w * hold;
        ui::fill_convex(&ui::chamfer(Rect::new(r.x, r.y, fw, r.h), 12.0), pal::alpha(pal::RED, 0.5 * a));
        let mut yy = r.y + 2.0;
        while yy < r.y + r.h - 2.0 {
            ui::line(vec2(r.x + 2.0, yy), vec2(r.x + fw - 2.0, yy), 1.0, Color::new(0.0, 0.0, 0.0, 0.18 * a));
            yy += 3.0;
        }
        ui::line(vec2(r.x + fw, r.y + 2.0), vec2(r.x + fw, r.y + r.h - 2.0), 2.0, pal::alpha(pal::WHITE, 0.9 * a * (!signed) as u8 as f32));
    }
    ui::polyline(&pts, 1.5 + hov * 0.8, pal::alpha(pal::scale(base, 0.85 + 0.3 * pulse), a), true);
    if on {
        ui::brackets(Rect::new(r.x - 5.0, r.y - 5.0, r.w + 10.0, r.h + 10.0), 8.0, 1.5, pal::alpha(base, a * (0.6 + 0.4 * hov)));
    }
    let label = if signed {
        tr("SIGNED")
    } else if holding {
        tr("KEEP HOLDING")
    } else {
        tr("HOLD TO SIGN")
    };
    let col = if on { pal::mix(pal::ICE, pal::WHITE, 0.4 + 0.6 * hold) } else { pal::scale(pal::DIM, 0.9) };
    text::draw_spaced_fit(g, Face::Semi, label, vec2(r.x + r.w / 2.0, r.y + r.h / 2.0 + 7.0), 19.0, pal::alpha(col, a), Align::Center, 4.0, r.w - 40.0);
}
