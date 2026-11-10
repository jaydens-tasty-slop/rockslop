//! Release packages: a plain `.tar.xz`, so any archive tool can open one.
//!
//! ```text
//! MANIFEST              key=value lines, first in the archive
//! SHA256SUMS            `sha256sum` format, one line per file below
//! flash/0x0000e000.bin  a flash image, written at the offset in its name
//! card/.rockbox/...     the SD card tree, copied to the card root
//! ```
//! MANIFEST keys: `format=N`, `target=yp3|ts17`, free-form metadata shown
//! in the UI (`title`, `version`, `built`, ...), and any number of
//! `expect=OFFSET SIZE SHA256` lines: the player's own flash must match
//! these before anything is written (bootloader/partition checks).
//!
//! `format` is the package layout's version. This flasher reads
//! `OLDEST_FORMAT..=FORMAT`; a newer package says to update the flasher,
//! an older one to get a newer package. Bump it (here and in
//! packaging/mkpkg.py) when a flasher that ignored the change would do the
//! wrong thing; keys a flasher can safely ignore don't need a bump.
//! Everything is streamed: the flash images come before the card tree, so
//! they can be read without decompressing the rest.

use crate::err;
use sha2::{Digest, Sha256};
use std::collections::HashMap;
use std::fs::File;
use std::io::{BufReader, Read};
use std::path::{Path, PathBuf};

/// The newest package format this flasher reads, and the oldest.
pub const FORMAT: u32 = 1;
pub const OLDEST_FORMAT: u32 = 1;

#[derive(Clone, Debug)]
pub struct Seg {
    pub offset: u32,
    pub size: u32,
    pub sha256: [u8; 32],
}

#[derive(Clone, Debug)]
pub struct FileEnt {
    /// relative to the card root, `/`-separated
    pub path: String,
    pub size: u64,
}

#[derive(Clone, Debug)]
pub struct Package {
    pub path: PathBuf,
    pub meta: Vec<(String, String)>,
    pub expect: Vec<Seg>,
    sums: HashMap<String, [u8; 32]>,
    /// filled in by verify()
    pub flash: Vec<Seg>,
    pub files: Vec<FileEnt>,
    pub dirs: Vec<String>,
    pub total: u64,
}

pub fn hex(b: &[u8]) -> String {
    b.iter().map(|x| format!("{x:02x}")).collect()
}

pub fn unhex32(s: &str) -> Option<[u8; 32]> {
    if s.len() != 64 || !s.is_ascii() {
        return None;
    }
    let mut out = [0u8; 32];
    for i in 0..32 {
        out[i] = u8::from_str_radix(&s[2 * i..2 * i + 2], 16).ok()?;
    }
    Some(out)
}

pub fn sha256(data: &[u8]) -> [u8; 32] {
    Sha256::digest(data).into()
}

pub fn num(s: &str) -> Option<u64> {
    if let Some(h) = s.strip_prefix("0x").or_else(|| s.strip_prefix("0X")) {
        u64::from_str_radix(h, 16).ok()
    } else {
        s.parse().ok()
    }
}

/// A card path we are willing to write: relative, no `..`, no odd bytes.
pub fn safe_path(p: &str) -> bool {
    !p.is_empty()
        && !p.starts_with('/')
        && !p.contains('\\')
        && !p.contains(':')
        && !p.chars().any(|c| c.is_control())
        && p.split('/').all(|c| !c.is_empty() && c != "." && c != "..")
}

/// Does this look like one of ours, by name?
pub fn looks_like(path: &Path) -> bool {
    let n = path.file_name().and_then(|n| n.to_str()).unwrap_or("").to_ascii_lowercase();
    n.ends_with(".tar.xz") || n.ends_with(".txz")
}

type Arch = tar::Archive<lzma_rust2::XzReader<BufReader<File>>>;

// Errors raised from more than one place.
fn open_err(e: std::io::Error) -> String {
    err!(E101, "open: {e}")
}
fn archive_err(e: std::io::Error) -> String {
    err!(E103, "archive: {e}")
}
fn unsafe_path(name: &str) -> String {
    err!(E130, "unsafe path in archive: {name}")
}
fn unlisted(name: &str) -> String {
    err!(E132, "{name} is not in SHA256SUMS")
}
/// A member that matched SHA256SUMS when checked, but not when read again.
fn changed(name: &str) -> String {
    err!(E140, "{name}: checksum mismatch")
}

fn archive(path: &Path) -> Result<Arch, String> {
    let f = File::open(path).map_err(open_err)?;
    let xz = lzma_rust2::XzReader::new(BufReader::with_capacity(1 << 16, f), true);
    Ok(tar::Archive::new(xz))
}

fn entry_path(e: &tar::Entry<impl Read>) -> Result<String, String> {
    let raw = e.path_bytes();
    let s = std::str::from_utf8(&raw).map_err(|_| err!(E104, "archive has a non-UTF-8 path"))?;
    Ok(s.trim_start_matches("./").trim_end_matches('/').to_string())
}

fn read_all(mut r: impl Read, limit: u64) -> Result<Vec<u8>, String> {
    let mut v = Vec::new();
    r.by_ref().take(limit + 1).read_to_end(&mut v).map_err(archive_err)?;
    if v.len() as u64 > limit {
        return Err(err!(E105, "archive member too large"));
    }
    Ok(v)
}

/// Stream an entry through SHA-256 (and optionally a sink), in chunks.
fn pump(mut r: impl Read, mut out: impl FnMut(&[u8]) -> Result<(), String>) -> Result<([u8; 32], u64), String> {
    let mut h = Sha256::new();
    let mut buf = vec![0u8; 1 << 16];
    let mut n = 0u64;
    loop {
        let k = r.read(&mut buf).map_err(archive_err)?;
        if k == 0 {
            break;
        }
        h.update(&buf[..k]);
        out(&buf[..k])?;
        n += k as u64;
    }
    Ok((h.finalize().into(), n))
}

fn flash_offset(name: &str) -> Option<u32> {
    let stem = name.strip_prefix("flash/")?.strip_suffix(".bin")?;
    num(stem).and_then(|v| u32::try_from(v).ok())
}

fn get<'a>(meta: &'a [(String, String)], key: &str) -> Option<&'a str> {
    meta.iter().find(|(k, _)| k == key).map(|(_, v)| v.as_str())
}

/// MANIFEST's metadata and `expect` lines; its format must be one this
/// flasher reads, and it must name a target.
fn parse_manifest(text: &str) -> Result<(Vec<(String, String)>, Vec<Seg>), String> {
    let (mut meta, mut expect) = (Vec::new(), Vec::new());
    for (n, line) in text.lines().enumerate() {
        let line = line.trim();
        if line.is_empty() || line.starts_with('#') {
            continue;
        }
        let bad = || err!(E112, "MANIFEST line {}: {line:?}", n + 1);
        let (k, v) = line.split_once('=').ok_or_else(bad)?;
        let (k, v) = (k.trim(), v.trim());
        if k == "expect" {
            let f: Vec<&str> = v.split_whitespace().collect();
            if f.len() != 3 {
                return Err(bad());
            }
            expect.push(Seg {
                offset: num(f[0]).ok_or_else(bad)? as u32,
                size: num(f[1]).ok_or_else(bad)? as u32,
                sha256: unhex32(f[2]).ok_or_else(bad)?,
            });
        } else {
            meta.push((k.to_string(), v.to_string()));
        }
    }
    match get(&meta, "format").map(|f| f.parse::<u32>()) {
        Some(Ok(f)) if (OLDEST_FORMAT..=FORMAT).contains(&f) => {}
        Some(Ok(f)) if f > FORMAT => {
            return Err(err!(E113, "this package (format {f}) needs a newer rockslop than {}: update rockslop", crate::VERSION))
        }
        Some(Ok(f)) => return Err(err!(E116, "this package (format {f}) is too old for rockslop {}: get a newer release", crate::VERSION)),
        _ => return Err(err!(E114, "MANIFEST has no format")),
    }
    if get(&meta, "target").is_none() {
        return Err(err!(E115, "MANIFEST names no target"));
    }
    Ok((meta, expect))
}

impl Package {
    /// Read MANIFEST and SHA256SUMS (the first two members): cheap.
    pub fn open(path: &Path) -> Result<Package, String> {
        let r = Self::open_head(path);
        match &r {
            Ok(p) => crate::log::line(
                "PKG",
                &format!("{}: {} {}", path.display(), p.get("target").unwrap_or("?"), p.get("version").unwrap_or("?")),
            ),
            Err(e) => crate::log::line("PKG", &format!("{}: {e}", path.display())),
        }
        r
    }

    fn open_head(path: &Path) -> Result<Package, String> {
        let total = std::fs::metadata(path).map_err(open_err)?.len();
        let mut a = archive(path)?;
        let mut manifest = None;
        let mut sums_text = None;
        let not_tar = |e: std::io::Error| err!(E102, "not a tar.xz release: {e}");
        for e in a.entries().map_err(not_tar)? {
            let e = e.map_err(not_tar)?;
            match entry_path(&e)?.as_str() {
                "MANIFEST" => manifest = Some(read_all(e, 1 << 20)?),
                "SHA256SUMS" => sums_text = Some(read_all(e, 16 << 20)?),
                _ => break,
            }
            if manifest.is_some() && sums_text.is_some() {
                break;
            }
        }
        let manifest =
            manifest.ok_or_else(|| err!(E110, "no MANIFEST at the start of the archive: not a Rockbox release for this flasher"))?;
        let sums_text = sums_text.ok_or_else(|| err!(E120, "no SHA256SUMS after MANIFEST"))?;
        let mut p = Package {
            path: path.to_path_buf(),
            meta: Vec::new(),
            expect: Vec::new(),
            sums: HashMap::new(),
            flash: Vec::new(),
            files: Vec::new(),
            dirs: Vec::new(),
            total,
        };
        let text = String::from_utf8(manifest).map_err(|_| err!(E111, "MANIFEST is not UTF-8"))?;
        (p.meta, p.expect) = parse_manifest(&text)?;
        let sums = String::from_utf8(sums_text).map_err(|_| err!(E121, "SHA256SUMS is not UTF-8"))?;
        for line in sums.lines().filter(|l| !l.trim().is_empty()) {
            let (h, name) = line.split_once(' ').ok_or_else(|| err!(E122, "bad SHA256SUMS line"))?;
            let name = name.trim_start_matches([' ', '*']).trim_start_matches("./");
            p.sums.insert(name.to_string(), unhex32(h).ok_or_else(|| err!(E123, "bad SHA256SUMS hash"))?);
        }
        Ok(p)
    }

    pub fn get(&self, key: &str) -> Option<&str> {
        get(&self.meta, key)
    }

    pub fn flash_bytes(&self) -> u64 {
        self.flash.iter().map(|s| s.size as u64).sum()
    }

    pub fn card_bytes(&self) -> u64 {
        self.files.iter().map(|f| f.size).sum()
    }

    /// Decompress everything once: every member must be listed in
    /// SHA256SUMS and match it, and every listed file must be present.
    /// Fills `flash`, `files` and `dirs`.
    pub fn verify(&mut self, progress: impl FnMut(f32, &str)) -> Result<(), String> {
        let r = self.verify_all(progress);
        let what = match &r {
            Ok(()) => format!("{} flash image(s), {} card files: checksums match", self.flash.len(), self.files.len()),
            Err(e) => e.clone(),
        };
        crate::log::line("PKG", &format!("{}: {what}", self.path.display()));
        r
    }

    fn verify_all(&mut self, mut progress: impl FnMut(f32, &str)) -> Result<(), String> {
        let f = File::open(&self.path).map_err(open_err)?;
        let total = self.total.max(1);
        // progress follows the compressed bytes consumed
        let counted = Counting { inner: BufReader::with_capacity(1 << 16, f), n: std::rc::Rc::new(std::cell::Cell::new(0)) };
        let pos = counted.n.clone();
        let mut a = tar::Archive::new(lzma_rust2::XzReader::new(counted, true));
        let (mut flash, mut files, mut dirs) = (Vec::new(), Vec::new(), Vec::new());
        let mut seen = 0usize;
        for e in a.entries().map_err(archive_err)? {
            let e = e.map_err(archive_err)?;
            let name = entry_path(&e)?;
            let kind = e.header().entry_type();
            if name == "MANIFEST" || name == "SHA256SUMS" || name.is_empty() || name == "flash" || name == "card" {
                continue;
            }
            if kind.is_dir() {
                if let Some(rel) = name.strip_prefix("card/") {
                    if !safe_path(rel) {
                        return Err(unsafe_path(&name));
                    }
                    dirs.push(rel.to_string());
                }
                continue;
            }
            if !kind.is_file() {
                return Err(err!(E131, "{name}: links and special files are not allowed"));
            }
            let want = *self.sums.get(&name).ok_or_else(|| unlisted(&name))?;
            progress(pos.get() as f32 / total as f32, &name);
            let (got, size) = pump(e, |_| Ok(()))?;
            if got != want {
                return Err(err!(E133, "{name}: checksum mismatch, the download is corrupt"));
            }
            seen += 1;
            if let Some(off) = flash_offset(&name) {
                flash.push(Seg { offset: off, size: size as u32, sha256: got });
            } else if let Some(rel) = name.strip_prefix("card/") {
                if !safe_path(rel) {
                    return Err(unsafe_path(&name));
                }
                files.push(FileEnt { path: rel.to_string(), size });
            } else {
                return Err(err!(E134, "unexpected member {name}"));
            }
        }
        if seen != self.sums.len() {
            return Err(err!(E135, "archive is missing {} file(s) listed in SHA256SUMS", self.sums.len() - seen));
        }
        if flash.is_empty() {
            return Err(err!(E136, "release contains no flash image"));
        }
        progress(1.0, "");
        self.flash = flash;
        self.files = files;
        self.dirs = dirs;
        Ok(())
    }

    /// The flash images, checked again as they are read.
    pub fn read_flash(&self) -> Result<Vec<(Seg, Vec<u8>)>, String> {
        let mut a = archive(&self.path)?;
        let mut out = Vec::new();
        for e in a.entries().map_err(archive_err)? {
            let e = e.map_err(archive_err)?;
            let name = entry_path(&e)?;
            if name.starts_with("card") {
                break; // flash members come first
            }
            let Some(off) = flash_offset(&name) else { continue };
            let want = *self.sums.get(&name).ok_or_else(|| unlisted(&name))?;
            let data = read_all(e, 64 << 20)?;
            if sha256(&data) != want {
                return Err(changed(&name));
            }
            out.push((Seg { offset: off, size: data.len() as u32, sha256: want }, data));
        }
        Ok(out)
    }

    /// Stream every card file to `each(rel_path, size, chunk_sink)`; each
    /// file's SHA-256 is checked after its last chunk.
    pub fn for_each_card_file(
        &self,
        mut each: impl FnMut(&str, u64) -> Result<Box<dyn FnMut(&[u8]) -> Result<(), String> + '_>, String>,
        mut done: impl FnMut(&str, [u8; 32]) -> Result<(), String>,
    ) -> Result<(), String> {
        let mut a = archive(&self.path)?;
        for e in a.entries().map_err(archive_err)? {
            let e = e.map_err(archive_err)?;
            let name = entry_path(&e)?;
            let Some(rel) = name.strip_prefix("card/") else { continue };
            if !e.header().entry_type().is_file() {
                continue;
            }
            if !safe_path(rel) {
                return Err(unsafe_path(&name));
            }
            let want = *self.sums.get(&name).ok_or_else(|| unlisted(&name))?;
            let size = e.header().size().map_err(archive_err)?;
            let rel = rel.to_string();
            let mut sink = each(&rel, size)?;
            let (got, _) = pump(e, |b| sink(b))?;
            drop(sink);
            if got != want {
                return Err(changed(&name));
            }
            done(&rel, got)?;
        }
        Ok(())
    }

    pub fn sum(&self, member: &str) -> Option<[u8; 32]> {
        self.sums.get(member).copied()
    }
}

struct Counting<R> {
    inner: R,
    n: std::rc::Rc<std::cell::Cell<u64>>,
}

impl<R: Read> Read for Counting<R> {
    fn read(&mut self, buf: &mut [u8]) -> std::io::Result<usize> {
        let k = self.inner.read(buf)?;
        self.n.set(self.n.get() + k as u64);
        Ok(k)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn paths() {
        assert!(safe_path(".rockbox/codecs/mpa.codec"));
        assert!(!safe_path("../etc/passwd"));
        assert!(!safe_path("/abs"));
        assert!(!safe_path("a//b"));
        assert!(!safe_path("C:/x"));
        assert!(!safe_path("a/./b"));
    }
    #[test]
    fn format_versions() {
        let m = |f: &str| parse_manifest(&format!("format={f}\ntarget=yp3\nversion=x\n"));
        assert!(m(&FORMAT.to_string()).is_ok());
        let newer = m(&(FORMAT + 1).to_string()).unwrap_err();
        assert!(newer.starts_with("E113") && newer.contains("update rockslop"), "{newer}");
        if OLDEST_FORMAT > 0 {
            assert!(m(&(OLDEST_FORMAT - 1).to_string()).unwrap_err().starts_with("E116"));
        }
        assert!(m("one").unwrap_err().starts_with("E114"));
        assert!(parse_manifest("target=yp3").unwrap_err().starts_with("E114"));
        assert!(parse_manifest("format=1").unwrap_err().starts_with("E115"));
        // a key this flasher doesn't know is metadata, not an error
        assert!(parse_manifest("format=1\ntarget=ts17\nsomething_new=1").is_ok());
    }

    #[test]
    fn mkpkg_writes_this_format() {
        let py = include_str!("../packaging/mkpkg.py");
        assert!(py.contains(&format!("\nFORMAT = {FORMAT} ")), "packaging/mkpkg.py's FORMAT is not {FORMAT}");
    }

    #[test]
    fn offsets() {
        assert_eq!(flash_offset("flash/0x0000e000.bin"), Some(0xe000));
        assert_eq!(flash_offset("flash/0x14000.bin"), Some(0x14000));
        assert_eq!(flash_offset("card/x.bin"), None);
    }
}
