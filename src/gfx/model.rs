//! Procedural wireframe + holographic shell of the two players, built from
//! measurements of the reference photos. Units are centimetres, origin at
//! the body centre, +y up, +z out of the screen, +x to the player's right.

use super::lines::{LineBatch, TriBatch};
use super::pal;
use macroquad::prelude::*;
use std::f32::consts::{FRAC_PI_2, PI, TAU};

pub use rockslop::device::Kind;

/// Things the HUD points at, and the cable plugs into.
#[derive(Clone, Copy, PartialEq, Eq, Debug, Hash)]
pub enum Feature {
    Usb,
    Headphone,
    Reset,
    Sd,
    Vol,
    Power,
    Down,
    Up,
    Select,
    Screen,
    Speaker,
}

#[derive(Clone, Copy, PartialEq, Debug)]
enum Part {
    Edge,  // main silhouette loops
    Minor, // bevel / longitudinal wires
    Face,  // screen glass, panel lines
    Wheel,
    Glyph,
    Port,
    Button(Feature),
}

struct Poly {
    pts: Vec<Vec3>,
    closed: bool,
    part: Part,
}

#[derive(Clone, Copy)]
pub struct Anchor {
    pub feature: Feature,
    pub pos: Vec3,
    pub normal: Vec3,
}

pub struct Player {
    pub kind: Kind,
    polys: Vec<Poly>,
    pub fill: TriBatch,
    pub anchors: Vec<Anchor>,
    pub lcd_center: Vec3,
    pub lcd_size: Vec2,
    pub dims: Vec3,
    /// y of the two-tone split (TS17), in local units
    pub split: Option<f32>,
}

pub struct Style {
    pub top: Color,
    pub bottom: Color,
    pub accent: Color,
    pub intensity: f32,
    /// 0..1: a scan plane sweeps down the model, materialising it
    pub reveal: f32,
    pub time: f32,
    /// features to make throb, with strength 0..1
    pub highlight: Vec<(Feature, f32)>,
    pub width: f32,
}

// ---------------------------------------------------------------- helpers

/// Rounded rectangle outline, CCW from the right-middle; `seg` per corner.
fn rrect(hw: f32, hh: f32, r: f32, seg: usize) -> Vec<(Vec2, Vec2)> {
    let r = r.min(hw).min(hh).max(0.0);
    let mut out = Vec::new();
    let corners =
        [(vec2(hw - r, hh - r), 0.0), (vec2(-hw + r, hh - r), FRAC_PI_2), (vec2(-hw + r, -hh + r), PI), (vec2(hw - r, -hh + r), 1.5 * PI)];
    for (c, a0) in corners {
        for i in 0..=seg {
            let a = a0 + FRAC_PI_2 * i as f32 / seg as f32;
            let n = vec2(a.cos(), a.sin());
            out.push((c + n * r, n));
        }
    }
    out
}

fn circle(c: Vec3, r: f32, n: usize, axis: u8) -> Vec<Vec3> {
    (0..n)
        .map(|i| {
            let a = TAU * i as f32 / n as f32;
            let (s, co) = (a.sin() * r, a.cos() * r);
            match axis {
                0 => c + vec3(0.0, co, s), // in the yz plane (side faces)
                1 => c + vec3(co, 0.0, s), // in the xz plane (top/bottom)
                _ => c + vec3(co, s, 0.0), // in the xy plane (front/back)
            }
        })
        .collect()
}

fn arc(c: Vec3, r: f32, a0: f32, a1: f32, n: usize) -> Vec<Vec3> {
    (0..=n)
        .map(|i| {
            let a = a0 + (a1 - a0) * i as f32 / n as f32;
            c + vec3(a.cos() * r, a.sin() * r, 0.0)
        })
        .collect()
}

/// A stadium (pill) outline in a plane: `u`,`v` are the plane axes.
fn stadium(c: Vec3, u: Vec3, v: Vec3, len: f32, thick: f32, n: usize) -> Vec<Vec3> {
    let r = thick / 2.0;
    let half = (len / 2.0 - r).max(0.0);
    let mut out = Vec::new();
    for i in 0..=n {
        let a = -FRAC_PI_2 + PI * i as f32 / n as f32;
        out.push(c + u * (half + a.cos() * r) + v * (a.sin() * r));
    }
    for i in 0..=n {
        let a = FRAC_PI_2 + PI * i as f32 / n as f32;
        out.push(c + u * (-half + a.cos() * r) + v * (a.sin() * r));
    }
    out
}

impl Player {
    pub fn new(kind: Kind) -> Player {
        let (w, h, d) = (3.97, 8.89, 0.95);
        let corner = if kind == Kind::Yp3 { 0.6 } else { 0.5 };
        let bevel = 0.3;
        let mut polys: Vec<Poly> = Vec::new();
        let mut fill = TriBatch::default();
        let mut anchors = Vec::new();
        let (hw, hh, hd) = (w / 2.0, h / 2.0, d / 2.0);
        let split = if kind == Kind::Ts17 { Some(hh - 0.535 * h) } else { None };

        // --- shell: loops through the bevel cross-section
        let arc_steps = 6;
        let mut cross: Vec<(f32, f32, Vec2)> = Vec::new(); // (outward offset, z, (n_o, n_z))
        for i in 0..=arc_steps {
            let t = FRAC_PI_2 * i as f32 / arc_steps as f32;
            cross.push((-bevel + bevel * t.sin(), hd - bevel + bevel * t.cos(), vec2(t.sin(), t.cos())));
        }
        for i in (0..=arc_steps).rev() {
            let t = FRAC_PI_2 * i as f32 / arc_steps as f32;
            cross.push((-bevel + bevel * t.sin(), -(hd - bevel) - bevel * t.cos(), vec2(t.sin(), -t.cos())));
        }
        let seg = 7;
        let base = rrect(hw, hh, corner, seg);
        let loop_at = |o: f32| -> Vec<(Vec2, Vec2)> { rrect(hw + o, hh + o, corner + o, seg) };
        let mut rings: Vec<Vec<u16>> = Vec::new();
        for (k, &(o, z, n2)) in cross.iter().enumerate() {
            let ring = loop_at(o);
            let mut ids = Vec::new();
            for &(p, n) in &ring {
                let nn = vec3(n.x * n2.x, n.y * n2.x, n2.y).normalize();
                ids.push(fill.vert(vec3(p.x, p.y, z), nn, WHITE));
            }
            rings.push(ids);
            let wire = matches!(k, 0 | 3 | 6) || k == cross.len() - 1 || k == cross.len() - 4 || k == cross.len() - 7;
            if wire {
                let edge = k == 0 || k == cross.len() - 1 || k == arc_steps || k == arc_steps + 1;
                polys.push(Poly {
                    pts: ring.iter().map(|(p, _)| vec3(p.x, p.y, z)).collect(),
                    closed: true,
                    part: if edge { Part::Edge } else { Part::Minor },
                });
            }
        }
        for k in 0..rings.len() - 1 {
            let (a, b) = (&rings[k], &rings[k + 1]);
            for i in 0..a.len() {
                let j = (i + 1) % a.len();
                fill.quad(a[i], a[j], b[j], b[i]);
            }
        }
        // front and back caps
        for (ring, z, nz) in [(&rings[0], hd, 1.0f32), (rings.last().unwrap(), -hd, -1.0)] {
            let c = fill.vert(vec3(0.0, 0.0, z), vec3(0.0, 0.0, nz), WHITE);
            let _ = ring;
            let edge = loop_at(-bevel);
            let ids: Vec<u16> = edge.iter().map(|(p, _)| fill.vert(vec3(p.x, p.y, z), vec3(0.0, 0.0, nz), WHITE)).collect();
            for i in 0..ids.len() {
                fill.tri(c, ids[i], ids[(i + 1) % ids.len()]);
            }
        }
        // longitudinal wires at corner arc samples and edge midpoints
        for (i, _) in base.iter().enumerate() {
            let in_corner = i % (seg + 1);
            if !(in_corner == 0 || in_corner == seg || in_corner == seg / 2 + 1) {
                continue;
            }
            let pts: Vec<Vec3> = cross
                .iter()
                .map(|&(o, z, _)| {
                    let r = loop_at(o);
                    vec3(r[i].0.x, r[i].0.y, z)
                })
                .collect();
            polys.push(Poly { pts, closed: false, part: Part::Minor });
        }
        // two-tone split ring (TS17)
        if let Some(sy) = split {
            let mut pts = Vec::new();
            for &(o, z, _) in cross.iter() {
                pts.push(vec3(hw + o, sy, z));
            }
            for &(o, z, _) in cross.iter().rev() {
                pts.push(vec3(-hw - o, sy, z));
            }
            polys.push(Poly { pts, closed: true, part: Part::Edge });
        }

        // --- front face details
        let zf = hd + 0.004;
        let (glass_c, glass_hw, glass_hh, lcd_size) = match kind {
            Kind::Yp3 => (vec2(0.0, hh - 0.33 - 2.18), 1.68, 2.18, vec2(2.72, 3.4)),
            Kind::Ts17 => (vec2(0.0, hh - 0.18 - 2.2), 1.68, 2.2, vec2(2.72, 3.36)),
        };
        let glass_rr = rrect(glass_hw, glass_hh, 0.22, 4);
        polys.push(Poly {
            pts: glass_rr.iter().map(|(p, _)| vec3(p.x + glass_c.x, p.y + glass_c.y, zf)).collect(),
            closed: true,
            part: Part::Face,
        });
        let lcd_center = match kind {
            Kind::Yp3 => vec3(0.0, glass_c.y + 0.25, zf + 0.002),
            Kind::Ts17 => vec3(0.0, glass_c.y + 0.14, zf + 0.002),
        };
        // LCD frame
        let lr = rrect(lcd_size.x / 2.0 + 0.05, lcd_size.y / 2.0 + 0.05, 0.03, 1);
        polys.push(Poly { pts: lr.iter().map(|(p, _)| vec3(p.x, p.y, 0.0) + lcd_center).collect(), closed: true, part: Part::Face });
        if kind == Kind::Ts17 {
            // "HiFi Player" label line under the LCD
            let y = lcd_center.y - lcd_size.y / 2.0 - 0.38;
            polys.push(Poly { pts: vec![vec3(-0.75, y, zf), vec3(0.45, y, zf)], closed: false, part: Part::Glyph });
            polys.push(Poly { pts: vec![vec3(0.62, y, zf), vec3(0.9, y, zf)], closed: false, part: Part::Glyph });
        }
        // wheel
        let (wheel_y, wheel_r, btn_r) = match kind {
            Kind::Yp3 => (hh - 0.759 * h, 1.32, 0.53),
            Kind::Ts17 => (hh - 0.757 * h, 1.45, 0.5),
        };
        let wc = vec3(0.0, wheel_y, zf);
        polys.push(Poly { pts: circle(wc, wheel_r, 72, 2), closed: true, part: Part::Wheel });
        polys.push(Poly { pts: circle(wc, wheel_r - 0.06, 72, 2), closed: true, part: Part::Minor });
        polys.push(Poly { pts: circle(wc, btn_r, 40, 2), closed: true, part: Part::Button(Feature::Select) });
        // glyphs on the ring
        let gr = (wheel_r + btn_r) / 2.0 + 0.03;
        let s = 0.13;
        let glyph = |pts: &[(f32, f32)], c: Vec3| -> Vec<Vec3> { pts.iter().map(|&(x, y)| c + vec3(x * s, y * s, 0.0)).collect() };
        let mut glyphs: Vec<Vec<Vec3>> = Vec::new();
        let up = wc + vec3(0.0, gr, 0.0);
        glyphs.push(glyph(&[(-0.8, -1.0), (-0.8, 1.0), (0.0, 0.0), (0.8, 1.0), (0.8, -1.0)], up));
        for (dirx, c) in [(1.0f32, wc + vec3(gr, 0.0, 0.0)), (-1.0, wc + vec3(-gr, 0.0, 0.0))] {
            // |<< and >>| : two triangles and a bar
            let t1 = [(-1.1, 0.8), (-0.1, 0.0), (-1.1, -0.8)];
            let t2 = [(-0.1, 0.8), (0.9, 0.0), (-0.1, -0.8)];
            let bar = [(1.1, 0.8), (1.1, -0.8)];
            for t in [&t1[..], &t2[..], &bar[..]] {
                let pts: Vec<(f32, f32)> = t.iter().map(|&(x, y)| (x * dirx, y)).collect();
                glyphs.push(glyph(&pts, c));
            }
        }
        let down = wc + vec3(0.0, -gr, 0.0);
        if kind == Kind::Yp3 {
            // VOL
            let k = 0.5;
            glyphs.push(glyph(&[(-2.6, k), (-2.0, -k), (-1.4, k)], down));
            let o: Vec<(f32, f32)> = (0..=12)
                .map(|i| {
                    let a = TAU * i as f32 / 12.0;
                    (a.cos() * 0.5, a.sin() * k)
                })
                .collect();
            glyphs.push(glyph(&o, down));
            glyphs.push(glyph(&[(1.3, k), (1.3, -k), (2.3, -k)], down));
            // play/pause in the centre
            glyphs.push(glyph(&[(-0.9, 0.7), (0.1, 0.0), (-0.9, -0.7), (-0.9, 0.7)], wc));
            glyphs.push(glyph(&[(0.5, 0.7), (0.5, -0.7)], wc));
            glyphs.push(glyph(&[(0.9, 0.7), (0.9, -0.7)], wc));
        } else {
            glyphs.push(glyph(&[(-0.35, 0.0), (0.35, 0.0)], down));
            glyphs.push(glyph(&[(-0.2, 0.2), (0.2, 0.2)], wc));
        }
        for g in glyphs {
            polys.push(Poly { pts: g, closed: false, part: Part::Glyph });
        }
        anchors.push(Anchor { feature: Feature::Down, pos: down, normal: Vec3::Z });
        anchors.push(Anchor { feature: Feature::Up, pos: up, normal: Vec3::Z });
        anchors.push(Anchor { feature: Feature::Select, pos: wc, normal: Vec3::Z });
        anchors.push(Anchor { feature: Feature::Screen, pos: lcd_center + vec3(lcd_size.x * 0.5, lcd_size.y * 0.3, 0.0), normal: Vec3::Z });
        // quarter-arc segments of the ring: the D-pad zones (for highlights)
        let quarter = |a: f32| arc(wc, wheel_r - 0.14, a - 0.55, a + 0.55, 14);
        polys.push(Poly { pts: quarter(-FRAC_PI_2), closed: false, part: Part::Button(Feature::Down) });
        polys.push(Poly { pts: quarter(FRAC_PI_2), closed: false, part: Part::Button(Feature::Up) });

        // --- bottom edge ports
        let yb = -hh - 0.004;
        let (usb_x, jack_x) = match kind {
            Kind::Yp3 => (0.85, -0.95),
            Kind::Ts17 => (-1.0, 1.0),
        };
        polys.push(Poly { pts: stadium(vec3(usb_x, yb, 0.0), Vec3::X, Vec3::Z, 0.9, 0.32, 8), closed: true, part: Part::Port });
        polys.push(Poly { pts: stadium(vec3(usb_x, yb, 0.0), Vec3::X, Vec3::Z, 0.66, 0.12, 6), closed: true, part: Part::Minor });
        anchors.push(Anchor { feature: Feature::Usb, pos: vec3(usb_x, -hh, 0.0), normal: -Vec3::Y });
        polys.push(Poly { pts: circle(vec3(jack_x, yb, 0.0), 0.2, 20, 1), closed: true, part: Part::Port });
        polys.push(Poly { pts: circle(vec3(jack_x, yb, 0.0), 0.12, 16, 1), closed: true, part: Part::Minor });
        anchors.push(Anchor { feature: Feature::Headphone, pos: vec3(jack_x, -hh, 0.0), normal: -Vec3::Y });

        match kind {
            Kind::Yp3 => {
                // reset pinhole on the right side, microSD slot on the left
                let ry = hh - 0.72 * h;
                polys.push(Poly { pts: circle(vec3(hw + 0.004, ry, 0.0), 0.07, 14, 0), closed: true, part: Part::Button(Feature::Reset) });
                polys.push(Poly { pts: circle(vec3(hw + 0.004, ry, 0.0), 0.16, 18, 0), closed: true, part: Part::Minor });
                anchors.push(Anchor { feature: Feature::Reset, pos: vec3(hw, ry, 0.0), normal: Vec3::X });
                polys.push(Poly {
                    pts: stadium(vec3(-hw - 0.004, ry, 0.0), Vec3::Y, Vec3::Z, 1.25, 0.16, 5),
                    closed: true,
                    part: Part::Port,
                });
                anchors.push(Anchor { feature: Feature::Sd, pos: vec3(-hw, ry, 0.0), normal: -Vec3::X });
                // speaker grille on the back
                let sc = vec3(0.0, hh - 0.145 * h, -hd - 0.004);
                for gy in -4..=4 {
                    for gx in -4..=4 {
                        let p = vec2(gx as f32, gy as f32) * 0.115;
                        if p.length() < 0.5 {
                            polys.push(Poly { pts: circle(sc + vec3(p.x, p.y, 0.0), 0.022, 6, 2), closed: true, part: Part::Minor });
                        }
                    }
                }
                anchors.push(Anchor { feature: Feature::Speaker, pos: sc, normal: -Vec3::Z });
            }
            Kind::Ts17 => {
                // power button between the ports
                let pb = stadium(vec3(0.0, -hh - 0.05, 0.0), Vec3::X, Vec3::Z, 0.5, 0.22, 6);
                polys.push(Poly { pts: pb, closed: true, part: Part::Button(Feature::Power) });
                polys.push(Poly {
                    pts: stadium(vec3(0.0, -hh - 0.004, 0.0), Vec3::X, Vec3::Z, 0.6, 0.3, 6),
                    closed: true,
                    part: Part::Minor,
                });
                anchors.push(Anchor { feature: Feature::Power, pos: vec3(0.0, -hh - 0.05, 0.0), normal: -Vec3::Y });
                // volume rocker on the right side
                for (cy, _) in [(hh - 0.68 * h, 0), (hh - 0.79 * h, 1)] {
                    let p = stadium(vec3(hw + 0.05, cy, 0.0), Vec3::Y, Vec3::Z, 0.75, 0.3, 6);
                    polys.push(Poly { pts: p, closed: true, part: Part::Button(Feature::Vol) });
                }
                anchors.push(Anchor { feature: Feature::Vol, pos: vec3(hw + 0.05, hh - 0.735 * h, 0.0), normal: Vec3::X });
                // SD slot on the left side
                let sy = hh - 0.70 * h;
                polys.push(Poly {
                    pts: stadium(vec3(-hw - 0.004, sy, 0.0), Vec3::Y, Vec3::Z, 0.9, 0.14, 5),
                    closed: true,
                    part: Part::Port,
                });
                anchors.push(Anchor { feature: Feature::Sd, pos: vec3(-hw, sy, 0.0), normal: -Vec3::X });
                // reset pinhole on the back near the bottom
                let rp = vec3(0.0, hh - 0.93 * h, -hd - 0.004);
                polys.push(Poly { pts: circle(rp, 0.06, 12, 2), closed: true, part: Part::Button(Feature::Reset) });
                anchors.push(Anchor { feature: Feature::Reset, pos: rp, normal: -Vec3::Z });
            }
        }

        Player { kind, polys, fill, anchors, lcd_center, lcd_size, dims: vec3(w, h, d), split }
    }

    pub fn anchor(&self, f: Feature) -> Option<Anchor> {
        self.anchors.iter().copied().find(|a| a.feature == f)
    }

    /// Emit this frame's wires into `batch` (local space).
    pub fn emit(&self, batch: &mut LineBatch, st: &Style) {
        let hh = self.dims.y / 2.0;
        // the scan plane, from top (reveal 0) to bottom (1), with overshoot
        let scan_y = hh + 0.4 - st.reveal * (self.dims.y + 0.8);
        let hl = |f: Feature| st.highlight.iter().find(|(g, _)| *g == f).map(|x| x.1).unwrap_or(0.0);
        for p in &self.polys {
            let (k, wmul) = match p.part {
                Part::Edge => (1.0, 1.25),
                Part::Minor => (0.32, 0.8),
                Part::Face => (0.75, 1.0),
                Part::Wheel => (0.9, 1.1),
                Part::Glyph => (0.8, 0.85),
                Part::Port => (0.9, 1.0),
                Part::Button(_) => (0.9, 1.0),
            };
            let mut boost = 0.0;
            let mut accent = 0.0;
            if let Part::Button(f) = p.part {
                let s = hl(f);
                if s > 0.0 {
                    let throb = 0.5 + 0.5 * (st.time * 7.0).sin();
                    boost = s * (1.2 + 1.3 * throb);
                    accent = s;
                }
            }
            if matches!(p.part, Part::Port) {
                let f = self
                    .anchors
                    .iter()
                    .filter(|a| matches!(a.feature, Feature::Usb | Feature::Sd | Feature::Headphone))
                    .min_by(|a, b| a.pos.distance(p.pts[0]).total_cmp(&b.pos.distance(p.pts[0])));
                if let Some(a) = f {
                    let s = hl(a.feature);
                    if s > 0.0 {
                        boost = s * (1.0 + (st.time * 7.0).sin().max(0.0));
                        accent = s;
                    }
                }
            }
            let n = p.pts.len();
            let segs = if p.closed { n } else { n.saturating_sub(1) };
            for i in 0..segs {
                let a = p.pts[i];
                let b = p.pts[(i + 1) % n];
                // materialise: only above the scan plane
                let top = a.y.max(b.y);
                if top < scan_y {
                    continue;
                }
                let base = |q: Vec3| -> Color {
                    let c = match self.split {
                        Some(sy) if q.y < sy - 0.001 => st.bottom,
                        _ => st.top,
                    };
                    let c = pal::mix(c, st.accent, accent);
                    let near = 1.0 - ((q.y - scan_y).abs() / 0.35).min(1.0);
                    let c = pal::mix(c, WHITE, near * (1.0 - st.reveal.floor().min(1.0)));
                    pal::scale(c, (k + boost) * st.intensity)
                };
                let (ca, cb) = (base(a), base(b));
                let cap_a = !p.closed && i == 0;
                let cap_b = !p.closed && i + 1 == segs;
                batch.seg_ex(a, b, ca, cb, st.width * wmul * (1.0 + boost * 0.35), cap_a, cap_b);
            }
        }
        // throbbing rings around highlighted features
        for &(f, s) in &st.highlight {
            if s <= 0.0 {
                continue;
            }
            if let Some(a) = self.anchor(f) {
                if matches!(f, Feature::Screen) {
                    continue;
                }
                for k in 0..3 {
                    let ph = ((st.time * 0.9 + k as f32 / 3.0) % 1.0) as f32;
                    let r = 0.15 + ph * 0.9;
                    let c = pal::scale(st.accent, s * (1.0 - ph) * 1.6);
                    let pts = ring_facing(a.pos + a.normal * 0.01, a.normal, r, 40);
                    batch.poly(&pts, c, st.width * 0.9, true);
                }
            }
        }
    }

    /// Model matrix for a pose.
    pub fn matrix(pos: Vec3, rot: Quat, scale: f32) -> Mat4 {
        Mat4::from_scale_rotation_translation(Vec3::splat(scale), rot, pos)
    }
}

/// A ring of radius r around `c`, in the plane facing `n`.
pub fn ring_facing(c: Vec3, n: Vec3, r: f32, segs: usize) -> Vec<Vec3> {
    let n = n.normalize();
    let t = if n.y.abs() < 0.9 { n.cross(Vec3::Y).normalize() } else { n.cross(Vec3::X).normalize() };
    let b = n.cross(t);
    (0..segs)
        .map(|i| {
            let a = TAU * i as f32 / segs as f32;
            c + (t * a.cos() + b * a.sin()) * r
        })
        .collect()
}

/// USB-C plug and cable. The plug's tip sits at the origin pointing +y
/// (into the port); the cable leaves along -y and bends towards the viewer.
pub struct Cable {
    plug: Vec<Vec<Vec3>>,
    pub spine: Vec<Vec3>,
}

impl Cable {
    pub fn new() -> Cable {
        let mut plug = Vec::new();
        // metal shell
        for y in [0.0, -0.32, -0.62] {
            plug.push(stadium(vec3(0.0, y, 0.0), Vec3::X, Vec3::Z, 0.83, 0.26, 6));
        }
        plug.push(stadium(vec3(0.0, -0.02, 0.0), Vec3::X, Vec3::Z, 0.56, 0.07, 4));
        // overmold
        for (y, l, t) in [(-0.66, 1.1, 0.5), (-0.9, 1.18, 0.56), (-1.9, 1.18, 0.56), (-2.45, 1.1, 0.5), (-2.75, 0.55, 0.4)] {
            plug.push(stadium(vec3(0.0, y, 0.0), Vec3::X, Vec3::Z, l, t, 7));
        }
        // longitudinal wires along the overmold
        for (x, z) in [(0.59, 0.0), (-0.59, 0.0), (0.0, 0.28), (0.0, -0.28), (0.31, 0.28), (-0.31, 0.28), (0.31, -0.28), (-0.31, -0.28)] {
            plug.push(vec![vec3(x * 0.93, -0.66, z * 0.9), vec3(x, -0.9, z), vec3(x, -1.9, z), vec3(x * 0.93, -2.45, z * 0.9)]);
        }
        for (x, z) in [(0.415, 0.0), (-0.415, 0.0)] {
            plug.push(vec![vec3(x, 0.0, z), vec3(x, -0.62, z)]);
        }
        // cable: a gentle S towards the viewer and down
        let mut spine = Vec::new();
        let n = 48;
        for i in 0..=n {
            let t = i as f32 / n as f32;
            let y = -2.75 - t * 9.0;
            let z = 1.6 * (t * 2.2).sin() * t + t * t * 5.0;
            let x = 0.9 * (t * 3.0).sin() * t;
            spine.push(vec3(x, y, z));
        }
        Cable { plug, spine }
    }

    /// Emit in plug space. `flow` animates data packets along the cable.
    pub fn emit(&self, batch: &mut LineBatch, color: Color, hot: Color, flow: f32, w: f32, time: f32) {
        for (i, p) in self.plug.iter().enumerate() {
            let closed = p.len() > 2 && p.first() != p.last() && i < 9;
            let k = if i < 4 { 1.0 } else { 0.8 };
            batch.poly(p, pal::scale(color, k), w, closed);
        }
        // the cable as three twisted strands
        for s in 0..3 {
            let ph = s as f32 * TAU / 3.0;
            let pts: Vec<Vec3> = self
                .spine
                .iter()
                .enumerate()
                .map(|(i, &p)| {
                    let a = ph + i as f32 * 0.55;
                    p + vec3(a.cos() * 0.13, 0.0, a.sin() * 0.13)
                })
                .collect();
            let cols: Vec<(Vec3, Color)> = pts
                .iter()
                .enumerate()
                .map(|(i, &p)| {
                    let t = i as f32 / (pts.len() - 1) as f32;
                    (p, pal::scale(color, 0.7 * (1.0 - t * 0.85)))
                })
                .collect();
            batch.poly_colors(&cols, w * 0.8, false);
        }
        // data packets travelling up the cable into the device
        if flow > 0.0 {
            let n = self.spine.len();
            for k in 0..6 {
                let t = 1.0 - ((time * 0.55 + k as f32 / 6.0) % 1.0);
                let f = t * (n - 1) as f32;
                let i = f as usize;
                let p = self.spine[i].lerp(self.spine[(i + 1).min(n - 1)], f.fract());
                batch.point(p, pal::scale(hot, flow * 1.4 * (1.0 - t * 0.7)), w * 3.0);
            }
        }
    }
}
