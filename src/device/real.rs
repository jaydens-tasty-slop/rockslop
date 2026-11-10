//! The real backend: finds players on USB and runs flash / restore jobs.
//!
//! A flash is: identify the player in download mode, back up its whole
//! flash (skipped when an identical backup is on file), check the package's
//! expectations against it, write only the sectors that change (never the
//! bootloader or the per-unit sectors), checksum, reboot into Rockbox, then
//! copy the card files over Rockbox's own USB disk, read them back, eject.

use super::backup::{self, Backup};
use super::rdm::{Rdm, XIP_RUN};
use super::sdcard::{self, Look};
use super::ts17::{self, Ts17};
use super::usb;
use super::yp3::{self, crc32, Yp3};
use super::*;
use crate::err;
use crate::pkg::{hex, sha256, Package};
use std::collections::HashMap;
use std::io::Write;
use std::time::Instant;

const ROCKBOX_VID: u16 = 0x1209;

fn ident(kind: Kind) -> (u16, u16, u16, u16) {
    match kind {
        Kind::Yp3 => (yp3::VID, yp3::PID_ROM, yp3::PID_STOCK, yp3::PID_ROCKBOX),
        Kind::Ts17 => (ts17::VID, ts17::PID_ADFU, ts17::PID_STOCK, ts17::PID_ROCKBOX),
    }
}

struct Geometry {
    size: u32,
    sector: u32,
    writable: (u32, u32),
    /// per-unit sectors that fingerprint a unit
    unit: (u32, u32),
}

fn geometry(kind: Kind) -> Geometry {
    match kind {
        Kind::Yp3 => Geometry { size: yp3::FLASH_SIZE, sector: yp3::SECTOR, writable: yp3::WRITABLE, unit: (0x1FC000, 0x200000) },
        Kind::Ts17 => Geometry { size: ts17::FLASH_SIZE, sector: ts17::SECTOR, writable: ts17::WRITABLE, unit: (0x332000, 0x337000) },
    }
}

/// One session with a player in download mode.
enum Dev {
    Yp3(Yp3),
    Ts17(Ts17),
}

/// Courtesy lock shared with the rockbox-yp3 dev tools, so a flash never
/// interleaves with their traffic.
struct DevLock(#[allow(dead_code)] Option<std::fs::File>);

#[cfg(unix)]
fn dev_lock(kind: Kind) -> Result<DevLock, String> {
    use std::os::unix::io::AsRawFd;
    let p = format!("/tmp/{}dev.lock", kind.id());
    let Ok(f) = std::fs::OpenOptions::new().write(true).open(&p) else { return Ok(DevLock(None)) };
    if unsafe { libc::flock(f.as_raw_fd(), libc::LOCK_EX | libc::LOCK_NB) } != 0 {
        return Err(err!(E205, "another tool holds the player ({p})"));
    }
    Ok(DevLock(Some(f)))
}

#[cfg(not(unix))]
fn dev_lock(_kind: Kind) -> Result<DevLock, String> {
    Ok(DevLock(None))
}

pub struct Real {
    guarded: HashMap<Kind, Instant>,
    rdm: Rdm,
}

impl Real {
    pub fn new() -> Real {
        Real { guarded: HashMap::new(), rdm: Rdm::new() }
    }
}

fn seen_now() -> Vec<Seen> {
    let mut out = Vec::new();
    for (v, p, ok, _) in usb::list() {
        for kind in [Kind::Yp3, Kind::Ts17] {
            let (vid, rom, stock, rb) = ident(kind);
            let mode = if v == vid && p == rom {
                Mode::Rom
            } else if v == vid && p == stock {
                Mode::Stock
            } else if v == ROCKBOX_VID && p == rb {
                Mode::Rockbox
            } else {
                continue;
            };
            let s = Seen { kind, mode, access: ok || mode == Mode::Rockbox };
            if !out.contains(&s) {
                out.push(s);
            }
        }
    }
    out
}

/// Wait for the player to show up in one of `modes` (stably).
fn wait_mode(kind: Kind, modes: &[Mode], timeout: Duration, mut tick: impl FnMut(f32)) -> Option<Mode> {
    let t0 = Instant::now();
    let mut stable = (None, 0);
    while t0.elapsed() < timeout {
        let m = seen_now().into_iter().find(|s| s.kind == kind && modes.contains(&s.mode)).map(|s| s.mode);
        stable = if m.is_some() && m == stable.0 { (m, stable.1 + 1) } else { (m, 1) };
        if m.is_some() && stable.1 >= 3 {
            return m;
        }
        tick(t0.elapsed().as_secs_f32());
        std::thread::sleep(Duration::from_millis(150));
    }
    None
}

fn mv_text(mv: Option<u32>) -> String {
    mv.map(|m| format!("{}.{:02} V", m / 1000, m % 1000 / 10)).unwrap_or_else(|| "--".into())
}

impl Backend for Real {
    fn scan(&mut self, ev: &Sink) -> Vec<Seen> {
        let now = seen_now();
        // Nothing stops the charger while a player sits in download mode:
        // look at it every minute.
        for s in &now {
            if s.mode != Mode::Rom || !s.access {
                continue;
            }
            if self.guarded.get(&s.kind).map(|t| t.elapsed() < Duration::from_secs(60)) == Some(true) {
                continue;
            }
            self.guarded.insert(s.kind, Instant::now());
            let Ok(_lock) = dev_lock(s.kind) else { continue };
            let r = match s.kind {
                Kind::Yp3 => Yp3::open().and_then(|mut y| y.battery_guard().map(|(mv, c, st)| (Some(mv), c, st))),
                Kind::Ts17 => Ts17::open().and_then(|mut t| {
                    t.install()?;
                    t.charger_guard()
                }),
            };
            if let Ok((mv, charging, stopped)) = r {
                let what = if stopped {
                    "CHARGER STOPPED"
                } else if charging {
                    "CHARGING"
                } else {
                    "CHARGER OFF"
                };
                ev.probe("CELL", format!("{}  {what}", mv_text(mv)), Some(true));
            }
        }
        now
    }

    fn run(&mut self, cmd: Cmd, ev: &Sink) -> Result<(), String> {
        match cmd {
            Cmd::Watch(_) => Ok(()),
            Cmd::Kick(kind) => {
                let _l = dev_lock(kind)?;
                kick(kind, ev)
            }
            Cmd::Probe(kind) => {
                let _l = dev_lock(kind)?;
                ev.phase(Phase::Connect);
                let mut d = self.connect(kind, ev)?;
                self.guarded.insert(kind, Instant::now());
                self.probe(&mut d, ev)?;
                ev.send(Event::Ready);
                Ok(())
            }
            Cmd::Flash { kind, package } => {
                let _l = dev_lock(kind)?;
                let r = self.flash(kind, &package, ev);
                if r.is_err() {
                    charger_safe(kind);
                }
                r
            }
            Cmd::Restore { kind, image } => {
                let _l = dev_lock(kind)?;
                let r = self.restore(kind, &image, ev);
                if r.is_err() {
                    charger_safe(kind);
                }
                r
            }
        }
    }
}

/// After a failure with the player still in download mode, make sure it is
/// not left charging unsupervised.
fn charger_safe(kind: Kind) {
    match kind {
        Kind::Yp3 => {
            if let Ok(mut y) = Yp3::open() {
                let _ = y.charge_off();
            }
        }
        Kind::Ts17 => {
            if let Ok(mut t) = Ts17::open() {
                let _ = t.install().and_then(|_| t.charger_guard());
            }
        }
    }
}

fn kick(kind: Kind, ev: &Sink) -> Result<(), String> {
    match kind {
        Kind::Ts17 => {
            ev.info("stock firmware on the bus: asking it for download mode");
            let id = ts17::kick_stock()?;
            ev.ok(format!("{id} acknowledged 0xCB: rebooting into ADFU"));
            if wait_mode(kind, &[Mode::Rom], Duration::from_secs(30), |_| {}).is_none() {
                return Err(err!(E251, "the player did not come back in download mode"));
            }
            ev.send(Event::Ready);
            Ok(())
        }
        Kind::Yp3 => Err(err!(E252, "the YP3 enters download mode by hand: hold Down while connecting USB")),
    }
}

impl Real {
    fn connect(&mut self, kind: Kind, ev: &Sink) -> Result<Dev, String> {
        let (vid, rom, _, _) = ident(kind);
        ev.info(format!("attaching to {vid:04x}:{rom:04x}"));
        Ok(match kind {
            Kind::Yp3 => Dev::Yp3(Yp3::open()?),
            Kind::Ts17 => Dev::Ts17(Ts17::open()?),
        })
    }

    /// Identity checks; every one must pass before anything is written.
    fn probe(&mut self, d: &mut Dev, ev: &Sink) -> Result<(), String> {
        ev.phase(Phase::Probe);
        // a failed check is marked on the panel; its value goes in the error
        let fail = |k: &str, v: String| -> String {
            ev.probe(k, v.clone(), Some(false));
            v
        };
        let cpu_fail = |id: u32| err!(E262, "CPU check failed: {}", fail("CPU", format!("CPUID {id:08x}")));
        let low_battery = |mv: u32| err!(E270, "battery too low to flash safely ({})", mv_text(Some(mv)));
        match d {
            Dev::Yp3(y) => {
                let inq = y.inquiry()?;
                if !inq.contains("SMTLINK") {
                    return Err(err!(E260, "USB check failed: {}", fail("USB", format!("301a:2800 {inq}"))));
                }
                ev.probe("USB", format!("301a:2800 {inq}"), Some(true));
                let serial = y.serial.clone().unwrap_or_default();
                ev.probe(
                    "CHIP",
                    format!("SL6801 (serial {serial})"),
                    Some(serial.starts_with("2020111100")).filter(|_| !serial.is_empty()),
                );
                let info = y.info()?;
                ev.probe("PAYLOAD", "svc @0x806000 ONLINE", Some(true));
                if info[8] != yp3::CPUID {
                    return Err(cpu_fail(info[8]));
                }
                ev.probe("CPU", format!("CORTEX-M4 r0p1 (0x{:08X})", info[8]), Some(true));
                let j = y.jedec()?;
                if j != yp3::JEDEC {
                    let v = format!("JEDEC {:02x} {:02x} {:02x}", j[0], j[1], j[2]);
                    return Err(err!(E263, "FLASH check failed: {}", fail("FLASH", v)));
                }
                ev.probe("FLASH", format!("P25Q16H 2 MiB ({:02x} {:02x} {:02x})", j[0], j[1], j[2]), Some(true));
                let (mv, charging, stopped) = y.battery_guard()?;
                let what = if stopped {
                    "CHARGER STOPPED"
                } else if charging {
                    "CHARGING"
                } else {
                    "CHARGER OFF"
                };
                ev.probe("CELL", format!("{}  {what}", mv_text(Some(mv))), Some(mv > 3300));
                if mv < 3300 {
                    return Err(low_battery(mv));
                }
            }
            Dev::Ts17(t) => {
                let chip = t.identify()?;
                ev.probe("USB", "10d6:10d6 ADFU", Some(true));
                if chip != ts17::CHIP {
                    return Err(err!(E261, "CHIP check failed: {}", fail("CHIP", format!("0x{chip:04x}"))));
                }
                ev.probe("CHIP", format!("ACTIONS LARK 0x{chip:04X}"), Some(true));
                let (_, vtor) = t.install()?;
                ev.probe("HOOK", format!("gadget bus @0x01018000, VTOR {vtor:08x}"), Some(true));
                let cpuid = t.read32(0xE000_ED00)?;
                if cpuid != ts17::CPUID {
                    return Err(cpu_fail(cpuid));
                }
                ev.probe("CPU", format!("CORTEX-M4F r0p1 (0x{cpuid:08X})"), Some(true));
                t.nor_setup(false)?;
                let head = t.read_chunk(0)?;
                if &head[0x1000..0x1004] != b"ACPT" {
                    return Err(err!(E264, "NOR check failed: {}", fail("NOR", "no ACPT partition table at 0x1000".into())));
                }
                // the firmware goes in fw0_sys: refuse a player whose table
                // puts it anywhere else (another model, another layout)
                match ts17::partition(&head[0x1000..], "fw0_sys") {
                    Some((off, size)) if (off, off + size) == ts17::WRITABLE => {}
                    p => {
                        let v = match p {
                            Some((o, s)) => format!("fw0_sys at 0x{o:X} +0x{s:X}, not 0x{:X} +0x{:X}", ts17::SYS, ts17::WRITABLE.1 - ts17::SYS),
                            None => "no fw0_sys in the partition table".into(),
                        };
                        return Err(err!(E265, "NOR check failed: {}", fail("NOR", v)));
                    }
                }
                ev.probe("NOR", "4 MiB, fw0_sys @ 0x14000", Some(true));
                let (mv, charging, stopped) = t.charger_guard()?;
                let what = if stopped {
                    "CHARGER STOPPED"
                } else if charging {
                    "CHARGER GUARDED"
                } else {
                    "CHARGER OFF"
                };
                ev.probe("CELL", format!("{}  {what}", mv_text(mv)), Some(mv.map(|m| m > 3300).unwrap_or(true)));
                if let Some(m) = mv.filter(|&m| m < 3300) {
                    return Err(low_battery(m));
                }
            }
        }
        Ok(())
    }

    /// The whole flash, as stored (TS17: raw cells). Reuses sectors from
    /// earlier backups when the player's own CRCs say they are identical.
    fn dump(&mut self, d: &mut Dev, ev: &Sink) -> Result<Vec<u8>, String> {
        ev.phase(Phase::Backup);
        match d {
            Dev::Yp3(y) => {
                let g = geometry(Kind::Yp3);
                let n = g.size / g.sector;
                ev.info("sector CRC32 scan on the player");
                let crcs = y.flash_crcs(0, n)?;
                // known sectors: erased, or in a backup already on disk
                let mut known: HashMap<u32, Vec<u8>> = HashMap::new();
                let blank = vec![0xffu8; g.sector as usize];
                known.insert(crc32(&blank), blank);
                for b in backup::list(Kind::Yp3).iter().take(4) {
                    if let Ok(img) = std::fs::read(&b.path) {
                        for s in img.chunks(g.sector as usize) {
                            known.entry(crc32(s)).or_insert_with(|| s.to_vec());
                        }
                    }
                }
                let mut img = Vec::with_capacity(g.size as usize);
                let mut read = 0;
                ev.info(format!("dumping {} KiB", g.size / 1024));
                for i in 0..n {
                    let addr = i * g.sector;
                    ev.sector(i, Sect::Read);
                    let data = match known.get(&crcs[i as usize]) {
                        Some(s) => s.clone(),
                        None => {
                            read += 1;
                            let mut s = y.read_sector(addr)?;
                            if crc32(&s) != crcs[i as usize] {
                                s = y.read_sector(addr)?;
                                if crc32(&s) != crcs[i as usize] {
                                    ev.sector(i, Sect::Bad);
                                    return Err(err!(E401, "sector 0x{addr:06x} reads inconsistently"));
                                }
                            }
                            s
                        }
                    };
                    hex_feed(ev, addr, &data[..256], 4);
                    img.extend_from_slice(&data);
                    let w = addr >= g.writable.0 && addr < g.writable.1;
                    ev.sector(i, if w { Sect::Same } else { Sect::Protected });
                    ev.progress((i + 1) as f32 / n as f32);
                }
                ev.ok(format!("{n} sectors captured ({read} read, {} already known); CRC32 verified", n - read));
                Ok(img)
            }
            Dev::Ts17(t) => {
                let g = geometry(Kind::Ts17);
                t.nor_setup(true)?;
                let chunk = Ts17::chunk_size();
                let per = chunk / g.sector;
                let mut img = Vec::with_capacity(g.size as usize);
                ev.info(format!("dumping {} KiB of raw NOR cells", g.size / 1024));
                for c in 0..g.size / chunk {
                    let off = c * chunk;
                    for k in 0..per {
                        ev.sector(c * per + k, Sect::Read);
                    }
                    let data = t.read_chunk(off)?;
                    hex_feed(ev, off, &data[..512], 2);
                    img.extend_from_slice(&data);
                    for k in 0..per {
                        let a = off + k * g.sector;
                        let w = a >= g.writable.0 && a < g.writable.1;
                        ev.sector(c * per + k, if w { Sect::Same } else { Sect::Protected });
                    }
                    ev.progress((c + 1) as f32 * 0.5 / (g.size / chunk) as f32);
                }
                ev.info("second pass: comparing");
                for c in 0..g.size / chunk {
                    let off = c * chunk;
                    if t.read_chunk(off)? != img[off as usize..(off + chunk) as usize] {
                        return Err(err!(E402, "flash at 0x{off:06x} reads inconsistently"));
                    }
                    ev.progress(0.5 + (c + 1) as f32 * 0.5 / (g.size / chunk) as f32);
                }
                ev.ok(format!("{} sectors captured; two reads agree", g.size / g.sector));
                Ok(img)
            }
        }
    }

    fn fingerprint(kind: Kind, img: &[u8]) -> u32 {
        let g = geometry(kind);
        crc32(&img[g.unit.0 as usize..g.unit.1 as usize])
    }

    /// The image as firmware sees it (TS17 cells decoded where XIP runs).
    fn plain_view(&self, kind: Kind, img: &[u8]) -> Vec<u8> {
        match kind {
            Kind::Yp3 => img.to_vec(),
            Kind::Ts17 => {
                let (a, b) = ts17::WRITABLE;
                self.rdm.decode(&img[a as usize..b as usize], XIP_RUN)
            }
        }
    }

    fn keep_backup(&mut self, kind: Kind, img: &[u8], ev: &Sink) -> Result<Backup, String> {
        let fp = Self::fingerprint(kind, img);
        let stock = !backup::has_rockbox(&self.plain_view(kind, img)) && !backup::has_rockbox(img);
        ev.probe("UNIT", format!("fingerprint {fp:08x}"), None);
        ev.probe("FIRMWARE", if stock { "STOCK" } else { "ROCKBOX / MODIFIED" }, None);
        if let Some(b) = backup::find_same(kind, fp, img) {
            ev.ok(format!("identical backup already on file: {}", b.path.display()));
            return Ok(b);
        }
        let b = backup::save(kind, fp, stock, img)?;
        ev.ok(format!("backup -> {}", b.path.display()));
        Ok(b)
    }

    /// The target image: the current flash with the package's segments laid
    /// over it. Refuses anything outside the writable window.
    fn compose(&self, kind: Kind, cur: &[u8], segs: &[(crate::pkg::Seg, Vec<u8>)]) -> Result<Vec<u8>, String> {
        let g = geometry(kind);
        let mut img = cur.to_vec();
        for (seg, data) in segs {
            let cells = match kind {
                Kind::Yp3 => data.clone(),
                Kind::Ts17 => {
                    let mut p = data.clone();
                    p.resize(p.len().div_ceil(XIP_RUN) * XIP_RUN, 0xff);
                    self.rdm.encode(&p, XIP_RUN)
                }
            };
            let start = seg.offset;
            let end = start as usize + cells.len().div_ceil(g.sector as usize) * g.sector as usize;
            if start < g.writable.0 || end > g.writable.1 as usize || start % g.sector != 0 {
                return Err(err!(
                    E153,
                    "package writes 0x{start:06x}..0x{end:06x}, outside the safe window 0x{:06x}..0x{:06x}",
                    g.writable.0,
                    g.writable.1
                ));
            }
            img[start as usize..start as usize + cells.len()].copy_from_slice(&cells);
            img[start as usize + cells.len()..end].fill(0xff);
        }
        Ok(img)
    }

    /// Write every sector of `want` that differs from `cur` (writable window
    /// only), then checksum the result against the player.
    fn program(&mut self, d: &mut Dev, kind: Kind, cur: &[u8], want: &[u8], ev: &Sink) -> Result<usize, String> {
        let g = geometry(kind);
        let ss = g.sector as usize;
        let todo: Vec<u32> = (g.writable.0 / g.sector..g.writable.1 / g.sector)
            .filter(|&i| cur[i as usize * ss..(i as usize + 1) * ss] != want[i as usize * ss..(i as usize + 1) * ss])
            .collect();
        ev.phase(Phase::Write);
        ev.info(format!("{} of {} sectors differ", todo.len(), (g.writable.1 - g.writable.0) / g.sector));
        if let Dev::Ts17(t) = d {
            t.nor_setup(true)?;
        }
        for (k, &i) in todo.iter().enumerate() {
            let addr = i * g.sector;
            let data = &want[i as usize * ss..(i as usize + 1) * ss];
            ev.sector(i, Sect::Erase);
            ev.send(Event::Hex(addr, data[..16].to_vec()));
            let r = match d {
                Dev::Yp3(y) => y.write_sector(addr, data).map(|_| ()),
                Dev::Ts17(t) => t.write_sector(addr, data),
            };
            if let Err(e) = r {
                ev.sector(i, Sect::Bad);
                return Err(e);
            }
            ev.sector(i, Sect::Good);
            ev.progress((k + 1) as f32 / todo.len().max(1) as f32);
            if k % 32 == 0 {
                ev.dim(format!("0x{addr:06x} erase / program / verify ok"));
            }
        }
        ev.phase(Phase::Check);
        let (lo, hi) = (g.writable.0 / g.sector, g.writable.1 / g.sector);
        let mismatch = |a: u32| err!(E321, "checksum mismatch at 0x{a:06x}");
        match d {
            Dev::Yp3(y) => {
                ev.info("CRC32 of every writable sector, computed on the player");
                let crcs = y.flash_crcs(lo, hi - lo)?;
                for (k, c) in crcs.iter().enumerate() {
                    let i = lo + k as u32;
                    if *c != crc32(&want[i as usize * ss..(i as usize + 1) * ss]) {
                        ev.sector(i, Sect::Bad);
                        return Err(mismatch(i * g.sector));
                    }
                    ev.progress((k + 1) as f32 / (hi - lo) as f32);
                }
            }
            Dev::Ts17(t) => {
                ev.info("reading the writable window back");
                let chunk = Ts17::chunk_size();
                let mut off = g.writable.0 & !(chunk - 1);
                while off < g.writable.1 {
                    let data = t.read_chunk(off)?;
                    for (k, s) in data.chunks(ss).enumerate() {
                        let a = off + (k * ss) as u32;
                        if a >= g.writable.0 && a < g.writable.1 && s != &want[a as usize..a as usize + ss] {
                            ev.sector(a / g.sector, Sect::Bad);
                            return Err(mismatch(a));
                        }
                    }
                    off += chunk;
                    ev.progress((off - g.writable.0) as f32 / (g.writable.1 - g.writable.0) as f32);
                }
            }
        }
        let sum = sha256(&want[g.writable.0 as usize..g.writable.1 as usize]);
        ev.ok(format!("flash checksum OK  sha256 {}..{}", &hex(&sum)[..8], &hex(&sum)[56..]));
        Ok(todo.len())
    }

    fn reboot(&mut self, d: Dev, kind: Kind, into: Mode, ev: &Sink) -> Result<bool, String> {
        ev.phase(Phase::Reboot);
        match d {
            Dev::Yp3(mut y) => y.boot()?,
            Dev::Ts17(mut t) => t.boot()?,
        }
        ev.warn(if into == Mode::Rockbox { "rebooting into Rockbox" } else { "rebooting into stock firmware" });
        std::thread::sleep(Duration::from_millis(800));
        let got = wait_mode(kind, &[into, Mode::Rom], Duration::from_secs(45), |_| {});
        match got {
            Some(Mode::Rom) => Err(err!(E331, "the player came back in download mode: the new firmware did not start")),
            Some(_) => {
                ev.ok("firmware is up");
                Ok(true)
            }
            None => Ok(false),
        }
    }

    fn flash(&mut self, kind: Kind, path: &Path, ev: &Sink) -> Result<(), String> {
        let t0 = Instant::now();
        let mut pkg = Package::open(path)?;
        if pkg.get("target") != Some(kind.id()) {
            return Err(err!(E150, "this package is for {}, not {}", pkg.get("target").unwrap_or("?"), kind.name()));
        }
        ev.phase(Phase::Connect);
        let mut d = self.connect(kind, ev)?;
        let g = geometry(kind);
        ev.send(Event::Layout { sectors: g.size / g.sector, sector_size: g.sector });
        self.probe(&mut d, ev)?;
        self.guarded.insert(kind, Instant::now());

        let cur = self.dump(&mut d, ev)?;
        let bk = self.keep_backup(kind, &cur, ev)?;

        ev.phase(Phase::Verify);
        for e in &pkg.expect {
            let (a, b) = (e.offset as usize, e.offset as usize + e.size as usize);
            if b > cur.len() || sha256(&cur[a..b]) != e.sha256 {
                ev.probe("BOOTLOADER", format!("0x{a:06x}+{:x} UNKNOWN", e.size), Some(false));
                return Err(err!(E301, "this player's flash at 0x{a:06x} is not what the package was built for; nothing was written"));
            }
            ev.probe("BOOTLOADER", format!("sha256 {}..{} KNOWN", &hex(&e.sha256)[..4], &hex(&e.sha256)[60..]), Some(true));
        }
        if pkg.files.is_empty() {
            pkg.verify(|f, _| ev.progress(f))?;
        }
        let segs = pkg.read_flash()?;
        if segs.is_empty() {
            return Err(err!(E152, "the package has no firmware image"));
        }
        let want = self.compose(kind, &cur, &segs)?;
        ev.ok(format!("{} image(s), {} KiB, sha256 verified", segs.len(), segs.iter().map(|s| s.1.len()).sum::<usize>() / 1024));

        let written = self.program(&mut d, kind, &cur, &want, ev)?;
        ev.dim(format!("{written} sectors rewritten"));

        let up = self.reboot(d, kind, Mode::Rockbox, ev)?;
        let mut out = Outcome { backup: Some(bk.path.clone()), ..Default::default() };
        if pkg.files.is_empty() {
            ev.phase(Phase::Done);
            ev.send(Event::Done(out));
            return Ok(());
        }
        if !up {
            ev.warn("Rockbox did not show up as a USB disk; the card was not updated");
            ev.phase(Phase::Done);
            ev.send(Event::Done(out));
            return Ok(());
        }
        let (files, bytes) = sync_card(kind, &pkg, ev)?;
        out.files = files;
        out.bytes = bytes;
        out.card = true;
        ev.phase(Phase::Done);
        ev.dim(format!("total {:.1} s", t0.elapsed().as_secs_f32()));
        ev.send(Event::Done(out));
        Ok(())
    }

    fn restore(&mut self, kind: Kind, image: &Path, ev: &Sink) -> Result<(), String> {
        let g = geometry(kind);
        ev.phase(Phase::Connect);
        let mut d = self.connect(kind, ev)?;
        ev.send(Event::Layout { sectors: g.size / g.sector, sector_size: g.sector });
        for i in 0..g.size / g.sector {
            let a = i * g.sector;
            if a < g.writable.0 || a >= g.writable.1 {
                ev.sector(i, Sect::Protected);
            }
        }
        self.probe(&mut d, ev)?;
        ev.phase(Phase::Verify);
        let img = backup::load(image, g.size as usize)?;
        ev.ok(format!("{} loaded, checksum OK", image.file_name().map(|n| n.to_string_lossy()).unwrap_or_default()));
        let cur = self.dump(&mut d, ev)?;
        if Self::fingerprint(kind, &cur) != Self::fingerprint(kind, &img) {
            ev.warn("backup is from another unit; its per-unit sectors are left alone");
        }
        // only the writable window comes from the backup
        let mut want = cur.clone();
        let (a, b) = (g.writable.0 as usize, g.writable.1 as usize);
        want[a..b].copy_from_slice(&img[a..b]);
        self.program(&mut d, kind, &cur, &want, ev)?;
        self.reboot(d, kind, Mode::Stock, ev)?;
        ev.phase(Phase::Done);
        ev.send(Event::Done(Outcome { restored: true, ..Default::default() }));
        Ok(())
    }
}

/// Copy the package's card tree onto the player's card and read it back.
fn sync_card(kind: Kind, pkg: &Package, ev: &Sink) -> Result<(usize, u64), String> {
    ev.phase(Phase::Sync);
    ev.info("waiting for Rockbox's USB disk");
    let mut asked = false;
    let mut hinted = false;
    let mut card = sdcard::wait_card(Duration::from_secs(600), |look, el| {
        // Rockbox is up but offers no disk: the card's settings can keep
        // it to charging (USB Mode, on both players), or it missed the
        // connect
        if matches!(look, Look::Absent) && el > 25.0 && !hinted {
            hinted = true;
            ev.warn("no USB disk from Rockbox yet: if it is set to charge only, unplug the player, hold any key and plug it back in (or set Settings > General Settings > System > USB > USB Mode to Mass Storage)");
        }
        let none = matches!(look, Look::NoMedia);
        if none != asked {
            asked = none;
            ev.send(Event::NeedCard(none));
            if none {
                ev.warn("no microSD card in the player: insert one, then press RESET");
            }
        }
        true
    })?;
    if asked {
        ev.send(Event::NeedCard(false));
    }
    ev.ok(format!("card online: {}", card.mount.display()));
    let total = pkg.card_bytes();
    if let Some(free) = sdcard::free_space(&card) {
        let old = dir_size(&card.mount.join(".rockbox"));
        if free + old < total + (1 << 20) {
            return Err(err!(E510, "the card needs {} MiB free; it has {} MiB", total >> 20, (free + old) >> 20));
        }
    }
    // Plugins and codecs are tied to the core's exact build: stale ones
    // left from an older build would refuse to load, so remove them.
    let keep: std::collections::HashSet<&str> = pkg.files.iter().map(|f| f.path.as_str()).collect();
    // The firmware is in flash. A copy on the card (Rockbox's own packaging
    // puts one there) makes Rockbox offer, once USB lets go, to reboot into
    // it from the card, which these players can't do ("ROLO error").
    let boot = format!(".rockbox/rockbox.{}", kind.id());
    if !keep.contains(boot.as_str()) && std::fs::remove_file(card.mount.join(&boot)).is_ok() {
        ev.dim(format!("removed {boot}: the firmware is in flash, not on the card"));
    }
    let stale = prune(&card.mount, &keep);
    if stale > 0 {
        ev.dim(format!("removed {stale} stale plugin/codec files"));
    }
    for dir in &pkg.dirs {
        let _ = std::fs::create_dir_all(card.mount.join(dir));
    }
    let mut done = 0u64;
    let mut sums: Vec<(String, [u8; 32])> = Vec::new();
    let mount = card.mount.clone();
    pkg.for_each_card_file(
        |rel, size| {
            let p = mount.join(rel);
            if let Some(parent) = p.parent() {
                std::fs::create_dir_all(parent).map_err(|e| err!(E520, "{}: {e}", parent.display()))?;
            }
            let f = std::fs::File::create(&p).map_err(|e| err!(E521, "{}: {e}", p.display()))?;
            let mut w = std::io::BufWriter::with_capacity(256 << 10, f);
            ev.send(Event::File(rel.to_string(), size));
            let mut left = size;
            Ok(Box::new(move |chunk: &[u8]| {
                let write_err = |e: std::io::Error| err!(E522, "card write: {e}");
                w.write_all(chunk).map_err(write_err)?;
                left = left.saturating_sub(chunk.len() as u64);
                if left == 0 {
                    w.flush().map_err(write_err)?;
                    sdcard::sync_file(w.get_ref()).map_err(|e| err!(E523, "card sync: {e}"))?;
                }
                Ok(())
            }))
        },
        |rel, sum| {
            let size = pkg.files.iter().find(|f| f.path == rel).map(|f| f.size).unwrap_or(0);
            done += size;
            ev.progress(done as f32 / total.max(1) as f32 * 0.7);
            sums.push((rel.to_string(), sum));
            Ok(())
        },
    )?;
    ev.ok(format!("{} files written", sums.len()));
    ev.info("reading every file back from the card");
    sdcard::drop_cache(&mut card)?;
    let mut checked = 0u64;
    for (k, (rel, sum)) in sums.iter().enumerate() {
        let data = sdcard::read_back(&card.mount.join(rel)).map_err(|e| err!(E540, "{rel}: {e}"))?;
        if sha256(&data) != *sum {
            return Err(err!(E541, "{rel} did not read back correctly from the card"));
        }
        checked += data.len() as u64;
        if k % 4 == 0 {
            ev.send(Event::ReadBack(rel.clone(), data.len() as u64));
        }
        ev.progress(0.7 + 0.3 * checked as f32 / total.max(1) as f32);
    }
    ev.ok(format!("{} files read back: sha256 OK", sums.len()));
    ev.phase(Phase::Eject);
    match sdcard::eject(&card) {
        Ok(()) => ev.ok("card ejected"),
        Err(e) => ev.warn(format!("could not eject the card ({e}); eject it before unplugging")),
    }
    Ok((sums.len(), total))
}

fn dir_size(p: &Path) -> u64 {
    let Ok(rd) = std::fs::read_dir(p) else { return 0 };
    rd.flatten()
        .map(|e| match e.file_type() {
            Ok(t) if t.is_dir() => dir_size(&e.path()),
            Ok(_) => e.metadata().map(|m| m.len()).unwrap_or(0),
            Err(_) => 0,
        })
        .sum()
}

fn prune(mount: &Path, keep: &std::collections::HashSet<&str>) -> usize {
    fn walk(root: &Path, dir: &Path, keep: &std::collections::HashSet<&str>, n: &mut usize) {
        let Ok(rd) = std::fs::read_dir(dir) else { return };
        for e in rd.flatten() {
            let p = e.path();
            if p.is_dir() {
                walk(root, &p, keep, n);
                continue;
            }
            let Ok(rel) = p.strip_prefix(root) else { continue };
            let rel = rel.to_string_lossy().replace('\\', "/");
            let binary = rel.ends_with(".rock") || rel.ends_with(".codec") || rel.ends_with(".ovl");
            if binary && !keep.contains(rel.as_str()) && std::fs::remove_file(&p).is_ok() {
                *n += 1;
            }
        }
    }
    let mut n = 0;
    for sub in [".rockbox/rocks", ".rockbox/codecs"] {
        walk(mount, &mount.join(sub), keep, &mut n);
    }
    n
}
