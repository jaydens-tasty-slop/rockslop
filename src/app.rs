//! The flasher's screens: boot, target select, download-mode link, payload,
//! execute, complete / fail. Both player models persist across screens on
//! springs, so every transition is a continuous move, never a cut.

use crate::anim::{approach, clamp01, ease_in_out_cubic, ease_out_back, ease_out_cubic, ease_out_expo, window, Spring, Spring3};
use crate::audio::{Audio, Sfx};
use crate::clippy::{self, Clippy, Say, Tip};
use crate::gfx::model::{self, Cable, Feature, Kind, Player, Style};
use crate::gfx::scene::World;
use crate::gfx::text::{self, Align, Face};
use crate::gfx::{pal, Gfx, LineBatch, Post};
use crate::credits::{self, Roll};
use crate::i18n::{self, tr, tf};
use crate::nowplaying::NowPlaying;
use crate::picker::Picker;
use crate::ui::{self, Tone, Ui};
use macroquad::prelude::*;
use rockslop::device::backup::{self, Backup};
use rockslop::device::{real::Real, sim::Sim, Cmd, Event, Hub, Lvl, Mode, Outcome, Phase, Sect};
use rockslop::pkg::Package;
use std::f32::consts::{PI, TAU};
use std::path::PathBuf;
use std::sync::mpsc::{channel, Receiver};

#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum Screen {
    Boot,
    Select,
    Link,
    Payload,
    Execute,
    Complete,
    Credits,
    Fail,
}

pub struct Opts {
    pub demo: bool,
    /// click through the flow by itself (demo / screenshots)
    pub auto: bool,
    pub kind: Kind,
    pub package: Option<PathBuf>,
    /// a fixed pointer position (screenshots)
    pub mouse: Option<Vec2>,
    /// Clippy speaks up at once (screenshots)
    pub tips_now: bool,
}

struct Actor {
    p: Player,
    pos: Spring3,
    yaw: Spring,
    pitch: Spring,
    scale: Spring,
    reveal: Spring,
    glow: Spring,
    spin: f32,
    spin_rate: f32,
    /// user orbit offset (yaw, pitch) with inertia
    orbit: Vec2,
    orbit_v: Vec2,
    since_drag: f32,
    shake: f32,
}

impl Actor {
    fn new(kind: Kind, x: f32) -> Actor {
        Actor {
            p: Player::new(kind),
            pos: Spring3::new(vec3(x, 0.2, -30.0), 30.0, 11.0),
            yaw: Spring::new(0.0, 26.0, 10.0),
            pitch: Spring::new(0.12, 26.0, 10.0),
            scale: Spring::new(1.0, 60.0, 14.0),
            reveal: Spring::new(0.0, 9.0, 6.0),
            glow: Spring::new(0.9, 40.0, 12.0),
            spin: if x < 0.0 { 0.3 } else { -0.4 },
            spin_rate: 0.35,
            orbit: Vec2::ZERO,
            orbit_v: Vec2::ZERO,
            since_drag: 10.0,
            shake: 0.0,
        }
    }

    fn rot(&self, time: f32) -> Quat {
        let sh = self.shake * (time * 61.0).sin() * 0.08;
        Quat::from_rotation_x(self.pitch.x + self.orbit.y) * Quat::from_rotation_y(self.yaw.x + self.spin + self.orbit.x + sh)
    }

    fn matrix(&self, time: f32) -> Mat4 {
        let jitter = vec3((time * 47.0).sin(), (time * 53.0).cos(), 0.0) * self.shake * 0.12;
        Player::matrix(self.pos.get() + jitter, self.rot(time), self.scale.x)
    }

    /// Fold the free spin into the yaw spring so it settles on `target`
    /// by the shortest way round.
    fn settle_yaw(&mut self, target: f32) {
        let cur = self.yaw.x + self.spin;
        self.spin = 0.0;
        let mut d = (cur - target) % TAU;
        if d > PI {
            d -= TAU;
        }
        if d < -PI {
            d += TAU;
        }
        self.yaw.x = target + d;
        self.yaw.target = target;
    }
}

#[derive(Clone)]
struct ProbeLine {
    key: String,
    val: String,
    ok: Option<bool>,
    t: f32,
}

#[derive(Default)]
struct LinkState {
    probing: bool,
    probed_at: Option<f32>,
    kicked: bool,
    lines: Vec<ProbeLine>,
    error: Option<String>,
    seen_at: Option<f32>,
    /// it was plugged in running firmware of its own during this link
    saw_os: bool,
}

enum PkgState {
    Checking(f32, String),
    Ok,
    Bad(String),
}

struct PkgView {
    /// the player it was loaded for: a package never carries over to the other
    kind: Kind,
    path: PathBuf,
    name: String,
    state: PkgState,
    target: String,
    version: String,
    flash: u64,
    card: u64,
    files: usize,
    segs: Vec<(u32, u32)>,
    t: f32,
}

enum PkgMsg {
    Progress(f32, String),
    Done(Result<Package, String>),
}

#[derive(Default)]
struct Job {
    restoring: bool,
    phase: Option<Phase>,
    phase_t: f32,
    progress: f32,
    shown: f32,
    sectors: Vec<Sect>,
    heat: Vec<f32>,
    sector_size: u32,
    hex: Vec<(u32, Vec<u8>)>,
    /// card files as they go by: path, size, when, and whether this is the read-back pass
    files: Vec<(String, u64, f32, bool)>,
    sync: SyncView,
    need_card: bool,
    outcome: Option<Outcome>,
    error: Option<String>,
    started: f32,
    ended: Option<f32>,
    written: usize,
    /// the new firmware started booting at this time: the Rockbox sign on
    /// the player's screen lights up
    booted: Option<f32>,
}

/// The SD sync as the card panel shows it.
#[derive(Default)]
struct SyncView {
    /// when the first file went over
    started: Option<f32>,
    written: usize,
    readback: bool,
    /// when the last file event came: the card's contacts flash on it
    last: f32,
    /// how full the card looks (0..1), eased toward the bytes written
    level: f32,
    /// bytes/s, sampled every half second
    rate: Vec<f32>,
    rate_at: f32,
    rate_done: f32,
    /// top-level folders: name, files, when one last landed
    dirs: Vec<(String, usize, f32)>,
}

const FLASH_PHASES: [Phase; 10] = [
    Phase::Connect,
    Phase::Probe,
    Phase::Backup,
    Phase::Verify,
    Phase::Write,
    Phase::Check,
    Phase::Reboot,
    Phase::Sync,
    Phase::Eject,
    Phase::Done,
];
const RESTORE_PHASES: [Phase; 8] =
    [Phase::Connect, Phase::Probe, Phase::Verify, Phase::Backup, Phase::Write, Phase::Check, Phase::Reboot, Phase::Done];

/// A phase's share of the progress bar: the firmware is the first half,
/// the card the second.
fn weight(p: Phase) -> f32 {
    match p {
        Phase::Connect => 0.02,
        Phase::Probe => 0.03,
        Phase::Backup => 0.15,
        Phase::Verify => 0.03,
        Phase::Write => 0.22,
        Phase::Check => 0.05,
        Phase::Reboot => 0.04,
        Phase::Sync => 0.44,
        Phase::Eject => 0.02,
        Phase::Done => 0.0,
    }
}

impl Job {
    fn phases(&self) -> &'static [Phase] {
        if self.restoring {
            &RESTORE_PHASES
        } else {
            &FLASH_PHASES
        }
    }
    fn overall(&self) -> f32 {
        let ph = self.phases();
        let Some(cur) = self.phase else { return 0.0 };
        if cur == Phase::Done {
            return 1.0;
        }
        let total: f32 = ph.iter().map(|p| weight(*p)).sum();
        let mut done = 0.0;
        for p in ph {
            if *p == cur {
                done += weight(*p) * clamp01(self.progress);
                break;
            }
            done += weight(*p);
        }
        done / total
    }
}

pub struct App {
    pub g: Gfx,
    world: World,
    batch: LineBatch,
    actors: [Actor; 2],
    cable: Cable,
    lcd: RenderTarget,
    /// the light behind the player's screen: what shines through it
    lcd_light: RenderTarget,
    ui: Ui,
    hub: Hub,
    opts: Opts,
    pub time: f32,
    screen: Screen,
    screen_t: f32,
    sel: usize,
    hover: Option<usize>,
    /// the select-screen hint box under the pointer: (player, feature)
    hint: Option<(usize, Feature)>,
    post: Post,
    kick: f32,
    flash: f32,
    flash_col: Color,
    tint: Color,
    drag: Option<(Vec2, Vec2, Option<usize>, bool)>,
    link: LinkState,
    pkg: Option<PkgView>,
    pkg_rx: Option<Receiver<PkgMsg>>,
    pick_rx: Option<Receiver<Option<PathBuf>>>,
    stock: Option<Backup>,
    job: Job,
    log: Vec<(f32, Lvl, String)>,
    plug: Spring,
    sd: Spring,
    usb_hot: f32,
    last_cycle: i32,
    audio: Audio,
    /// kick-drum pulse from the music, held and decayed
    beat: f32,
    /// the language picker, while it's open
    picker: Option<Picker>,
    /// the picker has been opened at some point (until then its button
    /// pulses now and then, so it gets noticed)
    lang_seen: bool,
    roll: Roll,
    np: NowPlaying,
    /// the credits haven't rolled for this job yet (they start by themselves once)
    credits_due: bool,
    /// the bottom row's free span, between the prompt and the sound badge
    bottom_free: (f32, f32),
    /// when a report was last copied to the clipboard
    copied: f32,
    /// RETURN TO STOCK was picked before the player was linked: restore as
    /// soon as it is (instead of showing the package again)
    to_stock: bool,
    clippy: Clippy,
    /// someone has spun a player (until then a ghost pointer shows how)
    spun: bool,
    /// when Clippy's encore started (the credits rolled all the way
    /// through), and whether it's the whole show; once it has played he
    /// stays on the credits
    encore: Option<(f32, bool)>,
    encored: bool,
}

/// Seconds on the complete screen before the credits roll by themselves.
const CREDITS_AFTER: f32 = 30.0;
/// The select screen's ghost drag: how long it takes, and how far round
/// it turns the player (past half way, so it carries on round when it settles).
const GHOST_DRAG: f32 = 1.3;
const GHOST_TURN: f32 = 3.4;

/// A download-mode step: what to do, the part of the player it's about
/// (traced from the row), and a part held meanwhile (lit too).
struct Step {
    title: &'static str,
    sub: String,
    feat: Option<Feature>,
    also: Option<Feature>,
}

/// Firmware of its own, to be left before download mode: Rockbox, or the
/// YP3's stock firmware.
fn os_mode(k: Kind, m: Mode) -> bool {
    m == Mode::Rockbox || (k == Kind::Yp3 && m == Mode::Stock)
}

/// 0..1 noise from an integer.
fn noise(i: u32) -> f32 {
    ((i as f32 * 12.9898).sin() * 43758.547).fract().abs()
}

/// A neon tube `st` seconds after it was switched on (at `time`): dark,
/// then sputtering, then steady with the odd stutter. 0..1.
fn neon(st: f32, time: f32) -> f32 {
    if st < 0.7 {
        return 0.0;
    }
    let warm = clamp01((st - 0.7) / 1.8);
    let tick = (time * 24.0) as u32;
    let h = noise(tick);
    if warm < 1.0 {
        if h < 0.12 + 0.88 * warm * warm {
            0.45 + 0.55 * warm
        } else {
            0.03
        }
    } else if (tick / 3) % 61 == 0 && h < 0.6 {
        0.25
    } else {
        1.0
    }
}

fn idx(k: Kind) -> usize {
    match k {
        Kind::Yp3 => 0,
        Kind::Ts17 => 1,
    }
}

fn kind_of(i: usize) -> Kind {
    if i == 0 {
        Kind::Yp3
    } else {
        Kind::Ts17
    }
}

fn mib(b: u64) -> String {
    if b >= 1 << 20 {
        format!("{:.1} MiB", b as f64 / (1u64 << 20) as f64)
    } else {
        format!("{:.0} KiB", b as f64 / 1024.0)
    }
}

impl App {
    pub fn new(hub: Hub, opts: Opts, audio: Audio) -> App {
        let lcd = render_target(128, 160);
        lcd.texture.set_filter(FilterMode::Nearest);
        let lcd_light = render_target(128, 160);
        lcd_light.texture.set_filter(FilterMode::Linear);
        let mut world = World::new();
        world.intensity = 0.0;
        App {
            g: Gfx::new(),
            world,
            batch: LineBatch::new(),
            actors: [Actor::new(Kind::Yp3, -4.4), Actor::new(Kind::Ts17, 4.4)],
            cable: Cable::new(),
            lcd,
            lcd_light,
            ui: Ui::new(),
            hub,
            opts,
            time: 0.0,
            screen: Screen::Boot,
            screen_t: 0.0,
            sel: 0,
            hover: None,
            hint: None,
            post: Post::default(),
            kick: 0.0,
            flash: 0.0,
            flash_col: WHITE,
            tint: pal::MAGENTA,
            drag: None,
            link: LinkState::default(),
            pkg: None,
            pkg_rx: None,
            pick_rx: None,
            stock: None,
            job: Job::default(),
            log: Vec::new(),
            plug: Spring::new(0.0, 40.0, 11.0),
            sd: Spring::new(0.0, 60.0, 9.0),
            usb_hot: 0.0,
            last_cycle: -1,
            audio,
            beat: 0.0,
            picker: None,
            lang_seen: i18n::saved().is_some(),
            roll: Roll::new(),
            np: NowPlaying::new(),
            credits_due: false,
            bottom_free: (0.0, 0.0),
            copied: -10.0,
            to_stock: false,
            clippy: Clippy::new(),
            spun: false,
            encore: None,
            encored: false,
        }
    }

    /// Ease the card's fill and sample the transfer rate during the SD sync.
    fn sync_tick(&mut self, dt: f32) {
        let total = self.pkg.as_ref().map(|p| p.card).unwrap_or(0) as f32;
        let (phase, p, now) = (self.job.phase, self.job.progress, self.time);
        let sv = &mut self.job.sync;
        if sv.started.is_none() {
            return;
        }
        let writing = phase == Some(Phase::Sync) && !sv.readback;
        sv.level = approach(sv.level, if writing { (p / 0.7).min(1.0) } else { 1.0 }, 5.0, dt);
        if phase != Some(Phase::Sync) || now - sv.rate_at < 0.5 {
            return;
        }
        // bytes through this pass: written, or read back and hashed
        let done = total * if sv.readback { clamp01((p - 0.7) / 0.3) } else { (p / 0.7).min(1.0) };
        if done < sv.rate_done {
            sv.rate_done = 0.0;
        }
        if sv.rate_at > 0.0 {
            sv.rate.push((done - sv.rate_done) / (now - sv.rate_at));
            if sv.rate.len() > 48 {
                sv.rate.remove(0);
            }
        }
        sv.rate_at = now;
        sv.rate_done = done;
    }

    /// Where the download-mode walkthrough is in its loop, in seconds.
    /// YP3: power off (0..1.6), hold Down (1.6..), plug in (3.0..7.4).
    /// TS17: plug in (1.4..6.2).
    fn link_loop(&self) -> f32 {
        let period = if self.kind() == Kind::Yp3 { 8.4 } else { 7.0 };
        (self.screen_t - 0.5).max(0.0) % period
    }

    fn kind(&self) -> Kind {
        kind_of(self.sel)
    }

    fn go(&mut self, s: Screen) {
        if s == self.screen {
            return;
        }
        let from = self.screen;
        self.screen = s;
        self.screen_t = 0.0;
        self.encore = None;
        self.kick = self.kick.max(0.55);
        if matches!(s, Screen::Select | Screen::Link | Screen::Payload) {
            self.audio.music(false);
        }
        self.audio.sfx(if s == Screen::Fail { Sfx::Error } else { Sfx::Transition });
        match s {
            Screen::Select => {
                self.to_stock = false;
                self.hub.send(Cmd::Watch(None));
                for a in &mut self.actors {
                    a.spin_rate = 0.35;
                }
            }
            Screen::Link => {
                self.link = LinkState::default();
                self.hub.send(Cmd::Watch(Some(self.kind())));
                self.last_cycle = -1;
            }
            Screen::Payload => {
                self.stock = backup::stock_for(self.kind(), None);
                if self.pkg.as_ref().is_some_and(|p| p.kind != self.kind()) {
                    self.pkg = None;
                    self.pkg_rx = None;
                }
            }
            Screen::Execute => {
                self.flash = 0.5;
                self.flash_col = pal::MAGENTA;
                self.audio.music(true);
            }
            Screen::Complete if from == Screen::Credits => {}
            Screen::Complete => {
                // a first flash has only just made the stock backup
                self.stock = backup::stock_for(self.kind(), None);
                self.audio.voice();
                self.flash = 0.9;
                self.flash_col = pal::ACID;
                self.world.speed = 22.0;
                self.credits_due = true;
            }
            Screen::Credits => {
                // he's been: he's back
                if self.encored {
                    self.encore = Some((self.time, false));
                }
                self.credits_due = false;
                self.flash = 0.6;
                self.flash_col = pal::MAGENTA;
                self.world.speed = 30.0;
                let (l, _, _) = credits::layout(self.g.size());
                self.roll.reset(l.h - 72.0);
            }
            Screen::Fail => {
                self.audio.tape_stop();
                self.flash = 0.8;
                self.flash_col = pal::RED;
                self.kick = 1.2;
                self.actors[self.sel].shake = 1.0;
            }
            Screen::Boot => {}
        }
    }

    fn say(&mut self, l: Lvl, s: impl Into<String>) {
        self.log.push((self.time, l, s.into()));
        if self.log.len() > 400 {
            self.log.drain(..100);
        }
    }

    // ------------------------------------------------------------ update

    pub fn update(&mut self, dt: f32) {
        self.time += dt;
        self.screen_t += dt;
        let m: Vec2 = self.opts.mouse.unwrap_or_else(|| mouse_position().into());
        self.ui.frame(
            dt,
            self.time,
            m,
            is_mouse_button_pressed(MouseButton::Left),
            is_mouse_button_released(MouseButton::Left),
            is_mouse_button_down(MouseButton::Left),
        );
        self.events();
        self.package_tick();
        for f in get_dropped_files() {
            if let Some(p) = f.path {
                if matches!(self.screen, Screen::Payload) {
                    self.load_package(p);
                }
            }
        }
        if let Some(rx) = &self.pick_rx {
            if let Ok(r) = rx.try_recv() {
                self.pick_rx = None;
                if let Some(p) = r {
                    self.load_package(p);
                }
            }
        }
        self.logic();
        self.pose(dt);
        let say = self.tip();
        if self.clippy.update(self.time, dt, say) {
            self.audio.sfx(Sfx::Drop);
        }
        // decaying effects
        self.kick = approach(self.kick, 0.0, 3.0, dt);
        self.flash = approach(self.flash, 0.0, 3.5, dt);
        self.usb_hot = approach(self.usb_hot, 0.0, 2.5, dt);
        let p = self.audio.pulse();
        self.beat = if p > self.beat { p } else { approach(self.beat, p, 7.0, dt) };
        for h in &mut self.job.heat {
            *h = approach(*h, 0.0, 4.0, dt);
        }
        self.job.shown = approach(self.job.shown, self.job.overall(), 6.0, dt);
        self.sync_tick(dt);
        let wt = match self.screen {
            Screen::Boot => pal::MAGENTA,
            Screen::Select | Screen::Link | Screen::Payload => pal::MAGENTA,
            Screen::Execute => {
                if self.job.need_card {
                    pal::AMBER
                } else {
                    pal::VIOLET
                }
            }
            Screen::Complete => pal::mix(pal::ACID, pal::TEAL, 0.5),
            // the party: the colour wheels round
            Screen::Credits => {
                let h = self.time * 0.15;
                pal::mix(pal::mix(pal::MAGENTA, pal::CYAN, 0.5 + 0.5 * h.sin()), pal::VIOLET, 0.5 + 0.5 * (h * 1.3).cos())
            }
            Screen::Fail => pal::RED,
        };
        self.tint = pal::mix(self.tint, wt, 1.0 - (-2.5 * dt).exp());
        self.world.tint = self.tint;
        let base_speed = match self.screen {
            Screen::Execute => 7.0,
            Screen::Complete => 5.0,
            Screen::Credits => 9.0 + self.beat * 6.0,
            Screen::Fail => 0.8,
            _ => 3.0,
        };
        self.world.speed = approach(self.world.speed, base_speed, 1.2, dt);
        let wi = if self.screen == Screen::Boot { clamp01((self.screen_t - 1.3) / 1.2) } else { 1.0 };
        self.world.intensity = approach(self.world.intensity, wi, 6.0, dt);
        self.world.update(dt);

        let cursor = if self.ui.cursor_hand || (self.screen == Screen::Select && self.hover.is_some()) {
            miniquad::CursorIcon::Pointer
        } else if self.drag.map(|d| d.3).unwrap_or(false) {
            miniquad::CursorIcon::Move
        } else {
            miniquad::CursorIcon::Default
        };
        miniquad::window::set_mouse_cursor(cursor);
    }

    fn events(&mut self) {
        for e in self.hub.poll() {
            match e {
                Event::Presence(_) => {}
                Event::Log(l, s) => self.say(l, i18n::msg(&s)),
                Event::Probe(k, v, ok) => {
                    let line = ProbeLine { key: k, val: v, ok, t: self.time };
                    if let Some(x) = self.link.lines.iter_mut().find(|x| x.key == line.key) {
                        x.val = line.val;
                        x.ok = line.ok;
                    } else if self.screen == Screen::Link || self.link.probing {
                        self.link.lines.push(line);
                    } else if line.key != "CELL" {
                        self.say(if ok == Some(false) { Lvl::Err } else { Lvl::Info }, format!("{} {}", line.key, line.val));
                    }
                }
                Event::Phase(p) => {
                    self.job.phase = Some(p);
                    self.job.phase_t = self.time;
                    if p == Phase::Reboot && !self.job.restoring && self.job.booted.is_none() {
                        self.job.booted = Some(self.time);
                    }
                    self.job.progress = 0.0;
                    if self.screen == Screen::Execute {
                        self.kick = self.kick.max(0.35);
                        if p == Phase::Reboot {
                            self.flash = 0.6;
                            self.flash_col = pal::ICE;
                        }
                    }
                }
                Event::Progress(f) => self.job.progress = f,
                Event::Layout { sectors, sector_size } => {
                    self.job.sectors = vec![Sect::Idle; sectors as usize];
                    self.job.heat = vec![0.0; sectors as usize];
                    self.job.sector_size = sector_size;
                }
                Event::Sector(i, s) => {
                    if let Some(x) = self.job.sectors.get_mut(i as usize) {
                        if s == Sect::Good && *x != Sect::Good {
                            self.job.written += 1;
                        }
                        *x = s;
                        self.job.heat[i as usize] = 1.0;
                    }
                }
                Event::Hex(a, d) => {
                    self.job.hex.push((a, d));
                    if self.job.hex.len() > 120 {
                        self.job.hex.drain(..40);
                    }
                }
                Event::File(n, s) => {
                    let sv = &mut self.job.sync;
                    sv.started.get_or_insert(self.time);
                    sv.written += 1;
                    sv.last = self.time;
                    let dir = card_dir(&n);
                    match sv.dirs.iter_mut().find(|d| d.0 == dir) {
                        Some(d) => {
                            d.1 += 1;
                            d.2 = self.time;
                        }
                        None => sv.dirs.push((dir, 1, self.time)),
                    }
                    self.job.files.push((n, s, self.time, false));
                    if self.job.files.len() > 120 {
                        self.job.files.drain(..40);
                    }
                }
                Event::ReadBack(n, s) => {
                    self.job.sync.readback = true;
                    self.job.sync.last = self.time;
                    self.job.files.push((n, s, self.time, true));
                    if self.job.files.len() > 120 {
                        self.job.files.drain(..40);
                    }
                }
                Event::NeedCard(b) => {
                    if b && !self.job.need_card {
                        self.kick = 0.7;
                        self.flash = 0.4;
                        self.flash_col = pal::AMBER;
                    }
                    self.job.need_card = b;
                }
                Event::Ready => {
                    if self.link.probing {
                        self.link.probing = false;
                        self.link.probed_at = Some(self.time);
                        self.audio.sfx(Sfx::Link);
                        self.flash = 0.45;
                        self.flash_col = pal::CYAN;
                        self.kick = 0.6;
                    }
                }
                Event::Done(o) => {
                    self.job.outcome = Some(o);
                    self.job.ended = Some(self.time);
                    self.job.need_card = false;
                }
                Event::Failed(e) => {
                    let e = i18n::msg(&e);
                    self.say(Lvl::Err, e.clone());
                    if self.screen == Screen::Execute {
                        self.job.error = Some(e);
                        self.job.ended = Some(self.time);
                        self.job.need_card = false;
                    } else {
                        self.link.probing = false;
                        self.link.error = Some(e);
                        self.kick = 0.8;
                    }
                }
            }
        }
    }

    /// A bug report for the clipboard: this build, the player, the package,
    /// the error, and the end of the log (or this run's messages without one).
    fn report(&self, error: &str) -> String {
        let mut s = format!("rockslop {} ({} {})\n", rockslop::VERSION, std::env::consts::OS, std::env::consts::ARCH);
        let mode = self.seen().map(|x| format!("{:?}", x.mode)).unwrap_or_else(|| "not on USB".into());
        s += &format!("player: {} ({mode})\n", self.kind().name());
        if let Some(p) = &self.pkg {
            s += &format!("package: {} ({} {})\n", p.name, p.target, p.version);
        }
        s += &format!("error: {error}\n");
        let tail = rockslop::log::tail(150);
        if tail.is_empty() {
            s += "\nmessages:\n";
            for (t, l, m) in &self.log {
                s += &format!("{t:8.1} {l:?} {m}\n");
            }
        } else {
            s += &format!("\nlog ({}):\n", rockslop::log::path().display());
            for l in tail {
                s += &l;
                s.push('\n');
            }
        }
        s
    }

    /// COPY REPORT, which says REPORT COPIED for a moment once pressed.
    fn copy_button(&mut self, r: Rect, error: &str, a: f32) {
        let label = if self.time - self.copied < 2.0 { tr("REPORT COPIED") } else { tr("COPY REPORT") };
        if ui::button(&mut self.ui, &self.g, r, label, Tone::Secondary, a, true) {
            miniquad::window::clipboard_set(&self.report(error));
            self.copied = self.time;
        }
    }

    fn seen(&self) -> Option<rockslop::device::Seen> {
        self.hub.find(self.kind())
    }

    fn toggle_mute(&mut self) {
        let m = self.audio.toggle_mute();
        self.say(Lvl::Info, if m { tr("audio muted") } else { tr("audio on") });
    }

    fn logic(&mut self) {
        let any_key = is_key_pressed(KeyCode::Space) || is_key_pressed(KeyCode::Enter) || is_key_pressed(KeyCode::Escape);
        if is_key_pressed(KeyCode::M) && self.audio.live() {
            self.toggle_mute();
        }
        self.np.update(self.audio.now_playing(), self.time);
        if let Some(p) = &mut self.picker {
            p.keys(self.time);
            if p.gone(self.time) {
                self.picker = None;
            }
            // the picker has the keys and the pointer
            return;
        }
        if is_key_pressed(KeyCode::L) && self.lang_ok() {
            self.open_picker();
            return;
        }
        match self.screen {
            Screen::Boot => {
                // the title decodes in
                if self.screen_t >= 0.35 && self.screen_t - self.ui.dt < 0.35 {
                    self.audio.sfx(Sfx::Boot);
                }
                if self.screen_t > 2.6 || (self.screen_t > 0.3 && (self.ui.released || any_key)) {
                    self.go(Screen::Select);
                }
            }
            Screen::Select => {
                if self.opts.auto && self.screen_t > 1.6 {
                    self.sel = idx(self.opts.kind);
                    self.go(Screen::Link);
                }
                if is_key_pressed(KeyCode::Left) || is_key_pressed(KeyCode::Key1) {
                    self.sel = 0;
                    self.go(Screen::Link);
                } else if is_key_pressed(KeyCode::Right) || is_key_pressed(KeyCode::Key2) {
                    self.sel = 1;
                    self.go(Screen::Link);
                }
                // the ghost drag turns its player as a real one would
                if let Some((i, f, _)) = self.ghost() {
                    if f > 0.0 && f < 1.0 {
                        let a = &mut self.actors[i];
                        let v = GHOST_TURN * PI / (2.0 * GHOST_DRAG) * (PI * f).sin();
                        a.orbit.x += v * self.ui.dt;
                        a.orbit_v = vec2(v, 0.0);
                        a.since_drag = 0.0;
                    }
                }
            }
            Screen::Link => {
                if is_key_pressed(KeyCode::Escape) {
                    self.go(Screen::Select);
                    return;
                }
                let seen = self.seen();
                if seen.is_some_and(|s| os_mode(self.kind(), s.mode)) {
                    self.link.saw_os = true;
                }
                if seen.is_some() && self.link.seen_at.is_none() {
                    self.link.seen_at = Some(self.time);
                    self.usb_hot = 1.0;
                    self.kick = 0.5;
                } else if seen.is_none() {
                    self.link.seen_at = None;
                }
                if let Some(s) = seen {
                    let busy = self.hub.busy;
                    if s.mode == Mode::Rom
                        && s.access
                        && !busy
                        && !self.link.probing
                        && self.link.probed_at.is_none()
                        && self.link.error.is_none()
                    {
                        self.link.probing = true;
                        self.link.lines.clear();
                        self.hub.send(Cmd::Probe(self.kind()));
                    }
                    if s.mode == Mode::Stock && self.kind() == Kind::Ts17 && !self.link.kicked && !busy {
                        self.link.kicked = true;
                        self.hub.send(Cmd::Kick(Kind::Ts17));
                    }
                } else if !self.link.probing {
                    self.link.probed_at = None;
                }
                if let Some(t) = self.link.probed_at {
                    if self.time - t > 1.6 && self.link.error.is_none() {
                        if self.to_stock {
                            self.to_stock = false;
                            self.stock = backup::stock_for(self.kind(), None);
                            if self.stock.is_some() {
                                self.start_restore();
                                return;
                            }
                        }
                        self.go(Screen::Payload);
                    }
                }
            }
            Screen::Payload => {
                if is_key_pressed(KeyCode::Escape) {
                    self.go(Screen::Select);
                    return;
                }
                if self.opts.auto {
                    if self.pkg.is_none() && self.screen_t > 1.2 {
                        let p = self.opts.package.clone().unwrap_or_else(|| self.demo_package());
                        self.load_package(p);
                    }
                    if matches!(self.pkg.as_ref().map(|p| &p.state), Some(PkgState::Ok)) && self.screen_t > 4.2 {
                        self.start_flash();
                    }
                }
                // lost the link: back to the instructions
                if self.seen().map(|s| s.mode) != Some(Mode::Rom) && self.screen_t > 0.5 {
                    self.link.seen_at = None;
                    self.go(Screen::Link);
                }
            }
            Screen::Execute => {
                if self.job.outcome.is_some() && self.job.ended.map(|t| self.time - t > 1.1).unwrap_or(false) {
                    self.go(Screen::Complete);
                }
                if self.job.error.is_some() && self.job.ended.map(|t| self.time - t > 0.5).unwrap_or(false) {
                    self.go(Screen::Fail);
                }
            }
            Screen::Complete => {
                if is_key_pressed(KeyCode::Escape) {
                    self.go(Screen::Select);
                } else if self.credits_due && self.screen_t > CREDITS_AFTER && !self.ui.captured {
                    self.go(Screen::Credits);
                }
            }
            Screen::Credits => {
                if is_key_pressed(KeyCode::Escape) {
                    self.go(Screen::Complete);
                }
            }
            Screen::Fail => {
                if is_key_pressed(KeyCode::Escape) {
                    self.go(Screen::Select);
                }
            }
        }
        self.pointer();
    }

    /// Orbit drags and model picking.
    fn pointer(&mut self) {
        let m = self.ui.mouse;
        let was = self.hover.take();
        if self.screen == Screen::Select && self.screen_t > 0.4 {
            for i in 0..2 {
                if self.bbox(i).map(|r| r.contains(m)).unwrap_or(false) {
                    self.hover = Some(i);
                }
            }
        }
        if self.hover.is_some() && self.hover != was {
            self.audio.sfx(Sfx::Hover);
        }
        if self.ui.pressed && !self.ui.captured {
            let target = if self.screen == Screen::Select { self.hover } else { Some(self.sel) };
            self.drag = Some((m, m, target, false));
        }
        if let Some((start, last, target, moved)) = self.drag {
            let d = m - last;
            let moved = moved || m.distance(start) > 5.0;
            if let Some(i) = target {
                if moved && self.screen != Screen::Boot {
                    self.spun = true;
                    let a = &mut self.actors[i];
                    a.orbit.x += d.x * 0.011;
                    a.orbit.y = (a.orbit.y + d.y * 0.008).clamp(-0.9, 0.9);
                    a.orbit_v = d / self.ui.dt.max(1e-3) * vec2(0.011, 0.008);
                    a.since_drag = 0.0;
                }
            }
            self.drag = Some((start, m, target, moved));
            if self.ui.released {
                self.drag = None;
                if !moved && self.screen == Screen::Select {
                    if let Some(i) = self.hover {
                        self.sel = i;
                        self.go(Screen::Link);
                    }
                }
            }
        }
    }

    /// The select screen's ghost drag, until someone spins a player
    /// themselves: which player, how far through the drag (0..1), and how
    /// much of the pointer shows.
    fn ghost(&self) -> Option<(usize, f32, f32)> {
        if self.spun || self.screen != Screen::Select || self.hover.is_some() || self.picker.is_some() {
            return None;
        }
        let t = self.screen_t - 3.0;
        if t < 0.0 {
            return None;
        }
        // one player then the other, every 6 s: the pointer fades in,
        // drags, and fades out
        let (k, c) = ((t / 6.0) as usize, t % 6.0);
        let f = clamp01((c - 0.4) / GHOST_DRAG);
        let vis = clamp01(c * 3.0).min(clamp01((GHOST_DRAG + 1.0 - c) * 3.0));
        (vis > 0.0).then_some((k % 2, f, vis))
    }

    /// The player runs firmware of its own that has to be left first (or
    /// did, during this link): Rockbox, or the YP3's stock firmware. (The
    /// TS17's stock firmware is kicked into download mode for you.)
    fn from_os(&self) -> bool {
        self.to_stock || self.link.saw_os || self.seen().is_some_and(|s| os_mode(self.kind(), s.mode))
    }

    /// The download-mode steps for this player and where it is: the steps,
    /// the one on now, and how many are done for sure (the walkthrough
    /// loops through the rest).
    fn link_plan(&self) -> (Vec<Step>, usize, usize) {
        let st = |title, sub: &str, feat, also| Step { title, sub: sub.to_string(), feat, also };
        let os = self.from_os();
        let mut steps = Vec::new();
        if os {
            steps.push(st(tr("UNPLUG"), tr("pull the USB cable out"), Some(Feature::Usb), None));
        }
        match self.kind() {
            Kind::Yp3 => {
                steps.push(st(tr("POWER OFF"), tr("unplugged: hold the centre key"), Some(Feature::Select), None));
                steps.push(st(tr("HOLD DOWN"), tr("bottom of the wheel"), Some(Feature::Down), None));
                steps.push(st(tr("CONNECT USB"), tr("keep holding"), Some(Feature::Usb), Some(Feature::Down)));
                steps.push(st(tr("RELEASE"), tr("once linked"), None, None));
            }
            Kind::Ts17 if os => {
                steps.push(st(tr("SWITCH OFF"), tr("hold POWER until the screen goes dark"), Some(Feature::Power), None));
                steps.push(st(tr("HOLD POWER"), tr("plug in while holding, ~4 s"), Some(Feature::Power), Some(Feature::Usb)));
                steps.push(st(tr("RELEASE"), tr("once linked"), None, None));
            }
            Kind::Ts17 => {
                steps.push(st(tr("POWER ON"), tr("hold POWER, on the bottom edge"), Some(Feature::Power), None));
                steps.push(st(tr("CONNECT USB"), tr("stock firmware"), Some(Feature::Usb), None));
                steps.push(st(tr("STAND BY"), tr("it reboots into download mode"), None, None));
            }
        }
        let n = steps.len();
        let first = os as usize;
        // the walkthrough's loop: YP3 power off, hold Down (1.6 s), plug
        // in (3.0 s); TS17 power, plug in (1.4 s)
        let cyc = self.link_loop();
        let walk = match self.kind() {
            Kind::Yp3 => (cyc >= 1.6) as usize + (cyc >= 3.0) as usize,
            Kind::Ts17 => (cyc >= 1.4) as usize,
        };
        let (active, done) = match self.seen().map(|s| s.mode) {
            Some(Mode::Rom) => (n - 1, n - 1),
            Some(m) if os_mode(self.kind(), m) => (0, 0),
            // the TS17 on stock firmware, being kicked
            Some(_) => (n - 1, n - 1),
            None => (first + walk, first),
        };
        (steps, active, done)
    }

    /// What Clippy would say here, and after how long.
    fn tip(&self) -> Option<Say> {
        if self.picker.is_some() {
            return None;
        }
        let say = |tip, after: f32, body: String| Some(Say { tip, after: if self.opts.tips_now { 0.5 } else { after }, body });
        let seen = self.seen();
        let cable = tr("Still nothing? Try another cable: some only charge.");
        match self.screen {
            Screen::Select => say(
                Tip::Model,
                8.0,
                tr("There's no model number on these players. Compare the wireframes instead: the buttons and ports differ. Drag a player to spin it.").into(),
            ),
            Screen::Link if self.link.error.is_some() => None,
            Screen::Link => match seen {
                None => {
                    let how = match (self.kind(), self.from_os()) {
                        (Kind::Yp3, _) => tr("Power the player off first, then hold DOWN before the cable goes in, and keep holding."),
                        (Kind::Ts17, true) => tr("Hold POWER while you plug the cable in, and keep holding for about 4 seconds."),
                        (Kind::Ts17, false) => tr("Switch the player on first: hold POWER on its bottom edge."),
                    };
                    say(Tip::Link, 20.0, format!("{how}\n{cable}"))
                }
                Some(s) if !s.access => say(Tip::Access, 4.0, format!("{}\n{}", tr("Your computer won't let rockslop use the player."), permission_hint())),
                Some(s) if os_mode(self.kind(), s.mode) => {
                    say(Tip::Firmware, 10.0, tr("Unplug the cable first: the player only listens in download mode. The steps say how.").into())
                }
                Some(_) => None,
            },
            Screen::Payload if self.pkg.is_none() => say(
                Tip::Package,
                15.0,
                tf("You need the Rockbox package built for the {0}: a .tar.xz file. Click SELECT IMAGE, or drop it on the panel.", &[&self.kind().name()]),
            ),
            Screen::Execute if self.job.need_card => {
                say(Tip::Card, 6.0, tr("Push a microSD card all the way into the player's slot, then press RESET.").into())
            }
            Screen::Execute if self.job.phase == Some(Phase::Sync) && self.job.sync.started.is_none() => {
                say(Tip::Disk, 35.0, tr("Rockbox hasn't shown its USB disk yet. If the player only charges, unplug it, hold any key and plug it back in.").into())
            }
            Screen::Fail => say(Tip::Fail, 4.0, tr("RETRY usually sorts it out. Still stuck? COPY REPORT puts the details on the clipboard for a bug report.").into()),
            _ => None,
        }
    }

    /// Screen-space box of a player.
    fn bbox(&self, i: usize) -> Option<Rect> {
        let a = &self.actors[i];
        let mtx = a.matrix(self.time);
        let d = a.p.dims / 2.0;
        let mut lo = vec2(f32::MAX, f32::MAX);
        let mut hi = vec2(f32::MIN, f32::MIN);
        for k in 0..8 {
            let c = vec3(if k & 1 == 0 { -d.x } else { d.x }, if k & 2 == 0 { -d.y } else { d.y }, if k & 4 == 0 { -d.z } else { d.z });
            let p = self.g.project(mtx.transform_point3(c))?;
            lo = lo.min(p);
            hi = hi.max(p);
        }
        Some(Rect::new(lo.x, lo.y, hi.x - lo.x, hi.y - lo.y))
    }

    fn proj(&self, i: usize, local: Vec3) -> Option<Vec2> {
        self.g.project(self.actors[i].matrix(self.time).transform_point3(local))
    }

    fn anchor_px(&self, i: usize, f: Feature) -> Option<Vec2> {
        let a = self.actors[i].p.anchor(f)?;
        self.proj(i, a.pos)
    }

    /// Project a point in the player's frame without its spin or orbit:
    /// where labels that follow the player but don't turn with it go.
    fn rest_px(&self, i: usize, local: Vec3) -> Option<Vec2> {
        let a = &self.actors[i];
        // the resting tilt, not the live one: a box that moved when its
        // feature tipped toward the camera would slide out from under the pointer
        let m = Player::matrix(a.pos.get(), Quat::from_rotation_x(0.12), a.scale.x);
        self.g.project(m.transform_point3(local))
    }

    /// Whether a feature faces the camera. Ports on the bottom edge count
    /// as facing: they sit on the silhouette.
    fn facing(&self, i: usize, f: Feature) -> bool {
        let a = &self.actors[i];
        let Some(an) = a.p.anchor(f) else { return false };
        if an.normal.y < -0.5 {
            return true;
        }
        let m = a.matrix(self.time);
        let p = m.transform_point3(an.pos);
        m.transform_vector3(an.normal).dot(self.g.cam.position - p) > 0.0
    }

    /// Where the players should be for this screen.
    fn pose(&mut self, dt: f32) {
        let t = self.time;
        let st = self.screen_t;
        let s = self.screen;
        let sel = self.sel;
        let size = self.g.size();
        // keep the models clear of the HUD on narrow windows
        let aspect = (size.x / size.y.max(1.0)).clamp(1.0, 2.2);
        let side = 4.4 * (aspect / 1.6).min(1.15);
        let left = -12.0 * (aspect / 1.6).min(1.2) * 0.47;
        // with Clippy up, the player steps right, clear of his bubble
        let left = left + 1.8 * self.clippy.presence();
        let phase = self.job.phase;
        let need_card = self.job.need_card;
        let hover = self.hover;
        let hint = self.hint;
        for i in 0..2 {
            let a = &mut self.actors[i];
            let chosen = i == sel;
            let (pos, yaw, pitch, scale, reveal, spin): (Vec3, Option<f32>, f32, f32, f32, f32) = match s {
                Screen::Boot => {
                    let x = if i == 0 { -side } else { side };
                    let r = if st > 1.2 { 1.0 } else { 0.0 };
                    (vec3(x, 0.2, if st > 1.2 { 0.0 } else { -30.0 }), None, 0.12, 1.0, r, 0.35)
                }
                Screen::Select => {
                    let x = if i == 0 { -side } else { side };
                    let h = hover == Some(i);
                    let other = hover.is_some() && !h;
                    // a hovered hint box turns its feature to the camera
                    let aim = hint.filter(|(j, _)| *j == i).and_then(|(_, f)| a.p.anchor(f)).map(|an| aim_at(an, a.yaw.x + a.spin));
                    (
                        vec3(x, 0.2, if h { 1.6 } else { 0.0 }),
                        aim.map(|(y, _)| y),
                        aim.map(|(_, p)| p).unwrap_or(0.12),
                        if h {
                            1.08
                        } else if other {
                            0.95
                        } else {
                            1.0
                        },
                        1.0,
                        if h { 0.9 } else { 0.35 },
                    )
                }
                _ if !chosen => {
                    let x = if i == 0 { -15.0 } else { 15.0 };
                    (vec3(x, -1.0, -14.0), None, 0.12, 0.8, 0.0, 0.35)
                }
                Screen::Link => {
                    let yaw = if a.p.kind == Kind::Yp3 { -0.42 } else { 0.42 };
                    (vec3(left, 1.2, 3.0), Some(yaw), -0.42, 1.0, 1.0, 0.0)
                }
                Screen::Payload => (vec3(left + 0.4, 0.5, 2.6), Some(1.15), 0.1, 1.0, 1.0, 0.0),
                Screen::Execute => {
                    if need_card {
                        (vec3(left + 0.6, 1.2, 2.4), Some(1.15), 0.08, 0.92, 1.0, 0.0)
                    } else if matches!(phase, Some(Phase::Sync | Phase::Eject)) {
                        // three-quarters round, so the card slot the files fly out of shows
                        (vec3(left, 1.4, 1.6), Some(0.8 + (t * 0.6).sin() * 0.12), 0.06, 0.9, 1.0, 0.0)
                    } else if matches!(phase, Some(Phase::Reboot | Phase::Done)) {
                        (vec3(left, 1.4, 1.6), Some((t * 0.6).sin() * 0.18), 0.06, 0.9, 1.0, 0.0)
                    } else {
                        (vec3(left, 1.4, 0.6), None, 0.12, 0.86, 1.0, 0.28)
                    }
                }
                Screen::Complete => (vec3(left + 0.4, 0.7, 3.8), Some((t * 0.7).sin() * 0.3), 0.04, 1.05, 1.0, 0.0),
                // centre stage between the two panes, turning
                Screen::Credits => (vec3(0.0, -0.4, 0.6), None, 0.1, 0.85, 1.0, 0.55),
                Screen::Fail => (vec3(left + 0.4, 0.3, 2.6), Some(-0.25), 0.18, 1.0, 1.0, 0.0),
            };
            a.pos.set(pos);
            a.pitch.target = pitch;
            a.scale.target = scale;
            a.reveal.target = reveal;
            match yaw {
                Some(y) => {
                    if a.spin != 0.0 {
                        a.settle_yaw(y);
                    }
                    a.yaw.target = y;
                }
                None => {
                    // free spin: fold the posed yaw into it, so the model
                    // carries on turning from wherever it is
                    a.spin += a.yaw.x;
                    a.yaw.x = 0.0;
                    a.yaw.target = 0.0;
                }
            }
            a.spin_rate = approach(a.spin_rate, spin, 3.0, dt);
            if yaw.is_none() {
                a.spin += a.spin_rate * dt;
            }
            let glow = match s {
                Screen::Select if hover == Some(i) => 1.25,
                Screen::Select if hover.is_some() => 0.6,
                Screen::Fail => 0.8,
                _ => 0.95,
            };
            a.glow.target = glow;
            // orbit inertia, then drift home
            a.since_drag += dt;
            if a.since_drag > 0.05 {
                a.orbit += a.orbit_v * dt;
                a.orbit_v *= (-3.5 * dt).exp();
            }
            if a.since_drag > 1.8 {
                a.orbit.x = approach(a.orbit.x, (a.orbit.x / TAU).round() * TAU, 1.6, dt);
                a.orbit.y = approach(a.orbit.y, 0.0, 1.6, dt);
            }
            a.orbit.y = a.orbit.y.clamp(-0.9, 0.9);
            a.shake = approach(a.shake, 0.0, 2.2, dt);
            a.pos.step(dt);
            a.yaw.step(dt);
            a.pitch.step(dt);
            a.scale.step(dt);
            a.reveal.step(dt);
            a.glow.step(dt);
        }
        // cable and SD card choreography
        let plugged = match s {
            Screen::Link => {
                if self.seen().is_some() {
                    1.0
                } else {
                    let plug = if self.kind() == Kind::Yp3 { 3.0..7.4 } else { 1.4..6.2 };
                    if plug.contains(&self.link_loop()) {
                        1.0
                    } else {
                        0.0
                    }
                }
            }
            Screen::Payload | Screen::Execute | Screen::Fail => 1.0,
            Screen::Complete => 1.0,
            _ => 0.0,
        };
        if plugged > self.plug.target && s == Screen::Link {
            self.last_cycle += 1;
        }
        self.plug.target = plugged;
        self.plug.step(dt);
        if self.plug.x > 0.93 && self.plug.v > 1.5 {
            self.usb_hot = 1.0;
        }
        let card = match s {
            // in once to show where it goes, and it stays
            Screen::Payload => (st > 0.8) as u8 as f32,
            Screen::Execute if need_card => {
                let p = st % 3.2;
                if (0.5..2.6).contains(&p) {
                    1.0
                } else {
                    0.0
                }
            }
            Screen::Execute | Screen::Complete => 1.0,
            _ => 0.0,
        };
        self.sd.target = card;
        self.sd.step(dt);
    }

    // ------------------------------------------------------------ actions

    fn load_package(&mut self, path: PathBuf) {
        let name = path.file_name().map(|n| n.to_string_lossy().to_string()).unwrap_or_default();
        let (tx, rx) = channel();
        let demo = self.opts.demo && !path.exists();
        let kind = self.kind();
        let p = path.clone();
        std::thread::spawn(move || {
            if demo {
                for i in 0..=30 {
                    let _ = tx.send(PkgMsg::Progress(i as f32 / 30.0, format!("card/.rockbox/rocks/{:03}.rock", i * 7)));
                    std::thread::sleep(std::time::Duration::from_millis(45));
                }
                let _ = tx.send(PkgMsg::Done(Err(format!("demo:{}", kind.id()))));
                return;
            }
            let r = (|| {
                let mut pkg = Package::open(&p)?;
                let tx2 = tx.clone();
                pkg.verify(move |f, s| {
                    let _ = tx2.send(PkgMsg::Progress(f, s.to_string()));
                })?;
                Ok(pkg)
            })();
            let _ = tx.send(PkgMsg::Done(r));
        });
        self.pkg = Some(PkgView {
            kind,
            path,
            name,
            state: PkgState::Checking(0.0, String::new()),
            target: String::new(),
            version: String::new(),
            flash: 0,
            card: 0,
            files: 0,
            segs: Vec::new(),
            t: self.time,
        });
        self.pkg_rx = Some(rx);
        self.kick = 0.3;
    }

    fn package_tick(&mut self) {
        let Some(rx) = &self.pkg_rx else { return };
        let kind = self.kind();
        let mut done = false;
        while let Ok(m) = rx.try_recv() {
            let Some(v) = self.pkg.as_mut() else { break };
            match m {
                PkgMsg::Progress(f, s) => v.state = PkgState::Checking(f, s),
                PkgMsg::Done(Ok(p)) => {
                    v.target = p.get("target").unwrap_or("?").to_uppercase();
                    v.version = p.get("version").unwrap_or("").to_string();
                    v.flash = p.flash_bytes();
                    v.card = p.card_bytes();
                    v.files = p.files.len();
                    v.segs = p.flash.iter().map(|s| (s.offset, s.size)).collect();
                    v.state = if p.get("target") == Some(kind.id()) {
                        PkgState::Ok
                    } else {
                        PkgState::Bad(tf("built for {0}, not {1}", &[&v.target, &kind.name()]))
                    };
                    done = true;
                }
                PkgMsg::Done(Err(e)) if e.starts_with("demo:") => {
                    v.target = e[5..].to_uppercase();
                    v.version = format!("v4.0-{} (8d1c2f7)", kind.id());
                    v.flash = 1_373_184;
                    let card = rockslop::device::sim::demo_card();
                    v.card = card.iter().map(|f| f.1).sum();
                    v.files = card.len();
                    v.segs = if kind == Kind::Yp3 { vec![(0xE000, 0xC2000), (0xE0000, 0x64000)] } else { vec![(0x14000, 0x13A000)] };
                    v.state = PkgState::Ok;
                    done = true;
                }
                PkgMsg::Done(Err(e)) => {
                    v.state = PkgState::Bad(i18n::msg(&e));
                    done = true;
                }
            }
        }
        if done {
            self.pkg_rx = None;
            let ok = matches!(self.pkg.as_ref().map(|p| &p.state), Some(PkgState::Ok));
            self.flash = 0.3;
            self.flash_col = if ok { pal::ACID } else { pal::RED };
            self.kick = if ok { 0.3 } else { 0.8 };
            self.audio.sfx(if ok { Sfx::Drop } else { Sfx::Error });
        }
    }

    /// The simulator's stand-in package: it doesn't exist on disk, and
    /// `load_package` fakes its contents in demo mode.
    fn demo_package(&self) -> PathBuf {
        PathBuf::from(format!("rockbox-{}-demo.tar.xz", self.kind().id()))
    }

    /// Switch to the simulated player (nothing touches USB) or back.
    fn set_demo(&mut self, on: bool) {
        self.opts.demo = on;
        let backend: Box<dyn rockslop::device::Backend> = if on { Box::new(Sim::new(1.0, None)) } else { Box::new(Real::new()) };
        // the old hub's thread ends with its channel
        self.hub = Hub::start(backend);
        // a demo package is no use to a real player, nor the other way round
        self.pkg = None;
        self.pkg_rx = None;
        rockslop::log::line("GUI", if on { "demo: a simulated player from here on" } else { "demo off: real players again" });
    }

    fn pick_file(&mut self) {
        if self.pick_rx.is_some() {
            return;
        }
        if self.opts.demo {
            self.load_package(self.demo_package());
            return;
        }
        let dialog = rfd::FileDialog::new()
            .set_title(tr("Rockbox package"))
            .add_filter(format!("{} (.tar.xz)", tr("Rockbox package")), &["xz", "txz"]);
        if cfg!(target_os = "macos") {
            // AppKit dialogs must run on the main thread
            if let Some(p) = dialog.pick_file() {
                self.load_package(p);
            }
            return;
        }
        let (tx, rx) = channel();
        std::thread::spawn(move || {
            let _ = tx.send(dialog.pick_file());
        });
        self.pick_rx = Some(rx);
    }

    fn reset_job(&mut self, restoring: bool) {
        self.job = Job { restoring, started: self.time, ..Default::default() };
    }

    fn start_flash(&mut self) {
        let Some(p) = &self.pkg else { return };
        let path = p.path.clone();
        self.reset_job(false);
        self.hub.send(Cmd::Flash { kind: self.kind(), package: path });
        self.go(Screen::Execute);
    }

    fn start_restore(&mut self) {
        let Some(b) = self.stock.clone() else { return };
        self.reset_job(true);
        self.hub.send(Cmd::Restore { kind: self.kind(), image: b.path });
        self.go(Screen::Execute);
    }

    // ------------------------------------------------------------ draw

    pub fn draw(&mut self) {
        let t = self.time;
        let size = self.g.size();
        // camera: slow sway plus mouse parallax
        let m = (self.ui.mouse / size.max(vec2(1.0, 1.0)) - 0.5).clamp(Vec2::splat(-0.5), Vec2::splat(0.5));
        let boot = if self.screen == Screen::Boot { 1.0 - ease_in_out_cubic(window(self.screen_t, 0.6, 2.0)) } else { 0.0 };
        self.g.cam.position = vec3((t * 0.13).sin() * 0.6 + m.x * 1.4, 1.5 + (t * 0.11).cos() * 0.3 - m.y * 0.8, 25.0 + boot * 8.0);
        self.g.cam.target = vec3(m.x * 0.4, 0.3, 0.0);
        self.draw_lcd();
        self.g.begin(t, self.tint);
        self.world.draw(&self.g, t);
        let order = if self.sel == 0 { [1, 0] } else { [0, 1] };
        for i in order {
            self.draw_actor(i);
        }
        if self.rockbox_lit() {
            self.lcd_rays();
        }
        self.g.set_2d();
        self.hud();
        // post
        let fail = if self.screen == Screen::Fail { 0.06 + 0.1 * (1.0 - window(self.screen_t, 0.0, 1.5)) } else { 0.0 };
        self.post.glitch = 0.012 + self.kick * 0.9 + fail;
        self.post.aberration = 0.85 + self.kick * 2.5 + self.beat * 0.7;
        self.post.flash = self.flash * 0.55;
        self.post.flash_color = self.flash_col;
        self.post.fade = if self.screen == Screen::Boot {
            1.0 - ease_out_cubic(window(self.screen_t, 0.0, 0.5)) * 0.0 - window(self.screen_t, 0.0, 0.45)
        } else {
            0.0
        };
        self.post.fade = self.post.fade.max(0.0);
        self.post.bloom =
            1.0 + self.beat * 0.14 + if self.screen == Screen::Complete { 0.25 * (1.0 - window(self.screen_t, 0.0, 2.0)) } else { 0.0 };
        self.g.finish(t, &self.post);
    }

    fn draw_actor(&mut self, i: usize) {
        let t = self.time;
        let a = &self.actors[i];
        if a.reveal.x < 0.004 {
            return;
        }
        let mtx = a.matrix(t);
        let chosen = i == self.sel && !matches!(self.screen, Screen::Select | Screen::Boot);
        let fail = self.screen == Screen::Fail && chosen;
        let (top, bottom) = match a.p.kind {
            Kind::Yp3 => (pal::CYAN, pal::CYAN),
            Kind::Ts17 => (pal::TEAL, pal::NAVY),
        };
        let red = if fail { 0.75 } else { 0.0 };
        let top = pal::mix(top, pal::RED, red);
        let bottom = pal::mix(bottom, pal::RED, red);
        let hl = self.highlights(i);
        let dims = a.p.dims;
        let fill_level = if chosen && self.screen == Screen::Execute && !self.job.restoring {
            -dims.y / 2.0 - 0.15 + self.job.shown * (dims.y + 0.3)
        } else if chosen && self.screen == Screen::Complete {
            dims.y
        } else {
            -100.0
        };
        let g = &self.g;
        g.set_3d();
        let gl = unsafe { get_internal_gl() };
        gl.quad_gl.push_model_matrix(mtx);
        // holographic shell
        gl_use_material(&g.fill);
        g.fill.set_uniform("CamPos", g.cam.position);
        g.fill.set_uniform("Rim", pal::scale(top, 0.8 * a.reveal.x.min(1.0)).to_vec());
        g.fill.set_uniform("FillColor", if fail { pal::RED } else { pal::ACID }.to_vec());
        g.fill.set_uniform("FillLevel", fill_level);
        g.fill.set_uniform("Time", t);
        g.fill.set_uniform("Alpha", 0.92f32);
        g.fill.set_uniform("Pulse", if self.hover == Some(i) { 0.6f32 } else { 0.0 });
        if a.reveal.x > 0.98 {
            let mut fill = a.p.fill.mesh();
            for v in &mut fill.vertices {
                v.color = [6, 8, 22, 255];
            }
            draw_mesh(&fill);
            // LCD
            let lcd_power = self.lcd_power();
            if lcd_power > 0.01 {
                gl_use_material(&g.lcd);
                g.lcd.set_uniform("LcdRes", vec2(128.0, 160.0));
                // seen through the back of the shell, it's dim (and mirrored)
                let behind = clamp01(-self.lcd_facing(i, &mtx).2 * 3.0);
                g.lcd.set_uniform("Power", if i == self.sel { lcd_power * (1.0 - 0.8 * behind) } else { 0.15 });
                if i == self.sel {
                    draw_mesh(&self.lcd_mesh(i, &self.lcd.texture));
                }
            }
        }
        // wires
        let mut batch = std::mem::take(&mut self.batch);
        batch.clear();
        let st = Style {
            top,
            bottom,
            accent: if fail { pal::AMBER } else { pal::MAGENTA },
            intensity: a.glow.x,
            reveal: a.reveal.x,
            time: t,
            highlight: hl,
            width: 1.6,
        };
        a.p.emit(&mut batch, &st);
        if chosen || self.screen == Screen::Link {
            self.extras(i, &mut batch);
        }
        gl_use_material(&g.line_hidden);
        g.set_line_uniforms(&g.line_hidden, 1.0, 0.16, 0.0, (100.0, 200.0));
        batch.draw();
        gl_use_material(&g.line);
        g.set_line_uniforms(&g.line, 1.0, 1.0, 0.0004, (100.0, 200.0));
        batch.draw();
        gl_use_default_material();
        gl.quad_gl.pop_model_matrix();
        self.batch = batch;
    }

    /// Cable and SD card, in the player's local space.
    fn extras(&self, i: usize, b: &mut LineBatch) {
        if i != self.sel || matches!(self.screen, Screen::Select | Screen::Boot) {
            return;
        }
        let a = &self.actors[i];
        let t = self.time;
        if let Some(usb) = a.p.anchor(Feature::Usb) {
            let k = self.plug.x;
            if k > 0.01 {
                // the tip slides in along the port's axis, from below
                let d = (1.0 - k) * 9.0 - 0.55;
                let base = usb.pos + usb.normal * d;
                let pts_xf = |p: Vec3| base + p;
                let mut tmp = LineBatch::new();
                let flow = if self.screen == Screen::Execute && self.job.ended.is_none() {
                    1.0
                } else if self.seen().is_some() {
                    0.5
                } else {
                    0.0
                };
                let fade = clamp01(k * 3.0);
                self.cable.emit(&mut tmp, pal::scale(pal::ICE, 0.8 * fade), pal::MAGENTA, flow, 1.4, t);
                for v in &mut tmp.verts {
                    let p = pts_xf(vec3(v.position.x, v.position.y, v.position.z));
                    v.position = p;
                    let o = pts_xf(vec3(v.normal.x, v.normal.y, v.normal.z));
                    v.normal = vec4(o.x, o.y, o.z, v.normal.w);
                }
                let off = b.verts.len() as u16;
                b.verts.extend(tmp.verts);
                b.idx.extend(tmp.idx.iter().map(|x| x + off));
                // plug-in spark
                if self.usb_hot > 0.02 {
                    for r in 0..3 {
                        let rr = 0.4 + (1.0 - self.usb_hot) * (1.5 + r as f32 * 0.8);
                        let ring = crate::gfx::model::ring_facing(usb.pos - usb.normal * 0.0, Vec3::Y, rr, 32);
                        b.poly(&ring, pal::scale(pal::MAGENTA, self.usb_hot * 1.8 / (r + 1) as f32), 1.6, true);
                    }
                }
            }
        }
        if let Some(slot) = a.p.anchor(Feature::Sd) {
            let k = ease_out_back(self.sd.x.clamp(0.0, 1.2));
            if self.sd.x > 0.01 || matches!(self.screen, Screen::Payload) {
                let out = (1.0 - k) * 3.2;
                let fade = clamp01(1.0 - (out - 2.6).max(0.0) * 2.0);
                let inward = -slot.normal;
                let c = pal::scale(if self.job.need_card { pal::AMBER } else { pal::ACID }, 0.95 * fade);
                // a microSD, 15 x 11 mm: u runs from the grip end (0) to the
                // contact end (1.5), which is the narrow part and goes in first
                let outline: [(f32, f32); 6] = [(0.0, -0.52), (1.5, -0.52), (1.5, 0.4), (1.0, 0.4), (0.88, 0.52), (0.0, 0.52)];
                let pt = |u: f32, v: f32, z: f32| slot.pos + inward * (u - 0.14 - out) + Vec3::Y * v + Vec3::Z * z;
                for z in [-0.04, 0.04] {
                    let pts: Vec<Vec3> = outline.iter().map(|&(u, v)| pt(u, v, z)).collect();
                    b.poly(&pts, c, 1.3, true);
                }
                for &(u, v) in &outline {
                    b.seg(pt(u, v, -0.04), pt(u, v, 0.04), pal::scale(c, 0.7), 1.0);
                }
                for kk in 0..8 {
                    let v = -0.43 + kk as f32 * 0.105;
                    b.seg(pt(1.14, v, 0.045), pt(1.42, v, 0.045), pal::scale(c, 0.6), 1.0);
                }
                // grip ridge at the end that stays out
                b.seg(pt(0.1, -0.5, 0.045), pt(0.1, 0.5, 0.045), pal::scale(c, 0.5), 1.0);
            }
        }
    }

    fn highlights(&self, i: usize) -> Vec<(Feature, f32)> {
        if self.screen == Screen::Select {
            return self.hint.filter(|(j, _)| *j == i).map(|(_, f)| vec![(f, 1.0)]).unwrap_or_default();
        }
        if i != self.sel {
            return Vec::new();
        }
        match self.screen {
            Screen::Link => {
                let (steps, active, _) = self.link_plan();
                let s = &steps[active];
                s.feat.into_iter().chain(s.also).map(|f| (f, 1.0)).collect()
            }
            Screen::Payload => vec![(Feature::Sd, 0.8)],
            Screen::Execute if self.job.need_card => vec![(Feature::Sd, 1.0), (Feature::Reset, 1.0)],
            Screen::Execute if self.job.phase == Some(Phase::Sync) && self.job.sync.started.is_some() => {
                vec![(Feature::Sd, 0.25 + 0.75 * (-(self.time - self.job.sync.last) * 6.0).exp())]
            }
            _ => Vec::new(),
        }
    }

    fn lcd_power(&self) -> f32 {
        let base = match self.screen {
            Screen::Execute => 0.9,
            Screen::Complete | Screen::Credits => 1.0,
            Screen::Fail => 0.7,
            Screen::Link | Screen::Payload => 0.35,
            _ => 0.18,
        };
        base
    }

    /// The player's screen: a quad in its local space, showing `tex`.
    fn lcd_mesh(&self, i: usize, tex: &Texture2D) -> Mesh {
        let p = &self.actors[i].p;
        let c = p.lcd_center + vec3(0.0, 0.0, 0.003);
        let h = p.lcd_size / 2.0;
        let v = |x: f32, y: f32, u: f32, w: f32| Vertex::new(c.x + x, c.y + y, c.z, u, w, WHITE);
        Mesh {
            vertices: vec![v(-h.x, -h.y, 0.0, 0.0), v(h.x, -h.y, 1.0, 0.0), v(h.x, h.y, 1.0, 1.0), v(-h.x, h.y, 0.0, 1.0)],
            indices: vec![0, 1, 2, 0, 2, 3],
            texture: Some(tex.clone()),
        }
    }

    /// Player `i`'s screen in world space: its centre, its normal, and how
    /// squarely it faces the camera (1 head on, 0 edge on, -1 away).
    fn lcd_facing(&self, i: usize, mtx: &Mat4) -> (Vec3, Vec3, f32) {
        let p = &self.actors[i].p;
        let c = mtx.transform_point3(p.lcd_center);
        let n = mtx.transform_vector3(Vec3::Z).normalize_or_zero();
        (c, n, n.dot((self.g.cam.position - c).normalize_or_zero()))
    }

    /// Rockbox is running on the chosen player (booting, its USB screen,
    /// then its menu): light comes through the player's screen.
    fn rockbox_lit(&self) -> bool {
        !self.job.restoring
            && match self.screen {
                Screen::Complete | Screen::Credits => true,
                Screen::Execute => matches!(self.job.phase, Some(Phase::Reboot | Phase::Sync | Phase::Eject | Phase::Done)),
                _ => false,
            }
    }

    /// The neon sign of Rockbox's name on the player's screen, 0..1: dark
    /// while the firmware is written, sputtering on as Rockbox boots.
    fn sign_lit(&self) -> f32 {
        let lit = self.job.booted.map(|b| neon(self.time - b, self.time));
        match self.screen {
            Screen::Execute => lit.unwrap_or(0.0),
            Screen::Complete | Screen::Credits => lit.unwrap_or(1.0),
            _ => 0.0,
        }
    }

    /// God rays: a light inside the player shines out through what is on
    /// its screen. The light sits behind the glass, so the shafts swing as
    /// the player turns, and die as it turns away.
    fn lcd_rays(&self) {
        let i = self.sel;
        let a = &self.actors[i];
        if a.reveal.x < 0.98 {
            return;
        }
        let t = self.time;
        let mtx = a.matrix(t);
        let (c, n, facing) = self.lcd_facing(i, &mtx);
        // a surge as the sign lights
        let since = self.job.booted.map(|b| t - b - 0.7).unwrap_or(-1.0);
        let surge = if since > 0.0 { 1.0 + 1.5 * (-since * 1.2).exp() } else { 1.0 };
        let k = clamp01(facing * 1.8) * surge;
        let Some(from) = self.g.project(c - n * a.p.lcd_size.y * 0.9) else { return };
        let up = mtx.transform_vector3(Vec3::Y) * a.p.lcd_size.y * 0.5;
        let inner = match (self.g.project(c), self.g.project(c + up)) {
            (Some(p), Some(q)) => p.distance(q),
            _ => 0.0,
        };
        let b = self.beat;
        let tint = pal::mix(pal::mix(pal::CYAN, pal::MAGENTA, 0.5 + 0.5 * (t * 0.4).sin()), pal::WHITE, b * 0.4);
        let mesh = self.lcd_mesh(i, &self.lcd_light.texture);
        self.g.god_rays(
            || {
                let gl = unsafe { get_internal_gl() };
                gl.quad_gl.push_model_matrix(mtx);
                draw_mesh(&mesh);
                gl.quad_gl.pop_model_matrix();
            },
            from,
            inner,
            k * (1.4 + 2.0 * b),
            0.8 + 0.12 * b,
            t,
            tint,
        );
    }

    /// Rockbox's name as a neon sign on the player's screen: `w` px wide,
    /// centred, baseline at `y`. Unlit (`lit` 0) its tubes show dark; lit,
    /// they glow and the letters fill with light. As `light`, the light
    /// behind the glass: solid letters, so it pours out of them. A band of
    /// light sweeps across it now and then (and so do the rays).
    fn lcd_logo(&self, y: f32, w: f32, light: bool, lit: f32) {
        if light && lit < 0.01 {
            return;
        }
        let t = self.time;
        let word = "ROCKbox";
        let (_, unit) = text::outline(&self.g, word, 1.0, 0.0);
        let size = w / unit.max(0.01);
        let band = ((t * 0.3) % 1.0) * 260.0 - 60.0;
        let mut x = 64.0 - w / 2.0;
        for (k, ch) in word.char_indices() {
            let ch = &word[k..k + ch.len_utf8()];
            let c = if k < 4 { pal::ICE } else { pal::MAGENTA };
            let (paths, adv) = text::outline(&self.g, ch, size, 0.0);
            let hot = (-(x + adv * 0.5 - band).powi(2) / 300.0).exp() * lit;
            if lit > 0.01 {
                let fill = if light { pal::scale(pal::mix(c, WHITE, 0.3 + 0.7 * hot), lit) } else { pal::alpha(c, (0.14 + 0.3 * hot) * lit) };
                text::draw(&self.g, Face::Display, ch, vec2(x, y), size, fill, Align::Left);
            }
            let on = pal::mix(c, WHITE, 0.25 + 0.75 * hot);
            let edge = if light { pal::scale(on, lit) } else { pal::mix(pal::scale(c, 0.5), on, lit) };
            for path in &paths {
                for j in 0..path.len() {
                    let (p, q) = (path[j], path[(j + 1) % path.len()]);
                    draw_line(x + p.x, y + p.y, x + q.x, y + q.y, if light { 1.8 } else { 1.0 }, edge);
                }
            }
            x += adv;
        }
    }

    /// Render the player's 128x160 screen for this moment, and the light
    /// that shines through it.
    fn draw_lcd(&self) {
        self.lcd_screen(&self.lcd, false);
        if self.rockbox_lit() {
            self.lcd_screen(&self.lcd_light, true);
        }
    }

    /// The player's screen into `rt`; `light`: the light behind it instead
    /// (the same, but Rockbox's name is solid, so it pours out of the
    /// letters).
    fn lcd_screen(&self, rt: &RenderTarget, light: bool) {
        let t = self.time;
        set_camera(&Camera2D {
            zoom: vec2(2.0 / 128.0, -2.0 / 160.0),
            target: vec2(64.0, 80.0),
            render_target: Some(rt.clone()),
            ..Default::default()
        });
        clear_background(BLACK);
        let font = &self.g.lcd_font;
        let txt = |s: &str, x: f32, y: f32, size: u16, c: Color| {
            draw_text_ex(s, x, y, TextParams { font: Some(font), font_size: size, font_scale: 1.0, color: c, ..Default::default() });
        };
        let phase = self.job.phase;
        let execute = self.screen == Screen::Execute;
        let usb = execute && !self.job.restoring && matches!(phase, Some(Phase::Sync | Phase::Eject | Phase::Done));
        let booting = execute && phase == Some(Phase::Reboot);
        if self.screen == Screen::Fail {
            for y in (0..160).step_by(3) {
                let k = ((y as f32 * 12.9898 + (t * 30.0).floor() * 78.233).sin() * 43758.5).fract().abs();
                draw_rectangle(0.0, y as f32, 128.0, 2.0, Color::new(0.4 * k, 0.0, 0.05, 1.0));
            }
            txt("ERR", 40.0, 86.0, 22, pal::RED);
            return;
        }
        // Rockbox's name across the top, from the start of a flash: an
        // unlit neon sign until the new firmware boots, then it sputters on
        let sign = !self.job.restoring && matches!(self.screen, Screen::Execute | Screen::Complete | Screen::Credits);
        let lit = self.sign_lit();
        if sign {
            self.lcd_logo(26.0, 112.0, light, lit);
            if !light || lit > 0.01 {
                draw_line(8.0, 34.0, 120.0, 34.0, 1.0, pal::scale(pal::CYAN, if light { 0.45 * lit } else { 0.15 + 0.3 * lit }));
            }
        }
        let top = if sign { 40.0 } else { 0.0 };
        let menu = self.screen == Screen::Complete && !self.job.restoring;
        if booting {
            // the boot splash's progress bar
            let k = clamp01((t - self.job.phase_t) / 2.0);
            draw_rectangle_lines(23.0, 97.0, 82.0, 5.0, 1.0, pal::scale(pal::CYAN, 0.6));
            draw_rectangle(24.0, 98.0, 80.0 * k, 3.0, pal::ACID);
        } else if self.screen == Screen::Credits {
            // Rockbox's credits plugin: the names scroll up under the sign
            let names = self.roll.names();
            // name i enters at the bottom and climbs to the rule
            let off = self.screen_t * 18.0;
            let first = ((off - 124.0) / 13.0).floor().max(0.0) as usize;
            for k in 0..13 {
                let i = first + k;
                let y = 160.0 + i as f32 * 13.0 - off;
                if !(40.0..175.0).contains(&y) {
                    continue;
                }
                let name = names[i % names.len()];
                let w = text::measure(&self.g, Face::Mono, name, 10.0).x;
                let c = pal::mix(pal::ICE, pal::ACID, ((i * 37) % 11) as f32 / 11.0);
                text::draw(&self.g, Face::Mono, name, vec2((64.0 - w / 2.0).max(2.0), y), 10.0, pal::alpha(c, clamp01((y - 40.0) / 24.0)), Align::Left);
            }
        } else if menu {
            // Rockbox's main menu under the sign (a liberty: the real one
            // has a status bar there), the cursor walking down it
            let items = ["Files", "Database", "Now Playing", "Settings", "Playlists", "Plugins", "System"];
            let sel = ((t * 0.8) as usize) % items.len();
            for (k, s) in items.iter().enumerate() {
                let y = 39.0 + k as f32 * 17.0;
                if k == sel {
                    draw_rectangle_lines(4.0, y, 120.0, 15.0, 1.0, pal::ACID);
                    draw_triangle(vec2(8.0, y + 4.0), vec2(12.0, y + 7.5), vec2(8.0, y + 11.0), pal::ACID);
                    txt(s, 16.0, y + 11.0, 11, WHITE);
                } else {
                    txt(s, 16.0, y + 11.0, 11, pal::scale(pal::ICE, 0.75));
                }
            }
        } else if usb {
            // Rockbox's USB screen while the card syncs, under the sign
            let c = pal::ICE;
            let cx = 64.0;
            let y = |v: f32| v + 18.0;
            draw_line(cx, y(50.0), cx, y(110.0), 3.0, c);
            draw_triangle(vec2(cx, y(42.0)), vec2(cx - 7.0, y(54.0)), vec2(cx + 7.0, y(54.0)), c);
            draw_line(cx, y(92.0), cx - 18.0, y(78.0), 3.0, c);
            draw_line(cx - 18.0, y(78.0), cx - 18.0, y(66.0), 3.0, c);
            draw_circle(cx - 18.0, y(63.0), 4.0, c);
            draw_line(cx, y(100.0), cx + 18.0, y(86.0), 3.0, c);
            draw_rectangle(cx + 14.0, y(70.0), 9.0, 9.0, c);
            draw_line(cx + 18.0, y(86.0), cx + 18.0, y(79.0), 3.0, c);
            draw_circle(cx, y(112.0), 6.0, c);
            if self.job.need_card {
                txt("NO CARD", 36.0, 153.0, 12, pal::AMBER);
            }
        } else if self.screen == Screen::Complete {
            // stock's menu, after a return to stock
            draw_rectangle(0.0, 0.0, 128.0, 12.0, Color::new(0.12, 0.14, 0.2, 1.0));
            draw_rectangle_lines(104.0, 3.0, 18.0, 7.0, 1.0, WHITE);
            draw_rectangle(105.0, 4.0, 14.0, 5.0, pal::ACID);
            let items = ["Music", "Video", "Photo", "Record", "Radio", "Settings"];
            let sel = ((t * 0.8) as usize) % items.len();
            for (k, s) in items.iter().enumerate() {
                let y = 18.0 + k as f32 * 16.0;
                if k == sel {
                    draw_rectangle(0.0, y, 128.0, 15.0, Color::new(0.1, 0.45, 0.9, 1.0));
                }
                txt(s, 8.0, y + 11.0, 11, WHITE);
            }
        } else if execute {
            // ROM mode, hacker cut: hex rain (under the sign)
            for col in 0..9 {
                let x = 3.0 + col as f32 * 14.0;
                let speed = 30.0 + (col * 37 % 23) as f32;
                for row in 0..12 {
                    let y = ((t * speed + row as f32 * 15.0 + col as f32 * 41.0) % 175.0) - 10.0;
                    if y < top + 4.0 {
                        continue;
                    }
                    let h = ((col * 131 + row * 71 + (t * 6.0) as usize) % 256) as u8;
                    let k = row as f32 / 12.0 * clamp01((y - top - 4.0) / 14.0);
                    txt(&format!("{h:02X}"), x, y, 10, pal::alpha(pal::mix(pal::MAGENTA, pal::CYAN, k), 0.4 + 0.6 * k));
                }
            }
            let p = self.job.shown;
            draw_rectangle(8.0, 146.0, 112.0, 6.0, BLACK);
            draw_rectangle_lines(8.0, 146.0, 112.0, 6.0, 1.0, WHITE);
            draw_rectangle(9.0, 147.0, 110.0 * p, 4.0, pal::ACID);
        } else {
            // idle: faint static
            for k in 0..60 {
                let h = ((k as f32 * 12.9898 + (t * 20.0).floor()).sin() * 43758.5).fract().abs();
                let y = (h * 160.0).floor();
                draw_rectangle(0.0, y, 128.0, 1.0, Color::new(0.1, 0.12, 0.2, 0.5));
            }
        }
    }

    // ------------------------------------------------------------ HUD

    fn hud(&mut self) {
        self.ui.begin_hud();
        let s = self.screen;
        // under the picker, nothing sees the pointer
        let input = (self.ui.mouse, self.ui.pressed, self.ui.released);
        if self.picker.is_some() {
            self.ui.mouse = vec2(-1e5, -1e5);
            self.ui.pressed = false;
            self.ui.released = false;
        }
        if s != Screen::Boot {
            self.chrome();
        }
        match s {
            Screen::Boot => self.hud_boot(),
            Screen::Select => self.hud_select(),
            Screen::Link => self.hud_link(),
            Screen::Payload => self.hud_payload(),
            Screen::Execute => self.hud_execute(),
            Screen::Complete => self.hud_complete(),
            Screen::Credits => self.hud_credits(),
            Screen::Fail => self.hud_fail(),
        }
        if s != Screen::Boot {
            // top left, under the logo: clear of the log and the panels
            self.np.draw(&mut self.ui, &self.g, vec2(30.0, 84.0), self.beat);
        }
        // bottom left, over the BACK button
        let feet = vec2(92.0, self.g.size().y - 118.0);
        self.clippy.draw(&mut self.ui, &self.g, feet, self.time);
        (self.ui.mouse, self.ui.pressed, self.ui.released) = input;
        if let Some(p) = &mut self.picker {
            p.draw(&mut self.ui, &self.g);
        }
        for s in self.ui.sfx.drain(..) {
            self.audio.sfx(s);
        }
    }

    /// The language can change here (not mid-flash).
    fn lang_ok(&self) -> bool {
        !matches!(self.screen, Screen::Boot | Screen::Execute)
    }

    fn open_picker(&mut self) {
        self.picker = Some(Picker::open(self.time));
        self.lang_seen = true;
    }

    fn hud_credits(&mut self) {
        let sz = self.g.size();
        let t = self.time;
        let st = self.screen_t;
        let (lr, rr, wave_y) = credits::layout(sz);
        let beat = self.beat;
        // the title, centre top between the panes
        let cw = rr.x - (lr.x + lr.w) - 24.0;
        let top = lr.y + 44.0;
        text::glitch_title(
            &self.g,
            &text::decode(tr("CREDITS"), st * 1.5, t, 3),
            vec2(sz.x / 2.0, top),
            30.0,
            pal::mix(pal::ICE, pal::WHITE, beat * 0.5),
            Align::Center,
            0.06 + beat * 0.25,
            t,
            7.0,
            cw,
        );
        text::draw_spaced_fit(
            &self.g,
            Face::Mono,
            &text::decode("xX ROCKSLOP FLASHER Xx", st * 1.2 - 0.3, t, 4),
            vec2(sz.x / 2.0, top + 22.0),
            11.0,
            pal::MAGENTA,
            Align::Center,
            3.0,
            cw,
        );
        self.roll.draw_ours(&mut self.ui, &self.g, lr, credits::appear(st, 0.25), beat);
        self.roll.draw_rockbox(&mut self.ui, &self.g, rr, credits::appear(st, 0.45), beat);
        let msg = format!("{}   //   xX ROCKSLOP FLASHER Xx   //   ", tr(credits::FRIENDS));
        credits::scroller(&self.g, wave_y, 36.0, sz.x - 36.0, &msg, t, beat, credits::appear(st, 0.9));
        // the whole roll watched: Clippy's encore (cut short, it comes
        // round again)
        if self.encore.is_none() && (self.roll.rolled() || (self.opts.tips_now && st > 1.0)) {
            self.encore = Some((t, true));
        }
        if let Some((at, full)) = self.encore {
            let (e, prev) = (t - at, t - at - self.ui.dt);
            self.clippy.encore(&self.g, e, full, t, wave_y, sz.x, beat);
            if prev < clippy::ENCORE_DROP && e >= clippy::ENCORE_DROP {
                self.audio.sfx(Sfx::Drop);
                self.kick = self.kick.max(0.4);
            }
            if full && prev < clippy::ENCORE_LAND && e >= clippy::ENCORE_LAND {
                self.audio.sfx(Sfx::Confirm);
                self.kick = 1.0;
                self.flash = 0.9;
                self.flash_col = pal::MAGENTA;
                self.encored = true;
            }
        }
        let ba = credits::appear(st, 0.7);
        let bw = 150.0f32.min(cw);
        if ui::button(&mut self.ui, &self.g, Rect::new(sz.x / 2.0 - bw / 2.0, lr.y + lr.h - 40.0, bw, 40.0), tr("BACK"), Tone::Secondary, ba, true) {
            self.go(Screen::Complete);
        }
    }

    fn chrome(&mut self) {
        let g = &self.g;
        let sz = g.size();
        let t = self.time;
        let a = if self.screen == Screen::Select && self.screen_t < 1.0 { ease_out_cubic(self.screen_t) } else { 1.0 };
        ui::brackets(Rect::new(14.0, 14.0, sz.x - 28.0, sz.y - 28.0), 26.0, 1.5, pal::alpha(pal::scale(pal::CYAN, 0.55), a));
        // logo
        text::draw_spaced(g, Face::Display, "ROCKSLOP", vec2(36.0, 50.0), 17.0, pal::alpha(pal::ICE, a), Align::Left, 3.0);
        let fw = text::draw(g, Face::Mono, "xX FLASHER Xx", vec2(37.0, 66.0), 11.0, pal::alpha(pal::MAGENTA, a), Align::Left);
        // which build, for bug reports: ICE keeps its shape in the corner's
        // colour split
        text::draw_spaced(g, Face::Mono, &format!("BUILD {}", rockslop::VERSION), vec2(37.0, 84.0), 11.0, pal::alpha(pal::scale(pal::ICE, 0.85), a), Align::Left, 1.0);
        if self.opts.demo {
            // a simulated player: say so on every screen
            let blink = if (t * 2.0) as i32 % 2 == 0 { 1.0 } else { 0.45 };
            draw_circle(37.0 + fw + 12.0, 62.5, 2.5, pal::alpha(pal::AMBER, a * blink));
            text::draw_spaced(g, Face::Mono, "DEMO", vec2(37.0 + fw + 20.0, 66.0), 10.0, pal::alpha(pal::AMBER, a), Align::Left, 2.0);
        }
        // steps
        let steps = [("01", tr("TARGET")), ("02", tr("LINK")), ("03", tr("PAYLOAD")), ("04", tr("EXECUTE"))];
        let cur = match self.screen {
            Screen::Select => 0,
            Screen::Link => 1,
            Screen::Payload => 2,
            _ => 3,
        };
        let total_w = 520.0f32.min(sz.x - 520.0).max(300.0);
        let x0 = sz.x / 2.0 - total_w / 2.0;
        let sw = total_w / 4.0;
        for (k, (n, l)) in steps.iter().enumerate() {
            let x = x0 + k as f32 * sw;
            let c = if k == cur {
                pal::CYAN
            } else if k < cur {
                pal::scale(pal::ACID, 0.7)
            } else {
                pal::scale(pal::DIM, 0.6)
            };
            text::draw(g, Face::Mono, n, vec2(x, 50.0), 11.0, pal::alpha(c, a), Align::Left);
            text::draw_spaced_fit(g, Face::Semi, l, vec2(x + 20.0, 50.0), 10.0, pal::alpha(c, a), Align::Left, 2.0, sw - 26.0);
            let lw = if k == cur { sw - 14.0 } else { 18.0 };
            draw_rectangle(x, 58.0, lw * a, if k == cur { 2.0 } else { 1.0 }, pal::alpha(c, a));
            if k == cur {
                let p = (t * 0.7) % 1.0;
                draw_rectangle(x + p * (lw - 10.0), 57.0, 10.0, 4.0, pal::alpha(pal::WHITE, 0.8 * a));
            }
        }
        // clock + bus
        let mm = (t / 60.0) as u32;
        text::draw(
            g,
            Face::Mono,
            &format!("T+{mm:02}:{:05.2}", t % 60.0),
            vec2(sz.x - 36.0, 50.0),
            12.0,
            pal::alpha(pal::ICE, a),
            Align::Right,
        );
        let n = self.hub.seen.len();
        let (bus, bc) = if n == 0 {
            (tr("BUS IDLE").to_string(), pal::DIM)
        } else {
            let s = self.hub.seen[0];
            let m = match s.mode {
                Mode::Rom => "DFU",
                Mode::Stock => tr("STOCK"),
                Mode::Rockbox => "ROCKBOX",
            };
            (format!("{} {m}", s.kind.name()), if s.access { pal::ACID } else { pal::AMBER })
        };
        let blink = if (t * 2.0) as i32 % 2 == 0 || n == 0 { 1.0 } else { 0.35 };
        let w = text::draw(g, Face::Mono, &bus, vec2(sz.x - 36.0, 66.0), 11.0, pal::alpha(bc, a), Align::Right);
        draw_circle(sz.x - 44.0 - w, 62.5, 3.0, pal::alpha(bc, a * blink));
        // ticker
        let tick: String = (0..64)
            .map(|k| {
                let h = ((k as f32 * 7.31 + (t * 9.0).floor() * 0.37).sin() * 9871.3).fract().abs();
                b"0123456789ABCDEF"[(h * 16.0) as usize % 16] as char
            })
            .collect();
        let tw = text::draw(
            g,
            Face::Mono,
            &tick,
            vec2(sz.x - 36.0, sz.y - 30.0),
            10.0,
            pal::alpha(pal::scale(pal::VIOLET, 0.8), a * 0.8),
            Align::Right,
        );
        let prompt = match self.screen {
            Screen::Select => tr("awaiting target"),
            Screen::Link => tr("scanning usb"),
            Screen::Payload => tr("awaiting payload"),
            Screen::Execute => tr("executing"),
            Screen::Complete => tr("session complete"),
            Screen::Credits => tr("rolling credits"),
            Screen::Fail => tr("session aborted"),
            Screen::Boot => "",
        };
        let pt = text::decode(prompt, self.screen_t * 2.0, t, 7);
        let cur_on = (t * 2.0) as i32 % 2 == 0;
        let w = text::draw_fit(g, Face::Mono, &format!("> {pt}"), vec2(36.0, sz.y - 30.0), 13.0, pal::alpha(pal::CYAN, a), Align::Left, sz.x * 0.3);
        if cur_on {
            draw_rectangle(38.0 + w, sz.y - 41.0, 8.0, 13.0, pal::alpha(pal::CYAN, a));
        }
        let badge = self.sound_badge(vec2(sz.x - 36.0 - tw - 26.0, sz.y - 30.0), a);
        self.bottom_free = (36.0 + w + 30.0, badge - 24.0);
        // the language button: top right, left of the clock, clear of the steps
        let clock_w = text::measure(&self.g, Face::Mono, "T+00:00.00", 12.0).x;
        let bus_w = text::measure(&self.g, Face::Mono, &bus, 11.0).x + 14.0;
        self.lang_button(sz.x - 36.0 - clock_w.max(bus_w) - 26.0, x0 + total_w + 20.0, a);
    }

    /// Left of the ticker: EQ bars that jump with the music; click or M
    /// mutes. Dim and inert without an audio device.
    /// Returns its left edge.
    fn sound_badge(&mut self, right: Vec2, a: f32) -> f32 {
        let t = self.time;
        let live = self.audio.live();
        let muted = self.audio.muted();
        let playing = live && !muted && matches!(self.screen, Screen::Execute | Screen::Complete | Screen::Credits);
        let label = if !live {
            tr("NO SND")
        } else if muted {
            tr("MUTED")
        } else {
            tr("SND")
        };
        let lw = text::measure(&self.g, Face::Mono, label, 11.0).x;
        let kw = text::measure(&self.g, Face::Mono, "[M]", 11.0).x;
        let (n, bw, gap, bh) = (7, 3.0, 2.0, 12.0);
        let bars_w = n as f32 * (bw + gap) - gap;
        let w = bars_w + 8.0 + lw + 6.0 + kw;
        let r = Rect::new(right.x - w - 9.0, right.y - 17.0, w + 18.0, 24.0);
        let over = live && self.ui.over(r);
        if self.ui.click(over, Sfx::Click) {
            self.toggle_mute();
        }
        if over {
            self.np.poke(t);
        }
        let g = &self.g;
        let col = if !live {
            pal::scale(pal::ICE, 0.6)
        } else if muted {
            pal::AMBER
        } else if playing {
            pal::MAGENTA
        } else {
            pal::scale(pal::VIOLET, 0.9)
        };
        // the label keeps some green, so the corner's colour split doesn't
        // pull it apart (magenta and violet have none)
        let tc = pal::mix(col, pal::ICE, 0.45);
        let hot = self.ui.hover("snd", over);
        ui::chip(r, col, hot, a);
        let ca = a * (0.85 + 0.15 * hot);
        let x0 = right.x - w;
        for k in 0..n {
            let h = if playing {
                let wob = 0.5 + 0.5 * (t * (5.0 + k as f32 * 1.7) + k as f32 * 2.1).sin();
                let kick = self.beat * (1.0 - (k as f32 - 1.0).abs() / n as f32);
                (0.18 + 0.45 * wob * (0.4 + self.beat) + 0.5 * kick).min(1.0)
            } else {
                0.12 + 0.05 * (t * 1.3 + k as f32).sin().abs()
            };
            let x = x0 + k as f32 * (bw + gap);
            draw_rectangle(x, right.y - h * bh, bw, h * bh, pal::alpha(col, ca));
        }
        if muted {
            ui::line(vec2(x0 - 2.0, right.y + 1.0), vec2(x0 + bars_w + 2.0, right.y - bh - 1.0), 1.5, pal::alpha(pal::AMBER, ca));
        }
        text::draw(g, Face::Mono, label, vec2(x0 + bars_w + 8.0, right.y), 11.0, pal::alpha(tc, ca), Align::Left);
        if live {
            text::draw(g, Face::Mono, "[M]", vec2(x0 + bars_w + 14.0 + lw, right.y), 11.0, pal::alpha(pal::scale(pal::ICE, 0.6), ca), Align::Left);
        }
        r.x
    }

    /// The language button: a globe and the language's own name (just the
    /// globe if the window is too narrow for both), right-aligned at
    /// `right`, never left of `min_x`. Click (or L) for the picker; dimmed
    /// while a job runs.
    fn lang_button(&mut self, right: f32, min_x: f32, a: f32) {
        let t = self.time;
        let ok = self.lang_ok() && self.picker.is_none();
        let name = if i18n::pseudo() { "[Ƥşḗŭḓǿ]" } else { i18n::current().name };
        let (size, sp) = text::fit(&self.g, Face::Semi, name, 11.0, 1.0, 130.0);
        let nw = text::spaced_width(&self.g, Face::Semi, name, size, sp).min(130.0);
        let full = 12.0 + 16.0 + 10.0 + nw + 14.0;
        let w = if right - full >= min_x { full } else { 40.0 };
        let r = Rect::new(right - w, 38.0, w, 30.0);
        let over = ok && self.ui.over(r);
        if self.ui.click(over, Sfx::Click) {
            self.open_picker();
        }
        let h = self.ui.hover("lang-button", over);
        let col = if ok { pal::mix(pal::CYAN, pal::WHITE, 0.4 * h) } else { pal::DIM };
        let g = &self.g;
        let pts = ui::chamfer(r, 8.0);
        ui::fill_convex(&pts, Color::new(0.01, 0.02, 0.05, 0.75 * a));
        if h > 0.01 {
            ui::fill_convex(&pts, pal::alpha(pal::MAGENTA, 0.16 * h * a));
        }
        ui::polyline(&pts, 1.2 + 0.6 * h, pal::alpha(pal::mix(pal::scale(pal::CYAN, 0.6), pal::MAGENTA, h), a * if ok { 1.0 } else { 0.5 }), true);
        // the globe, turning: rim, equator, two meridians
        let c = vec2(r.x + 20.0, r.y + r.h / 2.0);
        let rad = 8.0;
        let ca = pal::alpha(col, a);
        ui::ring(c, rad, 1.3, ca, 24);
        ui::line(c - vec2(rad, 0.0), c + vec2(rad, 0.0), 1.1, ca);
        for ph in [0.0, std::f32::consts::FRAC_PI_2] {
            let k = (t * 0.8 + ph).sin();
            let pts: Vec<Vec2> = (0..=12)
                .map(|i| {
                    let u = i as f32 / 12.0 * std::f32::consts::PI;
                    c + vec2(u.sin() * rad * k, -u.cos() * rad)
                })
                .collect();
            ui::polyline(&pts, 1.0, pal::alpha(col, a * (0.45 + 0.55 * k.abs())), false);
        }
        // until it's been used, a ring rolls off it now and then
        if !self.lang_seen && ok && self.screen == Screen::Select {
            let p = (t * 0.5) % 1.0;
            ui::ring(c, rad + 2.0 + p * 18.0, 1.2, pal::alpha(pal::CYAN, a * (1.0 - p) * 0.8), 32);
        }
        if w > 40.0 {
            text::draw_spaced(g, Face::Semi, name, vec2(r.x + 38.0, r.y + r.h / 2.0 + size * 0.38), size, ca, Align::Left, sp);
        }
        if over {
            ui::brackets(Rect::new(r.x - 4.0, r.y - 4.0, r.w + 8.0, r.h + 8.0), 6.0, 1.3, pal::alpha(pal::MAGENTA, a));
        }
    }

    fn hud_boot(&mut self) {
        let g = &self.g;
        let sz = g.size();
        let st = self.screen_t;
        let t = self.time;
        let c = sz / 2.0;
        let k = window(st, 0.35, 0.9);
        let out = window(st, 1.9, 0.6);
        if st < 0.35 {
            if (t * 6.0) as i32 % 2 == 0 {
                draw_rectangle(c.x - 6.0, c.y - 10.0, 12.0, 20.0, pal::CYAN);
            }
            return;
        }
        let a = 1.0 - out;
        let title = text::decode("ROCKSLOP", k, t, 99);
        text::glitch_title(
            g,
            &title,
            vec2(c.x, c.y + 10.0 - out * 40.0),
            76.0,
            pal::alpha(pal::ICE, a),
            Align::Center,
            0.15 + out * 0.8,
            t,
            12.0,
            g.size().x - 80.0,
        );
        let sub = text::decode("xX  FLASHER  Xx", window(st, 0.8, 0.6), t, 5);
        text::draw_spaced(g, Face::Mono, &sub, vec2(c.x, c.y + 52.0 - out * 40.0), 18.0, pal::alpha(pal::MAGENTA, a), Align::Center, 6.0);
        let bw = 380.0 * ease_out_expo(window(st, 0.5, 1.2));
        draw_rectangle(c.x - bw / 2.0, c.y + 70.0 - out * 40.0, bw, 2.0, pal::alpha(pal::CYAN, a));
        let lines = [tr("neural uplink"), tr("usb stack"), "smtlink / adfu", tr("flash maps"), tr("glyph cache"), tr("payloads")];
        for (i, l) in lines.iter().enumerate() {
            let lt = window(st, 0.6 + i as f32 * 0.16, 0.25);
            if lt > 0.0 {
                let s = format!("[ OK ] {}", text::decode(l, lt, t, i as u32));
                text::draw(
                    g,
                    Face::Mono,
                    &s,
                    vec2(40.0, sz.y - 150.0 + i as f32 * 18.0),
                    12.0,
                    pal::alpha(pal::scale(pal::ACID, 0.9), a),
                    Align::Left,
                );
            }
        }
    }

    fn hud_select(&mut self) {
        let sz = self.g.size();
        let t = self.time;
        let st = self.screen_t;
        let ta = window(st, 0.1, 0.8);
        let title = text::decode(tr("SELECT TARGET"), ta, t, 3);
        text::glitch_title(&self.g, &title, vec2(sz.x / 2.0, 100.0), 30.0, pal::ICE, Align::Center, 0.08 + self.kick * 0.4, t, 7.0, sz.x - 160.0);
        let subs = ["SL6801 \u{b7} 2 MiB NOR \u{b7} SMTLINK ROM", "ACTIONS LARK \u{b7} 4 MiB NOR \u{b7} ADFU"];
        let mut under = None;
        for i in 0..2 {
            let a = window(st, 0.3 + i as f32 * 0.15, 0.6);
            let h = self.hover == Some(i);
            let kind = kind_of(i);
            let hh = self.actors[i].p.dims.y / 2.0;
            let status = self.hub.find(kind).map(|s| match s.mode {
                Mode::Rom => (tr("DOWNLOAD MODE"), pal::ACID),
                Mode::Stock => (tr("STOCK FIRMWARE"), pal::AMBER),
                Mode::Rockbox => ("ROCKBOX", pal::CYAN),
            });
            let blink = if (t * 2.5) as i32 % 2 == 0 { 1.0 } else { 0.4 };
            // name tag above the player, as on the sketch
            let hot = self.ui.glow(&format!("tag{i}"), h);
            if let Some(p) = self.rest_px(i, vec3(0.0, hh + 0.75, 0.0)) {
                let name = text::decode(kind.name(), a, t, i as u32 + 11);
                let tw = text::measure(&self.g, Face::Display, kind.name(), 24.0).x + 6.0 * (kind.name().len() as f32 - 1.0);
                let r = ui::unfold(Rect::new(p.x - tw / 2.0 - 22.0, p.y - 19.0, tw + 44.0, 38.0), a);
                let c = pal::mix(pal::scale(pal::CYAN, 0.8), pal::MAGENTA, hot);
                ui::fill_convex(&ui::chamfer(r, 9.0), Color::new(0.01, 0.015, 0.04, 0.8 * a));
                ui::fill_convex(&ui::chamfer(r, 9.0), pal::alpha(pal::MAGENTA, 0.14 * hot * a));
                ui::polyline(&ui::chamfer(r, 9.0), 1.4 + hot * 0.6, pal::alpha(c, a), true);
                let tc = pal::mix(pal::ICE, pal::WHITE, hot);
                text::draw_spaced(&self.g, Face::Display, &name, vec2(p.x, p.y + 9.0), 24.0, pal::alpha(tc, a), Align::Center, 6.0);
                if let Some((_, c)) = status {
                    draw_circle(r.x + 11.0, p.y, 3.0, pal::alpha(c, a * blink));
                }
            }
            // what's different about it: hint boxes tracing to each feature
            for (k, hn) in hints(&self.actors[i].p).iter().enumerate() {
                let (Some(at), Some(anchor)) = (self.rest_px(i, hn.at.extend(0.0)), self.anchor_px(i, hn.f)) else { continue };
                let ht = window(st, 0.7 + i as f32 * 0.25 + k as f32 * 0.1, 0.8);
                let hot = self.ui.hover(&format!("hint{i}{k}"), self.hint == Some((i, hn.f)));
                let r = ui::hint(&self.g, anchor, at, hn.out, hn.label, hot, !self.facing(i, hn.f), ht, t);
                if ht > 0.5 && r.contains(self.ui.mouse) {
                    under = Some((i, hn.f));
                }
            }
            // one line under it all: what it is, what's plugged in, or the call to action
            if let Some(p) = self.rest_px(i, vec3(0.0, -hh - 2.35, 0.0)) {
                let y = p.y.min(sz.y - 66.0);
                // half the screen each, less a margin
                let colw = sz.x / 2.0 - 60.0;
                if h {
                    text::draw_spaced_fit(&self.g, Face::Semi, &format!("[ {} ]", tr("ENGAGE")), vec2(p.x, y), 11.0, pal::MAGENTA, Align::Center, 3.0, colw);
                } else if let Some((txt, c)) = status {
                    let w = text::draw_fit(&self.g, Face::Mono, txt, vec2(p.x + 6.0, y), 11.0, pal::alpha(c, a), Align::Center, colw - 12.0);
                    draw_circle(p.x - w / 2.0 - 4.0, y - 4.0, 3.0, pal::alpha(c, a * blink));
                } else {
                    text::draw_fit(&self.g, Face::Mono, subs[i], vec2(p.x, y), 11.0, pal::alpha(pal::scale(pal::CYAN, 0.6), a), Align::Center, colw);
                }
            }
            if h {
                if let Some(r) = self.bbox(i) {
                    let pad = 18.0 + (t * 4.0).sin() * 3.0;
                    ui::brackets(Rect::new(r.x - pad, r.y - pad, r.w + pad * 2.0, r.h + pad * 2.0), 22.0, 2.0, pal::MAGENTA);
                }
            }
        }
        self.hint = under;
        // until someone spins a player, a ghost pointer shows how
        if let Some((i, f, vis)) = self.ghost() {
            if let Some(b) = self.bbox(i) {
                let p = b.center() + vec2(-80.0 + 160.0 * ease_in_out_cubic(f), b.h * 0.1);
                ghost_pointer(p, f > 0.0 && f < 1.0, vis, t);
                text::draw_spaced(&self.g, Face::Semi, tr("DRAG TO SPIN"), p + vec2(18.0, 38.0), 12.0, pal::alpha(pal::MAGENTA, vis), Align::Left, 2.5);
            }
        }
        let ha = window(st, 1.0, 0.8) * 0.8;
        // on the bottom row, between the prompt and the sound badge
        let (l, r) = self.bottom_free;
        // the demo switch, small at the right of it: a simulated player
        let bw = text::spaced_width(&self.g, Face::Mono, "DEMO", 11.0, 2.0) + 30.0;
        let br = Rect::new(r - bw, sz.y - 47.0, bw, 24.0);
        let over = ha > 0.5 && self.ui.over(br);
        let hot = self.ui.hover("demo", over);
        let on = self.opts.demo;
        let c = if on { pal::MAGENTA } else { pal::scale(pal::CYAN, 0.6) };
        ui::chip(br, c, hot, ha);
        // text with some green in it, so it holds together in the corner's colour split
        let tc = pal::mix(c, pal::ICE, if on { 0.45 } else { 0.3 + 0.5 * hot });
        text::draw_spaced(&self.g, Face::Mono, "DEMO", vec2(br.x + 21.0, sz.y - 30.0), 11.0, pal::alpha(tc, ha), Align::Left, 2.0);
        let blink = if on && (t * 2.0) as i32 % 2 == 0 { 1.0 } else { 0.45 };
        draw_circle(br.x + 12.0, sz.y - 34.0, 2.5, pal::alpha(tc, ha * if on { blink } else { 0.6 + 0.4 * hot }));
        if self.ui.click(over, Sfx::Click) {
            self.set_demo(!on);
        }
        let r = r - bw - 18.0;
        text::draw_spaced_fit(
            &self.g,
            Face::Mono,
            &format!("{}  //  {}", tr("CLICK TO ENGAGE"), tr("DRAG TO SPIN")),
            vec2((l + r) / 2.0, sz.y - 30.0),
            11.0,
            pal::alpha(pal::DIM, ha),
            Align::Center,
            2.0,
            (r - l).max(0.0),
        );
    }

    fn hud_link(&mut self) {
        let sz = self.g.size();
        let t = self.time;
        let st = self.screen_t;
        let kind = self.kind();
        let px = sz.x * 0.54;
        let pw = (sz.x * 0.42).min(560.0);
        let panel = Rect::new(px, 112.0, pw, sz.y - 230.0);
        let ap = window(st, 0.15, 0.5);
        ui::panel(panel, pal::CYAN, ap);
        ui::header(&self.g, vec2(panel.x + 26.0, panel.y + 30.0), panel.w - 52.0, tr("DOWNLOAD MODE"), pal::CYAN, ap, t);
        if self.to_stock {
            // where this link leads: straight to the restore
            let pulse = 0.75 + 0.25 * (t * 4.0).sin();
            text::draw_spaced(&self.g, Face::Semi, tr("RETURN TO STOCK"), vec2(panel.x + panel.w, panel.y - 12.0), 11.0, pal::alpha(pal::AMBER, ap * pulse), Align::Right, 2.0);
        }
        // steps
        let (steps, active, done_n) = self.link_plan();
        let pitch = if steps.len() > 4 { 54.0 } else { 60.0 };
        let mut y = panel.y + 74.0;
        for (k, s) in steps.iter().enumerate() {
            let a = window(st, 0.35 + k as f32 * 0.15, 0.5);
            let on = k == active;
            let done = k < done_n;
            let c = if on {
                pal::MAGENTA
            } else if done {
                pal::scale(pal::ACID, 0.8)
            } else {
                pal::scale(pal::CYAN, 0.6)
            };
            let row = Rect::new(panel.x + 26.0, y - 8.0, panel.w - 52.0, pitch - 10.0);
            if on {
                ui::fill_convex(&ui::chamfer(row, 8.0), pal::alpha(pal::MAGENTA, 0.08 * a));
                draw_rectangle(row.x, row.y, 3.0, row.h, pal::alpha(pal::MAGENTA, a));
            }
            text::draw(&self.g, Face::Display, &format!("{:02}", k + 1), vec2(row.x + 14.0, y + 22.0), 22.0, pal::alpha(c, a), Align::Left);
            let title = text::decode(s.title, a * 1.5, t, k as u32 + 40);
            text::draw_spaced_fit(
                &self.g,
                Face::Semi,
                &title,
                vec2(row.x + 62.0, y + 13.0),
                16.0,
                pal::alpha(if on { pal::WHITE } else { c }, a),
                Align::Left,
                3.0,
                row.w - 100.0,
            );
            text::draw_fit(
                &self.g,
                Face::Mono,
                &s.sub,
                vec2(row.x + 62.0, y + 31.0),
                12.0,
                pal::alpha(pal::scale(pal::ICE, 0.65), a),
                Align::Left,
                row.w - 76.0,
            );
            if on {
                if let Some(f) = s.feat {
                    if let Some(p) = self.anchor_px(self.sel, f) {
                        trace(p, vec2(row.x - 6.0, row.y + row.h / 2.0), pal::MAGENTA, window(st, 0.6, 0.6), t);
                    }
                }
            }
            if done {
                check(vec2(row.x + row.w - 22.0, y + 12.0), pal::ACID, a);
            }
            y += pitch;
        }
        // alternative route
        let menu;
        let alt = match kind {
            Kind::Yp3 => (tr("OR"), tr("hold DOWN and tap RESET"), Some(Feature::Reset)),
            Kind::Ts17 if self.from_os() => {
                // the player's debug menu entry is English in every language
                menu = tf("on the player: System > Debug > {0}", &[&"Reboot to USB download mode"]);
                (tr("OR"), menu.as_str(), None)
            }
            Kind::Ts17 => (tr("ROCKBOX?"), tr("switch it off, then hold POWER ~4 s while connecting"), Some(Feature::Power)),
        };
        let aa = window(st, 1.0, 0.6);
        text::draw_spaced(&self.g, Face::Semi, alt.0, vec2(panel.x + 40.0, y + 8.0), 10.0, pal::alpha(pal::AMBER, aa), Align::Left, 2.0);
        let w = text::spaced_width(&self.g, Face::Semi, alt.0, 10.0, 2.0);
        text::draw_fit(
            &self.g,
            Face::Mono,
            alt.1,
            vec2(panel.x + 52.0 + w, y + 8.0),
            12.0,
            pal::alpha(pal::scale(pal::AMBER, 0.8), aa),
            Align::Left,
            panel.w - 78.0 - w,
        );
        if Rect::new(panel.x + 26.0, y - 8.0, panel.w - 52.0, 22.0).contains(self.ui.mouse) {
            if let Some(p) = alt.2.and_then(|f| self.anchor_px(self.sel, f)) {
                trace(p, vec2(panel.x + 20.0, y + 4.0), pal::AMBER, 1.0, t);
            }
        }
        // status block
        let sy = (y + 40.0).max(panel.y + panel.h - 200.0);
        let sb = Rect::new(panel.x + 26.0, sy, panel.w - 52.0, panel.y + panel.h - sy - 20.0);
        self.link_status(sb, ap);
        // back
        if ui::button(
            &mut self.ui,
            &self.g,
            Rect::new(36.0, sz.y - 100.0, 130.0, 38.0),
            tr("BACK"),
            Tone::Secondary,
            window(st, 0.6, 0.4),
            true,
        ) {
            self.go(Screen::Select);
        }
    }

    fn link_status(&mut self, r: Rect, a: f32) {
        let t = self.time;
        let seen = self.seen();
        let g = &self.g;
        // radar
        let c = vec2(r.x + 46.0, r.y + 52.0);
        let rad = 38.0;
        let col = match seen.map(|s| s.mode) {
            Some(Mode::Rom) if self.link.probed_at.is_some() => pal::ACID,
            Some(Mode::Rom) => pal::CYAN,
            Some(_) => pal::AMBER,
            None => pal::scale(pal::CYAN, 0.8),
        };
        let col = if self.link.error.is_some() { pal::RED } else { col };
        ui::ring(c, rad, 1.2, pal::alpha(col, 0.6 * a), 48);
        ui::ring(c, rad * 0.62, 1.0, pal::alpha(col, 0.35 * a), 40);
        ui::ring(c, rad * 0.25, 1.0, pal::alpha(col, 0.35 * a), 24);
        let ang = t * 3.2;
        for k in 0..14 {
            let aa = ang - k as f32 * 0.06;
            ui::line(c, c + Vec2::from_angle(aa) * rad, 1.6, pal::alpha(col, (1.0 - k as f32 / 14.0) * 0.6 * a));
        }
        if seen.is_some() {
            let p = c + Vec2::from_angle(1.1) * rad * 0.55;
            let pulse = (t * 3.0) % 1.0;
            draw_circle(p.x, p.y, 3.5, pal::alpha(col, a));
            ui::ring(p, 4.0 + pulse * 12.0, 1.2, pal::alpha(col, (1.0 - pulse) * a), 20);
        }
        let (title, sub) = if let Some(e) = &self.link.error {
            (tr("LINK FAILED"), e.clone())
        } else {
            match seen {
                None => (tr("SCANNING"), tr("no player on the bus yet").to_string()),
                Some(s) if !s.access => (tr("ACCESS DENIED"), permission_hint().to_string()),
                Some(s) => match (s.mode, self.kind()) {
                    (Mode::Rom, _) if self.link.probed_at.is_some() => (tr("LINK ESTABLISHED"), tr("identity verified").to_string()),
                    (Mode::Rom, _) => (tr("PROBING"), tr("boot ROM online").to_string()),
                    (Mode::Stock, Kind::Ts17) => (tr("STOCK FIRMWARE"), tr("forcing download mode").to_string()),
                    (Mode::Stock, Kind::Yp3) => (tr("STOCK FIRMWARE"), tr("follow the steps above").to_string()),
                    (Mode::Rockbox, _) => (tr("ROCKBOX RUNNING"), tr("follow the steps above").to_string()),
                },
            }
        };
        let tx = r.x + 104.0;
        let big = if self.link.probed_at.is_some() { 0.3 * (1.0 - window(t - self.link.probed_at.unwrap_or(t), 0.0, 0.8)) } else { 0.0 };
        text::glitch_title(
            g,
            title,
            vec2(tx, r.y + 34.0),
            18.0,
            pal::alpha(pal::mix(col, pal::WHITE, 0.3), a),
            Align::Left,
            0.04 + big,
            t,
            3.0,
            r.w - 110.0,
        );
        let lines = ui::wrap(g, Face::Mono, &sub, 12.0, r.w - 110.0);
        for (k, l) in lines.iter().take(3).enumerate() {
            text::draw(
                g,
                Face::Mono,
                l,
                vec2(tx, r.y + 54.0 + k as f32 * 15.0),
                12.0,
                pal::alpha(pal::scale(pal::ICE, 0.75), a),
                Align::Left,
            );
        }
        // identity lines
        let mut y = r.y + 112.0;
        for (k, l) in self.link.lines.iter().enumerate() {
            if y > r.y + r.h {
                break;
            }
            let la = clamp01((t - l.t) * 4.0) * a;
            let c = match l.ok {
                Some(true) => pal::ACID,
                Some(false) => pal::RED,
                None => pal::ICE,
            };
            text::draw(g, Face::Mono, &l.key, vec2(r.x + 4.0, y), 12.0, pal::alpha(pal::DIM, la), Align::Left);
            let v = text::decode(&l.val, clamp01((t - l.t) * 3.0), t, k as u32);
            text::draw_fit(g, Face::Mono, &v, vec2(r.x + 84.0, y), 12.0, pal::alpha(c, la), Align::Left, r.w - 110.0);
            match l.ok {
                Some(true) => check(vec2(r.x + r.w - 14.0, y - 4.0), pal::ACID, la),
                Some(false) => cross(vec2(r.x + r.w - 14.0, y - 4.0), pal::RED, la),
                None => {}
            }
            y += 16.0;
        }
        if let Some(e) = self.link.error.clone() {
            let cw = (r.w - 160.0).min(190.0);
            self.copy_button(Rect::new(r.x + r.w - 160.0 - cw, r.y + r.h - 44.0, cw, 38.0), &e, a);
            if ui::button(&mut self.ui, &self.g, Rect::new(r.x + r.w - 150.0, r.y + r.h - 44.0, 150.0, 38.0), tr("RETRY"), Tone::Primary, a, true) {
                self.link = LinkState::default();
            }
        }
    }

    fn hud_payload(&mut self) {
        let sz = self.g.size();
        let t = self.time;
        let st = self.screen_t;
        let px = sz.x * 0.54;
        let pw = (sz.x * 0.42).min(560.0);
        let panel = Rect::new(px, 112.0, pw, sz.y - 230.0);
        let ap = window(st, 0.1, 0.5);
        ui::panel(panel, pal::MAGENTA, ap);
        ui::header(&self.g, vec2(panel.x + 26.0, panel.y + 30.0), panel.w - 52.0, tr("PAYLOAD"), pal::MAGENTA, ap, t);
        // device summary
        let ident: Vec<String> = self
            .link
            .lines
            .iter()
            .filter(|l| matches!(l.key.as_str(), "CPU" | "FLASH" | "NOR" | "CELL"))
            .map(|l| l.val.split(" (").next().unwrap_or("").split("  ").next().unwrap_or("").to_string())
            .collect();
        // which player this is for, big: it decides the package
        let name = self.kind().name();
        let nw = text::draw_spaced(&self.g, Face::Display, name, vec2(panel.x + 26.0, panel.y + 74.0), 26.0, pal::alpha(pal::ICE, ap), Align::Left, 6.0);
        text::draw_fit(
            &self.g,
            Face::Mono,
            &ident.join("  \u{b7}  "),
            vec2(panel.x + 26.0 + nw + 18.0, panel.y + 72.0),
            11.0,
            pal::alpha(pal::scale(pal::ACID, 0.85), ap),
            Align::Left,
            panel.w - 70.0 - nw,
        );

        let inner = Rect::new(panel.x + 26.0, panel.y + 94.0, panel.w - 52.0, panel.h - 190.0);
        let mut flash_ok = false;
        let mut pick = false;
        match &self.pkg {
            None => {
                // drop zone
                let a = window(st, 0.3, 0.5);
                let dz = inner;
                dashed(&ui::chamfer(dz, 14.0), pal::alpha(pal::scale(pal::MAGENTA, 0.7), a), t);
                let cx = dz.x + dz.w / 2.0;
                let cy = dz.y + dz.h / 2.0;
                // a little archive glyph
                let gy = cy - 70.0 + (t * 2.0).sin() * 3.0;
                let box_ = Rect::new(cx - 22.0, gy - 26.0, 44.0, 52.0);
                ui::polyline(&ui::chamfer(box_, 12.0), 1.5, pal::alpha(pal::MAGENTA, a), true);
                for k in 0..4 {
                    ui::line(
                        vec2(cx - 12.0, gy - 10.0 + k as f32 * 8.0),
                        vec2(cx + 12.0, gy - 10.0 + k as f32 * 8.0),
                        1.0,
                        pal::alpha(pal::scale(pal::MAGENTA, 0.6), a),
                    );
                }
                let hint = if self.opts.demo {
                    self.demo_package().display().to_string()
                } else {
                    format!("rockbox-{}-*.tar.xz", self.kind().name().to_lowercase())
                };
                text::draw_fit(
                    &self.g,
                    Face::Semi,
                    &tf("THE {0} PACKAGE", &[&self.kind().name()]),
                    vec2(cx, cy - 30.0),
                    13.0,
                    pal::alpha(pal::ICE, a),
                    Align::Center,
                    dz.w - 40.0,
                );
                text::draw_fit(&self.g, Face::Mono, &hint, vec2(cx, cy - 12.0), 11.0, pal::alpha(pal::scale(pal::ICE, 0.7), a), Align::Center, dz.w - 40.0);
                pick = ui::button(
                    &mut self.ui,
                    &self.g,
                    Rect::new(cx - 130.0, cy + 4.0, 260.0, 52.0),
                    tr("SELECT IMAGE"),
                    Tone::Primary,
                    a,
                    self.pick_rx.is_none(),
                );
                text::draw_fit(&self.g, Face::Mono, tr("or drop it here"), vec2(cx, cy + 82.0), 11.0, pal::alpha(pal::DIM, a), Align::Center, dz.w - 40.0);
            }
            Some(p) => {
                let a = clamp01((t - p.t) * 3.0) * ap;
                let mut y = inner.y + 14.0;
                // the key column is as wide as its longest label
                let keys = [tr("IMAGE"), "SHA-256", tr("TARGET"), tr("BUILD"), tr("FLASH"), tr("CARD"), tr("BACKUP")];
                let kw = key_column(&self.g, &keys, 11.0, 90.0, inner.w * 0.4);
                text::draw_fit(&self.g, Face::Mono, tr("IMAGE"), vec2(inner.x, y), 11.0, pal::alpha(pal::DIM, a), Align::Left, kw - 8.0);
                text::draw_fit(
                    &self.g,
                    Face::Mono,
                    &p.name,
                    vec2(inner.x + kw, y),
                    13.0,
                    pal::alpha(pal::ICE, a),
                    Align::Left,
                    inner.w - kw - 112.0,
                );
                y += 26.0;
                match &p.state {
                    PkgState::Checking(f, s) => {
                        text::draw(&self.g, Face::Mono, "SHA-256", vec2(inner.x, y), 11.0, pal::alpha(pal::DIM, a), Align::Left);
                        ui::segbar(Rect::new(inner.x + kw, y - 10.0, inner.w - kw - 60.0, 10.0), *f, pal::CYAN, a);
                        text::draw(
                            &self.g,
                            Face::Mono,
                            &format!("{:3.0}%", f * 100.0),
                            vec2(inner.x + inner.w, y),
                            12.0,
                            pal::alpha(pal::CYAN, a),
                            Align::Right,
                        );
                        y += 20.0;
                        text::draw_fit(
                            &self.g,
                            Face::Mono,
                            s,
                            vec2(inner.x + kw, y),
                            11.0,
                            pal::alpha(pal::scale(pal::CYAN, 0.6), a),
                            Align::Left,
                            inner.w - kw,
                        );
                    }
                    PkgState::Bad(e) => {
                        text::glitch_title(&self.g, tr("REJECTED"), vec2(inner.x, y + 8.0), 20.0, pal::RED, Align::Left, 0.15, t, 4.0, inner.w);
                        y += 30.0;
                        for (k, l) in ui::wrap(&self.g, Face::Mono, e, 12.0, inner.w).iter().take(5).enumerate() {
                            text::draw(
                                &self.g,
                                Face::Mono,
                                l,
                                vec2(inner.x, y + k as f32 * 16.0),
                                12.0,
                                pal::alpha(pal::scale(pal::RED, 0.9), a),
                                Align::Left,
                            );
                        }
                    }
                    PkgState::Ok => {
                        flash_ok = true;
                        let rows = [
                            (tr("TARGET"), p.target.clone(), true),
                            (tr("BUILD"), if p.version.is_empty() { "-".to_string() } else { p.version.clone() }, false),
                            (tr("FLASH"), mib(p.flash), false),
                            (tr("CARD"), tf("{0} files  \u{b7}  {1}", &[&p.files, &mib(p.card)]), false),
                            ("SHA-256", tr("every member verified").to_string(), true),
                            (tr("BACKUP"), tr("stock flash saved first").to_string(), true),
                        ];
                        for (k, (key, val, ok)) in rows.iter().enumerate() {
                            let ra = clamp01((t - p.t - 0.1 * k as f32) * 4.0) * ap;
                            text::draw_fit(&self.g, Face::Mono, key, vec2(inner.x, y), 11.0, pal::alpha(pal::DIM, ra), Align::Left, kw - 8.0);
                            let v = text::decode(val, ra, t, k as u32 + 70);
                            text::draw_fit(
                                &self.g,
                                Face::Mono,
                                &v,
                                vec2(inner.x + kw, y),
                                13.0,
                                pal::alpha(pal::ICE, ra),
                                Align::Left,
                                inner.w - kw - 22.0,
                            );
                            if *ok {
                                check(vec2(inner.x + inner.w - 10.0, y - 4.0), pal::ACID, ra);
                            }
                            y += 24.0;
                        }
                    }
                }
                let busy = matches!(p.state, PkgState::Checking(..));
                if !busy
                    && ui::button(
                        &mut self.ui,
                        &self.g,
                        Rect::new(inner.x + inner.w - 104.0, inner.y - 6.0, 104.0, 28.0),
                        tr("CHANGE"),
                        Tone::Secondary,
                        a,
                        self.pick_rx.is_none(),
                    )
                {
                    pick = true;
                }
                if flash_ok {
                    let lr = Rect::new(inner.x, y + 18.0, inner.w, 46.0);
                    if lr.y + lr.h < inner.y + inner.h + 40.0 {
                        layout_bar(&self.g, lr, self.kind(), &p.segs, clamp01((t - p.t - 0.6) * 2.0) * ap, t);
                    }
                }
            }
        }
        if pick {
            self.pick_file();
        }
        // FLASH
        let fb = Rect::new(panel.x + 26.0, panel.y + panel.h - 82.0, panel.w - 52.0, 58.0);
        let fa = if flash_ok { window(st, 0.2, 0.5) } else { 0.0 };
        if flash_ok && ui::button(&mut self.ui, &self.g, fb, tr("FLASH"), Tone::Primary, fa, true) {
            self.start_flash();
        }
        if !flash_ok {
            ui::panel(fb, pal::DIM, 0.35 * ap);
            text::draw_spaced_fit(
                &self.g,
                Face::Semi,
                tr("FLASH"),
                vec2(fb.x + fb.w / 2.0, fb.y + fb.h / 2.0 + 6.0),
                18.0,
                pal::alpha(pal::DIM, ap),
                Align::Center,
                4.0,
                fb.w - 40.0,
            );
        }
        // the card reminder, traced to the slot
        if let Some(p) = self.anchor_px(self.sel, Feature::Sd) {
            let la = window(st, 0.7, 0.8);
            let at = vec2((p.x - 40.0).max(190.0), p.y + 110.0);
            let at = if at.x - 170.0 < 30.0 { vec2(p.x + 60.0, p.y + 110.0) } else { at };
            ui::callout(&self.g, p, at, tr("NEEDS A MICROSD CARD"), pal::ACID, la, t);
        }
        if ui::button(
            &mut self.ui,
            &self.g,
            Rect::new(36.0, sz.y - 100.0, 130.0, 38.0),
            tr("BACK"),
            Tone::Secondary,
            window(st, 0.4, 0.4),
            true,
        ) {
            self.go(Screen::Select);
        }
        if self.stock.is_some() {
            let label = tr("RETURN TO STOCK");
            let w = ui::button_width(&self.g, label, 38.0).clamp(200.0, panel.w - 150.0);
            if ui::button(
                &mut self.ui,
                &self.g,
                Rect::new(panel.x + panel.w - w, panel.y + panel.h + 22.0, w, 38.0),
                label,
                Tone::Secondary,
                window(st, 0.6, 0.4),
                true,
            ) {
                self.start_restore();
            }
        }
    }

    fn hud_execute(&mut self) {
        let sz = self.g.size();
        let t = self.time;
        let st = self.screen_t;
        let job = &self.job;
        let x0 = sz.x * 0.5;
        let w = sz.x - x0 - 40.0;
        // phase strip
        let pa = window(st, 0.1, 0.6);
        ui::phase_bar(
            &self.g,
            Rect::new(x0 - 60.0, 92.0, w + 60.0, 28.0),
            job.phases(),
            job.phase,
            job.progress,
            job.error.is_some(),
            pa,
            t,
        );
        // sector map, or the card while files go onto it
        let mh = (sz.y * 0.33).min(280.0);
        let mr = Rect::new(x0, 152.0, w, mh);
        let ma = window(st, 0.25, 0.5);
        let on_card = job.sync.started.is_some() || (job.phase == Some(Phase::Sync) && job.need_card);
        if on_card {
            self.sd_panel(mr, ma);
        } else {
            ui::panel(mr, pal::CYAN, ma);
            let size_txt = if job.sector_size > 0 {
                format!(
                    "{}  //  {}  //  {} x {}K",
                    tr("FLASH MAP"),
                    mib(job.sectors.len() as u64 * job.sector_size as u64),
                    job.sectors.len(),
                    job.sector_size / 1024
                )
            } else {
                tr("FLASH MAP").to_string()
            };
            ui::header(&self.g, vec2(mr.x + 22.0, mr.y + 26.0), mr.w - 44.0, &size_txt, pal::CYAN, ma, t);
            let grid = Rect::new(mr.x + 22.0, mr.y + 40.0, mr.w - 44.0, mr.h - 72.0);
            let hov = ui::sector_map(grid, &job.sectors, &job.heat, ma, t);
            // legend
            let legend = [Sect::Protected, Sect::Same, Sect::Read, Sect::Erase, Sect::Good, Sect::Bad];
            // the legend shrinks to leave room for the hovered sector's address
            let lsz = {
                let natural: f32 = legend.iter().map(|s| text::measure(&self.g, Face::Mono, sect_label(*s), 10.0).x + 22.0).sum();
                10.0 * ((mr.w - 44.0 - 150.0) / natural).clamp(0.7, 1.0)
            };
            let mut lx = mr.x + 22.0;
            for s in legend {
                draw_rectangle(lx, mr.y + mr.h - 22.0, 8.0, 8.0, pal::alpha(ui::sect_color(s), ma));
                lx += 14.0
                    + text::draw(&self.g, Face::Mono, sect_label(s), vec2(lx + 12.0, mr.y + mr.h - 14.0), lsz, pal::alpha(pal::DIM, ma), Align::Left)
                    + 8.0;
            }
            if let Some(i) = hov {
                let s = job.sectors.get(i).copied().unwrap_or(Sect::Idle);
                text::draw(
                    &self.g,
                    Face::Mono,
                    &format!("0x{:06X}  {}", i as u32 * job.sector_size, sect_label(s)),
                    vec2(mr.x + mr.w - 22.0, mr.y + mr.h - 14.0),
                    11.0,
                    pal::ICE,
                    Align::Right,
                );
            }
        }
        // hex / file stream, and the terminal under it, down to just above
        // the bottom row's chips (SND's top is 47 px up)
        let foot = sz.y - 56.0;
        let hy = mr.y + mr.h + 22.0;
        let hh = ((foot - hy) * 0.5).max(110.0);
        let hr = Rect::new(x0, hy, w, hh);
        let ha = window(st, 0.4, 0.5);
        let syncing = matches!(job.phase, Some(Phase::Sync | Phase::Eject));
        ui::panel(hr, pal::MAGENTA, ha);
        ui::header(
            &self.g,
            vec2(hr.x + 22.0, hr.y + 26.0),
            hr.w - 44.0,
            &if syncing { format!("SD:// {}", tr("FILES")) } else { tr("BUS TAP").to_string() },
            pal::MAGENTA,
            ha,
            t,
        );
        let inner = Rect::new(hr.x + 22.0, hr.y + 36.0, hr.w - 44.0, hr.h - 44.0);
        if syncing {
            let rows = (inner.h / 15.0) as usize;
            let start = job.files.len().saturating_sub(rows);
            for (k, (n, s, ft, rb)) in job.files[start..].iter().enumerate() {
                let y = inner.y + 12.0 + k as f32 * 15.0;
                let fa = clamp01((t - ft) * 6.0) * ha;
                let newest = start + k + 1 == job.files.len();
                let c = if newest {
                    pal::WHITE
                } else if *rb {
                    pal::scale(pal::ACID, 0.85)
                } else {
                    pal::mix(ext_color(n), pal::ICE, 0.35)
                };
                if *rb {
                    check(vec2(inner.x + 5.0, y - 4.0), pal::ACID, fa);
                }
                let nx = if *rb { inner.x + 18.0 } else { inner.x };
                text::draw(&self.g, Face::Mono, &shorten(n, 58), vec2(nx, y), 12.0, pal::alpha(c, fa), Align::Left);
                text::draw(&self.g, Face::Mono, &mib(*s), vec2(inner.x + inner.w, y), 12.0, pal::alpha(pal::DIM, fa), Align::Right);
            }
            self.file_flights(inner, start, ha);
        } else {
            ui::hexdump(&self.g, inner, &job.hex, ha, pal::MAGENTA);
        }
        // terminal
        let ty = hr.y + hr.h + 22.0;
        let tap = Rect::new(x0, ty, w, foot - ty);
        let ta = window(st, 0.55, 0.5);
        ui::panel(tap, pal::VIOLET, ta);
        ui::terminal(&self.g, Rect::new(tap.x + 18.0, tap.y + 8.0, tap.w - 36.0, tap.h - 12.0), &self.log, t, ta);
        // big counter
        let pct = (job.shown * 100.0).floor().min(100.0);
        let cx = sz.x * 0.25;
        let cy = sz.y - 120.0;
        let ca = window(st, 0.2, 0.6);
        let col = if job.error.is_some() {
            pal::RED
        } else if job.need_card {
            pal::AMBER
        } else {
            pal::ICE
        };
        let plate = Rect::new(cx - 210.0, cy - 74.0, 420.0, 132.0);
        ui::fill_convex(&ui::chamfer(plate, 18.0), Color::new(0.0, 0.0, 0.02, 0.62 * ca));
        ui::brackets(plate, 12.0, 1.2, pal::alpha(pal::scale(col, 0.5), ca));
        let num = format!("{pct:.0}");
        let nw = text::measure(&self.g, Face::Display, &num, 78.0).x + num.len() as f32 * 4.0;
        text::glitch_title(&self.g, &num, vec2(cx - 12.0, cy), 78.0, pal::alpha(col, ca), Align::Center, 0.05 + self.kick * 0.3, t, 4.0, plate.w);
        text::draw(&self.g, Face::Display, "%", vec2(cx - 12.0 + nw / 2.0 + 8.0, cy), 30.0, pal::alpha(pal::MAGENTA, ca), Align::Left);
        let label = tr(job.phase.map(|p| p.label()).unwrap_or("LINK"));
        let el = job.ended.unwrap_or(t) - job.started;
        let sub = format!("{}  //  {:02}:{:05.2}", label, (el / 60.0) as u32, el % 60.0);
        text::draw_spaced_fit(&self.g, Face::Semi, &sub, vec2(cx, cy + 30.0), 12.0, pal::alpha(pal::CYAN, ca), Align::Center, 3.0, plate.w - 40.0);
        ui::segbar(Rect::new(cx - 160.0, cy + 44.0, 320.0, 6.0), job.shown, if job.need_card { pal::AMBER } else { pal::ACID }, ca);
        // need card
        if job.need_card {
            let na = ease_out_cubic(clamp01(st.min(9.0)));
            let r = Rect::new(cx - 190.0, 150.0, 380.0, 92.0);
            ui::panel(r, pal::AMBER, na);
            text::glitch_title(&self.g, tr("INSERT SD CARD"), vec2(r.x + r.w / 2.0, r.y + 44.0), 26.0, pal::AMBER, Align::Center, 0.1, t, 5.0, r.w - 30.0);
            text::draw_spaced_fit(
                &self.g,
                Face::Semi,
                tr("THEN PRESS RESET"),
                vec2(r.x + r.w / 2.0, r.y + 72.0),
                12.0,
                pal::ICE,
                Align::Center,
                3.0,
                r.w - 30.0,
            );
            if let Some(p) = self.anchor_px(self.sel, Feature::Sd) {
                trace(p, vec2(r.x + 40.0, r.y + r.h), pal::AMBER, 1.0, t);
            }
            if let Some(p) = self.anchor_px(self.sel, Feature::Reset) {
                trace(p, vec2(r.x + r.w - 40.0, r.y + r.h), pal::AMBER, 1.0, t);
            }
        }
    }

    /// Files in flight between the list and the player's card slot: each
    /// one written flies from its line in the list into the slot; each one
    /// read back flies out of the slot to its line, to be ticked off.
    fn file_flights(&self, list: Rect, start: usize, a: f32) {
        let Some(slot) = self.anchor_px(self.sel, Feature::Sd) else { return };
        let g = &self.g;
        let t = self.time;
        let files = &self.job.files;
        let fly = 0.7;
        let land = 0.45;
        for (idx, (n, _, ft, rb)) in files.iter().enumerate().rev().take(24) {
            let age = t - ft;
            if age < 0.0 || age > fly + land {
                continue;
            }
            let row = vec2(list.x - 8.0, list.y + 8.0 + idx.saturating_sub(start) as f32 * 15.0);
            let (from, to) = if *rb { (slot, row) } else { (row, slot) };
            let c = if *rb { pal::ACID } else { ext_color(n) };
            if age < fly {
                let ctrl = vec2((from.x + to.x) / 2.0, from.y.min(to.y) - 110.0);
                let at = |u: f32| {
                    let v = 1.0 - u;
                    from * v * v + ctrl * 2.0 * v * u + to * u * u
                };
                let u = ease_in_out_cubic(age / fly);
                for k in 1..5 {
                    let p = at((u - k as f32 * 0.035).max(0.0));
                    draw_circle(p.x, p.y, 2.0 - k as f32 * 0.3, pal::alpha(c, (0.5 - k as f32 * 0.1) * a));
                }
                let p = at(u);
                let label = n.rsplit('.').next().unwrap_or("").to_uppercase();
                // small at the slot, full size at the list
                let sc = if *rb { 0.55 + 0.45 * u } else { 1.0 - 0.45 * u };
                let fade = 1.0 - clamp01((u - 0.88) / 0.12);
                let tw = text::measure(g, Face::Semi, &label, 8.0 * sc).x;
                let pr = Rect::new(p.x - tw / 2.0 - 5.0 * sc, p.y - 7.0 * sc, tw + 10.0 * sc, 14.0 * sc);
                ui::fill_convex(&ui::chamfer(pr, 3.0 * sc), Color::new(0.0, 0.02, 0.04, 0.85 * fade * a));
                ui::polyline(&ui::chamfer(pr, 3.0 * sc), 1.0, pal::alpha(c, fade * a), true);
                text::draw(g, Face::Semi, &label, vec2(p.x, p.y + 3.0 * sc), 8.0 * sc, pal::alpha(pal::mix(c, pal::WHITE, 0.4), fade * a), Align::Center);
            } else {
                // it lands: a ring, and a few sparks
                let s = (age - fly) / land;
                let lc = pal::alpha(pal::mix(if *rb { pal::ACID } else { pal::TEAL }, pal::WHITE, 0.5), (1.0 - s) * a);
                draw_circle_lines(to.x, to.y, 3.0 + s * 14.0, 1.2, lc);
                for d in 0..4 {
                    let ang = d as f32 * 1.57 + (idx % 7) as f32;
                    let p = to + Vec2::from_angle(ang) * (4.0 + s * 20.0);
                    draw_circle(p.x, p.y, 1.3, lc);
                }
            }
        }
    }

    /// The SD sync as the player's card, magnified, filling up: its level
    /// rises with the bytes; then a beam sweeps it while every file is read
    /// back and hashed, and it turns green from the bottom up.
    fn sd_panel(&self, r: Rect, a: f32) {
        let g = &self.g;
        let t = self.time;
        let job = &self.job;
        let sv = &job.sync;
        let (files, bytes) = self.pkg.as_ref().map(|p| (p.files, p.card)).unwrap_or((0, 0));
        let files = files.max(sv.written);
        let done = matches!(job.phase, Some(Phase::Eject | Phase::Done));
        let verified = if done {
            1.0
        } else if sv.readback {
            clamp01((job.progress - 0.7) / 0.3)
        } else {
            0.0
        };
        let stage = if job.need_card {
            tr("NO CARD").to_string()
        } else if done {
            tr("EJECT").to_string()
        } else if sv.readback {
            format!("{}  //  SHA-256", tr("READ BACK"))
        } else if sv.started.is_some() {
            tr("WRITE").to_string()
        } else {
            tr("MOUNT").to_string()
        };
        ui::panel(r, pal::TEAL, a);
        ui::header(g, vec2(r.x + 22.0, r.y + 26.0), r.w - 44.0, &format!("{}  //  {stage}", tr("SD CARD")), pal::TEAL, a, t);

        // the card, a microSD seen from its contact side: 11 x 15 mm, the
        // contacts on the narrow end at the top, a step out to full width
        let ch = (r.h - 62.0).min(200.0);
        let cw = ch * 11.0 / 15.0;
        let cr = Rect::new(r.x + 32.0, r.y + 44.0, cw, ch);
        let narrow = cr.x + cw * 0.87;
        let (step0, step1) = (cr.y + ch * 0.33, cr.y + ch * 0.41);
        let outline = [
            vec2(cr.x, cr.y),
            vec2(narrow, cr.y),
            vec2(narrow, step0),
            vec2(cr.x + cw, step1),
            vec2(cr.x + cw, cr.y + ch),
            vec2(cr.x, cr.y + ch),
        ];
        let pulse = (-(t - sv.last) * 7.0).exp();
        let dark = Color::new(0.0, 0.025, 0.035, 0.75 * a);
        ui::fill_convex(&[outline[0], outline[1], outline[2], vec2(cr.x, step0)], dark);
        ui::fill_convex(&[vec2(cr.x, step0), outline[2], outline[3], outline[4], outline[5]], dark);
        let edge = if job.need_card {
            pal::alpha(pal::AMBER, a * (0.35 + 0.65 * (0.5 + 0.5 * (t * 4.0).sin())))
        } else {
            pal::alpha(pal::mix(pal::TEAL, pal::WHITE, 0.15 + 0.4 * pulse), a)
        };
        ui::polyline(&outline, 1.6, edge, true);
        let pins = 8;
        let pw = (narrow - cr.x - 12.0) / pins as f32;
        for k in 0..pins {
            let lit = pulse * (0.55 + 0.45 * (k as f32 * 1.7 + t * 23.0).sin().abs());
            let c = pal::mix(pal::scale(pal::TEAL, 0.4), pal::WHITE, lit);
            draw_rectangle(cr.x + 8.0 + k as f32 * pw, cr.y + 7.0, pw * 0.62, ch * 0.17, pal::alpha(c, a * (0.55 + 0.45 * lit)));
        }
        // grip ridge at the far end
        ui::line(vec2(cr.x + 6.0, cr.y + ch - 7.0), vec2(cr.x + cw - 6.0, cr.y + ch - 7.0), 1.0, pal::alpha(pal::TEAL, 0.4 * a));

        // its contents, as a liquid level with a wavy top; the strip right
        // of the narrow part only fills below the step
        let fi = Rect::new(cr.x + 8.0, cr.y + ch * 0.25, cw - 16.0, ch * 0.75 - 12.0);
        let bottom = fi.y + fi.h;
        let surf = bottom - fi.h * sv.level;
        let amp = if sv.level > 0.002 && sv.level < 0.995 { 1.5 + 3.5 * pulse } else { 0.0 };
        let ceiling = |x: f32| if x > narrow - 7.0 { step1 + 6.0 } else { fi.y };
        let wave = |x: f32| (surf + amp * ((x - fi.x) * 0.09 + t * 3.1).sin()).max(ceiling(x));
        let cols = 30;
        let colw = fi.w / cols as f32;
        let vtop = bottom - (bottom - surf) * verified;
        for k in 0..cols {
            let x = fi.x + k as f32 * colw;
            let top = wave(x + colw).max(wave(x));
            if top < bottom {
                draw_rectangle(x, top, colw + 0.4, bottom - top, pal::alpha(pal::TEAL, 0.2 * a));
                let vt = vtop.max(top);
                if verified > 0.0 && vt < bottom {
                    draw_rectangle(x, vt, colw + 0.4, bottom - vt, pal::alpha(pal::ACID, 0.26 * a));
                }
            }
        }
        let mut y = bottom - 3.0;
        while y > surf + amp + 1.0 {
            let right = if y < step1 + 6.0 { narrow - 7.0 } else { fi.x + fi.w };
            ui::line(vec2(fi.x, y), vec2(right, y), 1.0, Color::new(0.0, 0.0, 0.02, 0.3 * a));
            y -= 4.0;
        }
        if sv.level > 0.002 {
            let pts: Vec<Vec2> = (0..=cols).map(|k| fi.x + k as f32 * colw).map(|x| vec2(x, wave(x))).collect();
            let sc = if verified >= 1.0 { pal::ACID } else { pal::mix(pal::TEAL, pal::WHITE, 0.2 + 0.5 * pulse) };
            ui::polyline(&pts, 1.6, pal::alpha(sc, a), false);
        }
        // bits rising through it
        if bottom - surf > 12.0 {
            for k in 0..16u32 {
                let h = hash32(k * 7919 + 13);
                let x = fi.x + 5.0 + (h % 1000) as f32 / 1000.0 * (narrow - fi.x - 14.0);
                let speed = 12.0 + ((h >> 10) % 30) as f32;
                let y = bottom - 2.0 - (t * speed + ((h >> 4) % 200) as f32) % (bottom - surf - 2.0);
                let fade = clamp01((y - surf) / 16.0);
                let bit = if (h >> 3) & 1 == 0 { "0" } else { "1" };
                text::draw(
                    g,
                    Face::Mono,
                    bit,
                    vec2(x, y),
                    9.0,
                    pal::alpha(pal::mix(pal::TEAL, pal::ACID, verified), 0.5 * fade * a),
                    Align::Center,
                );
            }
        }
        // read back: a beam sweeps the card
        if sv.readback && !done {
            let k = (t * 0.9) % 2.0;
            let y = cr.y + 4.0 + (ch - 8.0) * if k > 1.0 { 2.0 - k } else { k };
            let right = if y < step0 {
                narrow
            } else if y < step1 {
                narrow + (cr.x + cw - narrow) * (y - step0) / (step1 - step0)
            } else {
                cr.x + cw
            };
            ui::line(vec2(cr.x + 3.0, y), vec2(right - 3.0, y), 7.0, pal::alpha(pal::ACID, 0.15 * a));
            ui::line(vec2(cr.x + 3.0, y), vec2(right - 3.0, y), 1.8, pal::alpha(pal::ACID, 0.9 * a));
        }

        // it's the player's card, magnified: lines back to its slot
        if let Some(slot) = self.anchor_px(self.sel, Feature::Sd) {
            let c = pal::alpha(pal::TEAL, (0.18 + 0.3 * pulse) * a);
            for corner in [vec2(cr.x, cr.y), vec2(cr.x, cr.y + ch)] {
                let n = ((corner - slot).length() / 9.0) as usize;
                for k in (0..n).step_by(2) {
                    let (u0, u1) = (k as f32 / n as f32, (k + 1) as f32 / n as f32);
                    ui::line(slot.lerp(corner, u0), slot.lerp(corner, u1), 1.0, c);
                }
            }
            draw_circle_lines(slot.x, slot.y, 6.0 + 3.0 * pulse, 1.0, c);
        }

        // numbers
        let sx = cr.x + cw + 40.0;
        let sw = r.x + r.w - 24.0 - sx;
        let rate = sv.rate.last().copied().unwrap_or(0.0);
        let eta = match sv.started {
            _ if done => "00:00".to_string(),
            Some(t0) if job.progress > 0.02 => {
                let e = (t - t0) * (1.0 - job.progress) / job.progress;
                format!("{:02}:{:02}", (e / 60.0) as u32, e as u32 % 60)
            }
            _ => "--:--".to_string(),
        };
        let shown_files = if sv.readback || done { (verified * files as f32).round() as usize } else { sv.written };
        let rows = [
            (if sv.readback || done { tr("CHECKED") } else { tr("FILES") }, format!("{shown_files} / {files}")),
            (tr("DATA"), format!("{} / {}", mib((bytes as f32 * sv.level) as u64), mib(bytes))),
            (tr("RATE"), if sv.rate.is_empty() { "--".to_string() } else { rate_txt(rate) }),
            (tr("ETA"), eta),
        ];
        let kw = key_column(g, &[tr("CHECKED"), tr("FILES"), tr("DATA"), tr("RATE"), tr("ETA")], 10.0, 74.0, sw * 0.4);
        let mut y = r.y + 66.0;
        for (k, (l, v)) in rows.iter().enumerate() {
            text::draw_fit(g, Face::Mono, l, vec2(sx, y), 10.0, pal::alpha(pal::DIM, a), Align::Left, kw - 8.0);
            let c = if k == 0 && (sv.readback || done) { pal::ACID } else { pal::ICE };
            text::draw_spaced_fit(g, Face::Semi, v, vec2(sx + kw, y + 1.0), 14.0, pal::alpha(c, a), Align::Left, 1.5, sw - kw);
            y += 30.0;
        }
        // rate history beside RATE
        if sv.rate.len() > 1 {
            let hr = Rect::new(sx + kw + 126.0, r.y + 66.0 + 2.0 * 30.0 - 16.0, (sw - kw - 126.0).max(40.0), 20.0);
            let top = sv.rate.iter().copied().fold(1.0f32, f32::max);
            let bw = hr.w / 48.0;
            for (k, v) in sv.rate.iter().enumerate() {
                let h = (v / top).max(0.04) * hr.h;
                let x = hr.x + hr.w - (sv.rate.len() - k) as f32 * bw;
                draw_rectangle(x, hr.y + hr.h - h, bw * 0.7, h, pal::alpha(pal::TEAL, (0.35 + 0.65 * k as f32 / sv.rate.len() as f32) * a));
            }
        }
        // folders, each flashing as a file lands in it
        let mut x = sx;
        let mut y = r.y + r.h - 52.0;
        for (d, n, hit) in &sv.dirs {
            let txt = format!("{} {}", d.to_uppercase(), n);
            let w = text::measure(g, Face::Mono, &txt, 10.0).x + 14.0;
            if x + w > r.x + r.w - 22.0 {
                x = sx;
                y += 22.0;
                if y > r.y + r.h - 18.0 {
                    break;
                }
            }
            let fl = (-(t - hit) * 5.0).exp();
            let cr = Rect::new(x, y, w, 17.0);
            ui::fill_convex(&ui::chamfer(cr, 4.0), pal::alpha(pal::TEAL, 0.25 * fl * a));
            ui::polyline(&ui::chamfer(cr, 4.0), 1.0, pal::alpha(pal::mix(pal::scale(pal::TEAL, 0.5), pal::WHITE, fl * 0.6), a), true);
            text::draw(
                g,
                Face::Mono,
                &txt,
                vec2(x + 7.0, y + 12.5),
                10.0,
                pal::alpha(pal::mix(pal::scale(pal::TEAL, 0.9), pal::WHITE, fl), a),
                Align::Left,
            );
            x += w + 6.0;
        }
    }

    fn hud_complete(&mut self) {
        let sz = self.g.size();
        let t = self.time;
        let st = self.screen_t;
        let restored = self.job.restoring;
        let x0 = sz.x * 0.5;
        let w = (sz.x - x0 - 40.0).min(600.0);
        let ta = window(st, 0.0, 0.6);
        let title = if restored { tr("STOCK RESTORED") } else { tr("ACCESS GRANTED") };
        let tt = text::decode(title, ta * 1.3, t, 21);
        let ts = (w / 14.0).clamp(26.0, 46.0);
        let ty = 190.0;
        text::glitch_title(
            &self.g,
            &tt,
            vec2(x0, ty),
            ts,
            pal::ACID,
            Align::Left,
            0.06 + 0.5 * (1.0 - window(st, 0.0, 1.2)),
            t,
            ts * 0.18,
            sz.x - x0 - 40.0,
        );
        let sub = if restored { tr("the player boots stock firmware again") } else { tr("rockbox is live on the player") };
        text::draw_fit(
            &self.g,
            Face::Mono,
            &text::decode(sub, window(st, 0.4, 0.6), t, 4),
            vec2(x0 + 2.0, ty + 32.0),
            14.0,
            pal::CYAN,
            Align::Left,
            sz.x - x0 - 40.0,
        );
        let r = Rect::new(x0, ty + 66.0, w, 186.0);
        let pa = window(st, 0.5, 0.5);
        ui::panel(r, pal::ACID, pa);
        ui::header(&self.g, vec2(r.x + 22.0, r.y + 28.0), r.w - 44.0, tr("SESSION"), pal::ACID, pa, t);
        let o = self.job.outcome.clone().unwrap_or_default();
        let el = self.job.ended.unwrap_or(t) - self.job.started;
        let mut rows: Vec<(&str, String)> = Vec::new();
        if let Some(b) = &o.backup {
            rows.push((tr("BACKUP"), b.file_name().map(|n| n.to_string_lossy().to_string()).unwrap_or_default()));
        }
        rows.push((tr("FLASH"), tf("{0} sectors rewritten, checksummed", &[&self.job.written])));
        if !restored {
            rows.push((
                tr("CARD"),
                if o.card {
                    tf("{0} files  \u{b7}  {1}  \u{b7}  read back OK", &[&o.files, &mib(o.bytes)])
                } else {
                    tr("not updated").to_string()
                },
            ));
        }
        rows.push((tr("TIME"), format!("{:02}:{:05.2}", (el / 60.0) as u32, el % 60.0)));
        let keys: Vec<&str> = rows.iter().map(|r| r.0).collect();
        let kw = key_column(&self.g, &keys, 11.0, 88.0, r.w * 0.4);
        for (k, (key, val)) in rows.iter().enumerate() {
            let ra = window(st, 0.7 + k as f32 * 0.12, 0.4);
            let y = r.y + 64.0 + k as f32 * 28.0;
            text::draw_fit(&self.g, Face::Mono, key, vec2(r.x + 22.0, y), 11.0, pal::alpha(pal::DIM, ra), Align::Left, kw - 8.0);
            text::draw_fit(
                &self.g,
                Face::Mono,
                &text::decode(val, ra, t, k as u32),
                vec2(r.x + 22.0 + kw, y),
                13.0,
                pal::alpha(pal::ICE, ra),
                Align::Left,
                r.w - 44.0 - kw,
            );
        }
        let ba = window(st, 1.2, 0.5);
        text::draw_spaced_fit(
            &self.g,
            Face::Semi,
            &if restored { tr("UNPLUG TO FINISH").to_string() } else { format!("{}  //  {}", tr("EJECTED"), tr("UNPLUG AND ROCK")) },
            vec2(x0, r.y + r.h + 40.0),
            12.0,
            pal::alpha(pal::MAGENTA, ba),
            Align::Left,
            3.0,
            w,
        );
        // the credits: the big one, rainbow-rimmed, with a fuse burning down
        // to when they roll by themselves
        let cr = Rect::new(x0, r.y + r.h + 62.0, w.min(450.0), 52.0);
        let rim = pal::mix(pal::mix(pal::MAGENTA, pal::CYAN, 0.5 + 0.5 * (t * 2.3).sin()), pal::AMBER, 0.5 + 0.5 * (t * 1.7).cos());
        let glow = 6.0 + 5.0 * self.beat + 3.0 * (t * 4.0).sin().abs();
        ui::polyline(&ui::chamfer(Rect::new(cr.x - glow, cr.y - glow * 0.6, cr.w + glow * 2.0, cr.h + glow * 1.2), 14.0), 1.4, pal::alpha(rim, ba * 0.8), true);
        if ui::button(&mut self.ui, &self.g, cr, tr("CREDITS"), Tone::Primary, ba, true) {
            self.go(Screen::Credits);
        }
        if self.credits_due {
            let fuse = clamp01(1.0 - (st - 1.0) / (CREDITS_AFTER - 1.0));
            let fy = cr.y + cr.h + 14.0;
            draw_rectangle(cr.x, fy, cr.w * fuse, 2.0, pal::alpha(rim, ba));
            draw_circle(cr.x + cr.w * fuse, fy + 1.0, 2.5 + 1.5 * (t * 20.0).sin().abs(), pal::alpha(pal::WHITE, ba));
        }
        let room = sz.x - x0 - 40.0;
        let again_w = ui::button_width(&self.g, tr("AGAIN"), 40.0).clamp(200.0, room);
        let stock_w = ui::button_width(&self.g, tr("RETURN TO STOCK"), 40.0).clamp(200.0, room);
        let by = cr.y + cr.h + 34.0;
        if ui::button(&mut self.ui, &self.g, Rect::new(x0, by, again_w, 40.0), tr("AGAIN"), Tone::Secondary, ba, true) {
            self.pkg = None;
            self.go(Screen::Select);
        }
        let side = again_w + 20.0 + stock_w <= room;
        let sr = if side { Rect::new(x0 + again_w + 20.0, by, stock_w, 40.0) } else { Rect::new(x0, by + 54.0, stock_w, 40.0) };
        if !restored
            && self.stock.is_some()
            && ui::button(
                &mut self.ui,
                &self.g,
                sr,
                tr("RETURN TO STOCK"),
                Tone::Secondary,
                ba,
                true,
            )
        {
            self.to_stock = true;
            self.go(Screen::Link);
        }
    }

    fn hud_fail(&mut self) {
        let sz = self.g.size();
        let t = self.time;
        let st = self.screen_t;
        let x0 = sz.x * 0.5;
        let w = (sz.x - x0 - 40.0).min(600.0);
        let ts = (w / 14.0).clamp(26.0, 46.0);
        text::glitch_title(
            &self.g,
            tr("ACCESS DENIED"),
            vec2(x0, 190.0),
            ts,
            pal::RED,
            Align::Left,
            0.2 + 0.6 * (1.0 - window(st, 0.0, 1.5)),
            t,
            ts * 0.18,
            sz.x - x0 - 40.0,
        );
        let written = self.job.written > 0;
        let sub = if written { tr("the flash was partly rewritten: a backup is safe on disk") } else { tr("nothing was written") };
        text::draw_fit(
            &self.g,
            Face::Mono,
            &text::decode(sub, window(st, 0.3, 0.6), t, 9),
            vec2(x0 + 2.0, 222.0),
            14.0,
            pal::AMBER,
            Align::Left,
            sz.x - x0 - 40.0,
        );
        let r = Rect::new(x0, 256.0, w, 220.0);
        let pa = window(st, 0.4, 0.5);
        let e = self.job.error.clone().unwrap_or_default();
        ui::panel(r, pal::RED, pa);
        ui::header(&self.g, vec2(r.x + 22.0, r.y + 28.0), r.w - 44.0, &fault_title(&e), pal::RED, pa, t);
        for (k, l) in ui::wrap(&self.g, Face::Mono, &e, 13.0, r.w - 44.0).iter().take(8).enumerate() {
            text::draw(&self.g, Face::Mono, l, vec2(r.x + 22.0, r.y + 62.0 + k as f32 * 18.0), 13.0, pal::alpha(pal::ICE, pa), Align::Left);
        }
        let ba = window(st, 0.8, 0.5);
        if ui::button(&mut self.ui, &self.g, Rect::new(x0, r.y + r.h + 30.0, 180.0, 48.0), tr("RETRY"), Tone::Primary, ba, true) {
            // the same job again: a failed restore restores
            self.to_stock = self.job.restoring;
            self.go(Screen::Link);
        }
        if ui::button(&mut self.ui, &self.g, Rect::new(x0 + 200.0, r.y + r.h + 30.0, 140.0, 48.0), tr("BACK"), Tone::Secondary, ba, true) {
            self.go(Screen::Select);
        }
        self.copy_button(Rect::new(x0, r.y + r.h + 94.0, 220.0, 38.0), &e, ba);
    }
}

// ---------------------------------------------------------------- bits

fn permission_hint() -> &'static str {
    if cfg!(target_os = "linux") {
        tr("no permission: install the udev rule (see README)")
    } else if cfg!(target_os = "macos") {
        tr("macOS: run rockslop with sudo to take the device")
    } else {
        tr("install the WinUSB driver for it (Zadig)")
    }
}

/// The fault panel's title: FAULT, and the error's number if it has one.
fn fault_title(e: &str) -> String {
    match i18n::code(e) {
        Some(c) => format!("{}  //  {c}", tr("FAULT")),
        None => tr("FAULT").to_string(),
    }
}

/// A key column as wide as its longest label (within `min..max`).
fn key_column(g: &Gfx, keys: &[&str], size: f32, min: f32, max: f32) -> f32 {
    let w = keys.iter().map(|k| text::measure(g, Face::Mono, k, size).x).fold(0.0, f32::max) + 14.0;
    w.clamp(min, max.max(min))
}

fn sect_label(s: Sect) -> &'static str {
    match s {
        Sect::Protected => tr("LOCKED"),
        Sect::Same => tr("SAME"),
        Sect::Read => tr("READ"),
        Sect::Erase => tr("ERASE"),
        Sect::Good => tr("WRITTEN"),
        Sect::Bad => tr("BAD"),
        _ => "",
    }
}

/// The folder a card file counts toward in the sync panel: the top level
/// under .rockbox, or the plugin category under rocks/.
fn card_dir(path: &str) -> String {
    let rel = path.strip_prefix(".rockbox/").unwrap_or(path);
    let mut parts = rel.split('/');
    let first = parts.next().unwrap_or("");
    match (first, parts.next(), parts.next()) {
        ("rocks", Some(cat), Some(_)) => cat.to_string(),
        (d, Some(_), _) => d.to_string(),
        _ => "root".to_string(),
    }
}

/// A colour per kind of card file, for the packets and the file list.
fn ext_color(path: &str) -> Color {
    match path.rsplit('.').next().unwrap_or("") {
        "rock" | "ovl" => pal::MAGENTA,
        "codec" => pal::CYAN,
        "fnt" => pal::AMBER,
        "lng" => pal::VIOLET,
        "bmp" => pal::NAVY,
        _ => pal::TEAL,
    }
}

fn rate_txt(b: f32) -> String {
    if b >= 1024.0 * 1024.0 {
        format!("{:.1} MiB/s", b / (1024.0 * 1024.0))
    } else {
        format!("{:.0} KiB/s", b / 1024.0)
    }
}

fn hash32(mut x: u32) -> u32 {
    x ^= x >> 16;
    x = x.wrapping_mul(0x7feb_352d);
    x ^= x >> 15;
    x = x.wrapping_mul(0x846c_a68b);
    x ^ (x >> 16)
}

fn shorten(s: &str, n: usize) -> String {
    let c = s.chars().count();
    if c <= n {
        s.to_string()
    } else {
        let tail: String = s.chars().skip(c - (n - 1)).collect();
        format!("~{tail}")
    }
}

fn check(p: Vec2, c: Color, a: f32) {
    ui::polyline(&[p + vec2(-5.0, 0.0), p + vec2(-1.5, 3.5), p + vec2(5.0, -4.0)], 2.0, pal::alpha(c, a), false);
}

fn cross(p: Vec2, c: Color, a: f32) {
    ui::line(p + vec2(-4.0, -4.0), p + vec2(4.0, 4.0), 2.0, pal::alpha(c, a));
    ui::line(p + vec2(-4.0, 4.0), p + vec2(4.0, -4.0), 2.0, pal::alpha(c, a));
}

/// A line from a HUD element to a point on the model, with packets
/// running along it towards the model.
/// A select-screen callout: where its box sits (model units, in the
/// player's unturned frame) and which way its leader leaves the box
/// (screen space, toward the player).
struct Hint {
    f: Feature,
    label: &'static str,
    at: Vec2,
    out: Vec2,
}

/// What tells the two players apart, laid out as on the design sketch:
/// side features get a box beside the player, ports a box below it.
fn hints(p: &Player) -> Vec<Hint> {
    let (hw, hh) = (p.dims.x / 2.0, p.dims.y / 2.0);
    let x = |f: Feature| p.anchor(f).map(|a| a.pos.x).unwrap_or(0.0);
    let y = |f: Feature| p.anchor(f).map(|a| a.pos.y).unwrap_or(0.0);
    let (from_left, from_right, from_below) = (vec2(1.0, 0.0), vec2(-1.0, 0.0), vec2(0.0, -1.0));
    let below = -hh - 0.8;
    let h = |f, label, at, out| Hint { f, label, at, out };
    match p.kind {
        Kind::Yp3 => vec![
            h(Feature::Reset, tr("RESET"), vec2(hw + 0.8, y(Feature::Reset)), from_right),
            h(Feature::Headphone, tr("HEADPHONE"), vec2(x(Feature::Headphone) - 0.4, below), from_below),
            h(Feature::Usb, "USB-C", vec2(x(Feature::Usb) + 0.4, below), from_below),
        ],
        Kind::Ts17 => vec![
            // the pinhole is on the back: its leader runs dashed until the player turns
            h(Feature::Reset, tr("RESET"), vec2(-hw - 0.8, -3.0), from_left),
            h(Feature::Vol, tr("VOL"), vec2(hw + 0.8, y(Feature::Vol)), from_right),
            h(Feature::Usb, "USB-C", vec2(x(Feature::Usb) - 0.55, below), from_below),
            h(Feature::Power, tr("POWER"), vec2(0.0, below - 0.75), from_below),
            h(Feature::Headphone, tr("HEADPHONE"), vec2(x(Feature::Headphone) + 0.55, below), from_below),
        ],
    }
}

/// Yaw and pitch that show an anchor to the camera: a three-quarter view
/// of a side, the back square on, or the bottom tipped up. The yaw is the
/// turn nearest `cur`, so the player takes the short way round.
fn aim_at(an: model::Anchor, cur: f32) -> (f32, f32) {
    let n = an.normal;
    let (yaw, pitch) = if n.y < -0.5 {
        (-0.4 * an.pos.x.clamp(-1.0, 1.0), -0.5)
    } else {
        let full = (-n.x).atan2(n.z);
        (if n.x.abs() > 0.5 { full * 0.62 } else { full }, 0.12)
    };
    (yaw + ((cur - yaw) / TAU).round() * TAU, pitch)
}

/// A mouse pointer drawn in the scene, its tip at `p`: ringed while it
/// holds the button down.
fn ghost_pointer(p: Vec2, down: bool, a: f32, time: f32) {
    let s = 1.3;
    let pts: Vec<Vec2> = [(0.0, 0.0), (0.0, 17.0), (4.0, 13.0), (7.0, 20.0), (10.0, 19.0), (7.0, 12.0), (12.0, 12.0)]
        .iter()
        .map(|(x, y)| p + vec2(*x, *y) * s)
        .collect();
    let fill = Color::new(0.02, 0.01, 0.05, 0.85 * a);
    ui::fill_convex(&[pts[0], pts[1], pts[6]], fill);
    ui::fill_convex(&[pts[2], pts[3], pts[4], pts[5]], fill);
    ui::polyline(&pts, 1.6, pal::alpha(pal::WHITE, a), true);
    if down {
        let ph = (time * 2.2) % 1.0;
        ui::ring(p, 5.0 + ph * 10.0, 1.4, pal::alpha(pal::MAGENTA, (1.0 - ph) * a), 24);
        ui::ring(p, 4.0, 1.4, pal::alpha(pal::MAGENTA, a), 16);
    }
}

fn trace(anchor: Vec2, from: Vec2, c: Color, t: f32, time: f32) {
    if t <= 0.0 {
        return;
    }
    let elbow = vec2(from.x - 30.0, from.y);
    let pts = [from, elbow, anchor];
    let l0 = from.distance(elbow);
    let l1 = elbow.distance(anchor);
    let total = l0 + l1;
    let shown = total * ease_out_cubic(t);
    let at = |d: f32| if d < l0 { from.lerp(elbow, d / l0.max(0.01)) } else { elbow.lerp(anchor, ((d - l0) / l1.max(0.01)).min(1.0)) };
    let end = at(shown);
    ui::line(pts[0], if shown < l0 { end } else { elbow }, 1.4, pal::alpha(c, 0.9));
    if shown > l0 {
        ui::line(elbow, end, 1.4, pal::alpha(c, 0.9));
    }
    for k in 0..3 {
        let d = ((time * 0.8 + k as f32 / 3.0) % 1.0) * shown;
        let p = at(d);
        draw_circle(p.x, p.y, 2.2, pal::mix(c, pal::WHITE, 0.5));
    }
    if t >= 1.0 {
        let ph = (time * 1.3) % 1.0;
        ui::ring(anchor, 5.0, 1.6, c, 18);
        ui::ring(anchor, 7.0 + ph * 16.0, 1.2, pal::alpha(c, 1.0 - ph), 28);
        draw_circle(anchor.x, anchor.y, 2.0, pal::WHITE);
    }
    draw_rectangle(from.x - 3.0, from.y - 3.0, 6.0, 6.0, c);
}

/// The player's whole flash as one bar: locked regions, what the package
/// writes, what stays.
fn layout_bar(g: &Gfx, r: Rect, kind: Kind, segs: &[(u32, u32)], a: f32, time: f32) {
    use rockslop::device::{ts17, yp3};
    if a <= 0.0 {
        return;
    }
    let (size, wr) = match kind {
        Kind::Yp3 => (yp3::FLASH_SIZE, yp3::WRITABLE),
        Kind::Ts17 => (ts17::FLASH_SIZE, ts17::WRITABLE),
    };
    let bar = Rect::new(r.x, r.y + 12.0, r.w, 14.0);
    let x = |off: u32| bar.x + bar.w * off as f32 / size as f32;
    text::draw(g, Face::Mono, tr("LAYOUT"), vec2(r.x, r.y + 2.0), 11.0, pal::alpha(pal::DIM, a), Align::Left);
    let grow = ease_out_cubic(a);
    draw_rectangle(bar.x, bar.y, bar.w * grow, bar.h, pal::alpha(pal::scale(pal::TEAL, 0.22), a));
    // locked: hatched
    for (lo, hi) in [(0, wr.0), (wr.1, size)] {
        let (x0, x1) = (x(lo), x(hi));
        draw_rectangle(x0, bar.y, (x1 - x0) * grow, bar.h, pal::alpha(pal::scale(pal::VIOLET, 0.35), a));
        let mut hx = x0;
        while hx < x1 {
            ui::line(vec2(hx, bar.y + bar.h), vec2((hx + 6.0).min(x1), bar.y), 1.0, pal::alpha(pal::VIOLET, a));
            hx += 5.0;
        }
    }
    for &(off, len) in segs {
        let (x0, x1) = (x(off), x(off + len));
        let pulse = 0.75 + 0.25 * (time * 4.0).sin();
        draw_rectangle(x0, bar.y, ((x1 - x0) * grow).max(2.0), bar.h, pal::alpha(pal::scale(pal::MAGENTA, pulse), a));
    }
    ui::polyline(
        &[vec2(bar.x, bar.y), vec2(bar.x + bar.w, bar.y), vec2(bar.x + bar.w, bar.y + bar.h), vec2(bar.x, bar.y + bar.h)],
        1.0,
        pal::alpha(pal::scale(pal::ICE, 0.5), a),
        true,
    );
    let labels = [(0, "0x000000"), (wr.0, ""), (wr.1, ""), (size, "")];
    for (off, _) in labels.iter().skip(1).take(2) {
        ui::line(vec2(x(*off), bar.y - 3.0), vec2(x(*off), bar.y + bar.h + 3.0), 1.0, pal::alpha(pal::ICE, a));
    }
    let ty = bar.y + bar.h + 13.0;
    text::draw(g, Face::Mono, tr("LOCKED"), vec2(bar.x, ty), 10.0, pal::alpha(pal::VIOLET, a), Align::Left);
    text::draw(
        g,
        Face::Mono,
        &format!("ROCKBOX {:X}..{:X}", wr.0, wr.1),
        vec2(x(wr.0) + 4.0, ty),
        10.0,
        pal::alpha(pal::MAGENTA, a),
        Align::Left,
    );
    text::draw(g, Face::Mono, tr("LOCKED"), vec2(bar.x + bar.w, ty), 10.0, pal::alpha(pal::VIOLET, a), Align::Right);
}

/// Marching-dash outline.
fn dashed(pts: &[Vec2], c: Color, time: f32) {
    let n = pts.len();
    let mut phase = (time * 30.0) % 14.0;
    for i in 0..n {
        let a = pts[i];
        let b = pts[(i + 1) % n];
        let len = a.distance(b);
        let dir = (b - a) / len.max(0.001);
        let mut d = -phase;
        while d < len {
            let s = d.max(0.0);
            let e = (d + 8.0).min(len);
            if e > s {
                ui::line(a + dir * s, a + dir * e, 1.4, c);
            }
            d += 14.0;
        }
        phase = (phase + len) % 14.0;
    }
}
