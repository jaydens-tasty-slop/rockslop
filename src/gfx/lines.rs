//! Batches of glow line segments (see shaders::LINE_VS for the encoding).

use macroquad::prelude::*;

const MAX_SEGS: usize = 12_000; // per mesh: 48k vertices, 72k u16 indices

#[derive(Default, Clone)]
pub struct LineBatch {
    pub verts: Vec<Vertex>,
    pub idx: Vec<u16>,
    chunks: Vec<(usize, usize)>, // (vertex start, index start) of each mesh chunk
}

#[inline]
fn c8(c: Color) -> [u8; 4] {
    [
        (c.r.clamp(0.0, 1.0) * 255.0) as u8,
        (c.g.clamp(0.0, 1.0) * 255.0) as u8,
        (c.b.clamp(0.0, 1.0) * 255.0) as u8,
        (c.a.clamp(0.0, 1.0) * 255.0) as u8,
    ]
}

impl LineBatch {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn clear(&mut self) {
        self.verts.clear();
        self.idx.clear();
        self.chunks.clear();
    }

    fn base(&mut self) -> u16 {
        let segs = self.verts.len() / 4;
        if segs % MAX_SEGS == 0 {
            self.chunks.push((self.verts.len(), self.idx.len()));
        }
        let (v0, _) = *self.chunks.last().unwrap();
        (self.verts.len() - v0) as u16
    }

    /// One segment a-b. `w` is the half width in px, caps on open ends.
    pub fn seg_ex(&mut self, a: Vec3, b: Vec3, ca: Color, cb: Color, w: f32, cap_a: bool, cap_b: bool) {
        let i = self.base();
        let (xa, xb) = (if cap_a { 2.0 } else { 0.0 }, if cap_b { 3.0 } else { 1.0 });
        let (ka, kb) = (c8(ca), c8(cb));
        let na = b.extend(w);
        let nb = a.extend(w);
        self.verts.push(Vertex { position: a, uv: vec2(xa, -1.0), color: ka, normal: na });
        self.verts.push(Vertex { position: a, uv: vec2(xa, 1.0), color: ka, normal: na });
        self.verts.push(Vertex { position: b, uv: vec2(xb, 1.0), color: kb, normal: nb });
        self.verts.push(Vertex { position: b, uv: vec2(xb, -1.0), color: kb, normal: nb });
        self.idx.extend_from_slice(&[i, i + 1, i + 2, i, i + 2, i + 3]);
    }

    pub fn seg(&mut self, a: Vec3, b: Vec3, c: Color, w: f32) {
        self.seg_ex(a, b, c, c, w, true, true);
    }

    pub fn point(&mut self, p: Vec3, c: Color, w: f32) {
        self.seg_ex(p, p, c, c, w, true, true);
    }

    /// Polyline; `closed` joins the last point to the first.
    pub fn poly(&mut self, pts: &[Vec3], c: Color, w: f32, closed: bool) {
        let n = pts.len();
        if n < 2 {
            return;
        }
        let segs = if closed { n } else { n - 1 };
        for i in 0..segs {
            let a = pts[i];
            let b = pts[(i + 1) % n];
            let cap_a = !closed && i == 0;
            let cap_b = !closed && i == segs - 1;
            self.seg_ex(a, b, c, c, w, cap_a, cap_b);
        }
    }

    /// Polyline with a colour per point.
    pub fn poly_colors(&mut self, pts: &[(Vec3, Color)], w: f32, closed: bool) {
        let n = pts.len();
        if n < 2 {
            return;
        }
        let segs = if closed { n } else { n - 1 };
        for i in 0..segs {
            let (a, ca) = pts[i];
            let (b, cb) = pts[(i + 1) % n];
            self.seg_ex(a, b, ca, cb, w, !closed && i == 0, !closed && i == segs - 1);
        }
    }

    /// Draw with whatever line material is active.
    pub fn draw(&self) {
        for (k, &(v0, i0)) in self.chunks.iter().enumerate() {
            let (v1, i1) = self.chunks.get(k + 1).copied().unwrap_or((self.verts.len(), self.idx.len()));
            let mesh = Mesh { vertices: self.verts[v0..v1].to_vec(), indices: self.idx[i0..i1].to_vec(), texture: None };
            draw_mesh(&mesh);
        }
    }
}

/// Solid triangles (for the holographic body fill).
#[derive(Default, Clone)]
pub struct TriBatch {
    pub verts: Vec<Vertex>,
    pub idx: Vec<u16>,
}

impl TriBatch {
    pub fn vert(&mut self, p: Vec3, n: Vec3, c: Color) -> u16 {
        self.verts.push(Vertex { position: p, uv: Vec2::ZERO, color: c8(c), normal: n.extend(0.0) });
        (self.verts.len() - 1) as u16
    }
    pub fn tri(&mut self, a: u16, b: u16, c: u16) {
        self.idx.extend_from_slice(&[a, b, c]);
    }
    pub fn quad(&mut self, a: u16, b: u16, c: u16, d: u16) {
        self.idx.extend_from_slice(&[a, b, c, a, c, d]);
    }
    pub fn mesh(&self) -> Mesh {
        Mesh { vertices: self.verts.clone(), indices: self.idx.clone(), texture: None }
    }
}
