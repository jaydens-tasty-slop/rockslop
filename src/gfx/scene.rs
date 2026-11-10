//! The world the players float in: neon grids above and below, drifting
//! data motes, distant rotating polyhedra and falling data streaks.

use super::lines::LineBatch;
use super::{pal, Gfx};
use macroquad::prelude::*;
use macroquad::rand::{gen_range, srand};

struct Mote {
    p: Vec3,
    speed: f32,
    c: Color,
    size: f32,
}

struct Shape {
    verts: Vec<Vec3>,
    edges: Vec<(usize, usize)>,
    pos: Vec3,
    axis: Vec3,
    spin: f32,
    scale: f32,
    c: Color,
}

struct Streak {
    x: f32,
    z: f32,
    y: f32,
    len: f32,
    speed: f32,
    c: Color,
}

pub struct World {
    plane: Mesh,
    motes: Vec<Mote>,
    shapes: Vec<Shape>,
    streaks: Vec<Streak>,
    batch: LineBatch,
    pub scroll: f32,
    pub speed: f32,
    pub floor_y: f32,
    pub ceil_y: f32,
    pub intensity: f32,
    pub tint: Color,
}

fn icosahedron() -> (Vec<Vec3>, Vec<(usize, usize)>) {
    let t = (1.0 + 5f32.sqrt()) / 2.0;
    let v: Vec<Vec3> = [
        (-1.0, t, 0.0),
        (1.0, t, 0.0),
        (-1.0, -t, 0.0),
        (1.0, -t, 0.0),
        (0.0, -1.0, t),
        (0.0, 1.0, t),
        (0.0, -1.0, -t),
        (0.0, 1.0, -t),
        (t, 0.0, -1.0),
        (t, 0.0, 1.0),
        (-t, 0.0, -1.0),
        (-t, 0.0, 1.0),
    ]
    .iter()
    .map(|&(x, y, z)| vec3(x, y, z).normalize())
    .collect();
    let mut e = Vec::new();
    for i in 0..v.len() {
        for j in i + 1..v.len() {
            if (v[i] - v[j]).length() < 1.1 {
                e.push((i, j));
            }
        }
    }
    (v, e)
}

fn octahedron() -> (Vec<Vec3>, Vec<(usize, usize)>) {
    let v = vec![Vec3::X, -Vec3::X, Vec3::Y, -Vec3::Y, Vec3::Z, -Vec3::Z];
    let mut e = Vec::new();
    for i in 0..6 {
        for j in i + 1..6 {
            if v[i].dot(v[j]) > -0.5 {
                e.push((i, j));
            }
        }
    }
    (v, e)
}

fn cube() -> (Vec<Vec3>, Vec<(usize, usize)>) {
    let mut v = Vec::new();
    for i in 0..8 {
        v.push(vec3(if i & 1 == 0 { -1.0 } else { 1.0 }, if i & 2 == 0 { -1.0 } else { 1.0 }, if i & 4 == 0 { -1.0 } else { 1.0 }) * 0.6);
    }
    let mut e = Vec::new();
    for i in 0..8usize {
        for b in [1, 2, 4] {
            if i & b == 0 {
                e.push((i, i | b));
            }
        }
    }
    (v, e)
}

impl World {
    pub fn new() -> World {
        srand(0x5105_1337);
        let s = 400.0;
        let v = |x: f32, z: f32| Vertex { position: vec3(x, 0.0, z), uv: Vec2::ZERO, color: [255; 4], normal: Vec4::ZERO };
        let plane = Mesh { vertices: vec![v(-s, -s), v(s, -s), v(s, s), v(-s, s)], indices: vec![0, 1, 2, 0, 2, 3], texture: None };
        let colors = [pal::CYAN, pal::MAGENTA, pal::VIOLET, pal::ICE];
        let motes = (0..520)
            .map(|_| Mote {
                p: vec3(gen_range(-45.0, 45.0), gen_range(-6.5, 14.0), gen_range(-140.0, 14.0)),
                speed: gen_range(0.6, 1.6),
                c: colors[gen_range(0, colors.len())],
                size: gen_range(0.8, 2.2),
            })
            .collect();
        let mut shapes = Vec::new();
        let places = [
            vec3(-34.0, 9.0, -70.0),
            vec3(38.0, 4.0, -95.0),
            vec3(-18.0, 16.0, -120.0),
            vec3(22.0, 15.0, -60.0),
            vec3(-55.0, 2.0, -110.0),
            vec3(60.0, 12.0, -130.0),
        ];
        for (i, &pos) in places.iter().enumerate() {
            let (verts, edges) = match i % 3 {
                0 => icosahedron(),
                1 => octahedron(),
                _ => cube(),
            };
            shapes.push(Shape {
                verts,
                edges,
                pos,
                axis: vec3(gen_range(-1.0, 1.0), 1.0, gen_range(-1.0, 1.0)).normalize(),
                spin: gen_range(0.08, 0.25),
                scale: gen_range(3.0, 7.0),
                c: if i % 2 == 0 { pal::VIOLET } else { pal::CYAN },
            });
        }
        let streaks = (0..46)
            .map(|i| Streak {
                x: gen_range(-110.0, 110.0),
                z: gen_range(-170.0, -70.0),
                y: gen_range(-10.0, 50.0),
                len: gen_range(3.0, 11.0),
                speed: gen_range(6.0, 22.0),
                c: if i % 3 == 0 { pal::MAGENTA } else { pal::CYAN },
            })
            .collect();
        World {
            plane,
            motes,
            shapes,
            streaks,
            batch: LineBatch::new(),
            scroll: 0.0,
            speed: 3.0,
            floor_y: -7.0,
            ceil_y: 17.0,
            intensity: 1.0,
            tint: pal::MAGENTA,
        }
    }

    pub fn update(&mut self, dt: f32) {
        self.scroll -= self.speed * dt;
        for m in &mut self.motes {
            m.p.z += self.speed * m.speed * dt;
            if m.p.z > 16.0 {
                m.p.z -= 156.0;
            }
        }
        for s in &mut self.streaks {
            s.y -= s.speed * dt;
            if s.y + s.len < self.floor_y {
                s.y = gen_range(40.0, 60.0);
                s.x = gen_range(-110.0, 110.0);
            }
        }
    }

    pub fn draw(&mut self, g: &Gfx, time: f32) {
        g.set_3d();
        // grids
        gl_use_material(&g.grid);
        for (y, strength, cell) in [(self.floor_y, 1.0f32, 2.0f32), (self.ceil_y, 0.32f32, 4.0f32)] {
            g.grid.set_uniform("CamPos", g.cam.position);
            g.grid.set_uniform("Time", time);
            g.grid.set_uniform("Scroll", self.scroll);
            g.grid.set_uniform("LineColor", pal::scale(self.tint, 0.85).to_vec());
            g.grid.set_uniform("PulseColor", pal::CYAN.to_vec());
            g.grid.set_uniform("Cell", cell);
            g.grid.set_uniform("Strength", strength * self.intensity);
            unsafe { get_internal_gl() }.quad_gl.push_model_matrix(Mat4::from_translation(vec3(g.cam.position.x, y, 0.0)));
            draw_mesh(&self.plane);
            unsafe { get_internal_gl() }.quad_gl.pop_model_matrix();
        }
        // world wires
        let b = &mut self.batch;
        b.clear();
        let k = self.intensity;
        for m in &self.motes {
            let tw = 0.55 + 0.45 * (time * 2.0 + m.p.x).sin();
            b.point(m.p, pal::scale(m.c, 0.75 * tw * k), m.size);
        }
        for s in &self.shapes {
            let rot = Quat::from_axis_angle(s.axis, time * s.spin);
            let bob = vec3(0.0, (time * 0.3 + s.pos.x).sin() * 1.2, 0.0);
            for &(i, j) in &s.edges {
                let a = s.pos + bob + rot * s.verts[i] * s.scale;
                let c = s.pos + bob + rot * s.verts[j] * s.scale;
                b.seg(a, c, pal::scale(s.c, 0.55 * k), 1.6);
            }
            for v in &s.verts {
                b.point(s.pos + bob + rot * *v * s.scale, pal::scale(pal::ICE, 0.6 * k), 2.6);
            }
        }
        for s in &self.streaks {
            let top = vec3(s.x, s.y + s.len, s.z);
            let bot = vec3(s.x, s.y, s.z);
            b.seg_ex(top, bot, pal::scale(s.c, 0.0), pal::scale(s.c, 0.9 * k), 1.4, true, true);
        }
        gl_use_material(&g.line);
        g.set_line_uniforms(&g.line, 1.0, 1.0, 0.0, (25.0, 170.0));
        b.draw();
        gl_use_default_material();
    }
}
