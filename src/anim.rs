//! Easing, springs and small animation helpers.

use macroquad::prelude::*;

pub fn clamp01(x: f32) -> f32 {
    x.clamp(0.0, 1.0)
}

pub fn ease_out_cubic(t: f32) -> f32 {
    let t = clamp01(t);
    1.0 - (1.0 - t).powi(3)
}

pub fn ease_in_out_cubic(t: f32) -> f32 {
    let t = clamp01(t);
    if t < 0.5 {
        4.0 * t * t * t
    } else {
        1.0 - (-2.0 * t + 2.0).powi(3) / 2.0
    }
}

pub fn ease_out_back(t: f32) -> f32 {
    let t = clamp01(t);
    let c1 = 1.70158;
    let c3 = c1 + 1.0;
    1.0 + c3 * (t - 1.0).powi(3) + c1 * (t - 1.0).powi(2)
}

pub fn ease_out_expo(t: f32) -> f32 {
    let t = clamp01(t);
    if t >= 1.0 {
        1.0
    } else {
        1.0 - 2f32.powf(-10.0 * t)
    }
}

/// Progress of a window [start, start+dur] at time t, 0..1.
pub fn window(t: f32, start: f32, dur: f32) -> f32 {
    clamp01((t - start) / dur.max(1e-4))
}

/// Critically damped-ish spring on f32.
#[derive(Clone, Copy, Debug)]
pub struct Spring {
    pub x: f32,
    pub v: f32,
    pub target: f32,
    pub k: f32,
    pub damp: f32,
}

impl Spring {
    pub fn new(x: f32, k: f32, damp: f32) -> Spring {
        Spring { x, v: 0.0, target: x, k, damp }
    }
    pub fn step(&mut self, dt: f32) -> f32 {
        // semi-implicit Euler, substepped for stability at low fps
        let n = ((dt / 0.004).ceil() as usize).clamp(1, 16);
        let h = dt / n as f32;
        for _ in 0..n {
            let a = self.k * (self.target - self.x) - self.damp * self.v;
            self.v += a * h;
            self.x += self.v * h;
        }
        self.x
    }
}

#[derive(Clone, Copy, Debug)]
pub struct Spring3 {
    pub x: Spring,
    pub y: Spring,
    pub z: Spring,
}

impl Spring3 {
    pub fn new(v: Vec3, k: f32, damp: f32) -> Spring3 {
        Spring3 { x: Spring::new(v.x, k, damp), y: Spring::new(v.y, k, damp), z: Spring::new(v.z, k, damp) }
    }
    pub fn set(&mut self, v: Vec3) {
        self.x.target = v.x;
        self.y.target = v.y;
        self.z.target = v.z;
    }
    pub fn step(&mut self, dt: f32) -> Vec3 {
        vec3(self.x.step(dt), self.y.step(dt), self.z.step(dt))
    }
    pub fn get(&self) -> Vec3 {
        vec3(self.x.x, self.y.x, self.z.x)
    }
}

/// Exponential approach (frame-rate independent lerp).
pub fn approach(cur: f32, target: f32, rate: f32, dt: f32) -> f32 {
    cur + (target - cur) * (1.0 - (-rate * dt).exp())
}
