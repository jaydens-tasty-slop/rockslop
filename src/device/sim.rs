//! Demo backend: a convincing fake player, for `--demo` and screenshots.
//! Nothing here touches USB.

use super::*;
use crate::pkg::Package;
use std::sync::atomic::{AtomicU64, Ordering};
use std::sync::Arc;
use std::thread::sleep;
use std::time::Instant;

/// The app's clock in microseconds, for screenshots: the fake device then
/// runs on frame time instead of wall time, so a shot at t is repeatable.
pub type Clock = Arc<AtomicU64>;

pub struct Sim {
    watch: Option<(Kind, f64)>,
    state: Option<(Kind, Mode)>,
    kicked: Option<f64>,
    speed: f32,
    rng: u64,
    clock: Option<Clock>,
    base: Instant,
    /// fail partway through writing (demo of the failure path)
    pub fail: bool,
    /// the player first turns up running Rockbox, is unplugged, and comes
    /// back in download mode
    pub rockbox: bool,
}

impl Sim {
    pub fn new(speed: f32, clock: Option<Clock>) -> Sim {
        Sim {
            watch: None,
            state: None,
            kicked: None,
            speed: speed.max(0.05),
            rng: 0x1234_5678_9abc_def1,
            clock,
            base: Instant::now(),
            fail: false,
            rockbox: false,
        }
    }

    /// Seconds, scaled by speed.
    fn now(&self) -> f64 {
        let t = match &self.clock {
            Some(c) => c.load(Ordering::Relaxed) as f64 / 1e6,
            None => self.base.elapsed().as_secs_f64(),
        };
        t * self.speed as f64
    }

    fn rand(&mut self) -> u32 {
        self.rng ^= self.rng << 13;
        self.rng ^= self.rng >> 7;
        self.rng ^= self.rng << 17;
        (self.rng >> 16) as u32
    }

    fn nap(&self, ms: u64) {
        if self.clock.is_some() {
            let until = self.now() + ms as f64 / 1000.0;
            while self.now() < until {
                sleep(Duration::from_micros(500));
            }
        } else {
            sleep(Duration::from_micros((ms as f32 * 1000.0 / self.speed) as u64));
        }
    }

    /// Bytes that look like firmware: thumb-ish code, tables and strings.
    fn firmware(&mut self, addr: u32, n: usize) -> Vec<u8> {
        let mut v = Vec::with_capacity(n);
        let region = (addr >> 14) % 7;
        while v.len() < n {
            let r = self.rand();
            match (region, r % 23) {
                (6, _) => v.push(0xff),
                (_, 0) => v.extend_from_slice(b"Rockbox\0"),
                (_, 1) => v.extend_from_slice(b"yp3_2.0.43\0"),
                (_, 2) => v.extend_from_slice(b"SMTLINK\0"),
                (5, _) => v.extend_from_slice(&[0, 0, (r >> 8) as u8, 0]),
                _ => v.extend_from_slice(&[(r >> 3) as u8, 0xb5 ^ (r as u8 & 0x0f), (r >> 11) as u8, 0x46 + (r % 3) as u8]),
            }
        }
        v.truncate(n);
        v
    }

    fn probe(&mut self, kind: Kind, ev: &Sink) {
        ev.phase(Phase::Probe);
        let lines: Vec<(&str, String, Option<bool>)> = match kind {
            Kind::Yp3 => vec![
                ("USB", "301a:2800 SMTLINK DEVICE 2.00".into(), Some(true)),
                ("CHIP", "SL6801 (serial 20201111000001)".into(), Some(true)),
                ("PAYLOAD", "svc @0x806000 ONLINE".into(), Some(true)),
                ("CPU", "CORTEX-M4 r0p1 (0x410FC241)".into(), Some(true)),
                ("FLASH", "P25Q16H 2 MiB (85 60 15)".into(), Some(true)),
                ("CELL", "3.94 V  CHARGER OFF".into(), Some(true)),
            ],
            Kind::Ts17 => vec![
                ("USB", "10d6:10d6 ADFU".into(), Some(true)),
                ("CHIP", "ACTIONS LARK 0x2351".into(), Some(true)),
                ("HOOK", "gadget bus @0x01018000 ONLINE".into(), Some(true)),
                ("CPU", "CORTEX-M4F r0p1 (0x410FC241)".into(), Some(true)),
                ("NOR", "4 MiB, fw0_sys @ 0x14000".into(), Some(true)),
                ("CELL", "3.88 V  CHARGER GUARDED".into(), Some(true)),
            ],
        };
        for (k, v, ok) in lines {
            self.nap(260);
            ev.probe(k, v, ok);
        }
    }
}

impl Backend for Sim {
    fn scan(&mut self, _ev: &Sink) -> Vec<Seen> {
        let Some((kind, since)) = self.watch else {
            return Vec::new();
        };
        let el = (self.now() - since) as f32;
        if self.rockbox {
            let arrive = 2.5;
            if el < arrive || (arrive + 6.0..arrive + 12.0).contains(&el) {
                return Vec::new();
            }
            if el < arrive + 6.0 {
                return vec![Seen { kind, mode: Mode::Rockbox, access: true }];
            }
            self.rockbox = false;
            self.state = Some((kind, Mode::Rom));
        }
        if let Some((k, m)) = self.state {
            if let Some(t) = self.kicked {
                if self.now() - t > 1.2 {
                    self.state = Some((k, Mode::Rom));
                    self.kicked = None;
                } else {
                    return Vec::new(); // re-enumerating
                }
            }
            return vec![Seen { kind: k, mode: self.state.map(|s| s.1).unwrap_or(m), access: true }];
        }
        let arrive = match kind {
            Kind::Yp3 => 5.5,
            Kind::Ts17 => 3.5,
        };
        if el > arrive {
            let mode = if kind == Kind::Ts17 { Mode::Stock } else { Mode::Rom };
            self.state = Some((kind, mode));
            return vec![Seen { kind, mode, access: true }];
        }
        Vec::new()
    }

    fn run(&mut self, cmd: Cmd, ev: &Sink) -> Result<(), String> {
        match cmd {
            Cmd::Watch(k) => {
                match k {
                    Some(k) if self.watch.map(|w| w.0) != Some(k) => {
                        self.watch = Some((k, self.now()));
                        self.state = None;
                    }
                    None => {
                        self.watch = None;
                        self.state = None;
                    }
                    _ => {}
                }
                Ok(())
            }
            Cmd::Kick(kind) => {
                ev.info("stock firmware detected: ACTIONSUSBD");
                self.nap(300);
                ev.warn("injecting vendor command 0xCB -> ADFU");
                self.kicked = Some(self.now());
                self.state = Some((kind, Mode::Stock));
                self.nap(400);
                ev.send(Event::Ready);
                Ok(())
            }
            Cmd::Probe(kind) => {
                ev.phase(Phase::Connect);
                ev.info(format!("opening {} download mode", kind.name()));
                self.nap(350);
                self.probe(kind, ev);
                ev.send(Event::Ready);
                Ok(())
            }
            Cmd::Flash { kind, package } => self.flash(kind, Some(package), ev),
            Cmd::Restore { kind, .. } => self.flash(kind, None, ev),
        }
    }
}

impl Sim {
    fn flash(&mut self, kind: Kind, package: Option<PathBuf>, ev: &Sink) -> Result<(), String> {
        let restoring = package.is_none();
        let pkg = package.as_ref().and_then(|p| Package::open(p).ok());
        let (sectors, sector_size) = match kind {
            Kind::Yp3 => (512u32, 4096u32),
            Kind::Ts17 => (1024, 4096),
        };
        let (lo, hi) = match kind {
            Kind::Yp3 => (0xE000 / 4096, 0x1FC000 / 4096),
            Kind::Ts17 => (0x14000 / 4096, 0x1BB000 / 4096),
        };
        ev.phase(Phase::Connect);
        ev.info("reattaching to download mode");
        self.nap(300);
        ev.send(Event::Layout { sectors, sector_size });
        for i in 0..sectors {
            if i < lo || i >= hi {
                ev.sector(i, Sect::Protected);
            }
        }
        if !restoring {
            ev.phase(Phase::Backup);
            ev.info(format!("dumping {} KiB of stock flash", sectors * sector_size / 1024));
            for i in 0..sectors {
                ev.sector(i, Sect::Read);
                let data = self.firmware(i * sector_size, 4096);
                if i % 2 == 0 {
                    hex_feed(ev, i * sector_size, &data[..256], 4);
                }
                if i % 8 == 7 {
                    self.nap(40);
                }
                ev.sector(i, if i < lo || i >= hi { Sect::Protected } else { Sect::Same });
                ev.progress(i as f32 / sectors as f32);
            }
            ev.ok("backup -> ~/.local/share/rockslop/backups/yp3-3fa9c21e-20261106-stock.bin");
            ev.phase(Phase::Verify);
            ev.info("re-reading CRC32 of every sector");
            for i in 0..=20 {
                ev.progress(i as f32 / 20.0);
                self.nap(60);
            }
            ev.ok("backup verified: 512/512 sectors match");
            ev.probe("BOOTLOADER", "sha256 7c1e..9a04 KNOWN", Some(true));
        }
        ev.phase(Phase::Write);
        let n_write = if kind == Kind::Yp3 { 168 } else { 210 };
        ev.info(format!("{n_write} sectors differ; writing"));
        for k in 0..n_write {
            let i = lo + k;
            ev.sector(i, Sect::Erase);
            self.nap(45);
            if self.fail && k == 57 {
                ev.sector(i, Sect::Bad);
                return Err(yp3::sector_failed(i * sector_size, "verify failed (crc 3fa2e1c9, want 8d1c2f70)"));
            }
            ev.sector(i, Sect::Write);
            let data = self.firmware(i * sector_size, 64);
            ev.send(Event::Hex(i * sector_size, data[..16].to_vec()));
            self.nap(60);
            ev.sector(i, Sect::Good);
            ev.progress(k as f32 / n_write as f32);
            if k % 24 == 0 {
                ev.dim(format!("sector 0x{:06x} erase/program/verify ok", i * sector_size));
            }
        }
        ev.phase(Phase::Check);
        ev.info("full flash CRC scan");
        for i in 0..sectors {
            if i % 16 == 0 {
                self.nap(30);
                ev.progress(i as f32 / sectors as f32);
            }
        }
        ev.ok("flash checksum OK");
        ev.phase(Phase::Reboot);
        ev.warn(if restoring { "rebooting into stock firmware" } else { "rebooting into Rockbox" });
        self.state = None;
        self.watch = None;
        self.nap(1500);
        let mut out = Outcome { restored: restoring, ..Default::default() };
        if !restoring {
            ev.phase(Phase::Sync);
            ev.info("waiting for Rockbox USB mass storage");
            self.nap(900);
            ev.send(Event::NeedCard(true));
            ev.warn("no media: insert a microSD card");
            self.nap(2600);
            ev.send(Event::NeedCard(false));
            ev.ok("card online: /run/media/user/YP3CARD");
            let names: Vec<(String, u64)> = match &pkg {
                Some(p) if !p.files.is_empty() => p.files.iter().map(|f| (f.path.clone(), f.size)).collect(),
                _ => demo_card(),
            };
            // as the real sync: write everything (0..0.7), then read it all
            // back (0.7..1). Writes go at ~250 KB/s, read-back at ~1 MB/s.
            let total: u64 = names.iter().map(|x| x.1).sum::<u64>().max(1);
            let mut done = 0;
            for (n, sz) in &names {
                ev.send(Event::File(n.clone(), *sz));
                self.nap(4 + *sz / 250);
                done += sz;
                ev.progress(done as f32 / total as f32 * 0.7);
            }
            ev.ok(format!("{} files written", names.len()));
            ev.info("reading every file back from the card");
            done = 0;
            for (k, (n, sz)) in names.iter().enumerate() {
                done += sz;
                if k % 4 == 0 {
                    ev.send(Event::ReadBack(n.clone(), *sz));
                    self.nap(2 + *sz / 250);
                }
                ev.progress(0.7 + 0.3 * done as f32 / total as f32);
            }
            ev.ok(format!("{} files read back: sha256 OK", names.len()));
            out.files = names.len();
            out.bytes = total;
            out.card = true;
            ev.phase(Phase::Eject);
            ev.info("syncing and ejecting card");
            self.nap(800);
            out.backup = Some(PathBuf::from("~/.local/share/rockslop/backups/yp3-3fa9c21e-20261106-stock.bin"));
        }
        ev.phase(Phase::Done);
        ev.send(Event::Done(out));
        Ok(())
    }
}

/// The card tree the simulator pretends to sync: a plausible Rockbox
/// install, about 3.5 MB in some 120 files.
pub fn demo_card() -> Vec<(String, u64)> {
    let dirs: [(&str, &[&str], usize, u64); 8] = [
        ("codecs", &["mpa", "aac", "vorbis", "flac", "wav", "opus", "wma", "mpc"], 1, 120_000),
        (
            "rocks/games",
            &["pong", "rockblox", "snake", "chopper", "brickmania", "clix", "jewels", "minesweeper", "solitaire", "sokoban"],
            1,
            38_000,
        ),
        ("rocks/apps", &["rockpaint", "calculator", "stopwatch", "text_viewer", "properties", "calendar", "metronome"], 1, 26_000),
        ("rocks/demos", &["starfield", "fire", "plasma", "mosaique", "rockbox_logo", "cube", "oscilloscope"], 1, 14_000),
        ("fonts", &["08-Rockfont", "12-Nimbus", "14-Rockbox-Mix", "16-Jackash", "18-Adobe-Helvetica", "20-Terminus"], 4, 24_000),
        ("langs", &["english", "deutsch", "francais", "espanol", "nederlands", "svenska", "japanese", "polski"], 3, 31_000),
        ("themes", &["cabbiev2", "rockslop", "boxed", "nonhinged"], 6, 9_000),
        ("icons", &["tango_small", "tango_icons", "viewers"], 5, 16_000),
    ];
    let mut out = Vec::new();
    let mut seed = 0x2545_f491u64;
    for (dir, names, copies, size) in dirs {
        for k in 0..copies {
            for n in names {
                seed = seed.wrapping_mul(6364136223846793005).wrapping_add(1442695040888963407);
                let ext = match dir {
                    "codecs" => "codec",
                    "fonts" => "fnt",
                    "langs" => "lng",
                    "themes" => "cfg",
                    "icons" => "bmp",
                    _ => "rock",
                };
                let name = if k == 0 { format!(".rockbox/{dir}/{n}.{ext}") } else { format!(".rockbox/{dir}/{n}-{k}.{ext}") };
                out.push((name, size / 2 + (seed >> 33) % size));
            }
        }
    }
    out
}
