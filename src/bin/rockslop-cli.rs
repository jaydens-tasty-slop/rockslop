//! rockslop-cli: the same flasher, minus the theatrics.
//!
//!   rockslop-cli status                   players on USB
//!   rockslop-cli check PACKAGE            verify a package offline
//!   rockslop-cli probe yp3|ts17           identify a player in download mode
//!   rockslop-cli flash PACKAGE            back up, flash, checksum, sync card
//!   rockslop-cli restore yp3|ts17 [IMAGE] return to stock (newest backup)
//!   rockslop-cli backups [yp3|ts17]       list firmware backups
//!   rockslop-cli card                     where Rockbox's USB disk is mounted
//!
//! Options: --yes (don't ask), --wait SECS (how long to wait for the
//! player, default 300), --demo (a simulated player; touches nothing).

use rockslop::device::sdcard::{self, Look};
use rockslop::device::{backup, real::Real, sim::Sim, Cmd, Event, Hub, Kind, Lvl, Mode, Phase};
use rockslop::err;
use rockslop::pkg::Package;
use std::io::{IsTerminal, Write};
use std::path::{Path, PathBuf};
use std::process::ExitCode;
use std::time::{Duration, Instant};

struct Opts {
    yes: bool,
    wait: Duration,
    demo: bool,
}

fn usage() -> ExitCode {
    eprintln!(
        "usage: rockslop-cli status | check PACKAGE | probe yp3|ts17 | flash PACKAGE |\n       \
         restore yp3|ts17 [IMAGE] | backups [yp3|ts17] | card\n\
         options: --yes  --wait SECS  --demo  --version"
    );
    ExitCode::from(2)
}

fn kind_arg(s: Option<&String>) -> Option<Kind> {
    match s.map(|s| s.to_ascii_lowercase()).as_deref() {
        Some("yp3") => Some(Kind::Yp3),
        Some("ts17") => Some(Kind::Ts17),
        _ => None,
    }
}

fn main() -> ExitCode {
    let mut args: Vec<String> = std::env::args().skip(1).collect();
    let mut o = Opts { yes: false, wait: Duration::from_secs(300), demo: false };
    let mut i = 0;
    while i < args.len() {
        match args[i].as_str() {
            "--yes" | "-y" => {
                o.yes = true;
                args.remove(i);
            }
            "--demo" => {
                o.demo = true;
                args.remove(i);
            }
            "--wait" if i + 1 < args.len() => {
                let Ok(s) = args[i + 1].parse::<u64>() else { return usage() };
                o.wait = Duration::from_secs(s);
                args.drain(i..i + 2);
            }
            "-h" | "--help" => return usage(),
            "-V" | "--version" => {
                println!("rockslop-cli {}", rockslop::VERSION);
                return ExitCode::SUCCESS;
            }
            _ => i += 1,
        }
    }
    // the log, for the commands that talk to a player or check a package
    if !o.demo && matches!(args.first().map(String::as_str), Some("probe" | "flash" | "restore" | "check")) {
        rockslop::log::open(&format!("rockslop-cli {}", args.join(" ")));
    }
    let r = match args.first().map(String::as_str) {
        Some("status") => status(&o),
        Some("check") if args.len() == 2 => check(Path::new(&args[1])).map(|_| ()),
        Some("probe") => match kind_arg(args.get(1)) {
            Some(k) => probe(&o, k),
            None => return usage(),
        },
        Some("flash") if args.len() == 2 => flash(&o, Path::new(&args[1])),
        Some("restore") => match kind_arg(args.get(1)) {
            Some(k) => restore(&o, k, args.get(2).map(PathBuf::from)),
            None => return usage(),
        },
        Some("card") => {
            card();
            Ok(())
        }
        Some("backups") => {
            backups(kind_arg(args.get(1)));
            Ok(())
        }
        _ => return usage(),
    };
    match r {
        Ok(()) => ExitCode::SUCCESS,
        Err(e) => {
            // the error's number leads, as in the GUI
            eprintln!("{e}");
            if rockslop::log::enabled() {
                rockslop::log::line("FAIL", &e);
                eprintln!("log: {}", rockslop::log::path().display());
            }
            ExitCode::FAILURE
        }
    }
}

fn hub(o: &Opts) -> Hub {
    if o.demo {
        Hub::start(Box::new(Sim::new(4.0, None)))
    } else {
        Hub::start(Box::new(Real::new()))
    }
}

fn mode_name(m: Mode) -> &'static str {
    match m {
        Mode::Rom => "download mode",
        Mode::Stock => "stock firmware",
        Mode::Rockbox => "Rockbox",
    }
}

fn status(o: &Opts) -> Result<(), String> {
    let mut h = hub(o);
    let t0 = Instant::now();
    // the worker scans every 250 ms; give it a moment
    while t0.elapsed() < Duration::from_millis(700) {
        h.poll();
        std::thread::sleep(Duration::from_millis(50));
    }
    if h.seen.is_empty() {
        println!("no player connected");
    }
    for s in &h.seen {
        println!("{:5} {}{}", s.kind.name(), mode_name(s.mode), if s.access { "" } else { " (no permission to open it)" });
    }
    Ok(())
}

fn check(path: &Path) -> Result<Package, String> {
    let mut p = Package::open(path)?;
    let mut bar = Bar::new();
    p.verify(|f, _| bar.set("checking", f))?;
    bar.done();
    println!("target   {}", p.get("target").unwrap_or("?"));
    if let Some(v) = p.get("version") {
        println!("version  {v}");
    }
    println!("flash    {} bytes in {} image(s)", p.flash_bytes(), p.flash.len());
    println!("card     {} files, {} bytes", p.files.len(), p.card_bytes());
    println!("sha256   all members match SHA256SUMS");
    Ok(p)
}

fn confirm(o: &Opts, what: &str) -> Result<(), String> {
    if o.yes {
        return Ok(());
    }
    if !std::io::stdin().is_terminal() {
        return Err(err!(E802, "{what}: pass --yes to confirm when not on a terminal"));
    }
    print!("{what}? [y/N] ");
    let _ = std::io::stdout().flush();
    let mut s = String::new();
    std::io::stdin().read_line(&mut s).map_err(|e| err!(E803, "{e}"))?;
    if matches!(s.trim(), "y" | "Y" | "yes") {
        Ok(())
    } else {
        Err(err!(E801, "cancelled"))
    }
}

/// Wait for `kind` in download mode, explaining how to get it there and
/// nudging a TS17 out of stock firmware.
fn wait_rom(o: &Opts, h: &mut Hub, kind: Kind) -> Result<(), String> {
    h.send(Cmd::Watch(Some(kind)));
    let t0 = Instant::now();
    let mut said: Option<Option<(Mode, bool)>> = None;
    let mut kicked = false;
    loop {
        for e in h.poll() {
            match e {
                Event::Log(l, s) => log(l, &s),
                Event::Failed(e) => return Err(e),
                _ => {}
            }
        }
        let s = h.find(kind).map(|s| (s.mode, s.access));
        if said != Some(s) {
            said = Some(s);
            match (kind, s) {
                (_, Some((Mode::Rom, true))) => {
                    println!("{} found in download mode", kind.name());
                    return Ok(());
                }
                (_, Some((_, false))) => {
                    return Err(err!(E202, "{} is connected but this user may not open it: {}", kind.name(), permission_hint()))
                }
                (Kind::Yp3, None) => println!(
                    "waiting for the YP3: unplug it and power it off (hold the centre key), then hold Down\n  (bottom of the wheel) while connecting USB; release once it is found.\n  (Or hold Down and tap RESET.)"
                ),
                (Kind::Yp3, Some((m, _))) => {
                    println!("the YP3 is running {}: unplug it, power it off (hold the centre key), hold Down, and reconnect", mode_name(m))
                }
                (Kind::Ts17, None) => println!("waiting for the TS17: connect it over USB, switched on"),
                (Kind::Ts17, Some((Mode::Stock, _))) => println!("stock firmware: asking it to reboot into download mode"),
                (Kind::Ts17, Some((m, _))) => {
                    println!(
                        "the TS17 is running {}: unplug it and switch it off, then hold POWER about 4 s while connecting.\n  (Or in Rockbox: System > Debug > Reboot to USB download mode.)",
                        mode_name(m)
                    )
                }
            }
        }
        if kind == Kind::Ts17 && s.map(|x| x.0) == Some(Mode::Stock) && !kicked && !h.busy {
            kicked = true;
            h.send(Cmd::Kick(kind));
        }
        if t0.elapsed() > o.wait {
            return Err(err!(E201, "timed out waiting for the player"));
        }
        std::thread::sleep(Duration::from_millis(100));
    }
}

fn permission_hint() -> &'static str {
    if cfg!(target_os = "linux") {
        "install the udev rule from the README, or run as root"
    } else if cfg!(target_os = "macos") {
        "run with sudo"
    } else {
        "install the WinUSB driver for it with Zadig"
    }
}

fn probe(o: &Opts, kind: Kind) -> Result<(), String> {
    let mut h = hub(o);
    wait_rom(o, &mut h, kind)?;
    h.send(Cmd::Probe(kind));
    run(&mut h, false).map(|_| ())
}

fn flash(o: &Opts, path: &Path) -> Result<(), String> {
    let pkg = check(path)?;
    let kind = match pkg.get("target") {
        Some("yp3") => Kind::Yp3,
        Some("ts17") => Kind::Ts17,
        t => return Err(err!(E151, "package target {t:?} is not a supported player")),
    };
    if !pkg.files.is_empty() {
        println!("put a microSD card in the player before flashing: Rockbox needs it");
    }
    let mut h = hub(o);
    wait_rom(o, &mut h, kind)?;
    confirm(o, &format!("Back up and flash {} with {}", kind.name(), path.display()))?;
    h.send(Cmd::Flash { kind, package: path.to_path_buf() });
    run(&mut h, true).map(|_| ())
}

fn restore(o: &Opts, kind: Kind, image: Option<PathBuf>) -> Result<(), String> {
    let image = match image {
        Some(p) => p,
        None => backup::stock_for(kind, None)
            .map(|b| b.path)
            .ok_or_else(|| err!(E420, "no stock {} backup on file in {}", kind.name(), backup::dir().display()))?,
    };
    println!("image    {}", image.display());
    let mut h = hub(o);
    wait_rom(o, &mut h, kind)?;
    confirm(o, &format!("Restore {} from this image", kind.name()))?;
    h.send(Cmd::Restore { kind, image });
    run(&mut h, true).map(|_| ())
}

fn backups(kind: Option<Kind>) {
    println!("{}", backup::dir().display());
    let mut any = false;
    for k in [Kind::Yp3, Kind::Ts17] {
        if kind.is_some_and(|x| x != k) {
            continue;
        }
        for b in backup::list(k) {
            any = true;
            let name = b.path.file_name().map(|n| n.to_string_lossy().to_string()).unwrap_or_default();
            println!("  {:5} unit {:08x}  {}  {:5}  {name}", k.name(), b.fingerprint, b.stamp, if b.stock { "stock" } else { "other" });
        }
    }
    if !any {
        println!("  (none)");
    }
}

fn card() {
    match sdcard::look() {
        Look::Absent => println!("no Rockbox USB disk on this computer"),
        Look::NoMedia => println!("Rockbox's USB disk is there, but holds no microSD card"),
        Look::Unmounted(v) => println!("{v}: not mounted"),
        Look::Ready(c) => {
            let free = sdcard::free_space(&c).map(|b| format!(", {} MiB free", b >> 20)).unwrap_or_default();
            println!("{} on {} (disk {}){free}", c.volume, c.mount.display(), c.disk);
        }
    }
}

fn log(l: Lvl, s: &str) {
    let tag = match l {
        Lvl::Info => "  ",
        Lvl::Ok => "  ",
        Lvl::Warn => "! ",
        Lvl::Err => "x ",
        Lvl::Dim => return,
    };
    println!("{tag}{s}");
}

/// Print a job's events until it finishes.
fn run(h: &mut Hub, job: bool) -> Result<(), String> {
    let mut bar = Bar::new();
    let mut phase = None;
    loop {
        for e in h.poll() {
            match e {
                Event::Phase(p) => {
                    bar.done();
                    if p != Phase::Done {
                        println!("[{}]", p.label().to_lowercase());
                    }
                    phase = Some(p);
                }
                Event::Progress(f) => bar.set(phase.map(|p| p.label()).unwrap_or(""), f),
                Event::Log(l, s) => {
                    bar.done();
                    log(l, &s);
                }
                Event::Probe(k, v, ok) => {
                    bar.done();
                    let mark = match ok {
                        Some(true) => "ok",
                        Some(false) => "FAIL",
                        None => "",
                    };
                    println!("  {k:<10} {v:<44} {mark}");
                }
                Event::NeedCard(true) => {
                    bar.done();
                    println!("! no microSD card in the player: insert one, then press RESET");
                }
                Event::Ready if !job => return Ok(()),
                Event::Done(out) => {
                    bar.done();
                    if let Some(b) = out.backup {
                        println!("backup   {}", b.display());
                    }
                    if out.card {
                        println!("card     {} files, {} bytes, read back OK", out.files, out.bytes);
                    }
                    println!("{}", if out.restored { "done: stock firmware restored" } else { "done: unplug the player" });
                    return Ok(());
                }
                Event::Failed(e) => {
                    bar.done();
                    return Err(e);
                }
                _ => {}
            }
        }
        std::thread::sleep(Duration::from_millis(50));
    }
}

/// A one-line progress bar on a terminal; occasional lines otherwise.
struct Bar {
    tty: bool,
    shown: bool,
    last: f32,
}

impl Bar {
    fn new() -> Bar {
        Bar { tty: std::io::stdout().is_terminal(), shown: false, last: -1.0 }
    }
    fn set(&mut self, what: &str, f: f32) {
        let f = f.clamp(0.0, 1.0);
        if self.tty {
            let n = (f * 30.0) as usize;
            print!("\r  {:<9} [{}{}] {:3.0}%", what.to_lowercase(), "#".repeat(n), ".".repeat(30 - n), f * 100.0);
            let _ = std::io::stdout().flush();
            self.shown = true;
        } else if f - self.last >= 0.25 || (f >= 1.0 && self.last < 1.0) {
            println!("  {} {:3.0}%", what.to_lowercase(), f * 100.0);
            self.last = f;
        }
    }
    fn done(&mut self) {
        if self.shown {
            println!();
            self.shown = false;
        }
        self.last = -1.0;
    }
}
