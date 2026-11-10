//! The flasher's log: every command, phase, probe result, message and
//! failure the device layer reports, and each package check, appended to
//! `rockslop.log` in the data directory (beside `backups/`). A file past
//! 1 MiB is moved to `rockslop.log.1` when the next run starts. Off until a
//! front end calls `open` (the simulator and the tests leave it off).

use crate::device::{backup, Event, Lvl};
use std::io::{BufRead, Write};
use std::path::PathBuf;
use std::sync::Mutex;

static FILE: Mutex<Option<std::fs::File>> = Mutex::new(None);

const ROTATE: u64 = 1 << 20;

pub fn path() -> PathBuf {
    let b = backup::dir();
    b.parent().map(|p| p.to_path_buf()).unwrap_or(b).join("rockslop.log")
}

/// Start logging for this run; `what` heads its section (the command line).
/// Without a writable data directory there is no log, and nothing fails.
pub fn open(what: &str) {
    let p = path();
    if let Some(dir) = p.parent() {
        let _ = std::fs::create_dir_all(dir);
    }
    if std::fs::metadata(&p).map(|m| m.len() > ROTATE).unwrap_or(false) {
        let _ = std::fs::rename(&p, p.with_extension("log.1"));
    }
    let Ok(f) = std::fs::OpenOptions::new().create(true).append(true).open(&p) else { return };
    *FILE.lock().unwrap_or_else(|e| e.into_inner()) = Some(f);
    line(
        "START",
        &format!("rockslop {} ({} {}): {what}", crate::VERSION, std::env::consts::OS, std::env::consts::ARCH),
    );
}

pub fn enabled() -> bool {
    FILE.lock().map(|f| f.is_some()).unwrap_or(false)
}

/// One line, stamped with the UTC time.
pub fn line(tag: &str, msg: &str) {
    let mut f = FILE.lock().unwrap_or_else(|e| e.into_inner());
    if let Some(f) = f.as_mut() {
        // one message, one line: a multi-line error keeps its lines together
        let msg = msg.replace('\n', " | ");
        let _ = writeln!(f, "{} {tag:<5} {msg}", backup::stamp_now());
    }
}

/// What the device layer reports, as it reports it. Sector maps, hex
/// rows, progress and per-file traffic are left out: they are the UI's.
pub fn event(e: &Event) {
    if !enabled() {
        return;
    }
    match e {
        Event::Log(l, s) => {
            let tag = match l {
                Lvl::Info => "INFO",
                Lvl::Ok => "OK",
                Lvl::Warn => "WARN",
                Lvl::Dim => "DIM",
                Lvl::Err => "ERR",
            };
            line(tag, s)
        }
        Event::Probe(k, v, ok) => {
            let verdict = match ok {
                Some(true) => "pass",
                Some(false) => "FAIL",
                None => "-",
            };
            line("PROBE", &format!("{k}: {v} [{verdict}]"))
        }
        Event::Phase(p) => line("PHASE", &format!("{p:?}")),
        Event::Presence(seen) => line("USB", &format!("{seen:?}")),
        Event::NeedCard(b) => line("CARD", if *b { "no card in the player" } else { "card present" }),
        Event::Ready => line("READY", ""),
        Event::Done(o) => line("DONE", &format!("{o:?}")),
        Event::Failed(e) => line("FAIL", e),
        Event::Progress(_) | Event::Layout { .. } | Event::Sector(..) | Event::Hex(..) | Event::File(..) | Event::ReadBack(..) => {}
    }
}

/// The last `n` lines of the log, for a bug report (empty without one).
pub fn tail(n: usize) -> Vec<String> {
    if let Some(f) = FILE.lock().unwrap_or_else(|e| e.into_inner()).as_mut() {
        let _ = f.flush();
    }
    let Ok(f) = std::fs::File::open(path()) else { return Vec::new() };
    let mut v: std::collections::VecDeque<String> = std::collections::VecDeque::with_capacity(n + 1);
    for l in std::io::BufReader::new(f).lines().map_while(Result::ok) {
        if v.len() == n {
            v.pop_front();
        }
        v.push_back(l);
    }
    v.into()
}
