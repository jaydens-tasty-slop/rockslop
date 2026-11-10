//! The credits roll: who made this flasher (CREDITS.md: its sections and
//! bullets, with links) in one pane, and in the other everyone in Rockbox's
//! own credits list, the names its credits plugin scrolls. Hovering a pane
//! stops it, the wheel scrolls it, and a link opens in the browser.

use crate::anim::{approach, clamp01, ease_out_cubic};
use crate::audio::Sfx;
use crate::gfx::text::{self, Align, Face};
use crate::gfx::{pal, Gfx};
use crate::i18n::{self, tf, tr};
use crate::ui::{self, Ui};
use macroquad::prelude::*;

const OURS: &str = include_str!("../CREDITS.md");
const ROCKBOX: &str = include_str!("../assets/rockbox/CREDITS");
/// Rockbox's credits list starts with its logo and this line.
pub const FRIENDS: &str = "People that have contributed to the project, one way or another. Friends!";

#[derive(Debug, PartialEq)]
pub struct Entry {
    pub title: &'static str,
    pub by: Option<&'static str>,
    pub licence: Option<&'static str>,
    pub url: Option<&'static str>,
}

#[derive(Debug)]
pub struct Section {
    /// a key in strings.txt
    pub heading: &'static str,
    pub entries: Vec<Entry>,
}

/// CREDITS.md's sections and bullets (the format is in a comment at its top).
pub fn ours() -> Vec<Section> {
    let mut out: Vec<Section> = Vec::new();
    let mut comment = false;
    for line in OURS.lines() {
        if comment || line.starts_with("<!--") {
            comment = !line.contains("-->");
            continue;
        }
        if let Some(h) = line.strip_prefix("## ") {
            out.push(Section { heading: h.trim(), entries: Vec::new() });
        } else if let (Some(rest), Some(s)) = (line.strip_prefix("- **"), out.last_mut()) {
            s.entries.extend(entry(rest));
        }
    }
    out
}

/// `Title** by Author [licence] <url>`
fn entry(rest: &'static str) -> Option<Entry> {
    let (title, mut r) = rest.split_once("**")?;
    let mut url = None;
    if let Some(i) = r.find(" <") {
        url = r[i + 2..].split_once('>').map(|u| u.0);
        r = &r[..i];
    }
    let mut licence = None;
    if let Some(i) = r.find(" [") {
        licence = r[i + 2..].split_once(']').map(|l| l.0);
        r = &r[..i];
    }
    let by = r.strip_prefix(" by ").map(str::trim).filter(|s| !s.is_empty());
    Some(Entry { title, by, licence, url })
}

/// Rockbox's docs/CREDITS: the logo above the names, and the names.
pub fn rockbox() -> (Vec<&'static str>, Vec<&'static str>) {
    let lines: Vec<&'static str> = ROCKBOX.lines().collect();
    let k = lines.iter().position(|l| l.starts_with(FRIENDS)).unwrap_or(0);
    let logo = lines[..k].iter().map(|l| l.trim_end()).filter(|l| !l.is_empty()).collect();
    let names = lines[k + 1..].iter().map(|l| l.trim()).filter(|l| !l.is_empty()).collect();
    (logo, names)
}

/// Open a web link in the browser (http(s) only; never blocks).
pub fn open_url(url: &str) {
    if !(url.starts_with("https://") || url.starts_with("http://")) {
        return;
    }
    use std::process::Command;
    let child = if cfg!(windows) {
        // not `cmd /C start`: it splits the URL at its '&'s
        Command::new("rundll32").args(["url.dll,FileProtocolHandler", url]).spawn()
    } else if cfg!(target_os = "macos") {
        Command::new("open").arg(url).spawn()
    } else {
        Command::new("xdg-open").arg(url).spawn()
    };
    match child {
        Ok(mut c) => {
            std::thread::spawn(move || c.wait());
        }
        Err(e) => eprintln!("open {url}: {e}"),
    }
}

/// A link as shown: no scheme, no "www.".
fn short(url: &str) -> &str {
    let u = url.split_once("://").map(|p| p.1).unwrap_or(url);
    let u = u.strip_prefix("www.").unwrap_or(u);
    u.trim_end_matches('/')
}

/// A scrolling pane's position: it drifts, stops under the pointer and
/// follows the wheel.
struct Scroll {
    off: f32,
    v: f32,
    /// times it has come all the way round
    laps: u32,
}

impl Scroll {
    fn step(&mut self, ui: &Ui, view: Rect, speed: f32, len: f32) {
        let over = view.contains(ui.mouse);
        self.v = approach(self.v, if over { 0.0 } else { speed }, 5.0, ui.dt);
        self.off += self.v * ui.dt;
        if over {
            self.off -= mouse_wheel().1.clamp(-3.0, 3.0) * 48.0;
        }
        if self.off >= len.max(1.0) {
            self.laps += 1;
        }
        self.off = self.off.rem_euclid(len.max(1.0));
    }
}

enum Row {
    Heading(usize, &'static str),
    Entry(Entry),
}

impl Row {
    fn height(&self) -> f32 {
        match self {
            Row::Heading(..) => 50.0,
            Row::Entry(e) => 22.0 + if e.by.is_some() || e.licence.is_some() { 16.0 } else { 0.0 } + if e.url.is_some() { 16.0 } else { 0.0 } + 12.0,
        }
    }
}

pub struct Roll {
    rows: Vec<Row>,
    logo: Vec<&'static str>,
    names: Vec<&'static str>,
    left: Scroll,
    right: Scroll,
}

impl Roll {
    pub fn new() -> Roll {
        let mut rows = Vec::new();
        for (k, s) in ours().into_iter().enumerate() {
            rows.push(Row::Heading(k + 1, s.heading));
            rows.extend(s.entries.into_iter().map(Row::Entry));
        }
        let (logo, names) = rockbox();
        Roll { rows, logo, names, left: Scroll { off: 0.0, v: 0.0, laps: 0 }, right: Scroll { off: 0.0, v: 0.0, laps: 0 } }
    }

    pub fn reset(&mut self, view_h: f32) {
        // the first lines are already on screen
        self.left.off = view_h * 0.82;
        self.right.off = view_h * 0.82;
        self.left.laps = 0;
        self.right.laps = 0;
    }

    /// This flasher's credits (the left pane) have rolled all the way
    /// through since the roll started.
    pub fn rolled(&self) -> bool {
        self.left.laps > 0
    }

    pub fn names(&self) -> &[&'static str] {
        &self.names
    }

    /// This flasher's credits, in `r`.
    pub fn draw_ours(&mut self, ui: &mut Ui, g: &Gfx, r: Rect, appear: f32, beat: f32) {
        let t = ui.time;
        ui::panel(r, pal::MAGENTA, appear);
        if appear < 0.3 {
            return;
        }
        ui::header(g, vec2(r.x + 22.0, r.y + 28.0), r.w - 44.0, tr("THIS FLASHER"), pal::MAGENTA, appear, t);
        let view = Rect::new(r.x + 22.0, r.y + 50.0, r.w - 44.0, r.h - 78.0);
        text::draw_fit(
            g,
            Face::Mono,
            tr("links open in your browser"),
            vec2(r.x + r.w - 22.0, r.y + r.h - 12.0),
            10.0,
            pal::alpha(pal::DIM, appear),
            Align::Right,
            r.w - 44.0,
        );
        let len: f32 = self.rows.iter().map(Row::height).sum::<f32>() + view.h * 0.5;
        self.left.step(ui, view, 34.0, len);
        if view.contains(ui.mouse) {
            ui.captured = true;
        }
        let mut open = None;
        // two passes of the list, so it wraps without a seam
        for pass in 0..2 {
            let mut y = view.y + view.h - self.left.off + pass as f32 * len;
            for (k, row) in self.rows.iter().enumerate() {
                let h = row.height();
                let top = y;
                y += h;
                if top + h < view.y || top > view.y + view.h {
                    continue;
                }
                // fade out before the first line of ink reaches the pane's
                // header, and in at the bottom; decode in from the bottom
                let ink = top + if matches!(row, Row::Heading(..)) { 18.0 } else { 4.0 };
                let fade = clamp01((ink - view.y) / 30.0) * clamp01((view.y + view.h - top - h * 0.5) / 36.0) * appear;
                let reveal = clamp01((view.y + view.h - top) / 90.0);
                if fade <= 0.01 {
                    continue;
                }
                let seed = k as u32 * 31;
                match row {
                    Row::Heading(n, h) => {
                        let ty = top + 30.0;
                        let c = pal::mix(pal::MAGENTA, pal::CYAN, 0.5 + 0.5 * (t * 0.8 + *n as f32).sin());
                        let nw = text::draw(g, Face::Mono, &format!("{n:02}"), vec2(view.x, ty), 11.0, pal::alpha(pal::DIM, fade), Align::Left);
                        let label = text::decode(tr(h), reveal, t, seed);
                        let w = text::draw_spaced_fit(
                            g,
                            Face::Semi,
                            &label,
                            vec2(view.x + nw + 10.0, ty),
                            15.0,
                            pal::alpha(pal::mix(c, pal::WHITE, 0.25 + 0.4 * beat), fade),
                            Align::Left,
                            3.0,
                            view.w - nw - 30.0,
                        );
                        let lx = view.x + nw + w + 20.0;
                        if lx < view.x + view.w {
                            ui::line(vec2(lx, ty - 5.0), vec2(view.x + view.w, ty - 5.0), 1.0, pal::alpha(pal::scale(c, 0.5), fade));
                        }
                    }
                    Row::Entry(e) => {
                        let mut ly = top + 16.0;
                        // chevron
                        let cx = view.x + 4.0;
                        draw_triangle(vec2(cx, ly - 9.0), vec2(cx + 5.0, ly - 5.0), vec2(cx, ly - 1.0), pal::alpha(pal::MAGENTA, fade));
                        let title = text::decode(e.title, reveal, t, seed);
                        text::draw_spaced_fit(g, Face::Semi, &title, vec2(view.x + 16.0, ly), 14.0, pal::alpha(pal::ICE, fade), Align::Left, 1.0, view.w - 16.0);
                        if e.by.is_some() || e.licence.is_some() {
                            ly += 16.0;
                            let mut lw = 0.0;
                            if let Some(l) = e.licence {
                                // a tag, right-aligned
                                lw = text::measure(g, Face::Mono, l, 9.5).x + 12.0;
                                let lr = Rect::new(view.x + view.w - lw, ly - 10.0, lw, 14.0);
                                ui::polyline(&ui::chamfer(lr, 4.0), 1.0, pal::alpha(pal::scale(pal::AMBER, 0.8), fade), true);
                                text::draw(g, Face::Mono, l, vec2(lr.x + 6.0, ly + 0.5), 9.5, pal::alpha(pal::AMBER, fade), Align::Left);
                            }
                            if let Some(b) = e.by {
                                let b = text::decode(b, reveal, t, seed + 1);
                                text::draw_fit(
                                    g,
                                    Face::Mono,
                                    &b,
                                    vec2(view.x + 16.0, ly),
                                    12.0,
                                    pal::alpha(pal::scale(pal::CYAN, 0.8), fade),
                                    Align::Left,
                                    view.w - 30.0 - lw,
                                );
                            }
                        }
                        if let Some(u) = e.url {
                            ly += 16.0;
                            let s = text::ellipsize(g, Face::Mono, short(u), 11.0, 0.0, view.w - 34.0);
                            let w = text::measure(g, Face::Mono, &s, 11.0).x;
                            let hit = Rect::new(view.x + 12.0, ly - 12.0, w + 22.0, 16.0);
                            let over = fade > 0.5 && hit.contains(ui.mouse);
                            let hv = ui.hover(u, over);
                            if over {
                                ui.cursor_hand = true;
                            }
                            if ui.click(over, Sfx::Click) {
                                open = Some(u);
                            }
                            let c = pal::mix(pal::scale(pal::VIOLET, 1.2), pal::WHITE, hv);
                            text::draw(g, Face::Mono, &s, vec2(view.x + 16.0, ly), 11.0, pal::alpha(c, fade), Align::Left);
                            // underline, and an arrow out
                            ui::line(vec2(view.x + 16.0, ly + 2.5), vec2(view.x + 16.0 + w * (0.3 + 0.7 * hv), ly + 2.5), 1.0, pal::alpha(c, fade * (0.4 + 0.6 * hv)));
                            let ax = view.x + 22.0 + w + 2.0 * hv;
                            ui::line(vec2(ax, ly - 1.0), vec2(ax + 6.0, ly - 7.0), 1.2, pal::alpha(c, fade));
                            ui::line(vec2(ax + 2.0, ly - 7.0), vec2(ax + 6.0, ly - 7.0), 1.2, pal::alpha(c, fade));
                            ui::line(vec2(ax + 6.0, ly - 7.0), vec2(ax + 6.0, ly - 3.0), 1.2, pal::alpha(c, fade));
                        }
                    }
                }
            }
        }
        if let Some(u) = open {
            open_url(u);
        }
    }

    /// Rockbox's contributors, in `r`.
    pub fn draw_rockbox(&mut self, ui: &mut Ui, g: &Gfx, r: Rect, appear: f32, beat: f32) {
        let t = ui.time;
        ui::panel(r, pal::ACID, appear);
        if appear < 0.3 {
            return;
        }
        ui::header(g, vec2(r.x + 22.0, r.y + 28.0), r.w - 44.0, tr("ROCKBOX CONTRIBUTORS"), pal::ACID, appear, t);
        text::draw_fit(
            g,
            Face::Mono,
            &tf("{0} contributors", &[&self.names.len()]),
            vec2(r.x + r.w - 22.0, r.y + r.h - 12.0),
            10.0,
            pal::alpha(pal::DIM, appear),
            Align::Right,
            r.w - 44.0,
        );
        let view = Rect::new(r.x + 22.0, r.y + 50.0, r.w - 44.0, r.h - 78.0);
        // the logo, as wide as the pane allows
        let widest = self.logo.iter().map(|l| text::measure(g, Face::Mono, l, 10.0).x).fold(1.0, f32::max);
        let ls = (10.0 * view.w / widest).min(11.0);
        let llh = ls * 1.22;
        let friends = text::wrap(g, Face::Mono, tr(FRIENDS), 12.0, view.w);
        let cols = if view.w > 360.0 { 2 } else { 1 };
        let nh = 18.0;
        let head = self.logo.len() as f32 * llh + 18.0 + friends.len() as f32 * 17.0 + 22.0;
        let len = head + (self.names.len().div_ceil(cols)) as f32 * nh + view.h * 0.5;
        self.right.step(ui, view, 56.0, len);
        if view.contains(ui.mouse) {
            ui.captured = true;
        }
        let fade_at = |y: f32| clamp01((y - view.y) / 36.0) * clamp01((view.y + view.h - y) / 36.0) * appear;
        // a bright scanline sweeping up the pane, on the beat
        let scan = view.y + view.h - ((t * 0.31) % 1.0) * view.h;
        let colw = view.w / cols as f32;
        for pass in 0..2 {
            let base = view.y + view.h - self.right.off + pass as f32 * len;
            if base > view.y + view.h || base + len < view.y {
                continue;
            }
            let mut y = base;
            for (k, l) in self.logo.iter().enumerate() {
                y += llh;
                let f = fade_at(y);
                if f > 0.01 {
                    let c = pal::mix(pal::ACID, pal::CYAN, k as f32 / self.logo.len() as f32);
                    text::draw(g, Face::Mono, l, vec2(view.x, y), ls, pal::alpha(c, f), Align::Left);
                }
            }
            y += 18.0;
            for l in &friends {
                y += 17.0;
                let f = fade_at(y);
                if f > 0.01 {
                    text::draw(g, Face::Mono, l, vec2(view.x, y), 12.0, pal::alpha(pal::ICE, f), Align::Left);
                }
            }
            y += 22.0;
            let first = (((view.y - y) / nh).floor().max(0.0) as usize) * cols;
            for (i, name) in self.names.iter().enumerate().skip(first) {
                let ny = y + (i / cols) as f32 * nh + nh;
                if ny > view.y + view.h + nh {
                    break;
                }
                let f = fade_at(ny);
                if f <= 0.01 {
                    continue;
                }
                let x = view.x + (i % cols) as f32 * colw;
                let reveal = clamp01((view.y + view.h - ny) / 70.0);
                // the scanline lights what it passes; the beat sparks a few
                let near = clamp01(1.0 - (ny - scan).abs() / 26.0);
                let spark = if (i * 7 + (t * 6.0) as usize) % 23 == 0 { beat } else { 0.0 };
                let c = pal::mix(pal::mix(pal::scale(pal::TEAL, 0.85), pal::ACID, (i % 5) as f32 / 8.0), pal::WHITE, (near * 0.8 + spark).min(1.0));
                let s = text::decode(name, reveal, t, i as u32);
                text::draw_fit(g, Face::Mono, &s, vec2(x, ny), 12.0, pal::alpha(c, f), Align::Left, colw - 10.0);
            }
        }
        draw_rectangle(view.x, scan, view.w, 1.0, pal::alpha(pal::ACID, 0.25 * appear * fade_at(scan)));
    }
}

/// A wave scroller across `x0..x1` at baseline `y`: word by word (or
/// character by character where words aren't spaced), each riding a sine
/// wave and cycling colour. Right-to-left text scrolls the other way, whole.
pub fn scroller(g: &Gfx, y: f32, x0: f32, x1: f32, msg: &str, t: f32, beat: f32, appear: f32) {
    let size = 20.0;
    let speed = 110.0;
    let span = x1 - x0;
    let edge = |x: f32, w: f32| clamp01((x - x0) / 90.0) * clamp01((x1 - x - w) / 90.0) * appear;
    let colour = |x: f32| pal::mix(pal::MAGENTA, pal::CYAN, 0.5 + 0.5 * (x * 0.006 - t * 1.7).sin());
    if i18n::current().rtl {
        let w = text::measure(g, Face::Semi, msg, size).x;
        let x = x0 - w + (t * speed) % (w + span);
        text::draw(g, Face::Semi, msg, vec2(x, y), size, pal::alpha(colour(x), appear), Align::Left);
        return;
    }
    let units = units(msg);
    let widths: Vec<f32> = units.iter().map(|u| text::measure(g, Face::Semi, u, size).x).collect();
    let text_w: f32 = widths.iter().sum();
    let total = text_w + span * 0.35;
    // the newest copy comes in at the right; the ones before it carry on
    // out at the left
    let x = x1 - (t * speed) % total;
    let mut base = x;
    while base + text_w > x0 {
        let mut ux = base;
        base -= total;
        for (u, w) in units.iter().zip(&widths) {
            if ux + w > x0 && ux < x1 {
                let wave = crate::clippy::wave(ux, t, beat);
                let a = edge(ux, *w);
                if a > 0.01 {
                    let c = colour(ux);
                    // a soft echo under each word
                    text::draw(g, Face::Semi, u, vec2(ux + 2.0, y + wave * 0.6 + 6.0), size, pal::alpha(pal::scale(c, 0.35), a * 0.4), Align::Left);
                    text::draw(g, Face::Semi, u, vec2(ux, y + wave), size, pal::alpha(pal::mix(c, pal::WHITE, 0.2 + 0.4 * beat), a), Align::Left);
                }
            }
            ux += w;
        }
    }
}

/// Words (with their trailing space), or single characters of scripts
/// written without spaces.
fn units(s: &str) -> Vec<String> {
    let unspaced = |c: char| matches!(c as u32, 0x3040..=0x30FF | 0x3400..=0x9FFF | 0xF900..=0xFAFF | 0xFF00..=0xFFEF | 0x3000..=0x303F);
    let mut out = Vec::new();
    let mut cur = String::new();
    for c in s.chars() {
        if unspaced(c) {
            if !cur.is_empty() {
                out.push(std::mem::take(&mut cur));
            }
            out.push(c.to_string());
        } else {
            cur.push(c);
            if c == ' ' {
                out.push(std::mem::take(&mut cur));
            }
        }
    }
    if !cur.is_empty() {
        out.push(cur);
    }
    out
}

/// The roll's panes and the wave under them, for a screen of size `sz`:
/// (left pane, right pane, wave baseline).
pub fn layout(sz: Vec2) -> (Rect, Rect, f32) {
    let w = (sz.x * 0.34).clamp(300.0, 520.0);
    let top = 96.0;
    let h = sz.y - top - 128.0;
    (Rect::new(36.0, top, w, h), Rect::new(sz.x - 36.0 - w, top, w, h), sz.y - 70.0)
}

/// Fade-in for the panes (they unfold after the screen comes up).
pub fn appear(st: f32, delay: f32) -> f32 {
    ease_out_cubic(clamp01((st - delay) / 0.6))
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn credits_parse() {
        let s = ours();
        let heads: Vec<&str> = s.iter().map(|s| s.heading).collect();
        assert_eq!(heads, ["Made by", "Device protocols and payloads", "Music", "Voice lines", "Sound effects", "Fonts", "Rust crates", "Reference photos"]);
        let keys = i18n::keys();
        for h in &heads {
            assert!(keys.contains(h), "heading {h:?} is not in strings.txt");
        }
        let music = &s[2].entries;
        assert_eq!(music.len(), 8);
        assert_eq!(
            music[4],
            Entry {
                title: "Kick the beat",
                by: Some("Andreas Viklund (TSEC)"),
                licence: Some("CC BY 4.0"),
                url: Some("https://modarchive.org/index.php?request=view_by_moduleid&query=34320"),
            }
        );
        let crate_ = s[6].entries.iter().find(|e| e.title == "rusb").unwrap();
        assert_eq!((crate_.by, crate_.licence, crate_.url), (None, Some("MIT"), Some("https://crates.io/crates/rusb")));
        assert!(s.iter().all(|s| !s.entries.is_empty()), "an empty section");
    }

    #[test]
    fn rockbox_names() {
        let (logo, names) = rockbox();
        assert!(logo.len() >= 6 && logo[0].contains("__________"), "{logo:?}");
        assert!(names.len() > 700, "{}", names.len());
        assert_eq!(names[0], "Björn Stenberg");
        assert!(!names.iter().any(|n| n.starts_with(FRIENDS)));
    }

    #[test]
    fn scroller_units() {
        assert_eq!(units("ab cd"), ["ab ", "cd"]);
        assert_eq!(units("日本 x"), ["日", "本", " ", "x"]);
    }
}
