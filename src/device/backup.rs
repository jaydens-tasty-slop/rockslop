//! Firmware backups: whole-flash images kept in the user's data directory,
//! one per (player, unit, content), with a sha256sum-format sidecar.
//!
//!   <data>/rockslop/backups/yp3-1a2b3c4d-20261106-231500-stock.bin
//!   <data>/rockslop/backups/yp3-1a2b3c4d-20261106-231500-stock.bin.sha256
//!
//! A flash also leaves a copy of the stock backup on the player's card, as
//! `.rockbox/<the same name>`, so it can be found from another computer.

use super::rdm::{Rdm, BOOT_RUN, XIP_RUN};
use super::sdcard::{self, Look};
use super::yp3::crc32;
use super::Kind;
use crate::err;
use crate::pkg::{hex, sha256, unhex32};
use std::path::{Path, PathBuf};
use std::time::{Duration, SystemTime, UNIX_EPOCH};

#[derive(Clone, Debug)]
pub struct Backup {
    pub path: PathBuf,
    pub kind: Kind,
    /// identifies the unit (its calibration / settings sectors)
    pub fingerprint: u32,
    pub stamp: String,
    pub stock: bool,
}

/// rockslop's own folder in the user's data directory: the backups, and
/// the GUI's settings.
pub fn data_dir() -> PathBuf {
    let base = if cfg!(windows) {
        std::env::var_os("APPDATA").map(PathBuf::from)
    } else if cfg!(target_os = "macos") {
        std::env::var_os("HOME").map(|h| PathBuf::from(h).join("Library/Application Support"))
    } else {
        std::env::var_os("XDG_DATA_HOME")
            .map(PathBuf::from)
            .or_else(|| std::env::var_os("HOME").map(|h| PathBuf::from(h).join(".local/share")))
    };
    base.unwrap_or_else(|| PathBuf::from(".")).join("rockslop")
}

pub fn dir() -> PathBuf {
    data_dir().join("backups")
}

/// Local time is not worth a dependency: backups are stamped in UTC.
pub fn stamp_now() -> String {
    let s = SystemTime::now().duration_since(UNIX_EPOCH).map(|d| d.as_secs()).unwrap_or(0) as i64;
    let (days, rem) = (s.div_euclid(86400), s.rem_euclid(86400));
    // civil_from_days (H. Hinnant)
    let z = days + 719_468;
    let era = z.div_euclid(146_097);
    let doe = z - era * 146_097;
    let yoe = (doe - doe / 1460 + doe / 36524 - doe / 146_096) / 365;
    let doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    let mp = (5 * doy + 2) / 153;
    let d = doy - (153 * mp + 2) / 5 + 1;
    let m = if mp < 10 { mp + 3 } else { mp - 9 };
    let y = yoe + era * 400 + i64::from(m <= 2);
    format!("{y:04}{m:02}{d:02}-{:02}{:02}{:02}", rem / 3600, rem / 60 % 60, rem % 60)
}

fn parse(path: &Path) -> Option<Backup> {
    let name = path.file_name()?.to_str()?.strip_suffix(".bin")?;
    let mut it = name.split('-');
    let kind = match it.next()? {
        "yp3" => Kind::Yp3,
        "ts17" => Kind::Ts17,
        _ => return None,
    };
    let fingerprint = u32::from_str_radix(it.next()?, 16).ok()?;
    let stamp = format!("{}-{}", it.next()?, it.next()?);
    let stock = it.next()? == "stock";
    Some(Backup { path: path.to_path_buf(), kind, fingerprint, stamp, stock })
}

/// Every backup on file for `kind`, newest first.
pub fn list(kind: Kind) -> Vec<Backup> {
    let Ok(rd) = std::fs::read_dir(dir()) else { return Vec::new() };
    let mut v: Vec<Backup> = rd.filter_map(|e| parse(&e.ok()?.path())).filter(|b| b.kind == kind).collect();
    v.sort_by(|a, b| b.stamp.cmp(&a.stamp));
    v
}

/// The image to return this unit to stock: its own stock backup if we have
/// one, else any stock backup of the same player (the per-unit sectors are
/// never rewritten, so another unit's firmware region is equivalent).
pub fn stock_for(kind: Kind, fingerprint: Option<u32>) -> Option<Backup> {
    let all = list(kind);
    let mine = all.iter().find(|b| b.stock && Some(b.fingerprint) == fingerprint);
    mine.or_else(|| all.iter().find(|b| b.stock)).cloned()
}

/// An existing backup with exactly this content, if any.
pub fn find_same(kind: Kind, fingerprint: u32, image: &[u8]) -> Option<Backup> {
    let want = sha256(image);
    list(kind).into_iter().filter(|b| b.fingerprint == fingerprint).find(|b| read_sum(&b.path) == Some(want))
}

/// Which unit an image is from: its per-unit sectors' CRC32.
pub fn fingerprint(kind: Kind, img: &[u8]) -> u32 {
    let g = kind.geometry();
    crc32(&img[g.unit.0 as usize..g.unit.1 as usize])
}

/// No Rockbox anywhere in it: raw, or (TS17) the firmware window as XIP
/// reads it.
pub fn is_stock(kind: Kind, img: &[u8]) -> bool {
    let plain = match kind {
        Kind::Yp3 => None,
        Kind::Ts17 => {
            let (a, b) = kind.geometry().writable;
            Some(Rdm::new().decode(&img[a as usize..b as usize], XIP_RUN))
        }
    };
    !has_rockbox(img) && !plain.is_some_and(|p| has_rockbox(&p))
}

/// Is the player's partition table where its boot chain keeps it?
fn has_table(kind: Kind, img: &[u8]) -> bool {
    match kind {
        Kind::Yp3 => img.get(0xd010..0xd014) == Some(b"FIRM"),
        Kind::Ts17 => img.get(0x1000..0x1000 + BOOT_RUN).is_some_and(|c| Rdm::new().decode(c, BOOT_RUN).starts_with(b"ACPT")),
    }
}

/// Stock firmware rockslop knows, by the SHA-256 of its firmware window
/// (what a restore writes; it is the same on every unit). Add versions as
/// they turn up.
const KNOWN: &[(Kind, &str, &str)] = &[
    (Kind::Yp3, "yp3_2.0.43", "72c3484c208998740a8f7e5e69856947b81dce7a57ade8bc55a4b6f5ac9a9d44"),
    (Kind::Ts17, "TS17, 2026 units", "00d56c7165c63e1812a549b968310f978ee558bff6db5e2457615765bb9cf249"),
];

/// The known stock firmware an image holds, if any.
pub fn known(kind: Kind, img: &[u8]) -> Option<&'static str> {
    let (a, b) = kind.geometry().writable;
    let h = hex(&sha256(&img[a as usize..b as usize]));
    KNOWN.iter().find(|k| k.0 == kind && k.2 == h).map(|k| k.1)
}

/// An image from outside the backups folder, checked.
pub struct Checked {
    pub data: Vec<u8>,
    /// the known stock firmware it holds
    pub known: Option<&'static str>,
    pub stock: bool,
}

/// The checks an image from outside the backups folder (the player's card,
/// a file the user picked) must pass to be flashed at all: its size, its
/// checksum file, the player's partition table, and firmware where the
/// firmware goes. What firmware it is is the caller's call.
pub fn check_image(kind: Kind, path: &Path) -> Result<Checked, String> {
    let g = kind.geometry();
    let img = load(path, g.size as usize)?;
    let name = path.file_name().map(|n| n.to_string_lossy().to_string()).unwrap_or_default();
    if !has_table(kind, &img) {
        return Err(err!(E424, "{name} is not a {} flash backup (no partition table)", kind.name()));
    }
    let w = &img[g.writable.0 as usize..g.writable.1 as usize];
    if w.iter().all(|&b| b == 0xff) || w.iter().all(|&b| b == 0) {
        return Err(err!(E425, "{name} has no firmware in it (that part is blank)"));
    }
    Ok(Checked { known: known(kind, &img), stock: is_stock(kind, &img), data: img })
}

/// Take a stock image into the backups folder, or find the backup that
/// already holds it.
pub fn import(kind: Kind, path: &Path) -> Result<Backup, String> {
    let c = check_image(kind, path)?;
    if !c.stock {
        let name = path.file_name().map(|n| n.to_string_lossy().to_string()).unwrap_or_default();
        return Err(err!(E426, "{name} has Rockbox in it: it is not stock firmware"));
    }
    let fp = fingerprint(kind, &c.data);
    match find_same(kind, fp, &c.data) {
        Some(b) => Ok(b),
        None => save(kind, fp, true, &c.data),
    }
}

/// A stock backup on the card of a player running Rockbox, if one is
/// plugged in (its disk is mounted if need be), taken into the backups:
/// RETURN TO STOCK's next place to look after this computer.
pub fn from_card(kind: Kind) -> Result<Option<Backup>, String> {
    if matches!(sdcard::look(), Look::Absent | Look::NoMedia) {
        return Ok(None);
    }
    // unplugged meanwhile, or not mountable: nothing found
    let card = match sdcard::wait_card(Duration::from_secs(15), |look, _| !matches!(look, Look::Absent)) {
        Ok(c) => c,
        Err(e) => {
            crate::log::line("CARD", &format!("no search for a stock backup: {e}"));
            return Ok(None);
        }
    };
    let mut failed = None;
    for p in on_card(kind, &card.mount) {
        match import(kind, &p) {
            Ok(b) => return Ok(Some(b)),
            Err(e) => failed = Some(e),
        }
    }
    failed.map_or(Ok(None), Err)
}

/// Stock backups on a card rockslop has flashed: `.rockbox/<kind>-*-stock.bin`, newest first.
pub fn on_card(kind: Kind, mount: &Path) -> Vec<PathBuf> {
    let Ok(rd) = std::fs::read_dir(mount.join(".rockbox")) else { return Vec::new() };
    let mut v: Vec<Backup> = rd.filter_map(|e| parse(&e.ok()?.path())).filter(|b| b.kind == kind && b.stock).collect();
    v.sort_by(|a, b| b.stamp.cmp(&a.stamp));
    v.into_iter().map(|b| b.path).collect()
}

/// The checksum file next to a backup.
pub fn sidecar(path: &Path) -> PathBuf {
    let mut s = path.as_os_str().to_owned();
    s.push(".sha256");
    PathBuf::from(s)
}

fn read_sum(path: &Path) -> Option<[u8; 32]> {
    let s = std::fs::read_to_string(sidecar(path)).ok()?;
    unhex32(s.split_whitespace().next()?)
}

pub fn save(kind: Kind, fingerprint: u32, stock: bool, image: &[u8]) -> Result<Backup, String> {
    let d = dir();
    std::fs::create_dir_all(&d).map_err(|e| err!(E410, "backup folder {}: {e}", d.display()))?;
    let name = format!("{}-{fingerprint:08x}-{}-{}.bin", kind.id(), stamp_now(), if stock { "stock" } else { "other" });
    let path = d.join(&name);
    let tmp = d.join(format!(".{name}.part"));
    std::fs::write(&tmp, image).map_err(|e| err!(E411, "write backup: {e}"))?;
    if !std::fs::read(&tmp).map(|r| r == image).unwrap_or(false) {
        let _ = std::fs::remove_file(&tmp);
        return Err(err!(E412, "backup did not read back from disk"));
    }
    std::fs::rename(&tmp, &path).map_err(|e| err!(E413, "backup: {e}"))?;
    std::fs::write(sidecar(&path), format!("{}  {name}\n", hex(&sha256(image)))).map_err(|e| err!(E414, "backup checksum: {e}"))?;
    Ok(Backup { path, kind, fingerprint, stamp: stamp_now(), stock })
}

/// Load a backup (or any image the user picked) and check it against its
/// sidecar when there is one.
pub fn load(path: &Path, size: usize) -> Result<Vec<u8>, String> {
    let data = std::fs::read(path).map_err(|e| err!(E421, "{}: {e}", path.display()))?;
    if data.len() != size {
        return Err(err!(E422, "{} is {} bytes; a full flash image is {size}", path.display(), data.len()));
    }
    if let Some(want) = read_sum(path) {
        if sha256(&data) != want {
            return Err(err!(E423, "{} is damaged (checksum mismatch)", path.display()));
        }
    }
    Ok(data)
}

/// Does the image carry Rockbox anywhere (plain, or TS17 cells decoded)?
pub fn has_rockbox(plain: &[u8]) -> bool {
    plain.windows(7).any(|w| w == b"Rockbox")
}

#[cfg(test)]
mod tests {
    use super::*;

    /// What a picked image goes through before RETURN TO STOCK takes it.
    #[test]
    fn image_checks() {
        let path = std::env::temp_dir().join(format!("rockslop-test-{}.bin", std::process::id()));
        let check = |img: &[u8]| {
            std::fs::write(&path, img).unwrap();
            check_image(Kind::Yp3, &path).map(|c| (c.known, c.stock)).map_err(|e| e[..4].to_string())
        };
        let mut img = vec![0xffu8; 2 << 20];
        img[0xd010..0xd014].copy_from_slice(b"FIRM");
        assert_eq!(check(&img), Err("E425".into()), "blank firmware window");
        img[0x10000..0x10004].copy_from_slice(&[1, 2, 3, 4]);
        assert_eq!(check(&img), Ok((None, true)), "unknown firmware");
        assert_eq!(check(&img[..1 << 20]), Err("E422".into()), "size");
        img[0x20000..0x20007].copy_from_slice(b"Rockbox");
        assert_eq!(check(&img), Ok((None, false)), "not stock");
        assert_eq!(import(Kind::Yp3, &path).map_err(|e| e[..4].to_string()).err(), Some("E426".into()));
        img[0xd010] = 0;
        assert_eq!(check(&img), Err("E424".into()), "no partition table");
        let _ = std::fs::remove_file(&path);
        assert!(KNOWN.iter().all(|k| unhex32(k.2).is_some()));
    }

    #[test]
    fn stamp_shape() {
        let s = super::stamp_now();
        assert_eq!(s.len(), 15);
        assert!(s.starts_with("20"));
    }
}
