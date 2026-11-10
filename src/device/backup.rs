//! Firmware backups: whole-flash images kept in the user's data directory,
//! one per (player, unit, content), with a sha256sum-format sidecar.
//!
//!   <data>/rockslop/backups/yp3-1a2b3c4d-20261106-231500-stock.bin
//!   <data>/rockslop/backups/yp3-1a2b3c4d-20261106-231500-stock.bin.sha256

use super::Kind;
use crate::err;
use crate::pkg::{hex, sha256, unhex32};
use std::path::{Path, PathBuf};
use std::time::{SystemTime, UNIX_EPOCH};

#[derive(Clone, Debug)]
pub struct Backup {
    pub path: PathBuf,
    pub kind: Kind,
    /// identifies the unit (its calibration / settings sectors)
    pub fingerprint: u32,
    pub stamp: String,
    pub stock: bool,
}

pub fn dir() -> PathBuf {
    let base = if cfg!(windows) {
        std::env::var_os("APPDATA").map(PathBuf::from)
    } else if cfg!(target_os = "macos") {
        std::env::var_os("HOME").map(|h| PathBuf::from(h).join("Library/Application Support"))
    } else {
        std::env::var_os("XDG_DATA_HOME")
            .map(PathBuf::from)
            .or_else(|| std::env::var_os("HOME").map(|h| PathBuf::from(h).join(".local/share")))
    };
    base.unwrap_or_else(|| PathBuf::from(".")).join("rockslop").join("backups")
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

fn sidecar(path: &Path) -> PathBuf {
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
    #[test]
    fn stamp_shape() {
        let s = super::stamp_now();
        assert_eq!(s.len(), 15);
        assert!(s.starts_with("20"));
    }
}
