//! HUD widgets, drawn in logical px on the scene target (so they bloom):
//! chamfered panels and buttons, callouts that trace to the model, the
//! sector map, hex stream, terminal and phase bar.

use crate::anim::{approach, clamp01, ease_out_cubic};
use crate::gfx::text::{self, Align, Face};
use crate::gfx::{pal, Gfx};
use crate::i18n::tr;
use macroquad::prelude::*;
use rockslop::device::{Lvl, Phase, Sect};
use crate::audio::Sfx;
use std::collections::{HashMap, HashSet};

/// Per-frame input plus hover animation state for widgets.
pub struct Ui {
    pub mouse: Vec2,
    pub pressed: bool,
    pub released: bool,
    pub down: bool,
    pub dt: f32,
    pub time: f32,
    hot: HashMap<u64, f32>,
    /// a widget was under the mouse when the HUD was last drawn (so a click
    /// on it doesn't also orbit the model); both are read by the next update
    pub captured: bool,
    pub cursor_hand: bool,
    /// sounds the widgets asked for this frame (the app plays them)
    pub sfx: Vec<Sfx>,
    /// what was under the pointer last frame, and this frame
    over_before: HashSet<u64>,
    over_now: HashSet<u64>,
}

fn hash_str(s: &str) -> u64 {
    s.bytes().fold(0xcbf2_9ce4_8422_2325u64, |h, b| (h ^ b as u64).wrapping_mul(0x100_0000_01b3))
}

impl Ui {
    pub fn new() -> Ui {
        Ui {
            mouse: Vec2::ZERO,
            pressed: false,
            released: false,
            down: false,
            dt: 0.0,
            time: 0.0,
            hot: HashMap::new(),
            captured: false,
            cursor_hand: false,
            sfx: Vec::new(),
            over_before: HashSet::new(),
            over_now: HashSet::new(),
        }
    }

    pub fn frame(&mut self, dt: f32, time: f32, mouse: Vec2, pressed: bool, released: bool, down: bool) {
        self.dt = dt;
        self.time = time;
        self.mouse = mouse;
        self.pressed = pressed;
        self.released = released;
        self.down = down;
    }

    /// Call before drawing widgets: they set `captured` and `cursor_hand`
    /// again, for the next update to read.
    pub fn begin_hud(&mut self) {
        self.captured = false;
        self.cursor_hand = false;
        self.over_before = std::mem::take(&mut self.over_now);
    }

    /// Hover test for a custom widget: marks the mouse as taken.
    pub fn over(&mut self, r: Rect) -> bool {
        let o = r.contains(self.mouse);
        if o {
            self.captured = true;
            self.cursor_hand = true;
        }
        o
    }

    /// Animated hover amount for `id` (0..1).
    /// Whether a widget the pointer is `over` was just clicked; it sounds `s`.
    pub fn click(&mut self, over: bool, s: Sfx) -> bool {
        let hit = over && self.released;
        if hit {
            self.sfx.push(s);
        }
        hit
    }

    /// A tick sounds as the pointer arrives.
    pub fn hover(&mut self, id: &str, over: bool) -> f32 {
        let k = hash_str(id);
        if over && self.over_now.insert(k) && !self.over_before.contains(&k) {
            self.sfx.push(Sfx::Hover);
        }
        self.glow_key(k, over)
    }

    /// `hover`'s animation without the sound, for a highlight the pointer
    /// didn't cause.
    pub fn glow(&mut self, id: &str, on: bool) -> f32 {
        self.glow_key(hash_str(id), on)
    }

    fn glow_key(&mut self, k: u64, over: bool) -> f32 {
        let h = self.hot.entry(k).or_insert(0.0);
        *h = approach(*h, if over { 1.0 } else { 0.0 }, 14.0, self.dt);
        *h
    }
}

// ---------------------------------------------------------------- shapes

pub fn line(a: Vec2, b: Vec2, w: f32, c: Color) {
    draw_line(a.x, a.y, b.x, b.y, w, c);
}

pub fn polyline(pts: &[Vec2], w: f32, c: Color, closed: bool) {
    let n = pts.len();
    if n < 2 {
        return;
    }
    let segs = if closed { n } else { n - 1 };
    for i in 0..segs {
        line(pts[i], pts[(i + 1) % n], w, c);
    }
}

/// Fill a convex polygon (fan from the first point).
pub fn fill_convex(pts: &[Vec2], c: Color) {
    for i in 1..pts.len().saturating_sub(1) {
        draw_triangle(pts[0], pts[i], pts[i + 1], c);
    }
}

/// A rectangle with its top-left and bottom-right corners cut.
pub fn chamfer(r: Rect, cut: f32) -> Vec<Vec2> {
    let c = cut.min(r.w / 2.0).min(r.h / 2.0);
    vec![
        vec2(r.x + c, r.y),
        vec2(r.x + r.w, r.y),
        vec2(r.x + r.w, r.y + r.h - c),
        vec2(r.x + r.w - c, r.y + r.h),
        vec2(r.x, r.y + r.h),
        vec2(r.x, r.y + c),
    ]
}

pub fn brackets(r: Rect, len: f32, w: f32, c: Color) {
    let l = len.min(r.w / 2.0).min(r.h / 2.0);
    let (x0, y0, x1, y1) = (r.x, r.y, r.x + r.w, r.y + r.h);
    polyline(&[vec2(x0, y0 + l), vec2(x0, y0), vec2(x0 + l, y0)], w, c, false);
    polyline(&[vec2(x1 - l, y0), vec2(x1, y0), vec2(x1, y0 + l)], w, c, false);
    polyline(&[vec2(x1, y1 - l), vec2(x1, y1), vec2(x1 - l, y1)], w, c, false);
    polyline(&[vec2(x0 + l, y1), vec2(x0, y1), vec2(x0, y1 - l)], w, c, false);
}

pub fn ring(c: Vec2, r: f32, w: f32, col: Color, segs: usize) {
    let pts: Vec<Vec2> = (0..segs).map(|i| c + Vec2::from_angle(i as f32 / segs as f32 * std::f32::consts::TAU) * r).collect();
    polyline(&pts, w, col, true);
}

/// Grow a rect from its centre line as `t` goes 0..1 (panel "power on").
pub fn unfold(r: Rect, t: f32) -> Rect {
    let e = ease_out_cubic(t);
    let h = r.h * e;
    Rect::new(r.x, r.y + (r.h - h) / 2.0, r.w, h)
}

/// Translucent panel with a cut outline and corner ticks.
pub fn panel(r: Rect, accent: Color, appear: f32) {
    if appear <= 0.0 {
        return;
    }
    let r = unfold(r, appear);
    let pts = chamfer(r, 14.0);
    fill_convex(&pts, Color::new(0.01, 0.015, 0.04, 0.72 * appear));
    polyline(&pts, 1.2, pal::alpha(pal::scale(accent, 0.55), appear), true);
    brackets(Rect::new(r.x - 4.0, r.y - 4.0, r.w + 8.0, r.h + 8.0), 9.0, 1.6, pal::alpha(accent, appear));
}

/// Panel title strip: a small label with a rule.
pub fn header(g: &Gfx, pos: Vec2, w: f32, label: &str, c: Color, appear: f32, time: f32) {
    let s = text::decode(label, appear * 1.6, time, hash_str(label) as u32);
    // the label keeps at least a stub of rule after it
    let tw = text::draw_spaced_fit(g, Face::Semi, &s, pos, 11.0, pal::alpha(c, appear), Align::Left, 2.5, w - 24.0);
    let y = pos.y - 4.0;
    line(
        vec2(pos.x + tw + 10.0, y),
        vec2(pos.x + (w * ease_out_cubic(appear)).max(tw + 10.0), y),
        1.0,
        pal::alpha(pal::scale(c, 0.45), appear),
    );
    draw_rectangle(pos.x - 10.0, pos.y - 9.0, 4.0, 4.0, pal::alpha(c, appear));
}

// ---------------------------------------------------------------- button

#[derive(Clone, Copy, PartialEq)]
pub enum Tone {
    Primary,
    Secondary,
}

/// A chamfered button; returns true when clicked.
pub fn button(ui: &mut Ui, g: &Gfx, r: Rect, label: &str, tone: Tone, appear: f32, enabled: bool) -> bool {
    if appear <= 0.01 {
        return false;
    }
    let over = enabled && appear > 0.6 && r.contains(ui.mouse);
    let h = ui.hover(label, over);
    if over {
        ui.cursor_hand = true;
        ui.captured = true;
    }
    let base = match tone {
        Tone::Primary => pal::MAGENTA,
        Tone::Secondary => pal::CYAN,
    };
    let base = if enabled { base } else { pal::DIM };
    let t = ui.time;
    let r = unfold(r, appear);
    let grow = h * 3.0;
    let rr = Rect::new(r.x - grow, r.y - grow * 0.5, r.w + grow * 2.0, r.h + grow);
    let pts = chamfer(rr, 12.0);
    let pulse = if tone == Tone::Primary && enabled { 0.5 + 0.5 * (t * 3.2).sin() } else { 0.0 };
    fill_convex(&pts, pal::alpha(base, (0.07 + 0.16 * h + 0.05 * pulse) * appear));
    // sheen sweeping across on hover
    if h > 0.02 {
        let sx = rr.x + ((t * 0.9) % 1.0) * (rr.w + 80.0) - 40.0;
        for k in 0..3 {
            let x = sx - k as f32 * 9.0;
            if x > rr.x + 4.0 && x < rr.x + rr.w - 4.0 {
                line(vec2(x, rr.y + 3.0), vec2(x - 10.0, rr.y + rr.h - 3.0), 2.0, pal::alpha(base, 0.35 * h / (k + 1) as f32));
            }
        }
    }
    polyline(&pts, 1.4 + h * 0.8, pal::alpha(pal::scale(base, 0.8 + 0.6 * h + 0.3 * pulse), appear), true);
    brackets(
        Rect::new(rr.x - 5.0 - h * 4.0, rr.y - 5.0 - h * 2.0, rr.w + 10.0 + h * 8.0, rr.h + 10.0 + h * 4.0),
        8.0,
        1.5,
        pal::alpha(base, appear * (0.5 + 0.5 * h)),
    );
    // label, letter-spaced, with a chevron; a long translation shrinks,
    // then loses its tail, rather than run out of the button
    let size = (r.h * 0.34).clamp(11.0, 22.0);
    let room = r.w - 2.0 * (size * 0.32 + 18.0);
    let (size, spacing) = text::fit(g, Face::Semi, label, size, size * 0.22, room);
    let label = text::ellipsize(g, Face::Semi, label, size, spacing, room);
    let s = text::decode(&label, appear * 1.4, t, hash_str(&label) as u32);
    let col = pal::mix(pal::scale(base, 1.1), pal::WHITE, 0.35 + 0.45 * h);
    let cy = rr.y + rr.h / 2.0 + size * 0.38;
    text::draw_spaced(g, Face::Semi, &s, vec2(rr.x + rr.w / 2.0 + 8.0, cy), size, pal::alpha(col, appear), Align::Center, spacing);
    let tw = text::spaced_width(g, Face::Semi, &label, size, spacing);
    let ax = rr.x + rr.w / 2.0 - tw / 2.0 - 8.0 + (t * 6.0).sin() * 2.0 * h;
    let ay = rr.y + rr.h / 2.0;
    let a = size * 0.32;
    draw_triangle(vec2(ax - a, ay - a), vec2(ax, ay), vec2(ax - a, ay + a), pal::alpha(col, appear));
    ui.click(over, if tone == Tone::Primary { Sfx::Confirm } else { Sfx::Click })
}

// ---------------------------------------------------------------- callout

/// A hint box tracing to a point on the model: ring at the anchor, an
/// elbow line, and a label that decodes in. `t` animates it in (0..1).
/// A small control's plate: a dark chamfered chip, rimmed in `c`, brighter
/// while `hot`.
pub fn chip(r: Rect, c: Color, hot: f32, a: f32) {
    let pts = chamfer(r, 6.0);
    fill_convex(&pts, Color::new(0.01, 0.015, 0.04, 0.78 * a));
    fill_convex(&pts, pal::alpha(c, 0.1 * hot * a));
    polyline(&pts, 1.1, pal::alpha(c, (0.45 + 0.5 * hot) * a), true);
}

/// The width a `button` of height `h` needs to show `label` whole, at its
/// full size, chevron included.
pub fn button_width(g: &Gfx, label: &str, h: f32) -> f32 {
    let size = (h * 0.34).clamp(11.0, 22.0);
    text::spaced_width(g, Face::Semi, label, size, size * 0.22) + 2.0 * (size * 0.32 + 18.0) + 6.0
}

pub fn callout(g: &Gfx, anchor: Vec2, label_at: Vec2, label: &str, c: Color, t: f32, time: f32) {
    if t <= 0.0 {
        return;
    }
    let draw_t = clamp01(t * 1.6);
    let elbow = vec2(label_at.x + if label_at.x > anchor.x { -26.0 } else { 26.0 }, label_at.y);
    let path = [anchor, elbow, label_at];
    let len0 = anchor.distance(elbow);
    let len1 = elbow.distance(label_at);
    let total = len0 + len1;
    let mut d = total * ease_out_cubic(draw_t);
    let mut prev = anchor;
    for i in 0..2 {
        let seg = if i == 0 { len0 } else { len1 };
        let k = (d / seg.max(0.001)).min(1.0);
        let p = path[i].lerp(path[i + 1], k);
        line(prev, p, 1.3, pal::alpha(c, 0.9));
        prev = path[i + 1];
        d -= seg;
        if d <= 0.0 {
            break;
        }
    }
    let ph = (time * 1.4) % 1.0;
    ring(anchor, 4.0, 1.5, c, 16);
    ring(anchor, 6.0 + ph * 14.0, 1.2, pal::alpha(c, (1.0 - ph) * 0.8), 28);
    draw_circle(anchor.x, anchor.y, 1.8, pal::WHITE);
    if draw_t >= 1.0 {
        let lt = clamp01((t - 0.62) * 2.5);
        let s = text::decode(label, lt, time, hash_str(label) as u32);
        let right = label_at.x >= anchor.x;
        let w = text::spaced_width(g, Face::Semi, label, 12.0, 2.0) + 18.0;
        // kept on screen
        let bx = if right { label_at.x.min(screen_width() - w - 8.0) } else { (label_at.x - w).max(8.0) };
        let r = Rect::new(bx, label_at.y - 12.0, w, 24.0);
        fill_convex(&chamfer(r, 6.0), Color::new(0.01, 0.02, 0.05, 0.8));
        polyline(&chamfer(r, 6.0), 1.2, c, true);
        text::draw_spaced(g, Face::Semi, &s, vec2(r.x + 9.0, r.y + 16.5), 12.0, pal::mix(c, pal::WHITE, 0.4), Align::Left, 2.0);
    }
}

/// A line with gaps, for leaders to features on the far side of the model.
fn dash(a: Vec2, b: Vec2, w: f32, c: Color) {
    let len = a.distance(b);
    let dir = (b - a) / len.max(0.001);
    let mut d = 0.0;
    while d < len {
        line(a + dir * d, a + dir * (d + 4.0).min(len), w, c);
        d += 8.0;
    }
}

/// A select-screen hint box: `label` in a chamfered box whose inner edge
/// sits at `at`, with a leader that leaves along `out` (unit, pointing at
/// the model) and bends to `anchor`. The box comes in first, then the
/// leader traces to the feature (`t` 0..1). `hot` (0..1) lights it up;
/// `hidden` dashes the leader, for a feature on the far side of the model.
/// Returns the box, for hit testing.
#[allow(clippy::too_many_arguments)]
pub fn hint(g: &Gfx, anchor: Vec2, at: Vec2, out: Vec2, label: &str, hot: f32, hidden: bool, t: f32, time: f32) -> Rect {
    let size = 11.0;
    let spacing = 2.5;
    let tw = text::spaced_width(g, Face::Semi, label, size, spacing);
    let (w, h) = (tw + 22.0, 22.0);
    let c = at - vec2(out.x * w, out.y * h) / 2.0;
    let r = Rect::new(c.x - w / 2.0, c.y - h / 2.0, w, h);
    if t <= 0.0 {
        return r;
    }
    let col = pal::mix(pal::scale(pal::CYAN, 0.75), pal::MAGENTA, hot);
    // box
    let ba = clamp01(t * 3.0);
    let rb = unfold(r, ba);
    fill_convex(&chamfer(rb, 5.0), Color::new(0.01, 0.015, 0.04, 0.82 * ba));
    if hot > 0.0 {
        fill_convex(&chamfer(rb, 5.0), pal::alpha(pal::MAGENTA, 0.16 * hot * ba));
    }
    polyline(&chamfer(rb, 5.0), 1.1 + hot * 0.5, pal::alpha(col, (0.55 + 0.45 * hot) * ba), true);
    let lt = clamp01((t - 0.15) * 3.0);
    let s = text::decode(label, lt, time, hash_str(label) as u32);
    let tc = pal::mix(pal::mix(col, pal::WHITE, 0.3), pal::WHITE, hot);
    text::draw_spaced(g, Face::Semi, &s, vec2(c.x - tw / 2.0, c.y + 4.5), size, pal::alpha(tc, lt), Align::Left, spacing);
    // leader: out of the box, a short straight run, then to the feature
    let lt = ease_out_cubic(clamp01((t - 0.3) / 0.7));
    if lt <= 0.0 {
        return r;
    }
    let elbow = at + out * 16.0;
    let (l0, l1) = (at.distance(elbow), elbow.distance(anchor));
    let shown = (l0 + l1) * lt;
    let end = if shown < l0 { at.lerp(elbow, shown / l0) } else { elbow.lerp(anchor, ((shown - l0) / l1.max(0.01)).min(1.0)) };
    let lw = 1.0 + hot * 0.6;
    let lc = pal::alpha(col, if hidden { 0.35 + 0.4 * hot } else { 0.6 + 0.4 * hot });
    let seg = |a: Vec2, b: Vec2| if hidden { dash(a, b, lw, lc) } else { line(a, b, lw, lc) };
    seg(at, if shown < l0 { end } else { elbow });
    if shown > l0 {
        seg(elbow, end);
    }
    draw_rectangle(at.x - 2.0, at.y - 2.0, 4.0, 4.0, pal::alpha(col, 0.9));
    if lt >= 1.0 {
        if hidden {
            ring(anchor, 3.5, 1.0, lc, 14);
        } else {
            ring(anchor, 4.0, 1.3, pal::alpha(col, 0.9), 16);
            draw_circle(anchor.x, anchor.y, 1.6, pal::WHITE);
        }
        if hot > 0.0 {
            let ph = (time * 1.4) % 1.0;
            ring(anchor, 6.0 + ph * 16.0, 1.2, pal::alpha(pal::MAGENTA, (1.0 - ph) * hot), 28);
            // a pulse running down the leader to the feature
            let d = ((time * 1.1) % 1.0) * (l0 + l1);
            let p = if d < l0 { at.lerp(elbow, d / l0) } else { elbow.lerp(anchor, (d - l0) / l1.max(0.01)) };
            draw_circle(p.x, p.y, 2.2 * hot, pal::mix(pal::MAGENTA, pal::WHITE, 0.5));
        }
    }
    r
}

// ---------------------------------------------------------------- sector map

pub fn sect_color(s: Sect) -> Color {
    match s {
        Sect::Idle => pal::scale(pal::NAVY, 0.35),
        Sect::Protected => pal::scale(pal::VIOLET, 0.32),
        Sect::Read => pal::CYAN,
        Sect::Same => pal::scale(pal::TEAL, 0.45),
        Sect::Erase => pal::AMBER,
        Sect::Write => pal::MAGENTA,
        Sect::Good => pal::ACID,
        Sect::Bad => pal::RED,
    }
}

/// Grid of flash sectors. `heat` (per sector, decaying) makes recently
/// touched cells flare.
pub fn sector_map(r: Rect, sects: &[Sect], heat: &[f32], appear: f32, time: f32) -> Option<usize> {
    let n = sects.len().max(1);
    // columns so that cells are roughly square
    let cols = ((n as f32 * r.w / r.h).sqrt().ceil() as usize).max(1);
    let rows = n.div_ceil(cols);
    let cw = r.w / cols as f32;
    let ch = r.h / rows as f32;
    let gap = (cw.min(ch) * 0.18).clamp(0.6, 2.0);
    let shown = ((n as f32) * ease_out_cubic(appear)) as usize;
    let mut hovered = None;
    for i in 0..shown.min(sects.len()) {
        let (cx, cy) = (i % cols, i / cols);
        let x = r.x + cx as f32 * cw;
        let y = r.y + cy as f32 * ch;
        let h = heat.get(i).copied().unwrap_or(0.0);
        let s = sects[i];
        let mut c = sect_color(s);
        if s == Sect::Protected {
            // hatched: protected sectors are never written
            let k = if (cx + cy) % 2 == 0 { 1.0 } else { 0.6 };
            c = pal::scale(c, k);
        }
        let c = pal::mix(c, pal::WHITE, h * 0.7);
        let g = gap * (1.0 - h * 0.6);
        draw_rectangle(x + g / 2.0, y + g / 2.0, cw - g, ch - g, c);
    }
    // a sweeping read head over the grid
    let sweep = (time * 0.35) % 1.0;
    let sy = r.y + sweep * r.h;
    draw_rectangle(r.x, sy, r.w, 1.0, pal::alpha(pal::ICE, 0.12 * appear));
    if Rect::new(r.x, r.y, r.w, r.h).contains(mouse_position().into()) {
        let m: Vec2 = mouse_position().into();
        let i = ((m.y - r.y) / ch) as usize * cols + ((m.x - r.x) / cw) as usize;
        if i < n {
            hovered = Some(i);
        }
    }
    hovered
}

// ---------------------------------------------------------------- hex

/// The hex stream: newest line last, brightest.
pub fn hexdump(g: &Gfx, r: Rect, lines: &[(u32, Vec<u8>)], appear: f32, accent: Color) {
    let lh = 15.0;
    let rows = (r.h / lh) as usize;
    let start = lines.len().saturating_sub(rows);
    for (k, (addr, data)) in lines[start..].iter().enumerate() {
        let y = r.y + 12.0 + k as f32 * lh;
        let age = (lines.len() - start - 1 - k) as f32;
        let fade = (1.0 - age / rows as f32 * 0.85) * appear;
        let newest = age == 0.0;
        if newest {
            draw_rectangle(r.x - 4.0, y - 11.0, r.w + 8.0, lh, pal::alpha(accent, 0.12 * appear));
        }
        let mut x = r.x;
        x += text::draw(g, Face::Mono, &format!("{addr:08X}"), vec2(x, y), 12.0, pal::alpha(pal::DIM, fade), Align::Left) + 10.0;
        let mut ascii = String::new();
        for (i, b) in data.iter().enumerate() {
            let c = match *b {
                0x00 => pal::scale(pal::NAVY, 0.8),
                0xff => pal::scale(pal::VIOLET, 0.75),
                0x20..=0x7e => pal::TEAL,
                _ => accent,
            };
            let c = if newest { pal::mix(c, pal::WHITE, 0.45) } else { c };
            text::draw(g, Face::Mono, &format!("{b:02X}"), vec2(x, y), 12.0, pal::alpha(c, fade), Align::Left);
            x += 19.0 + if i == 7 { 6.0 } else { 0.0 };
            ascii.push(if (0x20..0x7f).contains(b) { *b as char } else { '.' });
        }
        if x + 8.0 + 16.0 * 7.0 < r.x + r.w + 4.0 {
            text::draw(g, Face::Mono, &ascii, vec2(x + 8.0, y), 12.0, pal::alpha(pal::scale(pal::TEAL, 0.8), fade), Align::Left);
        }
    }
}

// ---------------------------------------------------------------- terminal

pub fn lvl_color(l: Lvl) -> Color {
    match l {
        Lvl::Info => pal::ICE,
        Lvl::Ok => pal::ACID,
        Lvl::Warn => pal::AMBER,
        Lvl::Err => pal::RED,
        Lvl::Dim => pal::DIM,
    }
}

/// Log lines with a typewriter on the newest: (time logged, level, text).
pub fn terminal(g: &Gfx, r: Rect, lines: &[(f32, Lvl, String)], now: f32, appear: f32) {
    let lh = 16.0;
    let rows = (r.h / lh) as usize;
    let start = lines.len().saturating_sub(rows);
    for (k, (t, l, s)) in lines[start..].iter().enumerate() {
        let y = r.y + 12.0 + k as f32 * lh;
        let age = now - t;
        let typed = ((age * 160.0) as usize).min(s.chars().count());
        let shown: String = s.chars().take(typed).collect();
        let shown = text::ellipsize(g, Face::Mono, &shown, 12.0, 0.0, r.w - 72.0);
        let fade = appear * (0.45 + 0.55 * (1.0 - (lines.len() - start - 1 - k) as f32 / rows.max(1) as f32));
        let mm = (*t / 60.0) as u32;
        let ss = t % 60.0;
        text::draw(
            g,
            Face::Mono,
            &format!("{mm:02}:{ss:05.2}"),
            vec2(r.x, y),
            12.0,
            pal::alpha(pal::scale(pal::DIM, 0.8), fade),
            Align::Left,
        );
        let c = lvl_color(*l);
        let mark = match l {
            Lvl::Ok => "+",
            Lvl::Warn => "!",
            Lvl::Err => "x",
            Lvl::Dim => ".",
            Lvl::Info => ">",
        };
        text::draw(g, Face::Mono, mark, vec2(r.x + 52.0, y), 12.0, pal::alpha(c, fade), Align::Left);
        text::draw(g, Face::Mono, &shown, vec2(r.x + 66.0, y), 12.0, pal::alpha(c, fade), Align::Left);
        if typed < s.chars().count() && (now * 8.0) as i32 % 2 == 0 {
            let w = text::measure(g, Face::Mono, &shown, 12.0).x;
            draw_rectangle(r.x + 68.0 + w, y - 10.0, 7.0, 12.0, pal::alpha(c, fade));
        }
    }
}

// ---------------------------------------------------------------- phases

/// Chevron strip of job phases; `cur` lit with its progress.
pub fn phase_bar(g: &Gfx, r: Rect, phases: &[Phase], cur: Option<Phase>, progress: f32, failed: bool, appear: f32, time: f32) {
    let n = phases.len();
    let ci = cur.and_then(|c| phases.iter().position(|p| *p == c));
    let label = |p: Phase| tr(if p == Phase::Check { "CHECK" } else { p.label() });
    // chips as wide as their labels, the spare width shared out; the type
    // shrinks only if even that doesn't fit, and past that the chips share
    // the bar in proportion and each label fits its own chip
    let k = 8.0;
    let pad = 2.0 * k + 10.0;
    let natural: Vec<f32> = phases.iter().map(|p| text::spaced_width(g, Face::Semi, label(*p), 10.0, 1.5)).collect();
    let sum: f32 = natural.iter().sum();
    let fit = ((r.w - pad * n as f32) / sum.max(1.0)).min(1.0);
    let size = 10.0 * fit.max(0.75);
    let spare = ((r.w - pad * n as f32 - sum * size / 10.0) / n as f32).max(0.0);
    let widths: Vec<f32> = natural.iter().map(|nw| nw * size / 10.0 + pad + spare).collect();
    let squeeze = (r.w / widths.iter().sum::<f32>().max(1.0)).min(1.0);
    let mut x = r.x;
    for (i, p) in phases.iter().enumerate() {
        let w = widths[i] * squeeze;
        let a = clamp01(appear * 1.5 - i as f32 * 0.05);
        if a <= 0.0 {
            x += w;
            continue;
        }
        let state = match ci {
            Some(c) if i < c => 2,
            Some(c) if i == c => 1,
            _ => 0,
        };
        let c = match state {
            2 => pal::scale(pal::ACID, 0.75),
            1 if failed => pal::RED,
            1 => pal::CYAN,
            _ => pal::scale(pal::DIM, 0.55),
        };
        let pts = [
            vec2(x + 2.0, r.y),
            vec2(x + w - k - 2.0, r.y),
            vec2(x + w - 2.0, r.y + r.h / 2.0),
            vec2(x + w - k - 2.0, r.y + r.h),
            vec2(x + 2.0, r.y + r.h),
            vec2(x + 2.0 + k, r.y + r.h / 2.0),
        ];
        if state == 1 {
            let pulse = 0.5 + 0.5 * (time * 5.0).sin();
            fill_convex(&[pts[0], pts[1], pts[2], pts[3], pts[4]], pal::alpha(c, (0.10 + 0.08 * pulse) * a));
            let pw = (w - k - 4.0) * clamp01(progress);
            draw_rectangle(x + 2.0 + k, r.y + r.h - 3.0, pw, 3.0, pal::alpha(c, a));
        } else if state == 2 {
            fill_convex(&[pts[0], pts[1], pts[2], pts[3], pts[4]], pal::alpha(c, 0.08 * a));
        }
        polyline(&pts, if state == 1 { 1.6 } else { 1.0 }, pal::alpha(c, a), true);
        let tc = if state == 1 { pal::mix(c, pal::WHITE, 0.4) } else { c };
        text::draw_spaced_fit(
            g,
            Face::Semi,
            label(*p),
            vec2(x + w / 2.0 + k / 2.0, r.y + r.h / 2.0 + size * 0.4),
            size,
            pal::alpha(tc, a),
            Align::Center,
            1.5 * size / 10.0,
            w - pad + 4.0,
        );
        x += w;
    }
}

/// Segmented progress bar.
pub fn segbar(r: Rect, f: f32, c: Color, appear: f32) {
    let n = (r.w / 7.0) as usize;
    let lit = (clamp01(f) * n as f32) as usize;
    for i in 0..n {
        let x = r.x + i as f32 * 7.0;
        let col = if i < lit {
            c
        } else if i == lit {
            pal::mix(c, pal::WHITE, 0.6)
        } else {
            pal::alpha(pal::scale(c, 0.25), 0.8)
        };
        draw_rectangle(x, r.y, 5.0, r.h, pal::alpha(col, appear));
    }
}

/// Wrap text into lines of at most `width` px.
pub fn wrap(g: &Gfx, f: Face, s: &str, size: f32, width: f32) -> Vec<String> {
    text::wrap(g, f, s, size, width)
}
