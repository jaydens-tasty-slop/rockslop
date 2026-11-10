//! GLSL sources. All shaders are GLSL 1.00, like macroquad's own, so the same
//! text runs on GL2.1, GL3 core (macOS) and GLES.

/// Screen-space expanded glow lines. Each segment is a quad of four
/// vertices: `position` is this end, `normal.xyz` the other end and
/// `normal.w` the half width in px. `texcoord.x` is 0 at end A, 1 at end B
/// (+2 when that end is an open end that gets a cap); `texcoord.y` is the
/// side, -1 or +1. A zero-length segment renders as a point sprite.
pub const LINE_VS: &str = r#"#version 100
precision highp float;
attribute vec3 position;
attribute vec2 texcoord;
attribute vec4 color0;
attribute vec4 normal;
varying lowp vec4 vcolor;
varying vec3 vq;
varying float vlen;
varying float vfog;
uniform mat4 Model;
uniform mat4 Projection;
uniform vec2 Viewport;
uniform float Width;
uniform float DepthBias;
uniform vec3 CamPos;
uniform float FogNear;
uniform float FogFar;
void main() {
    vec4 wa = Model * vec4(position, 1.0);
    vec4 a = Projection * wa;
    vec4 b = Projection * Model * vec4(normal.xyz, 1.0);
    float aw = max(a.w, 0.0001);
    float bw = max(b.w, 0.0001);
    vec2 half_vp = Viewport * 0.5;
    vec2 sa = a.xy / aw * half_vp;
    vec2 sb = b.xy / bw * half_vp;
    vec2 d = sb - sa;
    float len = length(d);
    float endf = mod(texcoord.x, 2.0);
    float cap = step(1.5, texcoord.x);
    vec2 dir = len > 0.001 ? d / len : vec2(1.0, 0.0);
    vec2 ab = endf < 0.5 ? dir : -dir;
    if (len <= 0.001) { ab = vec2(1.0, 0.0); cap = 1.0; }
    vec2 perp = vec2(-ab.y, ab.x);
    float w = normal.w * Width;
    vec2 off = perp * texcoord.y * w + ab * cap * (endf < 0.5 ? -w : w);
    a.xy += off / half_vp * aw;
    a.z -= DepthBias * aw;
    gl_Position = a;
    vcolor = color0 / 255.0;
    // along-axis coordinate in half widths (0..L between the ends, beyond
    // that the round caps), interpolated linearly in screen space
    float L = len / max(w, 0.001);
    float along = endf < 0.5 ? -cap : L + cap;
    vq = vec3(along * aw, texcoord.y * aw, aw);
    vlen = L;
    float dist = length(wa.xyz - CamPos);
    vfog = 1.0 - clamp((dist - FogNear) / max(FogFar - FogNear, 0.001), 0.0, 1.0);
}
"#;

pub const LINE_FS: &str = r#"#version 100
precision mediump float;
varying lowp vec4 vcolor;
varying vec3 vq;
varying float vlen;
varying float vfog;
uniform float Intensity;
void main() {
    vec2 q = vq.xy / vq.z;
    float dx = q.x < 0.0 ? -q.x : max(q.x - vlen, 0.0);
    float d = length(vec2(dx, q.y));
    if (d > 1.0) discard;
    float core = 1.0 - smoothstep(0.0, 0.42, d);
    float halo = 1.0 - d;
    halo = halo * halo * 0.45;
    float a = (core + halo) * vcolor.a * Intensity * vfog;
    // hot white core on saturated lines
    vec3 c = mix(vcolor.rgb, vec3(1.0), core * core * 0.55);
    gl_FragColor = vec4(c * a, a);
}
"#;

/// Holographic body fill: dark translucent shell with a fresnel rim, slow
/// scan bands, and a "fill level" that rises while firmware is written.
pub const FILL_VS: &str = r#"#version 100
precision highp float;
attribute vec3 position;
attribute vec2 texcoord;
attribute vec4 color0;
attribute vec4 normal;
varying lowp vec4 vcolor;
varying vec3 vnormal;
varying vec3 vworld;
varying vec3 vlocal;
uniform mat4 Model;
uniform mat4 Projection;
void main() {
    vec4 w = Model * vec4(position, 1.0);
    gl_Position = Projection * w;
    vworld = w.xyz;
    vlocal = position;
    vnormal = (Model * vec4(normal.xyz, 0.0)).xyz;
    vcolor = color0 / 255.0;
}
"#;

pub const FILL_FS: &str = r#"#version 100
precision mediump float;
varying lowp vec4 vcolor;
varying vec3 vnormal;
varying vec3 vworld;
varying vec3 vlocal;
uniform vec3 CamPos;
uniform vec4 Rim;
uniform vec4 FillColor;
uniform float FillLevel;
uniform float Time;
uniform float Alpha;
uniform float Pulse;
void main() {
    vec3 n = normalize(vnormal);
    vec3 v = normalize(CamPos - vworld);
    float f = 1.0 - abs(dot(n, v));
    float rim = f * f * f;
    vec3 col = vcolor.rgb * (0.55 + 0.45 * abs(dot(n, v)));
    col += Rim.rgb * rim * 1.1;
    // holographic scan bands drifting up the shell
    float band = 0.5 + 0.5 * sin(vlocal.y * 26.0 - Time * 2.2);
    col += Rim.rgb * band * band * 0.035;
    // the firmware "fills" the device from the bottom up
    float below = step(vlocal.y, FillLevel);
    float hatch = step(0.5, fract((vlocal.x + vlocal.y) * 7.0));
    float edge = 1.0 - smoothstep(0.0, 0.06, abs(vlocal.y - FillLevel));
    col += FillColor.rgb * below * (0.16 + 0.08 * hatch);
    col += FillColor.rgb * edge * 1.6 * step(-50.0, FillLevel);
    col += Rim.rgb * Pulse * 0.25;
    gl_FragColor = vec4(col, Alpha);
}
"#;

/// Infinite-looking neon grid plane with travelling data pulses.
pub const GRID_VS: &str = r#"#version 100
precision highp float;
attribute vec3 position;
attribute vec2 texcoord;
attribute vec4 color0;
varying vec3 vworld;
uniform mat4 Model;
uniform mat4 Projection;
void main() {
    vec4 w = Model * vec4(position, 1.0);
    vworld = w.xyz;
    gl_Position = Projection * w;
}
"#;

pub const GRID_FS: &str = r#"#version 100
precision highp float;
varying vec3 vworld;
uniform vec3 CamPos;
uniform float Time;
uniform float Scroll;
uniform vec4 LineColor;
uniform vec4 PulseColor;
uniform float Cell;
uniform float Strength;
float hash(float n) { return fract(sin(n) * 43758.5453); }
float gridline(float coord, float width) {
    float c = abs(fract(coord) - 0.5);
    return smoothstep(0.5 - width, 0.5, c);
}
void main() {
    vec2 p = vworld.xz / Cell;
    p.y += Scroll / Cell;
    float dist = length(vworld - CamPos);
    float w = clamp(0.012 + dist * 0.00035, 0.0, 0.08);
    float gx = gridline(p.x, w);
    float gz = gridline(p.y, w);
    float major = max(gridline(p.x / 5.0, w / 5.0), gridline(p.y / 5.0, w / 5.0));
    float g = max(gx, gz) * 0.55 + major * 0.6;
    // pulses running along some z lanes
    float lane = floor(p.x + 0.5);
    float speed = 3.0 + hash(lane * 1.7) * 6.0;
    float pos = fract(hash(lane * 3.1) + Time * speed / 60.0) * 60.0 - 30.0;
    float along = p.y - floor(p.y / 60.0) * 60.0 - 30.0;
    float pulse = exp(-abs(along - pos) * 1.2) * gx * step(0.55, hash(lane * 7.3));
    float fade = exp(-dist * 0.022);
    float horizon = smoothstep(150.0, 20.0, dist);
    // far lines alias into a wash: let them dissolve into the haze
    g *= 1.0 - smoothstep(35.0, 120.0, dist) * 0.85;
    vec3 col = LineColor.rgb * g * fade + PulseColor.rgb * pulse * 1.8 * fade;
    col *= horizon * Strength;
    gl_FragColor = vec4(col, 1.0);
}
"#;

/// Full screen quad helper: positions are given directly in clip space.
pub const QUAD_VS: &str = r#"#version 100
precision highp float;
attribute vec3 position;
attribute vec2 texcoord;
varying vec2 uv;
uniform mat4 Model;
uniform mat4 Projection;
void main() {
    gl_Position = Projection * Model * vec4(position, 1.0);
    uv = texcoord;
}
"#;

/// Sky: deep gradient, faint nebula and stars, horizon haze.
pub const SKY_FS: &str = r#"#version 100
precision highp float;
varying vec2 uv;
uniform float Time;
uniform vec2 Res;
uniform float Horizon;
uniform vec4 Tint;
float hash(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }
float noise(vec2 p) {
    vec2 i = floor(p); vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i), hash(i + vec2(1.0, 0.0)), f.x),
               mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), f.x), f.y);
}
float fbm(vec2 p) {
    float s = 0.0; float a = 0.5;
    for (int i = 0; i < 5; i++) { s += noise(p) * a; p *= 2.03; a *= 0.5; }
    return s;
}
void main() {
    vec2 p = uv;
    float aspect = Res.x / Res.y;
    float y = 1.0 - p.y;
    vec3 col = mix(vec3(0.010, 0.006, 0.030), vec3(0.0, 0.0, 0.006), smoothstep(Horizon, 1.0, y));
    // nebula
    vec2 q = vec2(p.x * aspect, y) * 2.2 + vec2(Time * 0.006, 0.0);
    float n = fbm(q + fbm(q * 1.7 + Time * 0.01));
    col += vec3(0.10, 0.02, 0.18) * pow(n, 3.0) * smoothstep(Horizon - 0.1, 1.0, y) * 1.3;
    col += vec3(0.0, 0.08, 0.14) * pow(fbm(q * 0.7 - 3.0), 4.0) * 1.2;
    // stars
    vec2 sp = vec2(p.x * aspect, y) * 180.0;
    float st = hash(floor(sp));
    float tw = 0.6 + 0.4 * sin(Time * (1.0 + st * 4.0) + st * 40.0);
    col += vec3(0.6, 0.8, 1.0) * step(0.9975, st) * tw * smoothstep(Horizon, Horizon + 0.25, y)
        * (1.0 - smoothstep(0.15, 0.5, length(fract(sp) - 0.5)));
    // horizon haze
    float h = exp(-abs(y - Horizon) * 18.0);
    col += Tint.rgb * h * 0.55;
    col += Tint.rgb * exp(-abs(y - Horizon) * 4.0) * 0.08;
    gl_FragColor = vec4(col, 1.0);
}
"#;

/// A texture added onto the target as it is (with an additive pipeline):
/// the god rays, upscaled from half resolution.
pub const ADD_FS: &str = r#"#version 100
precision mediump float;
varying vec2 uv;
uniform sampler2D Texture;
void main() {
    gl_FragColor = vec4(texture2D(Texture, uv).rgb, 1.0);
}
"#;

/// Soft-knee bright pass for bloom (scene -> half res).
pub const BRIGHT_FS: &str = r#"#version 100
precision mediump float;
varying vec2 uv;
uniform sampler2D Texture;
uniform vec2 Texel;
uniform float Threshold;
void main() {
    vec3 c = texture2D(Texture, uv + Texel * vec2(-0.5, -0.5)).rgb
           + texture2D(Texture, uv + Texel * vec2( 0.5, -0.5)).rgb
           + texture2D(Texture, uv + Texel * vec2(-0.5,  0.5)).rgb
           + texture2D(Texture, uv + Texel * vec2( 0.5,  0.5)).rgb;
    c *= 0.25;
    float l = max(c.r, max(c.g, c.b));
    float k = 0.25;
    float soft = clamp(l - Threshold + k, 0.0, 2.0 * k);
    soft = soft * soft / (4.0 * k + 0.0001);
    float contrib = max(soft, l - Threshold) / max(l, 0.0001);
    gl_FragColor = vec4(c * contrib, 1.0);
}
"#;

/// 9-tap gaussian with linear sampling, along Dir (in texels).
pub const BLUR_FS: &str = r#"#version 100
precision mediump float;
varying vec2 uv;
uniform sampler2D Texture;
uniform vec2 Dir;
void main() {
    vec3 c = texture2D(Texture, uv).rgb * 0.2270270270;
    c += texture2D(Texture, uv + Dir * 1.3846153846).rgb * 0.3162162162;
    c += texture2D(Texture, uv - Dir * 1.3846153846).rgb * 0.3162162162;
    c += texture2D(Texture, uv + Dir * 3.2307692308).rgb * 0.0702702703;
    c += texture2D(Texture, uv - Dir * 3.2307692308).rgb * 0.0702702703;
    gl_FragColor = vec4(c, 1.0);
}
"#;

/// God rays: light dragged outward from `Center` through the mask (what
/// shines through the player's screen), so shafts stream out from the
/// edges of what's on it. Added onto the scene.
pub const RAYS_FS: &str = r#"#version 100
precision mediump float;
varying vec2 uv;
uniform sampler2D Texture;
uniform vec2 Center;
uniform float Inner;
uniform float Aspect;
uniform float Reach;
uniform float Strength;
uniform float Time;
uniform vec4 Tint;
void main() {
    vec2 d = uv - Center;
    vec2 stp = d * Reach / 56.0;
    vec2 p = uv;
    float decay = 1.0;
    vec3 acc = vec3(0.0);
    for (int i = 0; i < 56; i++) {
        p -= stp;
        acc += texture2D(Texture, p).rgb * decay;
        decay *= 0.977;
    }
    acc *= 1.0 / 12.0;
    vec2 q = vec2(d.x * Aspect, d.y);
    float ang = atan(q.y, q.x);
    // the light's own shape makes the shafts; two faint sets of spokes
    // turning against each other keep them moving
    float a1 = 0.5 + 0.5 * sin(ang * 9.0 + Time * 0.45);
    float a2 = 0.5 + 0.5 * sin(ang * 23.0 - Time * 0.7);
    float shafts = 0.6 + 0.4 * a1 * (0.5 + 0.5 * a2);
    float r = length(q);
    float fall = 1.0 / (1.0 + r * r * 2.2);
    // hold back over the light's own window, so what's on it stays legible
    float clear = mix(0.25, 1.0, smoothstep(Inner * 0.6, Inner * 1.5, r));
    vec3 c = acc * mix(vec3(1.0), Tint.rgb, 0.65) * Strength * shafts * fall * clear;
    gl_FragColor = vec4(c, 1.0);
}
"#;

/// Final composite: bloom, chromatic aberration, glitch slices, CRT
/// curvature, scanlines, vignette, grain, flash and fade.
pub const FINAL_FS: &str = r#"#version 100
precision highp float;
varying vec2 uv;
uniform sampler2D Texture;
uniform sampler2D Bloom1;
uniform sampler2D Bloom2;
uniform sampler2D Bloom3;
uniform vec2 Res;
// one logical px, in uv
uniform vec2 Px;
uniform float Time;
uniform float Glitch;
uniform float Aberration;
uniform float Flash;
uniform vec4 FlashColor;
uniform float Fade;
uniform float BloomStrength;
uniform float Scan;
float hash(vec2 p) { return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453); }
vec3 bloom_at(vec2 p) {
    return texture2D(Bloom1, p).rgb * 0.55 + texture2D(Bloom2, p).rgb * 0.8
         + texture2D(Bloom3, p).rgb * 1.1;
}
void main() {
    vec2 c = uv - 0.5;
    float r2 = dot(c, c);
    vec2 p = 0.5 + c * (1.0 + 0.035 * r2 + 0.02 * r2 * r2);
    // glitch: displaced horizontal slices, re-rolled ~20x per second
    float tq = floor(Time * 22.0);
    float band = floor(p.y * (18.0 + 30.0 * hash(vec2(tq, 3.0))));
    float hit = step(1.0 - Glitch * 0.45, hash(vec2(band, tq)));
    float shift = (hash(vec2(band, tq + 1.0)) - 0.5) * 0.12 * Glitch * hit;
    p.x += shift;
    // chromatic aberration, red out and blue in, measured in logical px
    // so it doesn't grow with the window (glitches add a screen-relative
    // kick). Each is smeared along the split, as a lens disperses, so it
    // reads as a fringe rather than a second copy of the text.
    float ca = Aberration * (1.4 + 9.0 * r2);
    vec2 off = normalize(c * Res + 0.0001) * ca * Px + normalize(c + 0.0001) * (abs(shift) * 0.35 + Glitch * 0.004);
    vec3 c0 = texture2D(Texture, p).rgb;
    vec3 col = vec3(0.0, c0.g, 0.0);
    col.r = (texture2D(Texture, p + off * 0.5).r + texture2D(Texture, p + off).r + texture2D(Texture, p + off * 1.5).r) / 3.0;
    col.b = (texture2D(Texture, p - off * 0.5).b + texture2D(Texture, p - off).b + texture2D(Texture, p - off * 1.5).b) / 3.0;
    // and the light stays put: most of the brightness comes from the
    // unsplit image, so small text keeps its shape (magenta and violet
    // text have no green to hold them in place)
    const vec3 LUMA = vec3(0.299, 0.587, 0.114);
    col = max(col + (dot(c0, LUMA) - dot(col, LUMA)) * 0.7, 0.0);
    vec3 bl;
    bl.r = bloom_at(p + off * 2.0).r;
    bl.g = bloom_at(p).g;
    bl.b = bloom_at(p - off * 2.0).b;
    col += bl * BloomStrength;
    // RGB block corruption during heavy glitch
    float blk = step(1.0 - Glitch * 0.08, hash(floor(p * vec2(24.0, 14.0)) + tq));
    col = mix(col, col.gbr * 1.4, blk);
    // scanlines + aperture
    float sl = 0.5 + 0.5 * sin(gl_FragCoord.y * 3.14159 * 0.6667);
    col *= 1.0 - Scan * (0.16 * sl);
    col *= 1.0 - Scan * 0.035 * step(0.5, fract(gl_FragCoord.x / 3.0));
    // roll bar
    col *= 1.0 + 0.035 * exp(-pow(fract(uv.y * 0.6 - Time * 0.07) - 0.5, 2.0) * 120.0);
    // vignette
    float vig = smoothstep(0.95, 0.25, length(c * vec2(1.0, 1.15)));
    col *= mix(0.35, 1.0, vig);
    // grain
    col += (hash(uv * Res + fract(Time) * 91.7) - 0.5) * 0.045;
    // tonemap: keep neon saturated but roll off highlights
    col = col / (1.0 + max(col - 1.0, 0.0) * 0.35);
    col = mix(col, FlashColor.rgb, Flash);
    col *= 1.0 - Fade;
    // outside the curved screen
    vec2 edge = step(vec2(0.0), p) * step(p, vec2(1.0));
    col *= edge.x * edge.y;
    gl_FragColor = vec4(col, 1.0);
}
"#;

/// Small LCD on the model: nearest-sampled pixel art with a subpixel grid.
pub const LCD_FS: &str = r#"#version 100
precision mediump float;
varying vec2 uv;
uniform sampler2D Texture;
uniform vec2 LcdRes;
uniform float Power;
void main() {
    vec2 px = uv * LcdRes;
    vec3 c = texture2D(Texture, (floor(px) + 0.5) / LcdRes).rgb;
    vec2 f = fract(px);
    float grid = smoothstep(0.0, 0.12, f.x) * smoothstep(0.0, 0.12, f.y);
    c *= 0.55 + 0.45 * grid;
    c *= Power;
    gl_FragColor = vec4(c, 1.0);
}
"#;
