//! The language picker: every Rockbox language under its own name, the
//! system's guess first. The whole interface previews each language as the
//! pointer passes over it (or the arrow keys reach it); a click or Enter
//! picks, and the choice is remembered.

use crate::anim::{clamp01, ease_out_cubic, ease_out_expo};
use crate::audio::Sfx;
use crate::gfx::text::{self, Align, Face};
use crate::gfx::{pal, Gfx};
use crate::i18n::{self, tr, LANGS};
use crate::ui::{self, Tone, Ui};
use macroquad::prelude::*;

/// Grid cell 0 is "automatic", cell i the language LANGS[i - 1].
const CELLS: usize = 1 + LANGS.len();

pub struct Picker {
    t0: f32,
    sel: usize,
    /// the cell being previewed by the pointer; held while the pointer is
    /// over the grid, so crossing the gaps between cells doesn't flicker
    hover: Option<usize>,
    /// the cell whose language is showing
    shown: usize,
    /// what to go back to on cancel: a language id, or the pseudo-locale
    was: &'static str,
    was_pseudo: bool,
    detected: Option<&'static str>,
    locale: String,
    /// picked (or cancelled) at this time: the picker is closing
    pub closed: Option<f32>,
    cols: usize,
}

fn lang_of(cell: usize, detected: Option<&'static str>) -> &'static str {
    match cell {
        0 => detected.unwrap_or("english"),
        i => LANGS[i - 1].id,
    }
}

impl Picker {
    pub fn open(time: f32) -> Picker {
        let detected = i18n::system();
        let cur = i18n::current().id;
        let auto = i18n::saved().map(|s| s == i18n::AUTO).unwrap_or(true);
        let sel = if auto && Some(cur) == detected.or(Some("english")) && !i18n::pseudo() {
            0
        } else {
            LANGS.iter().position(|l| l.id == cur).map(|i| i + 1).unwrap_or(0)
        };
        Picker {
            t0: time,
            sel,
            hover: None,
            shown: sel,
            was: cur,
            was_pseudo: i18n::pseudo(),
            detected,
            locale: sys_locale::get_locale().unwrap_or_else(|| "?".into()),
            closed: None,
            cols: 4,
        }
    }

    /// The close animation has finished.
    pub fn gone(&self, time: f32) -> bool {
        self.closed.map(|c| time - c > 0.35).unwrap_or(false)
    }

    fn pick(&mut self, time: f32) {
        let id = lang_of(self.sel, self.detected);
        i18n::set(id);
        i18n::save(if self.sel == 0 { i18n::AUTO } else { id });
        self.shown = self.sel;
        self.closed = Some(time);
    }

    fn cancel(&mut self, time: f32) {
        i18n::set(if self.was_pseudo { i18n::PSEUDO } else { self.was });
        self.closed = Some(time);
    }

    /// Keys: arrows move, Enter or Space picks, Escape cancels.
    pub fn keys(&mut self, time: f32) {
        if self.closed.is_some() {
            return;
        }
        let c = self.cols as isize;
        let mv = |d: isize, sel: usize| (sel as isize + d).clamp(0, CELLS as isize - 1) as usize;
        let mut moved = None;
        for (k, d) in [(KeyCode::Left, -1), (KeyCode::Right, 1), (KeyCode::Up, -c), (KeyCode::Down, c)] {
            if is_key_pressed(k) {
                moved = Some(mv(d, self.sel));
            }
        }
        if is_key_pressed(KeyCode::Home) {
            moved = Some(0);
        }
        if is_key_pressed(KeyCode::End) {
            moved = Some(CELLS - 1);
        }
        if let Some(m) = moved {
            self.sel = m;
            self.hover = None;
        }
        if is_key_pressed(KeyCode::Enter) || is_key_pressed(KeyCode::KpEnter) || is_key_pressed(KeyCode::Space) {
            self.pick(time);
        } else if is_key_pressed(KeyCode::Escape) {
            self.cancel(time);
        }
    }

    pub fn draw(&mut self, ui: &mut Ui, g: &Gfx) {
        let t = ui.time;
        let st = t - self.t0;
        let out = self.closed.map(|c| clamp01((t - c) / 0.35)).unwrap_or(0.0);
        let a = ease_out_cubic(clamp01(st / 0.4)) * (1.0 - out);
        let sz = g.size();
        // the room behind dims, and everything is ours
        draw_rectangle(0.0, 0.0, sz.x, sz.y, Color::new(0.0, 0.0, 0.02, 0.62 * a));
        ui.captured = true;
        let pw = (sz.x - 100.0).min(1060.0);
        let ph = (sz.y - 130.0).min(660.0);
        let p = Rect::new((sz.x - pw) / 2.0, (sz.y - ph) / 2.0 + 8.0 - 30.0 * out, pw, ph);
        ui::panel(p, pal::CYAN, ease_out_expo(clamp01(st / 0.5)) * (1.0 - out));
        if a < 0.05 {
            return;
        }
        let x = p.x + 30.0;
        text::glitch_title(
            g,
            &text::decode(tr("LANGUAGE"), st * 2.5, t, 5),
            vec2(x, p.y + 54.0),
            28.0,
            pal::alpha(pal::ICE, a),
            Align::Left,
            0.06,
            t,
            5.0,
            pw * 0.6,
        );
        // what the system asked for, and what it got
        let det = match self.detected {
            Some(d) => format!("locale {}  ->  {d}.lang", self.locale),
            None => format!("locale {}  ->  --", self.locale),
        };
        text::draw_fit(g, Face::Mono, &det, vec2(p.x + pw - 30.0, p.y + 50.0), 11.0, pal::alpha(pal::DIM, a), Align::Right, pw * 0.36);
        // the grid
        let gx = Rect::new(x, p.y + 82.0, pw - 60.0, ph - 82.0 - 76.0);
        let gap = 6.0;
        self.cols = ((gx.w / 172.0) as usize).clamp(3, 6);
        let rows = CELLS.div_ceil(self.cols);
        let cw = (gx.w - gap * (self.cols - 1) as f32) / self.cols as f32;
        let ch = ((gx.h - gap * (rows - 1) as f32) / rows as f32).min(46.0);
        let over_grid = Rect::new(gx.x - gap, gx.y - gap, gx.w + 2.0 * gap, rows as f32 * (ch + gap) + gap).contains(ui.mouse);
        if !over_grid {
            self.hover = None;
        }
        let mut clicked = None;
        for i in 0..CELLS {
            let r = Rect::new(gx.x + (i % self.cols) as f32 * (cw + gap), gx.y + (i / self.cols) as f32 * (ch + gap), cw, ch);
            let ca = ease_out_cubic(clamp01((st - 0.15) * 4.0 - i as f32 * 0.035)) * (1.0 - out);
            if ca <= 0.0 {
                continue;
            }
            let over = self.closed.is_none() && r.contains(ui.mouse);
            if over {
                self.hover = Some(i);
                ui.cursor_hand = true;
            }
            if ui.click(over, Sfx::Click) {
                clicked = Some(i);
            }
            let id = format!("lang{i}");
            let h = if over { ui.hover(&id, true) } else { ui.glow(&id, self.hover.is_none() && i == self.sel) };
            let chosen = i == self.sel;
            let rr = ui::unfold(r, ca);
            let pts = ui::chamfer(rr, 7.0);
            ui::fill_convex(&pts, Color::new(0.01, 0.02, 0.05, 0.85 * ca));
            if h > 0.01 {
                ui::fill_convex(&pts, pal::alpha(pal::MAGENTA, 0.14 * h * ca));
            }
            let edge = if chosen { pal::ACID } else { pal::mix(pal::scale(pal::CYAN, 0.45), pal::MAGENTA, h) };
            ui::polyline(&pts, if chosen { 1.6 } else { 1.0 + h * 0.6 }, pal::alpha(edge, ca), true);
            if chosen {
                ui::brackets(Rect::new(rr.x - 3.0, rr.y - 3.0, rr.w + 6.0, rr.h + 6.0), 6.0, 1.4, pal::alpha(pal::ACID, ca));
                let ay = rr.y + rr.h / 2.0;
                draw_triangle(vec2(rr.x + 8.0, ay - 4.0), vec2(rr.x + 13.0, ay), vec2(rr.x + 8.0, ay + 4.0), pal::alpha(pal::ACID, ca));
            }
            // the language's own name; under it, its Rockbox name
            let two = ch >= 36.0;
            let ny = if two { r.y + ch * 0.5 + 1.0 } else { r.y + ch * 0.5 + 5.0 };
            let (name, sub, nc) = if i == 0 {
                let d = self.detected.and_then(|d| LANGS.iter().find(|l| l.id == d)).map(|l| l.name).unwrap_or("English (UK)");
                (tr("AUTOMATIC"), d.to_string(), pal::ACID)
            } else {
                let l = &LANGS[i - 1];
                (l.name, l.id.to_string(), pal::ICE)
            };
            let nc = pal::mix(nc, pal::WHITE, h * 0.6);
            text::draw_fit(g, Face::Semi, name, vec2(r.x + 20.0, ny), 14.0, pal::alpha(nc, ca), Align::Left, cw - 30.0);
            if two {
                text::draw_fit(g, Face::Mono, &sub, vec2(r.x + 20.0, ny + 14.0), 10.0, pal::alpha(pal::DIM, ca), Align::Left, cw - 30.0);
            }
            if i == self.shown && self.closed.is_none() {
                // the language on screen right now
                let pulse = 0.5 + 0.5 * (t * 6.0).sin();
                draw_circle(r.x + r.w - 11.0, r.y + 11.0, 2.5, pal::alpha(pal::MAGENTA, ca * (0.5 + 0.5 * pulse)));
            }
        }
        if let Some(i) = clicked {
            self.sel = i;
            self.pick(t);
        }
        // preview: the interface follows the pointer, else the selection
        let want = self.hover.unwrap_or(self.sel);
        if want != self.shown && self.closed.is_none() {
            i18n::set(lang_of(want, self.detected));
            self.shown = want;
        }
        // footer
        let fy = p.y + ph - 40.0;
        text::draw(g, Face::Mono, "\u{2190} \u{2191} \u{2193} \u{2192}   ENTER   ESC", vec2(x, fy + 4.0), 11.0, pal::alpha(pal::DIM, a), Align::Left);
        let br = Rect::new(p.x + pw - 30.0 - 220.0, fy - 20.0, 220.0, 42.0);
        if self.closed.is_none() && ui::button(ui, g, br, tr("ENGAGE"), Tone::Primary, a, true) {
            self.pick(t);
        }
    }
}
