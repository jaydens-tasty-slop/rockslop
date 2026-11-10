//! Rendering core: an MSAA scene target, glow lines, holographic fills, a
//! neon world, and a bloom + CRT post chain.

pub mod lines;
pub mod model;
pub mod scene;
pub mod shaders;
pub mod text;

use macroquad::miniquad::{BlendFactor, BlendState, BlendValue, Comparison, CullFace, Equation, PipelineParams, UniformType};
use macroquad::prelude::*;

pub use lines::LineBatch;

/// Palette: stark black with vivid neon.
pub mod pal {
    use macroquad::prelude::Color;
    pub const BG: Color = Color::new(0.008, 0.006, 0.02, 1.0);
    pub const CYAN: Color = Color::new(0.0, 0.94, 1.0, 1.0);
    pub const MAGENTA: Color = Color::new(1.0, 0.17, 0.84, 1.0);
    pub const ACID: Color = Color::new(0.35, 1.0, 0.12, 1.0);
    pub const AMBER: Color = Color::new(1.0, 0.69, 0.0, 1.0);
    pub const RED: Color = Color::new(1.0, 0.09, 0.27, 1.0);
    pub const VIOLET: Color = Color::new(0.48, 0.25, 1.0, 1.0);
    pub const ICE: Color = Color::new(0.75, 0.95, 1.0, 1.0);
    pub const WHITE: Color = Color::new(1.0, 1.0, 1.0, 1.0);
    pub const DIM: Color = Color::new(0.35, 0.42, 0.55, 1.0);
    pub const TEAL: Color = Color::new(0.25, 1.0, 0.85, 1.0);
    pub const NAVY: Color = Color::new(0.22, 0.32, 1.0, 1.0);

    pub fn alpha(c: Color, a: f32) -> Color {
        Color::new(c.r, c.g, c.b, c.a * a)
    }
    pub fn mix(a: Color, b: Color, t: f32) -> Color {
        Color::new(a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t, a.a + (b.a - a.a) * t)
    }
    pub fn scale(c: Color, k: f32) -> Color {
        Color::new(c.r * k, c.g * k, c.b * k, c.a)
    }
}


/// Post effect knobs, animated by the app every frame.
#[derive(Clone, Copy)]
pub struct Post {
    pub glitch: f32,
    pub aberration: f32,
    pub flash: f32,
    pub flash_color: Color,
    pub fade: f32,
    pub bloom: f32,
    pub scan: f32,
    pub threshold: f32,
}

impl Default for Post {
    fn default() -> Self {
        Post { glitch: 0.0, aberration: 1.0, flash: 0.0, flash_color: WHITE, fade: 0.0, bloom: 1.0, scan: 1.0, threshold: 0.42 }
    }
}

/// What the 3D camera looks at this frame.
#[derive(Clone, Copy)]
pub struct Cam {
    pub position: Vec3,
    pub target: Vec3,
    pub fovy: f32,
}

impl Cam {
    pub fn camera(&self, rt: &RenderTarget) -> Camera3D {
        Camera3D {
            position: self.position,
            target: self.target,
            up: vec3(0.0, 1.0, 0.0),
            fovy: self.fovy,
            render_target: Some(rt.clone()),
            z_near: 0.05,
            z_far: 400.0,
            ..Default::default()
        }
    }
    pub fn view_proj(&self, aspect: f32) -> Mat4 {
        Mat4::perspective_rh_gl(self.fovy, aspect, 0.05, 400.0) * Mat4::look_at_rh(self.position, self.target, vec3(0.0, 1.0, 0.0))
    }
}

/// Picks the render density (target pixels per logical pixel).
///
/// Everything (the scene, the HUD, the post effects) renders into one
/// target at this density, then onto the window. It starts at the screen's
/// own (at most 2) and steps down while frames come too slowly: a high-DPI
/// screen on integrated graphics then draws a quarter of the pixels or
/// fewer, softer but smooth. It never steps back up, so it can't
/// oscillate; and if a step didn't make frames faster (the pixels weren't
/// what was slow), it goes back and stops there.
struct Pace {
    /// a fixed fraction of the screen's own resolution (`--render-scale`)
    fixed: Option<f32>,
    /// the density it stepped down to, if it has
    cap: Option<f32>,
    /// seconds before measuring (again): startup, and after each step
    wait: f32,
    frames: Vec<f32>,
    /// slow seconds in a row
    slow: u32,
    /// the density and frame time before the last step
    tried: Option<(f32, f32)>,
    done: bool,
}

/// Densities to step down through, and the floor.
const STEPS: [f32; 7] = [2.0, 1.75, 1.5, 1.25, 1.0, 0.85, 0.7];
/// The frame time to aim for, and the one that counts as slow.
const AIM: f32 = 1.0 / 60.0;
const SLOW: f32 = 1.0 / 48.0;

impl Pace {
    fn new() -> Pace {
        Pace { fixed: None, cap: None, wait: 1.5, frames: Vec::new(), slow: 0, tried: None, done: false }
    }

    /// The density to render at on a screen of density `native`.
    fn density(&self, native: f32) -> f32 {
        match self.fixed {
            Some(f) => native * f,
            None => native.min(2.0).min(self.cap.unwrap_or(f32::MAX)),
        }
    }

    /// One frame took `secs` (wall time) at density `now`. Returns the
    /// frame rate when it steps down.
    fn frame(&mut self, secs: f32, now: f32) -> Option<f32> {
        if self.fixed.is_some() || self.done {
            return None;
        }
        if self.wait > 0.0 {
            self.wait -= secs;
            return None;
        }
        self.frames.push(secs);
        if self.frames.iter().sum::<f32>() < 1.0 {
            return None;
        }
        // a second's frames, the slowest tenth (hitches) left out
        let mut f = std::mem::take(&mut self.frames);
        f.sort_by(f32::total_cmp);
        let keep = &f[..(f.len() * 9 / 10).max(1)];
        let typical = keep.iter().sum::<f32>() / keep.len() as f32;
        if typical > 0.25 {
            // a hidden (throttled) window or a stall: says nothing
            self.slow = 0;
            return None;
        }
        if let Some((before, was)) = self.tried.take() {
            if typical > was * 0.85 {
                self.cap = Some(before);
                self.done = true;
                return None;
            }
        }
        self.slow = if typical > SLOW { self.slow + 1 } else { 0 };
        if self.slow < 2 {
            return None;
        }
        // if the pixels are what's slow, time goes with their count: the
        // density squared
        let want = now * (AIM / typical).sqrt();
        let lower = || STEPS.into_iter().filter(|&s| s < now - 0.01);
        let Some(next) = lower().find(|&s| s <= want).or(lower().last()) else {
            self.done = true;
            return None;
        };
        self.tried = Some((now, typical));
        self.cap = Some(next);
        self.slow = 0;
        self.wait = 1.0;
        Some(1.0 / typical)
    }
}

struct Chain {
    a: RenderTarget,
    b: RenderTarget,
    w: u32,
    h: u32,
}

pub struct Gfx {
    pub line: Material,
    pub line_hidden: Material,
    pub fill: Material,
    pub grid: Material,
    sky: Material,
    bright: Material,
    blur: Material,
    final_: Material,
    pub lcd: Material,
    rays: Material,
    add: Material,
    /// the god rays' light source, and the rays: at half resolution (56
    /// samples a pixel, and soft anyway), added onto the scene after
    ray_mask: RenderTarget,
    ray_out: RenderTarget,
    pub scene: RenderTarget,
    chain: Vec<Chain>,
    pub phys: (u32, u32),
    pub text: text::Fonts,
    /// the LCD mock-up's font
    pub lcd_font: Font,
    /// target pixels per logical pixel: what everything renders at
    pub density: f32,
    pace: Pace,
    pub cam: Cam,
    quad: Mesh,
}

fn additive() -> BlendState {
    BlendState::new(Equation::Add, BlendFactor::One, BlendFactor::One)
}

fn alpha_blend() -> BlendState {
    BlendState::new(Equation::Add, BlendFactor::Value(BlendValue::SourceAlpha), BlendFactor::OneMinusValue(BlendValue::SourceAlpha))
}

fn u(name: &str, t: UniformType) -> UniformDesc {
    UniformDesc::new(name, t)
}

fn mat(vs: &str, fs: &str, pipeline: PipelineParams, uniforms: Vec<UniformDesc>, textures: &[&str]) -> Material {
    load_material(
        ShaderSource::Glsl { vertex: vs, fragment: fs },
        MaterialParams { pipeline_params: pipeline, uniforms, textures: textures.iter().map(|s| s.to_string()).collect() },
    )
    .unwrap_or_else(|e| panic!("shader: {e:?}"))
}

fn line_uniforms() -> Vec<UniformDesc> {
    vec![
        u("Viewport", UniformType::Float2),
        u("Width", UniformType::Float1),
        u("DepthBias", UniformType::Float1),
        u("Intensity", UniformType::Float1),
        u("CamPos", UniformType::Float3),
        u("FogNear", UniformType::Float1),
        u("FogFar", UniformType::Float1),
    ]
}

fn quad_mesh() -> Mesh {
    let v = |x: f32, y: f32, s: f32, t: f32| Vertex { position: vec3(x, y, 0.0), uv: vec2(s, t), color: [255; 4], normal: Vec4::ZERO };
    Mesh {
        vertices: vec![v(-1.0, -1.0, 0.0, 0.0), v(1.0, -1.0, 1.0, 0.0), v(1.0, 1.0, 1.0, 1.0), v(-1.0, 1.0, 0.0, 1.0)],
        indices: vec![0, 1, 2, 0, 2, 3],
        texture: None,
    }
}

fn make_rt(w: u32, h: u32, msaa: bool) -> RenderTarget {
    let rt = render_target_ex(w.max(1), h.max(1), RenderTargetParams { sample_count: if msaa { 4 } else { 1 }, depth: msaa });
    rt.texture.set_filter(FilterMode::Linear);
    rt
}

impl Gfx {
    pub fn new() -> Gfx {
        let line_pipe = PipelineParams {
            color_blend: Some(additive()),
            alpha_blend: Some(additive()),
            depth_test: Comparison::LessOrEqual,
            depth_write: false,
            cull_face: CullFace::Nothing,
            ..Default::default()
        };
        let line = mat(shaders::LINE_VS, shaders::LINE_FS, line_pipe, line_uniforms(), &[]);
        let line_hidden =
            mat(shaders::LINE_VS, shaders::LINE_FS, PipelineParams { depth_test: Comparison::Greater, ..line_pipe }, line_uniforms(), &[]);
        let fill = mat(
            shaders::FILL_VS,
            shaders::FILL_FS,
            PipelineParams {
                color_blend: Some(alpha_blend()),
                depth_test: Comparison::LessOrEqual,
                depth_write: true,
                cull_face: CullFace::Nothing,
                ..Default::default()
            },
            vec![
                u("CamPos", UniformType::Float3),
                u("Rim", UniformType::Float4),
                u("FillColor", UniformType::Float4),
                u("FillLevel", UniformType::Float1),
                u("Time", UniformType::Float1),
                u("Alpha", UniformType::Float1),
                u("Pulse", UniformType::Float1),
            ],
            &[],
        );
        let grid = mat(
            shaders::GRID_VS,
            shaders::GRID_FS,
            PipelineParams { color_blend: Some(additive()), depth_test: Comparison::LessOrEqual, depth_write: false, ..Default::default() },
            vec![
                u("CamPos", UniformType::Float3),
                u("Time", UniformType::Float1),
                u("Scroll", UniformType::Float1),
                u("LineColor", UniformType::Float4),
                u("PulseColor", UniformType::Float4),
                u("Cell", UniformType::Float1),
                u("Strength", UniformType::Float1),
            ],
            &[],
        );
        let opaque = PipelineParams::default();
        let sky = mat(
            shaders::QUAD_VS,
            shaders::SKY_FS,
            opaque,
            vec![
                u("Time", UniformType::Float1),
                u("Res", UniformType::Float2),
                u("Horizon", UniformType::Float1),
                u("Tint", UniformType::Float4),
            ],
            &[],
        );
        let bright = mat(
            shaders::QUAD_VS,
            shaders::BRIGHT_FS,
            opaque,
            vec![u("Texel", UniformType::Float2), u("Threshold", UniformType::Float1)],
            &[],
        );
        let blur = mat(shaders::QUAD_VS, shaders::BLUR_FS, opaque, vec![u("Dir", UniformType::Float2)], &[]);
        let final_ = mat(
            shaders::QUAD_VS,
            shaders::FINAL_FS,
            opaque,
            vec![
                u("Res", UniformType::Float2),
                u("Px", UniformType::Float2),
                u("Time", UniformType::Float1),
                u("Glitch", UniformType::Float1),
                u("Aberration", UniformType::Float1),
                u("Flash", UniformType::Float1),
                u("FlashColor", UniformType::Float4),
                u("Fade", UniformType::Float1),
                u("BloomStrength", UniformType::Float1),
                u("Scan", UniformType::Float1),
            ],
            &["Bloom1", "Bloom2", "Bloom3"],
        );
        let lcd = mat(
            shaders::QUAD_VS,
            shaders::LCD_FS,
            PipelineParams { color_blend: Some(additive()), depth_test: Comparison::LessOrEqual, depth_write: false, ..Default::default() },
            vec![u("LcdRes", UniformType::Float2), u("Power", UniformType::Float1)],
            &[],
        );
        let rays = mat(
            shaders::QUAD_VS,
            shaders::RAYS_FS,
            PipelineParams { color_blend: Some(additive()), alpha_blend: Some(additive()), ..Default::default() },
            vec![
                u("Center", UniformType::Float2),
                u("Inner", UniformType::Float1),
                u("Aspect", UniformType::Float1),
                u("Reach", UniformType::Float1),
                u("Strength", UniformType::Float1),
                u("Time", UniformType::Float1),
                u("Tint", UniformType::Float4),
            ],
            &[],
        );
        let add = mat(
            shaders::QUAD_VS,
            shaders::ADD_FS,
            PipelineParams { color_blend: Some(additive()), alpha_blend: Some(additive()), ..Default::default() },
            vec![],
            &[],
        );
        // the player's own screen: a plain bitmap-ish font, Latin only
        let lcd_font = load_ttf_font_from_bytes(include_bytes!("../../assets/fonts/ShareTechMono-Regular.ttf")).unwrap();
        let mut g = Gfx {
            line,
            line_hidden,
            fill,
            grid,
            sky,
            bright,
            blur,
            final_,
            lcd,
            rays,
            add,
            ray_mask: make_rt(1, 1, false),
            ray_out: make_rt(1, 1, false),
            scene: make_rt(1, 1, true),
            chain: Vec::new(),
            phys: (0, 0),
            text: text::Fonts::new(),
            lcd_font,
            density: 1.0,
            pace: Pace::new(),
            cam: Cam { position: vec3(0.0, 0.0, 14.0), target: Vec3::ZERO, fovy: 0.6 },
            quad: quad_mesh(),
        };
        g.resize_if_needed();
        g
    }

    /// Logical (DPI independent) screen size; the HUD is laid out in these.
    pub fn size(&self) -> Vec2 {
        vec2(screen_width(), screen_height())
    }

    /// Render at a fixed `fraction` of the screen's own resolution.
    pub fn fix_scale(&mut self, fraction: f32) {
        self.pace.fixed = Some(fraction);
    }

    /// Account for the last frame, which took `secs`: the render density
    /// steps down if frames are slow.
    pub fn pace(&mut self, secs: f32) {
        if let Some(fps) = self.pace.frame(secs, self.density) {
            let d = self.pace.density(screen_dpi_scale());
            eprintln!("rockslop: {fps:.0} fps at {:.2} px per point; rendering at {d:.2}", self.density);
        }
    }

    fn resize_if_needed(&mut self) {
        self.density = self.pace.density(screen_dpi_scale());
        let w = (screen_width() * self.density).round().max(1.0) as u32;
        let h = (screen_height() * self.density).round().max(1.0) as u32;
        if (w, h) == self.phys {
            return;
        }
        self.phys = (w, h);
        self.scene = make_rt(w, h, true);
        self.ray_mask = make_rt(w / 2, h / 2, false);
        self.ray_out = make_rt(w / 2, h / 2, false);
        self.chain.clear();
        let (mut cw, mut ch) = (w / 2, h / 2);
        for _ in 0..3 {
            self.chain.push(Chain { a: make_rt(cw, ch, false), b: make_rt(cw, ch, false), w: cw, h: ch });
            cw = (cw / 2).max(1);
            ch = (ch / 2).max(1);
        }
    }

    pub fn aspect(&self) -> f32 {
        self.phys.0 as f32 / self.phys.1.max(1) as f32
    }

    /// Start a frame: (re)size targets, clear the scene, draw the sky.
    pub fn begin(&mut self, time: f32, horizon_tint: Color) {
        self.resize_if_needed();
        set_camera(&Camera2D { render_target: Some(self.scene.clone()), ..Default::default() });
        clear_background(pal::BG);
        self.fullscreen_pass(Some(&self.scene.clone()), &self.sky.clone(), None, |m| {
            m.set_uniform("Time", time);
            m.set_uniform("Res", vec2(self.phys.0 as f32, self.phys.1 as f32));
            m.set_uniform("Horizon", 0.47f32);
            m.set_uniform("Tint", horizon_tint.to_vec());
        });
    }

    /// Switch to world space on the scene target.
    pub fn set_3d(&self) {
        set_camera(&self.cam.camera(&self.scene));
    }

    /// Switch to HUD space (logical px, origin top-left) on the scene target.
    pub fn set_2d(&self) {
        let s = self.size();
        set_camera(&Camera2D {
            zoom: vec2(2.0 / s.x, -2.0 / s.y),
            target: s / 2.0,
            render_target: Some(self.scene.clone()),
            ..Default::default()
        });
    }

    pub fn view_proj(&self) -> Mat4 {
        self.cam.view_proj(self.aspect())
    }

    /// Project a world point to logical screen px. None if behind the camera.
    pub fn project(&self, p: Vec3) -> Option<Vec2> {
        let c = self.view_proj() * p.extend(1.0);
        if c.w <= 0.0001 {
            return None;
        }
        let n = c.truncate() / c.w;
        let s = self.size();
        Some(vec2((n.x * 0.5 + 0.5) * s.x, (0.5 - n.y * 0.5) * s.y))
    }

    pub fn set_line_uniforms(&self, m: &Material, width: f32, intensity: f32, bias: f32, fog: (f32, f32)) {
        m.set_uniform("Viewport", vec2(self.phys.0 as f32, self.phys.1 as f32));
        m.set_uniform("Width", width * self.density);
        m.set_uniform("DepthBias", bias);
        m.set_uniform("Intensity", intensity);
        m.set_uniform("CamPos", self.cam.position);
        m.set_uniform("FogNear", fog.0);
        m.set_uniform("FogFar", fog.1);
    }

    /// God rays: `light` draws the light that comes through (in world
    /// space, this frame's camera) and the rays stream out of it, away from
    /// `from` (logical px). Within about `inner` px of `from` they hold
    /// back, so what the light comes through stays legible. `strength`
    /// (0 = none) and `reach` (0..1, how far they stream) follow the music.
    #[allow(clippy::too_many_arguments)]
    pub fn god_rays(&self, light: impl FnOnce(), from: Vec2, inner: f32, strength: f32, reach: f32, time: f32, tint: Color) {
        if strength <= 0.001 {
            return;
        }
        set_camera(&self.cam.camera(&self.ray_mask));
        clear_background(BLACK);
        light();
        let s = self.size();
        let mask = self.ray_mask.texture.clone();
        let out = self.ray_out.clone();
        set_camera(&Camera2D { render_target: Some(out.clone()), ..Default::default() });
        clear_background(BLACK);
        self.fullscreen_pass(Some(&out), &self.rays, Some(&mask), |m| {
            m.set_uniform("Center", vec2(from.x / s.x, 1.0 - from.y / s.y));
            m.set_uniform("Inner", inner / s.y);
            m.set_uniform("Aspect", s.x / s.y);
            m.set_uniform("Reach", reach);
            m.set_uniform("Strength", strength);
            m.set_uniform("Time", time);
            m.set_uniform("Tint", tint.to_vec());
        });
        self.fullscreen_pass(Some(&self.scene.clone()), &self.add, Some(&out.texture), |_| {});
    }

    /// Draw a full-target quad with `material`, sampling `src` as Texture.
    fn fullscreen_pass(&self, target: Option<&RenderTarget>, material: &Material, src: Option<&Texture2D>, setup: impl FnOnce(&Material)) {
        // An identity projection: our quad is already in clip space.
        set_camera(&Camera2D {
            zoom: vec2(1.0, if target.is_some() { 1.0 } else { -1.0 }),
            target: Vec2::ZERO,
            render_target: target.cloned(),
            ..Default::default()
        });
        gl_use_material(material);
        setup(material);
        let q = Mesh { vertices: self.quad.vertices.clone(), indices: self.quad.indices.clone(), texture: src.cloned() };
        draw_mesh(&q);
        gl_use_default_material();
    }

    /// Bloom chain and final composite to the window.
    pub fn finish(&self, time: f32, post: &Post) {
        let scene = self.scene.texture.clone();
        let c0 = &self.chain[0];
        self.fullscreen_pass(Some(&c0.a), &self.bright, Some(&scene), |m| {
            m.set_uniform("Texel", vec2(1.0 / self.phys.0 as f32, 1.0 / self.phys.1 as f32));
            m.set_uniform("Threshold", post.threshold);
        });
        for i in 0..self.chain.len() {
            let c = &self.chain[i];
            if i > 0 {
                // downsample previous level's result into this level
                let prev = self.chain[i - 1].a.texture.clone();
                self.fullscreen_pass(Some(&c.a), &self.blur, Some(&prev), |m| {
                    m.set_uniform("Dir", vec2(0.5 / c.w as f32, 0.0));
                });
            }
            let texel = vec2(1.0 / c.w as f32, 1.0 / c.h as f32);
            for _ in 0..2 {
                self.fullscreen_pass(Some(&c.b), &self.blur, Some(&c.a.texture.clone()), |m| {
                    m.set_uniform("Dir", vec2(texel.x, 0.0));
                });
                self.fullscreen_pass(Some(&c.a), &self.blur, Some(&c.b.texture.clone()), |m| {
                    m.set_uniform("Dir", vec2(0.0, texel.y));
                });
            }
        }
        self.final_.set_texture("Bloom1", self.chain[0].a.texture.clone());
        self.final_.set_texture("Bloom2", self.chain[1].a.texture.clone());
        self.final_.set_texture("Bloom3", self.chain[2].a.texture.clone());
        self.fullscreen_pass(None, &self.final_, Some(&scene), |m| {
            m.set_uniform("Res", vec2(self.phys.0 as f32, self.phys.1 as f32));
            m.set_uniform("Px", 1.0 / self.size());
            m.set_uniform("Time", time);
            m.set_uniform("Glitch", post.glitch);
            m.set_uniform("Aberration", post.aberration);
            m.set_uniform("Flash", post.flash);
            m.set_uniform("FlashColor", post.flash_color.to_vec());
            m.set_uniform("Fade", post.fade);
            m.set_uniform("BloomStrength", post.bloom);
            m.set_uniform("Scan", post.scan);
        });
        set_default_camera();
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    /// Run `p` for `secs` of frames: `frame(density)` is each frame's time.
    fn run(p: &mut Pace, native: f32, secs: f32, frame: impl Fn(f32) -> f32) -> f32 {
        let mut t = 0.0;
        while t < secs {
            let d = p.density(native);
            let f = frame(d);
            p.frame(f, d);
            t += f;
        }
        p.density(native)
    }

    /// A GPU that takes `ms` a frame at density 2, its time going with the
    /// pixel count, under 60 Hz vsync.
    fn gpu(ms: f32) -> impl Fn(f32) -> f32 {
        move |d: f32| (ms / 1000.0 * d * d / 4.0).max(AIM)
    }

    #[test]
    fn fast_enough_stays_sharp() {
        let mut p = Pace::new();
        assert_eq!(run(&mut p, 2.0, 30.0, gpu(15.0)), 2.0);
        let mut p = Pace::new();
        assert_eq!(run(&mut p, 3.0, 30.0, gpu(15.0)), 2.0, "at most 2x");
    }

    #[test]
    fn slow_pixels_step_down_and_settle() {
        // 45 ms a frame at 2x (22 fps): fill-bound
        let mut p = Pace::new();
        let d = run(&mut p, 2.0, 30.0, gpu(45.0));
        assert!((1.0..=1.25).contains(&d), "{d}");
        assert!(gpu(45.0)(d) <= SLOW);
        // and it stays there
        assert_eq!(run(&mut p, 2.0, 60.0, gpu(45.0)), d);
    }

    #[test]
    fn slow_cpu_goes_back_and_stops() {
        // 40 ms whatever the density: fewer pixels don't help
        let mut p = Pace::new();
        assert_eq!(run(&mut p, 2.0, 30.0, |_| 0.04), 2.0);
        assert!(p.done);
    }

    #[test]
    fn hidden_window_says_nothing() {
        let mut p = Pace::new();
        assert_eq!(run(&mut p, 2.0, 30.0, |_| 0.5), 2.0);
        assert!(!p.done);
    }

    #[test]
    fn fixed_scale_is_fixed() {
        let mut p = Pace::new();
        p.fixed = Some(0.5);
        assert_eq!(run(&mut p, 2.0, 30.0, gpu(200.0)), 1.0);
    }
}
