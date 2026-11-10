//! Device side: USB presence, jobs (probe, flash, restore) on a worker
//! thread, and the events they report to the UI.

pub mod backup;
pub mod bot;
pub mod rdm;
pub mod real;
pub mod sdcard;
pub mod sim;
pub mod ts17;
pub mod usb;
pub mod yp3;

use std::path::{Path, PathBuf};
use std::sync::mpsc::{channel, Receiver, Sender};
use std::time::Duration;

/// An error message with its number in front ("E270: battery too low ..."),
/// so a translated error can still be looked up. The first argument is the
/// number (`E` and three digits), the rest is `format!` arguments; the text
/// after the number is the English key in strings.txt, so leave it alone.
/// Each number is written once, where its failure is raised (a failure
/// raised from several places goes through one helper); a test checks.
///
/// 1xx package, 2xx USB link / download mode / identity / battery, 3xx flash
/// write and checksum, 4xx backup and restore, 5xx SD card, 8xx rockslop-cli
/// prompts, 9xx internal.
#[macro_export]
macro_rules! err {
    ($code:ident, $($fmt:tt)+) => {
        format!("{}: {}", stringify!($code), format_args!($($fmt)+))
    };
}

/// The supported players.
#[derive(Clone, Copy, PartialEq, Eq, Debug, Hash)]
pub enum Kind {
    Yp3,
    Ts17,
}

impl Kind {
    pub fn name(self) -> &'static str {
        match self {
            Kind::Yp3 => "YP3",
            Kind::Ts17 => "TS17",
        }
    }
    pub fn id(self) -> &'static str {
        match self {
            Kind::Yp3 => "yp3",
            Kind::Ts17 => "ts17",
        }
    }
}

/// What we can see on the bus.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum Mode {
    /// Boot ROM recovery / download mode: ready to flash
    Rom,
    /// stock firmware running
    Stock,
    /// Rockbox running (mass storage or charge-only)
    Rockbox,
}

#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub struct Seen {
    pub kind: Kind,
    pub mode: Mode,
    /// false: present but this process may not open it (udev / driver)
    pub access: bool,
}

#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum Phase {
    Connect,
    Probe,
    Backup,
    Verify,
    Write,
    Check,
    Reboot,
    Sync,
    Eject,
    Done,
}

impl Phase {
    pub fn label(self) -> &'static str {
        match self {
            Phase::Connect => "LINK",
            Phase::Probe => "PROBE",
            Phase::Backup => "BACKUP",
            Phase::Verify => "VERIFY",
            Phase::Write => "WRITE",
            Phase::Check => "CHECKSUM",
            Phase::Reboot => "REBOOT",
            Phase::Sync => "SD SYNC",
            Phase::Eject => "EJECT",
            Phase::Done => "DONE",
        }
    }
}

#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum Sect {
    Idle,
    Protected,
    Read,
    Same,
    Erase,
    Write,
    Good,
    Bad,
}

#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum Lvl {
    Info,
    Ok,
    Warn,
    Err,
    Dim,
}

#[derive(Clone, Debug)]
pub enum Event {
    Presence(Vec<Seen>),
    Log(Lvl, String),
    /// identity check results: key, value, verdict
    Probe(String, String, Option<bool>),
    Phase(Phase),
    Progress(f32),
    Layout {
        sectors: u32,
        sector_size: u32,
    },
    Sector(u32, Sect),
    Hex(u32, Vec<u8>),
    /// a card file is being written: its path under the card root, and size
    File(String, u64),
    /// a card file read back and hashed (some of them: the UI only samples)
    ReadBack(String, u64),
    NeedCard(bool),
    /// a kick (stock -> download mode) or probe finished
    Ready,
    Done(Outcome),
    Failed(String),
}

#[derive(Clone, Debug, Default)]
pub struct Outcome {
    pub backup: Option<PathBuf>,
    pub restored: bool,
    pub files: usize,
    pub bytes: u64,
    pub card: bool,
}

#[derive(Clone, Debug)]
pub enum Cmd {
    /// which player the UI is waiting for (sim uses it to stage arrivals)
    Watch(Option<Kind>),
    /// stock firmware -> download mode, where the player supports it
    Kick(Kind),
    /// identify the player in download mode
    Probe(Kind),
    Flash {
        kind: Kind,
        package: PathBuf,
    },
    Restore {
        kind: Kind,
        image: PathBuf,
    },
}

/// Something that can find and drive players: real USB, or the demo.
pub trait Backend: Send {
    /// Who is on the bus; may also run idle housekeeping (charger guard).
    fn scan(&mut self, ev: &Sink) -> Vec<Seen>;
    fn run(&mut self, cmd: Cmd, ev: &Sink) -> Result<(), String>;
}

/// Event sender for jobs, with helpers.
#[derive(Clone)]
pub struct Sink(pub Sender<Event>);

impl Sink {
    pub fn send(&self, e: Event) {
        crate::log::event(&e);
        let _ = self.0.send(e);
    }
    pub fn log(&self, l: Lvl, s: impl Into<String>) {
        self.send(Event::Log(l, s.into()));
    }
    pub fn info(&self, s: impl Into<String>) {
        self.log(Lvl::Info, s)
    }
    pub fn ok(&self, s: impl Into<String>) {
        self.log(Lvl::Ok, s)
    }
    pub fn warn(&self, s: impl Into<String>) {
        self.log(Lvl::Warn, s)
    }
    pub fn dim(&self, s: impl Into<String>) {
        self.log(Lvl::Dim, s)
    }
    pub fn phase(&self, p: Phase) {
        self.send(Event::Phase(p))
    }
    pub fn progress(&self, f: f32) {
        self.send(Event::Progress(f.clamp(0.0, 1.0)))
    }
    pub fn probe(&self, k: &str, v: impl Into<String>, ok: Option<bool>) {
        self.send(Event::Probe(k.to_string(), v.into(), ok))
    }
    pub fn sector(&self, i: u32, s: Sect) {
        self.send(Event::Sector(i, s))
    }
}

pub struct Hub {
    rx: Receiver<Event>,
    tx: Sender<Cmd>,
    pub seen: Vec<Seen>,
    pub busy: bool,
}

impl Hub {
    pub fn start(mut backend: Box<dyn Backend>) -> Hub {
        let (etx, erx) = channel();
        let (ctx, crx) = channel::<Cmd>();
        std::thread::spawn(move || {
            let sink = Sink(etx);
            let mut last: Option<Vec<Seen>> = None;
            loop {
                match crx.recv_timeout(Duration::from_millis(250)) {
                    Ok(cmd) => {
                        let job = !matches!(cmd, Cmd::Watch(_));
                        if job {
                            crate::log::line("CMD", &format!("{cmd:?}"));
                        }
                        let r = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| backend.run(cmd, &sink)));
                        match r {
                            Ok(Ok(())) => {}
                            Ok(Err(e)) => sink.send(Event::Failed(e)),
                            Err(p) => {
                                let msg = p
                                    .downcast_ref::<String>()
                                    .cloned()
                                    .or_else(|| p.downcast_ref::<&str>().map(|s| s.to_string()))
                                    .unwrap_or_else(|| "internal error".into());
                                sink.send(Event::Failed(err!(E901, "internal error: {msg}")));
                            }
                        }
                        if job {
                            last = None; // re-announce presence after any job
                        }
                    }
                    Err(std::sync::mpsc::RecvTimeoutError::Timeout) => {
                        let now = backend.scan(&sink);
                        if last.as_ref() != Some(&now) {
                            sink.send(Event::Presence(now.clone()));
                            last = Some(now);
                        }
                    }
                    Err(_) => return,
                }
            }
        });
        Hub { rx: erx, tx: ctx, seen: Vec::new(), busy: false }
    }

    pub fn send(&mut self, c: Cmd) {
        if !matches!(c, Cmd::Watch(_)) {
            self.busy = true;
        }
        let _ = self.tx.send(c);
    }

    /// Drain pending events (presence is folded into `seen`).
    pub fn poll(&mut self) -> Vec<Event> {
        let mut out = Vec::new();
        while let Ok(e) = self.rx.try_recv() {
            match &e {
                Event::Presence(s) => self.seen = s.clone(),
                Event::Done(_) | Event::Failed(_) | Event::Ready => self.busy = false,
                _ => {}
            }
            out.push(e);
        }
        out
    }

    pub fn find(&self, kind: Kind) -> Option<Seen> {
        self.seen.iter().copied().find(|s| s.kind == kind)
    }
}

/// Hex dump helper for the UI feed: one event per 16 bytes, sampled.
pub fn hex_feed(ev: &Sink, addr: u32, data: &[u8], every: usize) {
    for (i, chunk) in data.chunks(16).enumerate() {
        if i % every.max(1) == 0 {
            ev.send(Event::Hex(addr + (i * 16) as u32, chunk.to_vec()));
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    /// Every error number is written once, as E and three digits, so a
    /// number someone reports names exactly one failure.
    #[test]
    fn error_numbers_are_unique() {
        let srcs = [
            ("device/mod.rs", include_str!("mod.rs")),
            ("device/backup.rs", include_str!("backup.rs")),
            ("device/bot.rs", include_str!("bot.rs")),
            ("device/rdm.rs", include_str!("rdm.rs")),
            ("device/real.rs", include_str!("real.rs")),
            ("device/sdcard.rs", include_str!("sdcard.rs")),
            ("device/sim.rs", include_str!("sim.rs")),
            ("device/ts17.rs", include_str!("ts17.rs")),
            ("device/usb.rs", include_str!("usb.rs")),
            ("device/yp3.rs", include_str!("yp3.rs")),
            ("pkg.rs", include_str!("../pkg.rs")),
            ("bin/rockslop-cli.rs", include_str!("../bin/rockslop-cli.rs")),
        ];
        let mut seen = std::collections::HashMap::new();
        for (file, src) in srcs {
            let code = src.split("#[cfg(test)]").next().unwrap();
            for (i, pat) in code.match_indices(concat!("err", "!(")) {
                let line = &code[code[..i].rfind('\n').map_or(0, |n| n + 1)..i];
                if line.trim_start().starts_with("//") {
                    continue;
                }
                let n = code[i + pat.len()..].split(',').next().unwrap().trim();
                let b = n.as_bytes();
                let ok = b.len() == 4 && b[0] == b'E' && (b'1'..=b'9').contains(&b[1]) && b[2..].iter().all(u8::is_ascii_digit);
                assert!(ok, "{file}: {n:?} is not an error number like E214");
                if let Some(other) = seen.insert(n, file) {
                    panic!("{n} is raised in {other} and again in {file}: share one helper");
                }
            }
        }
        assert!(seen.len() > 80, "only {} error numbers found", seen.len());
    }

    fn failure(backend: Box<dyn Backend>, cmd: Cmd) -> String {
        let mut hub = Hub::start(backend);
        hub.send(cmd);
        let t0 = std::time::Instant::now();
        while t0.elapsed() < Duration::from_secs(30) {
            for e in hub.poll() {
                match e {
                    Event::Failed(e) => return e,
                    Event::Done(_) => panic!("the job succeeded"),
                    _ => {}
                }
            }
            std::thread::sleep(Duration::from_millis(5));
        }
        panic!("no failure reported");
    }

    #[test]
    fn failures_lead_with_their_number() {
        let line = 7;
        assert_eq!(err!(E999, "x {} {line}", 1), "E999: x 1 7");

        struct Boom;
        impl Backend for Boom {
            fn scan(&mut self, _: &Sink) -> Vec<Seen> {
                Vec::new()
            }
            fn run(&mut self, _: Cmd, _: &Sink) -> Result<(), String> {
                panic!("boom")
            }
        }
        assert_eq!(failure(Box::new(Boom), Cmd::Probe(Kind::Yp3)), "E901: internal error: boom");

        let mut sim = sim::Sim::new(1000.0, None);
        sim.fail = true;
        let e = failure(Box::new(sim), Cmd::Flash { kind: Kind::Yp3, package: "missing.tar.xz".into() });
        assert_eq!(e, "E315: sector 0x047000: verify failed (crc 3fa2e1c9, want 8d1c2f70)");
    }
}
