//! Clippy, in techno shades. When someone has been stuck on a screen for a
//! while he pops up in the bottom-left corner with a tip; GOT IT sends that
//! tip away for the session. Drawn from scratch as a neon wireframe: a bent
//! wire, eyebrows, and visor shades that drop on as he arrives and that he
//! now and then peeks over (click him to make him do it). Once the credits
//! have rolled all the way through he comes back for an encore, and stays
//! surfing the scroller: click him for another kickflip.

use crate::anim::{clamp01, ease_in_out_cubic, ease_out_back, ease_out_cubic, Spring};
use crate::audio::Sfx;
use crate::gfx::text::{self, Align, Face};
use crate::gfx::{pal, Gfx};
use crate::i18n::{self, tr};
use crate::ui::{self, Tone, Ui};
use macroquad::prelude::*;
use std::f32::consts::{PI, TAU};

/// What he can help with.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum Tip {
    /// the select screen: there is no model number to find
    Model,
    /// no player on the bus
    Link,
    /// the player is plugged in running its own firmware or Rockbox
    Firmware,
    Access,
    Package,
    Card,
    Disk,
    Fail,
}

/// A tip for the situation the user is in now.
pub struct Say {
    pub tip: Tip,
    /// seconds in that situation before he speaks up
    pub after: f32,
    pub body: String,
}

pub struct Clippy {
    /// the situation, and since when
    since: Option<(Tip, f32)>,
    shown: Option<Say>,
    shown_at: f32,
    out: Spring,
    gone: Vec<Tip>,
    /// when he last started peeking over his shades
    peek_at: f32,
    /// when he last took off on a trick someone clicked for
    trick_at: f32,
}

const BUBBLE_W: f32 = 268.0;
/// px per unit of the wire's own coordinates
const U: f32 = 16.0;
/// feet to the top of the wire, px
const TALL: f32 = 10.1 * U * 1.08;

impl Clippy {
    pub fn new() -> Clippy {
        Clippy { since: None, shown: None, shown_at: 0.0, out: Spring::new(0.0, 70.0, 11.0), gone: Vec::new(), peek_at: -10.0, trick_at: -10.0 }
    }

    /// Each frame: the tip for where the user is (None: nothing to help
    /// with). Returns true as he pops up.
    pub fn update(&mut self, now: f32, dt: f32, say: Option<Say>) -> bool {
        match (&say, self.since) {
            (Some(s), Some((t, _))) if t == s.tip => {}
            (Some(s), _) => self.since = Some((s.tip, now)),
            (None, _) => self.since = None,
        }
        let since = self.since.map(|s| s.1).unwrap_or(now);
        let ready = say.filter(|s| !self.gone.contains(&s.tip) && now - since >= s.after);
        let mut popped = false;
        match ready {
            // the same tip: keep it (its text can change with the language)
            Some(s) if self.shown.as_ref().is_some_and(|x| x.tip == s.tip) => {
                self.shown = Some(s);
                self.out.target = 1.0;
            }
            // a new one, once the last has gone
            Some(s) if self.shown.is_none() || self.out.x < 0.05 => {
                self.shown = Some(s);
                self.shown_at = now;
                self.out.target = 1.0;
                popped = true;
            }
            _ => self.out.target = 0.0,
        }
        self.out.step(dt);
        if self.out.target == 0.0 && self.out.x < 0.02 && self.out.v.abs() < 0.1 {
            self.shown = None;
        }
        popped
    }

    /// How far he's out (0..1): the scene makes room for him.
    pub fn presence(&self) -> f32 {
        if self.shown.is_some() { clamp01(self.out.x) } else { 0.0 }
    }

    /// Draw him with his feet at `feet`, the bubble above.
    pub fn draw(&mut self, ui: &mut Ui, g: &Gfx, feet: Vec2, time: f32) {
        let Some(s) = &self.shown else { return };
        let (tip, body) = (s.tip, s.body.clone());
        let o = self.out.x.max(0.0);
        if o < 0.01 {
            return;
        }
        let age = time - self.shown_at;
        let a = clamp01(o * 1.4);
        // he rises out of the floor, wobbling a little as he lands
        let feet = feet + vec2(0.0, (1.0 - o) * 170.0);
        // the bubble, above and to the right
        let bx = (feet.x - 52.0).max(24.0);
        let bw = BUBBLE_W.min(screen_width() - bx - 24.0);
        let pad = 14.0;
        let open = tr("It looks like you're trying to hack an MP3 player. Would you like help?");
        let l1 = ui::wrap(g, Face::Semi, open, 12.0, bw - 2.0 * pad);
        let mut l2 = Vec::new();
        for p in body.split('\n') {
            l2.extend(ui::wrap(g, Face::Mono, p, 12.0, bw - 2.0 * pad));
        }
        let got = tr("GOT IT");
        let btn_w = ui::button_width(g, got, 30.0).max(110.0).min(bw - 2.0 * pad);
        let bh = pad + l1.len() as f32 * 16.0 + 8.0 + l2.len() as f32 * 15.0 + 12.0 + 30.0 + pad;
        let head = feet.y - TALL;
        let r = Rect::new(bx, head - 26.0 - bh, bw, bh);
        let ba = clamp01((age - 0.25) * 4.0) * a;
        if ba > 0.0 {
            let rr = ui::unfold(r, ease_out_cubic(ba));
            balloon(rr, vec2(feet.x + 2.0, head - 2.0), ba);
            // right to left: the text from the right, the button on the left
            let rtl = i18n::current().rtl;
            let (tx, al) = if rtl { (rr.x + rr.w - pad, Align::Right) } else { (rr.x + pad, Align::Left) };
            let mut y = rr.y + pad + 11.0;
            for l in &l1 {
                text::draw(g, Face::Semi, l, vec2(tx, y), 12.0, pal::alpha(YELLOW, ba), al);
                y += 16.0;
            }
            y += 8.0;
            let ta = clamp01((age - 0.6) * 3.0) * ba;
            for l in &l2 {
                text::draw(g, Face::Mono, l, vec2(tx, y), 12.0, pal::alpha(pal::ICE, ta), al);
                y += 15.0;
            }
            let bx = if rtl { rr.x + pad } else { rr.x + rr.w - pad - btn_w };
            let br = Rect::new(bx, rr.y + rr.h - pad - 30.0, btn_w, 30.0);
            if ui::button(ui, g, br, got, Tone::Secondary, ta, ba > 0.9 && self.out.target > 0.0) {
                self.gone.push(tip);
                self.out.target = 0.0;
            }
            // the bubble isn't a drag on whatever is under it
            if ba > 0.5 {
                ui.captured |= r.contains(ui.mouse);
            }
        }
        // click him: he peeks
        let me = Rect::new(feet.x - 2.2 * U, feet.y - TALL, 4.4 * U, TALL);
        if ui.over(me) && ui.released {
            self.peek_at = time;
        }
        // the shades drop on as he arrives, and now and then slide down his
        // nose so he can peek over them
        let peek_t = time - self.peek_at;
        let peek = if peek_t < 1.6 {
            peek_window(peek_t)
        } else if age > 2.5 {
            peek_window((age - 2.5) % 7.0 - 4.0)
        } else {
            0.0
        };
        let drop = 1.0 - ease_out_back(clamp01((age - 0.45) / 0.45));
        let pose = Pose {
            a,
            shades: drop * 4.0 - peek * 1.05,
            raise: 0.35 * peek + 0.4 * (1.0 - clamp01(age / 0.8)),
            paper: true,
            ..Pose::default()
        };
        figure(feet, time, &pose);
    }

    /// The encore, `e` seconds in, once the credits have rolled all the way
    /// through: he drops onto the wave scroller (along `wave_y`, across a
    /// screen `width` wide) and surfs it on his paper; at centre stage he
    /// backflips out of his shades and they drop back on; then he stays,
    /// cruising up and down the wave, and a click on him (`encore_click`)
    /// sends him into another kickflip. Not `full`: back on the credits
    /// after all that, he just drops in and cruises.
    pub fn encore(&self, g: &Gfx, e: f32, full: bool, time: f32, wave_y: f32, width: f32, beat: f32) {
        if e < 0.0 {
            return;
        }
        let Ride { at, feet, tilt, flip, trick, .. } = self.ride(e, full, time, wave_y, width, beat);
        // the shades: on as he falls in, off for the flip, dealt back on
        let shades = if e < ENCORE_FLIP || !full {
            0.0
        } else if e < ENCORE_LAND {
            7.0 * ease_out_cubic(clamp01(flip * 2.0))
        } else {
            7.0 * (1.0 - ease_out_back(clamp01((e - ENCORE_LAND - 0.3) / 0.45)))
        };
        let a = clamp01(e * 4.0);
        // the board: his paper, turned over (a kickflip with the backflip)
        let turn = (flip * TAU).cos();
        board(at, tilt, turn, a, time);
        let pose = Pose {
            a,
            tilt,
            flip: -flip * TAU,
            shades,
            raise: if trick || (full && (ENCORE_FLIP..ENCORE_LAND + 1.0).contains(&e)) { 0.5 } else { 0.1 * beat },
            beat,
            ..Pose::default()
        };
        figure(feet, time, &pose);
        // what he says, overhead
        if !full {
            return;
        }
        let (say, from, to) = if e < ENCORE_FLIP {
            (tr("It looks like you watched all the credits. Nobody does that. Respect."), ENCORE_DROP + 0.3, ENCORE_FLIP - 0.4)
        } else {
            (tr("DEAL WITH IT"), ENCORE_LAND + 0.7, ENCORE_OFF - 0.2)
        };
        let ba = clamp01((e - from) * 4.0) * clamp01((to - e) * 4.0);
        if ba > 0.0 {
            let bw = 280.0f32.min(width - 48.0);
            let lines = ui::wrap(g, Face::Semi, say, 14.0, bw - 28.0);
            let bh = 22.0 + lines.len() as f32 * 19.0;
            let head = feet.y - TALL;
            let bx = (feet.x - bw * 0.35).clamp(24.0, width - 24.0 - bw);
            let r = ui::unfold(Rect::new(bx, head - 24.0 - bh, bw, bh), ease_out_cubic(ba));
            balloon(r, vec2(feet.x, head - 4.0), ba);
            for (k, l) in lines.iter().enumerate() {
                let c = pal::mix(YELLOW, pal::WHITE, 0.3 * beat);
                text::draw(g, Face::Semi, l, vec2(r.x + r.w / 2.0, r.y + 26.0 + k as f32 * 19.0), 14.0, pal::alpha(c, ba), Align::Center);
            }
        }
    }
}

/// Where he is on the wave, `e` seconds into the encore.
struct Ride {
    /// the board's centre, and his feet on it
    at: Vec2,
    feet: Vec2,
    /// where the board meets the wave under him, jumping or not
    ground: f32,
    tilt: f32,
    /// how far through a flip he is (0..1): the show's, or a clicked one
    flip: f32,
    /// on a clicked trick
    trick: bool,
}

impl Clippy {
    /// Clicking him while he surfs (once the show's flip has landed, or
    /// straight away on a later visit) sends him into another kickflip.
    /// He's in front of the panes and BACK, so call it before them: while
    /// he's on the wave they don't see the pointer over him, or its clicks.
    /// Returns true the frame a trick lands.
    pub fn encore_click(&mut self, ui: &mut Ui, e: f32, full: bool, time: f32, wave_y: f32, width: f32, beat: f32) -> bool {
        let landed = self.trick(time).is_none() && self.trick(time - ui.dt).is_some();
        if e < ENCORE_DROP {
            return landed;
        }
        let r = self.ride(e, full, time, wave_y, width, beat);
        // him, and the air under him while he jumps: a second click must
        // not land on what he jumped off
        let top = r.feet.y - TALL;
        let me = Rect::new(r.feet.x - 2.6 * U, top, 5.2 * U, r.ground + U - top);
        if !me.contains(ui.mouse) {
            return landed;
        }
        let ready = self.trick(time).is_none() && e >= if full { ENCORE_LAND + 1.0 } else { ENCORE_DROP };
        if ready {
            let over = ui.over(me);
            if ui.click(over, Sfx::Transition) {
                self.trick_at = time;
            }
        }
        ui.captured = true;
        ui.hide_pointer();
        landed
    }

    /// How far through a clicked trick he is (0..1), while he's on one.
    fn trick(&self, time: f32) -> Option<f32> {
        let f = (time - self.trick_at) / TRICK;
        (0.0..1.0).contains(&f).then_some(f)
    }

    fn ride(&self, e: f32, full: bool, time: f32, wave_y: f32, width: f32, beat: f32) -> Ride {
        let (mid, start) = (width * 0.5, width * 0.8);
        let show = if full { clamp01((e - ENCORE_FLIP) / (ENCORE_LAND - ENCORE_FLIP)) } else { 0.0 };
        let trick = self.trick(time);
        // the show's backflip, else a clicked one: a flip turn and a hop
        let (flip, hop) = match trick {
            _ if show > 0.0 && show < 1.0 => (show, 170.0),
            Some(f) => (f, 120.0),
            None => (0.0, 0.0),
        };
        // cruising: a slow swing either side of centre, from where he was
        let cruise = |from: f32, phase: f32| mid + (start - mid) * (phase + (e - from) * 0.3).sin();
        let x = if e < ENCORE_DROP {
            start
        } else if !full {
            cruise(ENCORE_DROP, PI / 2.0)
        } else if e < ENCORE_FLIP {
            start + (mid - start) * ease_in_out_cubic((e - ENCORE_DROP) / (ENCORE_FLIP - ENCORE_DROP))
        } else if e < ENCORE_OFF {
            mid
        } else {
            cruise(ENCORE_OFF, 0.0)
        };
        // riding the words: on top of them, tipped with the wave
        let ride = wave_y - 24.0 + wave(x, time, beat);
        let slope = wave_slope(x, time, beat);
        let (y, tilt) = if e < ENCORE_DROP {
            let f = e / ENCORE_DROP;
            (-120.0 + (ride + 120.0) * f * f, (1.0 - f) * -0.5)
        } else {
            (ride - hop * 4.0 * flip * (1.0 - flip), slope.atan())
        };
        Ride { at: vec2(x, y), feet: vec2(x, y - 0.3 * U), ground: ride.max(y), tilt, flip, trick: trick.is_some() }
    }
}

/// A clicked trick's length, s.
const TRICK: f32 = 0.85;

/// The encore's beats, in seconds: he lands on the wave, starts the
/// backflip, lands it, and starts cruising.
pub const ENCORE_DROP: f32 = 0.9;
const ENCORE_FLIP: f32 = 6.0;
pub const ENCORE_LAND: f32 = 7.0;
const ENCORE_OFF: f32 = 10.6;

/// The credits scroller's wave at `x` (px), and its slope.
pub fn wave(x: f32, t: f32, beat: f32) -> f32 {
    (x * 0.011 + t * 3.2).sin() * (8.0 + 10.0 * beat)
}

fn wave_slope(x: f32, t: f32, beat: f32) -> f32 {
    0.011 * (x * 0.011 + t * 3.2).cos() * (8.0 + 10.0 * beat)
}

/// His speech bubble: a dark box, yellow-rimmed, with a tail to `tip`.
fn balloon(r: Rect, tip: Vec2, a: f32) {
    let fill = Color::new(0.03, 0.025, 0.06, 0.93 * a);
    let pts = ui::chamfer(r, 10.0);
    ui::fill_convex(&pts, fill);
    ui::fill_convex(&pts, pal::alpha(YELLOW, 0.05 * a));
    let bx = tip.x.clamp(r.x + 18.0, r.x + r.w - 30.0);
    let (t0, t1) = (vec2(bx - 8.0, r.y + r.h - 1.0), vec2(bx + 12.0, r.y + r.h - 1.0));
    draw_triangle(t0, t1, tip, fill);
    ui::polyline(&pts, 1.4, pal::alpha(YELLOW, 0.9 * a), true);
    ui::line(t0, tip, 1.4, pal::alpha(YELLOW, 0.9 * a));
    ui::line(t1, tip, 1.4, pal::alpha(YELLOW, 0.9 * a));
    draw_line(t0.x + 1.0, t0.y, t1.x - 1.0, t1.y, 2.0, Color::new(0.03, 0.025, 0.06, a));
}

/// His ruled paper as a surfboard, centred at `c`, tipped by `tilt`;
/// `turn` (1..-1) is how far it has rolled over.
fn board(c: Vec2, tilt: f32, turn: f32, a: f32, time: f32) {
    let (s, co) = tilt.sin_cos();
    let along = vec2(co, s);
    let up = vec2(s, -co);
    let half = 3.4 * U;
    let thick = 0.45 * U * turn.abs().max(0.12);
    let nose = |k: f32| c + along * (half * k);
    let pts = [
        nose(-1.0),
        nose(-0.85) - up * thick,
        nose(0.75) - up * thick,
        nose(1.08) - up * thick * 0.2,
        nose(0.85) + up * thick,
        nose(-0.85) + up * thick,
    ];
    let amber = Color::new(1.0, 0.86, 0.35, 1.0);
    ui::fill_convex(&pts, Color::new(0.08, 0.06, 0.01, 0.9 * a));
    ui::fill_convex(&pts, pal::alpha(amber, 0.12 * a));
    for k in 1..5 {
        let x = -0.8 + 1.6 * k as f32 / 5.0;
        ui::line(nose(x) - up * thick * 0.8, nose(x) + up * thick * 0.8, 1.0, pal::alpha(amber, 0.3 * a));
    }
    let rim = pal::mix(amber, pal::MAGENTA, 0.5 + 0.5 * (time * 3.0).sin());
    ui::polyline(&pts, 1.6, pal::alpha(rim, a), true);
    // the fin, underneath
    let side = turn.signum();
    let fb = nose(-0.6) + up * thick * side;
    ui::line(fb, fb + up * 0.6 * U * side - along * 0.3 * U, 2.0, pal::alpha(amber, 0.8 * a));
}

const YELLOW: Color = Color::new(1.0, 0.93, 0.45, 1.0);

/// How he's drawn: all of it fades with `a`.
struct Pose {
    a: f32,
    /// tipped about his feet
    tilt: f32,
    /// turned about his middle (a backflip)
    flip: f32,
    /// the shades above their place on his eyes, in wire units
    shades: f32,
    /// eyebrows up
    raise: f32,
    /// the music's pulse: he bounces to it
    beat: f32,
    /// standing on his ruled paper
    paper: bool,
}

impl Default for Pose {
    fn default() -> Pose {
        Pose { a: 1.0, tilt: 0.0, flip: 0.0, shades: 0.0, raise: 0.0, beat: 0.0, paper: false }
    }
}

/// Clippy himself, his feet at `feet`.
fn figure(feet: Vec2, time: f32, p: &Pose) {
    let a = p.a;
    // idle: a bob, a sway, and the top of the wire leaning in
    let bob = (time * 2.1).sin() * 2.0;
    let lean = 0.14 + 0.06 * (time * 1.3).sin();
    let squash = 1.0 + 0.08 * (time * 0.9).cos() + 0.12 * p.beat;
    let stretch = 1.08 * (1.0 - 0.1 * p.beat);
    let mid = vec2(0.0, -TALL * 0.45);
    let (fs, fc) = p.flip.sin_cos();
    let (ts, tc) = p.tilt.sin_cos();
    // the wire's own frame: x right, y up, feet at y = -4.8
    let pt = |q: Vec2| -> Vec2 {
        let mut q = q;
        if q.y > 0.4 {
            let (s, c) = (-(lean * (q.y - 0.4) / 4.4)).sin_cos();
            let d = vec2(q.x, q.y - 0.4);
            q = vec2(d.x * c - d.y * s, d.x * s + d.y * c + 0.4);
        }
        let v = vec2(q.x * U * squash, -(q.y + 4.8) * U * stretch + bob);
        let d = v - mid;
        let v = mid + vec2(d.x * fc - d.y * fs, d.x * fs + d.y * fc);
        feet + vec2(v.x * tc - v.y * ts, v.x * ts + v.y * tc)
    };
    if p.paper {
        // the paper behind him, ruled, its far edge curling in the draught
        let flap = (time * 1.7).sin() * 0.25;
        let corners = [vec2(-2.9, -5.2), vec2(2.5, -5.9), vec2(3.4 + flap, 4.3), vec2(-1.8 + flap * 0.6, 5.3)];
        let pc: Vec<Vec2> = corners.iter().map(|c| vec2(feet.x + c.x * U, feet.y - (c.y + 4.8) * U + bob * 0.4)).collect();
        let amber = Color::new(1.0, 0.86, 0.35, 1.0);
        ui::fill_convex(&pc, Color::new(0.08, 0.06, 0.01, 0.85 * a));
        ui::fill_convex(&pc, pal::alpha(amber, 0.05 * a));
        for k in 1..9 {
            let f = k as f32 / 9.0;
            let l = pc[0].lerp(pc[3], f);
            let r = pc[1].lerp(pc[2], f);
            ui::line(l.lerp(r, 0.04), l.lerp(r, 0.96), 1.0, pal::alpha(amber, 0.2 * a));
        }
        ui::line(pc[0].lerp(pc[1], 0.16), pc[3].lerp(pc[2], 0.16), 1.0, pal::alpha(pal::MAGENTA, 0.35 * a));
        ui::polyline(&pc, 1.2, pal::alpha(amber, 0.6 * a), true);
    }

    // the wire: a gem clip, legs joined by three U-turns
    let mut path: Vec<Vec2> = Vec::new();
    let leg = |path: &mut Vec<Vec2>, x: f32, y0: f32, y1: f32| {
        for k in 0..=6 {
            path.push(vec2(x, y0 + (y1 - y0) * k as f32 / 6.0));
        }
    };
    let turn = |path: &mut Vec<Vec2>, c: Vec2, r: f32, from: f32, to: f32| {
        for k in 1..=14 {
            let t = from + (to - from) * k as f32 / 14.0;
            path.push(c + vec2(t.cos(), t.sin()) * r);
        }
    };
    leg(&mut path, 0.6, 2.4, -2.6);
    turn(&mut path, vec2(0.0, -2.6), 0.6, 0.0, -PI);
    leg(&mut path, -0.6, -2.6, 4.4);
    turn(&mut path, vec2(0.3, 4.4), 0.9, PI, 0.0);
    leg(&mut path, 1.2, 4.4, -3.6);
    turn(&mut path, vec2(0.0, -3.6), 1.2, 0.0, -PI);
    leg(&mut path, -1.2, -3.6, 1.5);
    let pts: Vec<Vec2> = path.iter().map(|q| pt(*q)).collect();
    let wire = pal::mix(pal::ICE, pal::VIOLET, 0.35);
    for (w, c) in [(10.0, pal::alpha(pal::VIOLET, 0.22 * a)), (4.6, pal::alpha(wire, a)), (1.4, pal::alpha(pal::WHITE, 0.75 * a))] {
        for s in pts.windows(2) {
            let off = if w < 2.0 { vec2(-1.0, -1.0) } else { Vec2::ZERO };
            draw_line(s[0].x + off.x, s[0].y + off.y, s[1].x + off.x, s[1].y + off.y, w, c);
        }
        if w > 4.0 && w < 5.0 {
            for q in &pts {
                draw_circle(q.x, q.y, w / 2.0, c);
            }
        }
    }

    let dy = p.shades;
    let eyes = [vec2(-0.75, 2.7), vec2(1.1, 2.25)];
    let er = 0.72;
    let blink = if (time % 4.3) < 0.12 { 0.15 } else { 1.0 };
    let look = vec2(0.25, 0.15);
    // (behind the shades they aren't drawn, so none of the white shows round them)
    if dy.abs() > 0.05 {
        for e in eyes {
            let c = pt(e);
            draw_ellipse(c.x, c.y, er * U, er * U * blink, p.flip + p.tilt, pal::alpha(pal::WHITE, 0.92 * a));
            ui::ring(c, er * U, 1.2, pal::alpha(wire, a), 24);
            let q = c + vec2(look.x, -look.y) * U * 0.9;
            if blink > 0.5 {
                draw_circle(q.x, q.y, 0.36 * U, Color::new(0.02, 0.0, 0.05, a));
                draw_circle(q.x - 1.5, q.y - 1.5, 1.2, pal::alpha(pal::WHITE, a));
            }
        }
    }
    // eyebrows
    let raise = p.raise + 0.05 * (time * 3.1).sin();
    for (k, e) in eyes.iter().enumerate() {
        let tilt = if k == 0 { 0.35 } else { -0.15 };
        let base = *e + vec2(0.0, 1.2 + raise + if k == 0 { 0.15 } else { 0.0 });
        let arc: Vec<Vec2> = (0..=8)
            .map(|j| {
                let f = j as f32 / 8.0 * 2.0 - 1.0;
                pt(base + vec2(f * 0.9, (1.0 - f * f) * 0.35 + f * tilt * 0.4))
            })
            .collect();
        // thick in the middle, thin at the ends
        for (j, s) in arc.windows(2).enumerate() {
            let w = 2.5 + 3.0 * (1.0 - ((j as f32 + 0.5) / 4.0 - 1.0).abs());
            draw_line(s[0].x, s[0].y, s[1].x, s[1].y, w, pal::alpha(pal::WHITE, a));
            draw_circle(s[1].x, s[1].y, w / 2.0, pal::alpha(pal::WHITE, a));
        }
    }
    // the shades: flat across the top, wider than tall, the outer corners
    // swept back
    let lens = |c: Vec2, outer_left: bool| -> [Vec2; 6] {
        let c = c + vec2(0.0, dy);
        let (l, r) = if outer_left { (-1.15, 0.85) } else { (-0.85, 1.15) };
        let (cut_l, cut_r) = if outer_left { (0.6, 0.2) } else { (0.2, 0.6) };
        [
            pt(c + vec2(l, 0.5)),
            pt(c + vec2(r, 0.5)),
            pt(c + vec2(r, -0.5 + cut_r)),
            pt(c + vec2(r - cut_r, -0.5)),
            pt(c + vec2(l + cut_l, -0.5)),
            pt(c + vec2(l, -0.5 + cut_l)),
        ]
    };
    let ls = [lens(eyes[0], true), lens(eyes[1], false)];
    let dark = Color::new(0.01, 0.0, 0.03, 0.96 * a);
    // the bar across the top, then the lenses
    ui::line(ls[0][0], ls[1][1], 3.5, pal::alpha(pal::mix(pal::MAGENTA, pal::WHITE, 0.3), a));
    for (k, q) in ls.iter().enumerate() {
        ui::fill_convex(q, dark);
        // scanlines and a glint sweeping across
        for f in [0.3, 0.55, 0.8] {
            let l = q[0].lerp(q[5], f);
            let r = q[1].lerp(q[2], f);
            ui::line(l, r, 1.0, pal::alpha(pal::CYAN, 0.18 * a));
        }
        let sweep = (time * 0.45 + k as f32 * 0.12) % 1.6 - 0.3;
        if (0.0..1.0).contains(&sweep) {
            let top = q[0].lerp(q[1], sweep);
            let bot = q[5].lerp(q[2], (sweep - 0.25).clamp(0.0, 1.0));
            ui::line(top, bot, 2.0, pal::alpha(pal::WHITE, 0.55 * a));
        }
        ui::polyline(q, 1.6, pal::alpha(pal::MAGENTA, a), true);
    }
    // the arms, back to the wire
    ui::line(ls[0][0], pt(eyes[0] + vec2(-1.5, 0.55 + dy)), 2.0, pal::alpha(pal::MAGENTA, a));
    ui::line(ls[1][1], pt(eyes[1] + vec2(1.2, 0.6 + dy)), 2.0, pal::alpha(pal::MAGENTA, a));
}

/// How far down his nose the shades are, `t` seconds into a peek.
fn peek_window(t: f32) -> f32 {
    if !(0.0..1.6).contains(&t) {
        return 0.0;
    }
    ease_out_cubic(clamp01(t / 0.2)) * (1.0 - ease_out_cubic(clamp01((t - 1.3) / 0.3)))
}
