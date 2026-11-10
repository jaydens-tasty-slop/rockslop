//! Text in HUD space, DPI-crisp, in any language, with hacker-movie effects.
//!
//! A string is split into runs by font (the UI's own fonts first, then Noto
//! for the script, then GNU Unifont for anything left), put in visual order
//! (Unicode bidi, for Arabic and Hebrew), shaped by rustybuzz (Arabic
//! joining, Indic conjuncts, Thai marks, kerning) and drawn from a glyph
//! atlas filled on demand. Layouts are cached per string.

use super::{pal, Gfx};
use crate::i18n::{self, Cjk};
use ab_glyph_rasterizer::{point, Point, Rasterizer};
use macroquad::prelude::*;
use rustybuzz::ttf_parser::{self, GlyphId, OutlineBuilder};
use rustybuzz::{Direction, UnicodeBuffer};
use std::cell::{OnceCell, RefCell};
use std::collections::HashMap;
use std::rc::Rc;
use unicode_bidi::{bidi_class, BidiClass, BidiInfo, Level};

#[derive(Clone, Copy, PartialEq)]
pub enum Align {
    Left,
    Center,
    Right,
}

#[derive(Clone, Copy, PartialEq, Eq, Hash, Debug)]
pub enum Face {
    Display,
    Semi,
    Mono,
}

/// The smallest a label shrinks to fit its box, as a share of its size;
/// past that it is cut short with an ellipsis.
pub const MIN_FIT: f32 = 0.62;

const SCRAMBLE: &[u8] = b"0123456789ABCDEF#$%&@*+=<>/\\|{}[]?!";

// ---------------------------------------------------------------- fonts

/// Letter-spacing is for scripts whose letters stand apart. Joined scripts
/// (Arabic, Devanagari, Thai) are never spaced inside a word.
#[derive(Clone, Copy, PartialEq)]
enum Spacing {
    Free,
    Joined,
}

enum Source {
    Plain(&'static [u8]),
    /// xz-compressed: unpacked the first time a glyph is needed from it
    Xz(&'static [u8]),
}

struct File {
    src: Source,
    spacing: Spacing,
    /// size relative to the UI font it stands in for (cap heights matched)
    rel: f32,
    face: OnceCell<Option<(rustybuzz::Face<'static>, f32)>>,
}

impl File {
    const fn new(src: Source, spacing: Spacing, rel: f32) -> File {
        File { src, spacing, rel, face: OnceCell::new() }
    }

    /// The parsed face and its em scale (rel / units per em).
    fn get(&self) -> Option<&(rustybuzz::Face<'static>, f32)> {
        self.face
            .get_or_init(|| {
                let bytes: &'static [u8] = match self.src {
                    Source::Plain(b) => b,
                    Source::Xz(z) => {
                        let mut out = Vec::new();
                        let mut r = lzma_rust2::XzReader::new(z, true);
                        std::io::Read::read_to_end(&mut r, &mut out).ok()?;
                        Box::leak(out.into_boxed_slice())
                    }
                };
                let f = rustybuzz::Face::from_slice(bytes, 0)?;
                let k = self.rel / f.units_per_em() as f32;
                Some((f, k))
            })
            .as_ref()
    }

    fn has(&self, c: char) -> bool {
        self.get().map(|f| f.0.glyph_index(c).is_some()).unwrap_or(false)
    }
}

macro_rules! font {
    ($path:literal) => {
        Source::Plain(include_bytes!(concat!("../../assets/fonts/", $path)))
    };
}

// indices into Fonts::files
const ORBITRON_BLACK: usize = 0;
const ORBITRON_SEMI: usize = 1;
const SHARE_TECH: usize = 2;
const BOLD: usize = 3; // Noto, bold: Latin/Greek/Cyrillic, Arabic, Hebrew, Devanagari, Thai, then CJK SC TC JP KR
const REGULAR: usize = 3 + 9;
const UNIFONT: usize = 3 + 18;

fn files() -> Vec<File> {
    use Spacing::{Free, Joined};
    // Noto's cap height is 0.714 em; Orbitron's 0.72, Share Tech Mono's 0.7,
    // Unifont's 0.625 (40/64).
    let (ob, st) = (1.0, 0.98);
    vec![
        File::new(font!("Orbitron-Black.ttf"), Free, 1.0),
        File::new(font!("Orbitron-SemiBold.ttf"), Free, 1.0),
        File::new(font!("ShareTechMono-Regular.ttf"), Free, 1.0),
        File::new(font!("fallback/NotoSans-Bold.ttf"), Free, ob),
        File::new(font!("fallback/NotoSansArabic-Bold.ttf"), Joined, ob * 1.05),
        File::new(font!("fallback/NotoSansHebrew-Bold.ttf"), Free, ob),
        File::new(font!("fallback/NotoSansDevanagari-Bold.ttf"), Joined, ob),
        File::new(font!("fallback/NotoSansThai-Bold.ttf"), Joined, ob),
        File::new(font!("fallback/NotoSansSC-Bold.otf"), Free, ob * 0.92),
        File::new(font!("fallback/NotoSansTC-Bold.otf"), Free, ob * 0.92),
        File::new(font!("fallback/NotoSansJP-Bold.otf"), Free, ob * 0.92),
        File::new(font!("fallback/NotoSansKR-Bold.otf"), Free, ob * 0.92),
        File::new(font!("fallback/NotoSans-Regular.ttf"), Free, st),
        File::new(font!("fallback/NotoSansArabic-Regular.ttf"), Joined, st * 1.05),
        File::new(font!("fallback/NotoSansHebrew-Regular.ttf"), Free, st),
        File::new(font!("fallback/NotoSansDevanagari-Regular.ttf"), Joined, st),
        File::new(font!("fallback/NotoSansThai-Regular.ttf"), Joined, st),
        File::new(font!("fallback/NotoSansSC-Regular.otf"), Free, st * 0.92),
        File::new(font!("fallback/NotoSansTC-Regular.otf"), Free, st * 0.92),
        File::new(font!("fallback/NotoSansJP-Regular.otf"), Free, st * 0.92),
        File::new(font!("fallback/NotoSansKR-Regular.otf"), Free, st * 0.92),
        File::new(Source::Xz(include_bytes!("../../assets/fonts/fallback/unifont.otf.xz")), Free, 1.15),
    ]
}

#[derive(Clone, Copy)]
struct Glyph {
    font: u16,
    id: u16,
    /// pen position in em of the requested size, before letter-spacing
    x: f32,
    y: f32,
    /// letter-spacing gaps before this glyph
    gap: u16,
}

struct Layout {
    glyphs: Vec<Glyph>,
    adv: f32,
    gaps: u16,
}

#[derive(Clone, Copy)]
struct Slot {
    u: f32,
    v: f32,
    w: f32,
    h: f32,
    /// from the pen (on the baseline) to the bitmap's top left, physical px
    left: f32,
    top: f32,
}

const ATLAS: u16 = 2048;

struct Atlas {
    tex: Texture2D,
    x: u16,
    y: u16,
    row: u16,
    slots: HashMap<(u16, u16, u16), Option<Slot>>,
}

pub struct Fonts {
    files: Vec<File>,
    layouts: RefCell<HashMap<(Face, String), Rc<Layout>>>,
    /// the language the cached layouts were made for (CJK forms differ)
    lang: RefCell<&'static str>,
    atlas: RefCell<Option<Atlas>>,
}

impl Fonts {
    pub fn new() -> Fonts {
        Fonts { files: files(), layouts: Default::default(), lang: RefCell::new(""), atlas: RefCell::new(None) }
    }

    /// `text`'s glyph outlines in Orbitron Black (Latin only, no shaping),
    /// as closed polylines in px from the left end of the baseline, and its
    /// width.
    fn outline(&self, text: &str, size: f32, spacing: f32) -> (Vec<Vec<Vec2>>, f32) {
        let Some((face, k)) = self.files[ORBITRON_BLACK].get() else { return (Vec::new(), 0.0) };
        struct P {
            s: f32,
            x: f32,
            cur: Vec<Vec2>,
            out: Vec<Vec<Vec2>>,
        }
        impl P {
            fn p(&self, x: f32, y: f32) -> Vec2 {
                vec2(self.x + x * self.s, -y * self.s)
            }
            fn last(&self) -> Vec2 {
                self.cur.last().copied().unwrap_or_default()
            }
            fn end(&mut self) {
                if self.cur.len() > 1 {
                    self.out.push(std::mem::take(&mut self.cur));
                }
                self.cur.clear();
            }
        }
        impl OutlineBuilder for P {
            fn move_to(&mut self, x: f32, y: f32) {
                self.end();
                let p = self.p(x, y);
                self.cur.push(p);
            }
            fn line_to(&mut self, x: f32, y: f32) {
                let p = self.p(x, y);
                self.cur.push(p);
            }
            fn quad_to(&mut self, x1: f32, y1: f32, x: f32, y: f32) {
                let (a, c, b) = (self.last(), self.p(x1, y1), self.p(x, y));
                for i in 1..=6 {
                    let t = i as f32 / 6.0;
                    self.cur.push(a.lerp(c, t).lerp(c.lerp(b, t), t));
                }
            }
            fn curve_to(&mut self, x1: f32, y1: f32, x2: f32, y2: f32, x: f32, y: f32) {
                let (a, c1, c2, b) = (self.last(), self.p(x1, y1), self.p(x2, y2), self.p(x, y));
                for i in 1..=8 {
                    let t = i as f32 / 8.0;
                    let u = 1.0 - t;
                    self.cur.push(a * (u * u * u) + c1 * (3.0 * u * u * t) + c2 * (3.0 * u * t * t) + b * (t * t * t));
                }
            }
            fn close(&mut self) {
                self.end();
            }
        }
        let mut p = P { s: size * k, x: 0.0, cur: Vec::new(), out: Vec::new() };
        for c in text.chars() {
            let Some(id) = face.glyph_index(c) else { continue };
            face.outline_glyph(id, &mut p);
            p.end();
            p.x += face.glyph_hor_advance(id).unwrap_or(0) as f32 * p.s + spacing;
        }
        let w = (p.x - spacing).max(0.0);
        (p.out, w)
    }

    /// The fonts to try for `face`, best first.
    fn chain(&self, face: Face) -> Vec<usize> {
        let (first, base) = match face {
            Face::Display => (ORBITRON_BLACK, BOLD),
            Face::Semi => (ORBITRON_SEMI, BOLD),
            Face::Mono => (SHARE_TECH, REGULAR),
        };
        let mut v = vec![first, base, base + 1, base + 2, base + 3, base + 4];
        // CJK: the current language's forms first
        let order = [Cjk::Sc, Cjk::Tc, Cjk::Jp, Cjk::Kr];
        let cur = i18n::current().cjk;
        let pos = |c: Cjk| base + 5 + order.iter().position(|o| *o == c).unwrap_or(0);
        v.push(pos(cur));
        v.extend(order.iter().filter(|c| **c != cur).map(|c| pos(*c)));
        v.push(UNIFONT);
        v
    }

    fn layout(&self, face: Face, text: &str) -> Rc<Layout> {
        let lang = i18n::current().id;
        if *self.lang.borrow() != lang {
            *self.lang.borrow_mut() = lang;
            self.layouts.borrow_mut().clear();
        }
        let key = (face, text.to_string());
        if let Some(l) = self.layouts.borrow().get(&key) {
            return l.clone();
        }
        let l = Rc::new(self.build(face, text));
        let mut cache = self.layouts.borrow_mut();
        if cache.len() > 4096 {
            cache.clear();
        }
        cache.insert(key, l.clone());
        l
    }

    fn build(&self, face: Face, text: &str) -> Layout {
        let chain = self.chain(face);
        // a font for every character: marks stay with their base, and
        // spaces and punctuation with the run they are in, when it has them
        let mut fonts: Vec<(usize, usize)> = Vec::new(); // (byte offset, font)
        let mut prev: Option<usize> = None;
        for (i, c) in text.char_indices() {
            let class = bidi_class(c);
            let attach = matches!(class, BidiClass::NSM | BidiClass::BN);
            let neutral = matches!(
                class,
                BidiClass::WS | BidiClass::CS | BidiClass::ES | BidiClass::ET | BidiClass::ON | BidiClass::S | BidiClass::B
            );
            let f = match prev {
                Some(p) if attach => p,
                Some(p) if neutral && self.files[p].has(c) => p,
                _ => chain.iter().copied().find(|&f| self.files[f].has(c)).unwrap_or(UNIFONT),
            };
            fonts.push((i, f));
            prev = Some(f);
        }
        let font_at = |byte: usize| fonts.partition_point(|x| x.0 <= byte).saturating_sub(1);

        // visual runs
        let has_rtl = text.chars().any(|c| matches!(bidi_class(c), BidiClass::R | BidiClass::AL));
        let mut runs: Vec<(std::ops::Range<usize>, bool)> = Vec::new();
        if has_rtl {
            let level = if i18n::current().rtl { Some(Level::rtl()) } else { None };
            let info = BidiInfo::new(text, level);
            for para in &info.paragraphs {
                let (levels, vr) = info.visual_runs(para, para.range.clone());
                for r in vr {
                    let rtl = levels[r.start].is_rtl();
                    runs.push((r, rtl));
                }
            }
        } else if !text.is_empty() {
            runs.push((0..text.len(), false));
        }

        let mut glyphs = Vec::new();
        let (mut x, mut gaps) = (0.0f32, 0u16);
        let mut last_free: Option<bool> = None;
        for (range, rtl) in runs {
            // split by font, in logical order, then lay out in visual order
            let mut subs: Vec<(std::ops::Range<usize>, usize)> = Vec::new();
            for (i, _) in text[range.clone()].char_indices() {
                let at = range.start + i;
                let f = fonts[font_at(at)].1;
                match subs.last_mut() {
                    Some(s) if s.1 == f => s.0.end = at,
                    _ => subs.push((at..at, f)),
                }
            }
            for s in subs.iter_mut() {
                // extend each sub-run to the end of its last character
                let end = text[s.0.end..].chars().next().map(|c| s.0.end + c.len_utf8()).unwrap_or(s.0.end);
                s.0.end = end;
            }
            if rtl {
                subs.reverse();
            }
            for (sr, f) in subs {
                let Some((bf, k)) = self.files[f].get() else { continue };
                let free = self.files[f].spacing == Spacing::Free;
                let mut buf = UnicodeBuffer::new();
                buf.push_str(&text[sr]);
                buf.set_direction(if rtl { Direction::RightToLeft } else { Direction::LeftToRight });
                buf.guess_segment_properties();
                let out = rustybuzz::shape(bf, &[], buf);
                let mut cluster = u32::MAX;
                for (gi, gp) in out.glyph_infos().iter().zip(out.glyph_positions()) {
                    if gi.cluster != cluster {
                        cluster = gi.cluster;
                        if last_free == Some(true) && free {
                            gaps += 1;
                        }
                        last_free = Some(free);
                    }
                    glyphs.push(Glyph {
                        font: f as u16,
                        id: gi.glyph_id as u16,
                        x: x + gp.x_offset as f32 * k,
                        y: -gp.y_offset as f32 * k,
                        gap: gaps,
                    });
                    x += gp.x_advance as f32 * k;
                }
            }
        }
        Layout { glyphs, adv: x, gaps }
    }

    /// The atlas slot for a glyph at `px` physical pixels (rasterized now
    /// if it is new; None for a blank glyph).
    fn slot(&self, atlas: &mut Atlas, font: u16, id: u16, px: u16) -> Option<Slot> {
        let key = (font, id, px);
        if let Some(s) = atlas.slots.get(&key) {
            return *s;
        }
        let (face, k) = self.files[font as usize].get()?;
        let s = px as f32 * k / self.files[font as usize].rel.max(0.01) * self.files[font as usize].rel;
        let bitmap = raster(face, GlyphId(id), s);
        let slot = bitmap.and_then(|(w, h, left, top, cov)| {
            if w > ATLAS || h > ATLAS {
                return None;
            }
            if atlas.x + w + 1 > ATLAS {
                atlas.x = 1;
                atlas.y += atlas.row + 1;
                atlas.row = 0;
            }
            if atlas.y + h + 1 > ATLAS {
                // full: start again (glyphs are re-rasterized as they're drawn)
                atlas.slots.clear();
                atlas.x = 1;
                atlas.y = 1;
                atlas.row = 0;
            }
            let mut rgba = vec![255u8; w as usize * h as usize * 4];
            for (i, a) in cov.iter().enumerate() {
                rgba[i * 4 + 3] = *a;
            }
            let img = Image { bytes: rgba, width: w, height: h };
            atlas.tex.update_part(&img, atlas.x as i32, atlas.y as i32, w as i32, h as i32);
            let s = Slot { u: atlas.x as f32, v: atlas.y as f32, w: w as f32, h: h as f32, left, top };
            atlas.x += w + 1;
            atlas.row = atlas.row.max(h);
            Some(s)
        });
        atlas.slots.insert(key, slot);
        slot
    }

    fn paint(&self, l: &Layout, origin: Vec2, size: f32, spacing: f32, color: Color, dpi: f32) {
        if l.glyphs.is_empty() || color.a <= 0.0 || size <= 0.0 {
            return;
        }
        let mut guard = self.atlas.borrow_mut();
        let atlas = guard.get_or_insert_with(|| {
            let n = ATLAS as usize;
            let tex = Texture2D::from_rgba8(ATLAS, ATLAS, &vec![0u8; n * n * 4]);
            tex.set_filter(FilterMode::Linear);
            Atlas { tex, x: 1, y: 1, row: 0, slots: HashMap::new() }
        });
        let px = (size * dpi).round().clamp(1.0, 600.0) as u16;
        let snap = px as f32 / (size * dpi);
        for g in &l.glyphs {
            let Some(s) = self.slot(atlas, g.font, g.id, px) else { continue };
            let pen = (origin + vec2(g.x * size + g.gap as f32 * spacing, g.y * size)) * dpi;
            let (x, y) = (pen.x.round() + s.left, pen.y.round() + s.top);
            let _ = snap;
            draw_texture_ex(
                &atlas.tex,
                x / dpi,
                y / dpi,
                color,
                DrawTextureParams {
                    dest_size: Some(vec2(s.w / dpi, s.h / dpi)),
                    source: Some(Rect::new(s.u, s.v, s.w, s.h)),
                    ..Default::default()
                },
            );
        }
    }
}

/// Rasterize one glyph at `scale` px per font unit: (w, h, left, top,
/// coverage), with a pixel of padding all round. None if it has no ink.
fn raster(face: &ttf_parser::Face, id: GlyphId, scale: f32) -> Option<(u16, u16, f32, f32, Vec<u8>)> {
    let bb = face.glyph_bounding_box(id)?;
    let x0 = (bb.x_min as f32 * scale).floor() - 1.0;
    let x1 = (bb.x_max as f32 * scale).ceil() + 1.0;
    let y0 = (-bb.y_max as f32 * scale).floor() - 1.0;
    let y1 = (-bb.y_min as f32 * scale).ceil() + 1.0;
    let (w, h) = ((x1 - x0) as usize, (y1 - y0) as usize);
    if w == 0 || h == 0 || w > 4096 || h > 4096 {
        return None;
    }
    struct B {
        r: Rasterizer,
        s: f32,
        o: (f32, f32),
        last: Point,
        start: Point,
    }
    impl B {
        fn p(&self, x: f32, y: f32) -> Point {
            point(x * self.s - self.o.0, -y * self.s - self.o.1)
        }
    }
    impl OutlineBuilder for B {
        fn move_to(&mut self, x: f32, y: f32) {
            self.last = self.p(x, y);
            self.start = self.last;
        }
        fn line_to(&mut self, x: f32, y: f32) {
            let p = self.p(x, y);
            self.r.draw_line(self.last, p);
            self.last = p;
        }
        fn quad_to(&mut self, x1: f32, y1: f32, x: f32, y: f32) {
            let (c, p) = (self.p(x1, y1), self.p(x, y));
            self.r.draw_quad(self.last, c, p);
            self.last = p;
        }
        fn curve_to(&mut self, x1: f32, y1: f32, x2: f32, y2: f32, x: f32, y: f32) {
            let (c1, c2, p) = (self.p(x1, y1), self.p(x2, y2), self.p(x, y));
            self.r.draw_cubic(self.last, c1, c2, p);
            self.last = p;
        }
        fn close(&mut self) {
            if self.last != self.start {
                self.r.draw_line(self.last, self.start);
            }
            self.last = self.start;
        }
    }
    let mut b = B { r: Rasterizer::new(w, h), s: scale, o: (x0, y0), last: point(0.0, 0.0), start: point(0.0, 0.0) };
    face.outline_glyph(id, &mut b)?;
    let mut cov = vec![0u8; w * h];
    b.r.for_each_pixel_2d(|x, y, a| cov[y as usize * w + x as usize] = (a.clamp(0.0, 1.0) * 255.0) as u8);
    Some((w as u16, h as u16, x0, y0, cov))
}

// ---------------------------------------------------------------- measuring

/// `text`'s outlines in the display face (Latin only): closed polylines in
/// px from the left end of the baseline, and the width.
pub fn outline(g: &Gfx, text: &str, size: f32, spacing: f32) -> (Vec<Vec<Vec2>>, f32) {
    g.text.outline(text, size, spacing)
}

pub fn measure(g: &Gfx, f: Face, text: &str, size: f32) -> Vec2 {
    vec2(g.text.layout(f, text).adv * size, size * 0.72)
}

/// Width of `text` as `draw_spaced` lays it out.
pub fn spaced_width(g: &Gfx, f: Face, text: &str, size: f32, spacing: f32) -> f32 {
    let l = g.text.layout(f, text);
    l.adv * size + l.gaps as f32 * spacing
}

/// The size and spacing at which `text` fits `max_w` (never below
/// MIN_FIT of the asked size).
pub fn fit(g: &Gfx, f: Face, text: &str, size: f32, spacing: f32, max_w: f32) -> (f32, f32) {
    let w = spaced_width(g, f, text, size, spacing);
    if w <= max_w || w <= 0.0 {
        return (size, spacing);
    }
    let k = (max_w / w).max(MIN_FIT);
    (size * k, spacing * k)
}

/// `text` cut short with an ellipsis to fit `max_w`.
pub fn ellipsize(g: &Gfx, f: Face, text: &str, size: f32, spacing: f32, max_w: f32) -> String {
    if spaced_width(g, f, text, size, spacing) <= max_w {
        return text.to_string();
    }
    let chars: Vec<char> = text.chars().collect();
    let (mut lo, mut hi) = (0usize, chars.len());
    while lo < hi {
        let mid = (lo + hi).div_ceil(2);
        let s: String = chars[..mid].iter().collect::<String>() + "…";
        if spaced_width(g, f, &s, size, spacing) <= max_w {
            lo = mid;
        } else {
            hi = mid - 1;
        }
    }
    // don't leave a dangling mark or a trailing space
    let mut n = lo;
    while n > 0 && (matches!(bidi_class(chars[n - 1]), BidiClass::WS) || matches!(bidi_class(chars[n]), BidiClass::NSM)) {
        n -= 1;
    }
    chars[..n].iter().collect::<String>() + "…"
}

/// Wrap `text` into lines at most `width` wide: at spaces, or anywhere
/// between characters for a word too long for a line (CJK and Thai have
/// no spaces).
pub fn wrap(g: &Gfx, f: Face, text: &str, size: f32, width: f32) -> Vec<String> {
    let fits = |s: &str| measure(g, f, s, size).x <= width;
    let mut out = Vec::new();
    for para in text.split('\n') {
        let mut cur = String::new();
        for word in para.split(' ') {
            let try_ = if cur.is_empty() { word.to_string() } else { format!("{cur} {word}") };
            if fits(&try_) {
                cur = try_;
                continue;
            }
            if !cur.is_empty() {
                if fits(word) {
                    // it goes whole on the next line
                    out.push(std::mem::take(&mut cur));
                    cur = word.to_string();
                    continue;
                }
                // too long for any line (a run of CJK, say): it starts here
                cur.push(' ');
            }
            // break the word itself where it must
            for c in word.chars() {
                let mut t = cur.clone();
                t.push(c);
                if !fits(&t) && !cur.is_empty() && !matches!(bidi_class(c), BidiClass::NSM) {
                    out.push(std::mem::take(&mut cur));
                    cur.push(c);
                } else {
                    cur = t;
                }
            }
        }
        out.push(cur);
    }
    out
}

// ---------------------------------------------------------------- drawing

fn x_for(align: Align, x: f32, w: f32) -> f32 {
    match align {
        Align::Left => x,
        Align::Center => x - w / 2.0,
        Align::Right => x - w,
    }
}

/// Draw `text` with its baseline at `pos.y`; returns its width.
pub fn draw(g: &Gfx, f: Face, text: &str, pos: Vec2, size: f32, color: Color, align: Align) -> f32 {
    draw_spaced(g, f, text, pos, size, color, align, 0.0)
}

/// Letter-spaced text (for the display titles and labels).
#[allow(clippy::too_many_arguments)]
pub fn draw_spaced(g: &Gfx, f: Face, text: &str, pos: Vec2, size: f32, color: Color, align: Align, spacing: f32) -> f32 {
    let l = g.text.layout(f, text);
    let w = l.adv * size + l.gaps as f32 * spacing;
    g.text.paint(&l, vec2(x_for(align, pos.x, w), pos.y), size, spacing, color, g.density);
    w
}

/// `draw`, shrunk (then cut short) to fit `max_w`.
#[allow(clippy::too_many_arguments)]
pub fn draw_fit(g: &Gfx, f: Face, text: &str, pos: Vec2, size: f32, color: Color, align: Align, max_w: f32) -> f32 {
    draw_spaced_fit(g, f, text, pos, size, color, align, 0.0, max_w)
}

/// `draw_spaced`, shrunk (then cut short) to fit `max_w`.
#[allow(clippy::too_many_arguments)]
pub fn draw_spaced_fit(g: &Gfx, f: Face, text: &str, pos: Vec2, size: f32, color: Color, align: Align, spacing: f32, max_w: f32) -> f32 {
    let (sz, sp) = fit(g, f, text, size, spacing, max_w);
    if spaced_width(g, f, text, sz, sp) <= max_w + 0.5 {
        return draw_spaced(g, f, text, pos, sz, color, align, sp);
    }
    let cut = ellipsize(g, f, text, sz, sp, max_w);
    draw_spaced(g, f, &cut, pos, sz, color, align, sp)
}

fn hash(a: u32) -> u32 {
    let mut x = a.wrapping_mul(0x9E37_79B9) ^ 0x85EB_CA6B;
    x ^= x >> 15;
    x = x.wrapping_mul(0x2C1B_3C6D);
    x ^= x >> 12;
    x
}

/// "Decrypting" reveal: characters left of `t*len` are final, the next few
/// cycle through random glyphs, the rest are hidden.
pub fn decode(text: &str, t: f32, time: f32, seed: u32) -> String {
    if t >= 1.0 {
        return text.to_string();
    }
    let n = text.chars().count();
    let shown = (t.clamp(0.0, 1.0) * (n as f32 + 4.0)) as usize;
    let tick = (time * 30.0) as u32;
    text.chars()
        .enumerate()
        .map(|(i, c)| {
            if c == ' ' || i < shown.saturating_sub(4) {
                c
            } else if i < shown {
                let h = hash(seed ^ (i as u32 * 7919) ^ tick);
                SCRAMBLE[(h as usize) % SCRAMBLE.len()] as char
            } else {
                ' '
            }
        })
        .collect()
}

/// Big glitchy title: RGB split ghosts, slice offsets and a bright core.
/// Shrinks to fit `max_w`.
#[allow(clippy::too_many_arguments)]
pub fn glitch_title(g: &Gfx, text: &str, pos: Vec2, size: f32, color: Color, align: Align, amount: f32, time: f32, spacing: f32, max_w: f32) {
    let (size, spacing) = fit(g, Face::Display, text, size, spacing, max_w);
    let text = &ellipsize(g, Face::Display, text, size, spacing, max_w);
    let tick = (time * 18.0) as u32;
    let h = hash(tick ^ 0xA11CE);
    let burst = (h % 1000) as f32 / 1000.0 < 0.08 + amount * 0.6;
    let off = if burst { ((h >> 10) % 9) as f32 - 4.0 } else { 0.0 } * (0.5 + amount);
    let split = 1.5 + amount * 5.0 + if burst { 3.0 } else { 0.0 };
    draw_spaced(g, Face::Display, text, pos + vec2(-split + off, 0.0), size, pal::alpha(pal::MAGENTA, 0.55 * color.a), align, spacing);
    draw_spaced(g, Face::Display, text, pos + vec2(split - off * 0.5, 0.0), size, pal::alpha(pal::CYAN, 0.55 * color.a), align, spacing);
    let w = draw_spaced(g, Face::Display, text, pos, size, color, align, spacing);
    if burst && amount > 0.05 {
        // a stray horizontal tear across the title
        let x0 = x_for(align, pos.x, w);
        let y = pos.y - size * (0.2 + ((h >> 4) % 60) as f32 / 100.0);
        draw_rectangle(x0 + off * 3.0, y, w, 2.0 + (h % 4) as f32, pal::alpha(pal::ICE, 0.5 * color.a));
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn fonts() -> Fonts {
        Fonts { files: files(), layouts: Default::default(), lang: RefCell::new(""), atlas: RefCell::new(None) }
    }

    /// Every character of every translation has a real glyph somewhere in
    /// the chain (no tofu), and every font file parses.
    #[test]
    fn every_translation_has_glyphs() {
        let f = fonts();
        for (i, file) in f.files.iter().enumerate() {
            assert!(file.get().is_some(), "font {i} does not parse");
        }
        let mut missing = Vec::new();
        for lang in i18n::LANGS {
            i18n::set(lang.id);
            let chain = f.chain(Face::Mono);
            let bold = f.chain(Face::Display);
            for k in i18n::keys() {
                for c in i18n::tr(k).chars().chain(lang.name.chars()) {
                    if c.is_whitespace() || c.is_control() {
                        continue;
                    }
                    for ch in [&chain, &bold] {
                        if !ch.iter().any(|&x| x != UNIFONT && f.files[x].has(c)) {
                            missing.push(format!("{} U+{:04X} {c}", lang.id, c as u32));
                        }
                    }
                }
            }
        }
        i18n::set("english");
        missing.sort();
        missing.dedup();
        assert!(missing.is_empty(), "only Unifont has: {missing:?}");
    }

    /// Arabic comes out joined (contextual forms, not the isolated ones)
    /// and right to left; Devanagari forms conjuncts.
    #[test]
    fn shaping() {
        let f = fonts();
        // "شكرا": the shaped glyphs are not the plain cmap glyphs
        let l = f.build(Face::Mono, "شكرا");
        let (face, _) = f.files[REGULAR + 1].get().unwrap();
        let plain: Vec<u16> = "شكرا".chars().map(|c| face.glyph_index(c).unwrap().0).collect();
        let shaped: Vec<u16> = l.glyphs.iter().map(|g| g.id).collect();
        // (Noto splits some letters into a base and its dots, so there may
        // be more glyphs than letters)
        assert!(shaped.len() >= 4, "{shaped:?}");
        assert!(!plain.iter().all(|p| shaped.contains(p)), "not joined: {shaped:?} vs {plain:?}");
        // right to left: the alef (last letter) is drawn first, leftmost,
        // as the plain isolated-or-final form, and the shin last
        let first = l.glyphs.iter().map(|g| g.x).fold(f32::MAX, f32::min);
        assert_eq!(l.glyphs[0].x, first, "the first glyph drawn is the leftmost");
        assert!(l.glyphs.iter().all(|g| g.font as usize == REGULAR + 1));
        // क्ष is one conjunct glyph, not three
        let l = f.build(Face::Mono, "क्ष");
        assert!(l.glyphs.len() < 3, "{} glyphs", l.glyphs.len());
        // letter-spacing gaps only between free-standing letters
        assert_eq!(f.build(Face::Semi, "ABC").gaps, 2);
        assert_eq!(f.build(Face::Semi, "شكرا").gaps, 0);
    }

    #[test]
    fn unifont_catches_the_rest() {
        let f = fonts();
        // Georgian is in no Noto subset we ship
        let l = f.build(Face::Mono, "ქ");
        assert_eq!(l.glyphs[0].font as usize, UNIFONT);
        assert_ne!(l.glyphs[0].id, 0, "tofu");
    }
}
