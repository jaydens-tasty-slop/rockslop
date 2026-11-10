//! The player's microSD card, as Rockbox's USB mass storage exposes it
//! (inquiry vendor "Rockbox", product "Internal Storage"): find its volume,
//! mount it if the desktop has not, read files back past the host's cache,
//! and eject it.

use crate::err;
use std::path::{Path, PathBuf};
use std::time::{Duration, Instant};

pub struct Card {
    pub mount: PathBuf,
    /// the volume's block device / drive (platform specific)
    pub volume: String,
    /// the whole disk
    pub disk: String,
}

pub enum Look {
    /// Rockbox's disk is not on the system (yet)
    Absent,
    /// the disk is there but holds no card
    NoMedia,
    /// a volume that is not mounted
    Unmounted(String),
    Ready(Card),
}

/// Wait for the card to be mounted (by the desktop, or by us).
pub fn wait_card(timeout: Duration, mut tick: impl FnMut(&Look, f32) -> bool) -> Result<Card, String> {
    let t0 = Instant::now();
    // the unmounted volume: which, since when, failed mounts, next attempt
    let mut vol: Option<(String, Instant)> = None;
    let mut fails = 0;
    let mut next = Instant::now();
    let mut said = String::new();
    loop {
        let look = imp::look();
        let el = t0.elapsed().as_secs_f32();
        let now = match &look {
            Look::Absent => "no Rockbox disk".to_string(),
            Look::NoMedia => "Rockbox disk with no card".to_string(),
            Look::Unmounted(v) => format!("{v} is not mounted"),
            Look::Ready(c) => format!("{} mounted at {}", c.volume, c.mount.display()),
        };
        if now != said {
            crate::log::line("CARD", &now);
            said = now;
        }
        if !tick(&look, el) {
            return Err(err!(E501, "cancelled"));
        }
        match look {
            Look::Ready(c) => return Ok(c),
            Look::Unmounted(v) => {
                if vol.as_ref().map(|(x, _)| *x != v).unwrap_or(true) {
                    vol = Some((v.clone(), Instant::now()));
                    fails = 0;
                }
                // Counted from when the volume appeared: the system's own
                // automounter goes first, and the system needs a moment to
                // look at a new volume before it will mount it.
                let seen = vol.as_ref().map(|(_, t)| t.elapsed()).unwrap_or_default();
                if seen > imp::AUTOMOUNT && Instant::now() >= next {
                    next = Instant::now() + Duration::from_secs(3);
                    if let Err(e) = imp::mount(&v) {
                        crate::log::line("CARD", &format!("mounting {v}: {e}"));
                        fails += 1;
                        if fails == 5 {
                            return Err(err!(E502, "could not mount the player's card: {e}"));
                        }
                    }
                }
            }
            _ => vol = None,
        }
        if t0.elapsed() > timeout {
            return Err(err!(E503, "the player's card did not appear"));
        }
        std::thread::sleep(Duration::from_millis(300));
    }
}

/// What is there right now (no mounting, no device traffic).
pub fn look() -> Look {
    imp::look()
}

/// Bytes free on the card.
pub fn free_space(card: &Card) -> Option<u64> {
    imp::free_space(&card.mount)
}

/// Flush a written file to the card.
pub fn sync_file(f: &std::fs::File) -> std::io::Result<()> {
    f.sync_all()
}

/// Make the next reads come from the card, not the host's cache; returns
/// the (possibly new) mount point.
pub fn drop_cache(card: &mut Card) -> Result<(), String> {
    imp::drop_cache(card)
}

/// Read a file as the card holds it (after drop_cache).
pub fn read_back(path: &Path) -> std::io::Result<Vec<u8>> {
    imp::read_uncached(path)
}

/// The error is a plain reason, without a number: callers only log it.
pub fn eject(card: &Card) -> Result<(), String> {
    imp::eject(card)
}

/// Run a helper tool (udisksctl, diskutil); stdout, or stderr as the error
/// (a plain reason: callers that fail with it give it a number).
#[cfg(unix)]
fn run(cmd: &str, args: &[&str]) -> Result<String, String> {
    let mut c = std::process::Command::new(cmd);
    c.args(args);
    let out = c.output().map_err(|e| format!("{cmd}: {e}"))?;
    if !out.status.success() {
        let err = String::from_utf8_lossy(&out.stderr).trim().to_string();
        return Err(if err.is_empty() { format!("{cmd} failed") } else { err });
    }
    Ok(String::from_utf8_lossy(&out.stdout).to_string())
}

#[cfg(target_os = "linux")]
mod imp {
    use super::*;

    /// a desktop automounter's head start
    pub const AUTOMOUNT: Duration = Duration::from_millis(2500);
    use std::os::unix::io::AsRawFd;

    fn read(p: impl AsRef<Path>) -> String {
        std::fs::read_to_string(p).map(|s| s.trim().to_string()).unwrap_or_default()
    }

    /// udisksctl, through the host when we run inside a flatpak
    fn udisks(args: &[&str]) -> Result<String, String> {
        if Path::new("/.flatpak-info").exists() {
            let mut a = vec!["--host", "udisksctl"];
            a.extend_from_slice(args);
            run("flatpak-spawn", &a)
        } else {
            run("udisksctl", args)
        }
    }

    fn rockbox_disks() -> Vec<String> {
        let Ok(rd) = std::fs::read_dir("/sys/block") else { return Vec::new() };
        let mut out = Vec::new();
        for e in rd.flatten() {
            let name = e.file_name().to_string_lossy().to_string();
            let dev = e.path().join("device");
            if read(dev.join("vendor")) != "Rockbox" || !read(dev.join("model")).starts_with("Internal Storage") {
                continue;
            }
            out.push(name);
        }
        out.sort();
        out
    }

    fn mounts() -> Vec<(String, PathBuf)> {
        let s = read("/proc/self/mountinfo");
        let unesc = |s: &str| s.replace("\\040", " ").replace("\\011", "\t").replace("\\134", "\\");
        s.lines()
            .filter_map(|l| {
                let (a, b) = l.split_once(" - ")?;
                let mp = a.split(' ').nth(4)?;
                let src = b.split(' ').nth(1)?;
                Some((src.to_string(), PathBuf::from(unesc(mp))))
            })
            .collect()
    }

    pub fn look() -> Look {
        let disks = rockbox_disks();
        let Some(disk) = disks.first() else { return Look::Absent };
        if read(format!("/sys/block/{disk}/size")).parse::<u64>().unwrap_or(0) == 0 {
            return Look::NoMedia;
        }
        let mut parts: Vec<String> = std::fs::read_dir(format!("/sys/block/{disk}"))
            .map(|rd| {
                rd.flatten()
                    .map(|e| e.file_name().to_string_lossy().to_string())
                    .filter(|n| n.starts_with(disk.as_str()) && Path::new(&format!("/sys/block/{disk}/{n}/partition")).exists())
                    .collect()
            })
            .unwrap_or_default();
        parts.sort();
        let vol = format!("/dev/{}", parts.first().unwrap_or(disk));
        match mounts().into_iter().find(|(src, _)| *src == vol) {
            Some((_, mp)) => Look::Ready(Card { mount: mp, volume: vol, disk: format!("/dev/{disk}") }),
            None => Look::Unmounted(vol),
        }
    }

    pub fn mount(vol: &str) -> Result<(), String> {
        udisks(&["mount", "--no-user-interaction", "-b", vol]).map(|_| ())
    }

    pub fn free_space(p: &Path) -> Option<u64> {
        let c = std::ffi::CString::new(p.as_os_str().as_encoded_bytes()).ok()?;
        let mut s: libc::statvfs = unsafe { std::mem::zeroed() };
        if unsafe { libc::statvfs(c.as_ptr(), &mut s) } != 0 {
            return None;
        }
        Some(s.f_bavail as u64 * s.f_frsize as u64)
    }

    pub fn drop_cache(_card: &mut Card) -> Result<(), String> {
        Ok(()) // per file, in read_uncached
    }

    pub fn read_uncached(path: &Path) -> std::io::Result<Vec<u8>> {
        let f = std::fs::File::open(path)?;
        // Written pages are clean once synced; DONTNEED drops them, so the
        // read below goes to the card.
        f.sync_all()?;
        unsafe { libc::posix_fadvise(f.as_raw_fd(), 0, 0, libc::POSIX_FADV_DONTNEED) };
        std::fs::read(path)
    }

    pub fn eject(card: &Card) -> Result<(), String> {
        udisks(&["unmount", "--no-user-interaction", "-b", &card.volume])?;
        // power-off also tells the player the host is done with the disk
        let _ = udisks(&["power-off", "--no-user-interaction", "-b", &card.disk]);
        Ok(())
    }
}

/// Reading diskutil's text output (macOS; here so the tests run anywhere).
#[cfg(any(test, target_os = "macos"))]
mod diskutil {
    /// `Key: value` from `diskutil info`.
    pub fn field<'a>(info: &'a str, key: &str) -> Option<&'a str> {
        info.lines().find_map(|l| l.trim().strip_prefix(key).map(|v| v.trim()))
    }

    /// The whole disk is Rockbox's USB disk (its SCSI vendor and product).
    pub fn rockbox(info: &str) -> bool {
        field(info, "Device / Media Name:").map(|m| m.contains("Rockbox") || m.contains("Internal Storage")).unwrap_or(false)
    }

    /// `diskutil list`: each whole disk ("disk4") and its partitions
    /// ("disk4s1", ...), in order.
    pub fn disks(list: &str) -> Vec<(String, Vec<String>)> {
        let mut out: Vec<(String, Vec<String>)> = Vec::new();
        for l in list.lines() {
            if let Some(d) = l.strip_prefix("/dev/") {
                let d = d.split_whitespace().next().unwrap_or_default();
                out.push((d.to_string(), Vec::new()));
            } else if let (Some((disk, parts)), Some(id)) = (out.last_mut(), l.split_whitespace().last()) {
                let slice = id.strip_prefix(disk.as_str()).and_then(|r| r.strip_prefix('s'));
                if slice.map(|n| !n.is_empty() && n.bytes().all(|b| b.is_ascii_digit())).unwrap_or(false) {
                    parts.push(id.to_string());
                }
            }
        }
        out
    }
}

#[cfg(target_os = "macos")]
mod imp {
    use super::*;

    use super::diskutil::{disks, field, rockbox};

    /// macOS mounts every external volume itself, after checking a FAT one
    /// (fsck_msdos), which takes a while on a big card: mount it only if
    /// that never happens
    pub const AUTOMOUNT: Duration = Duration::from_secs(30);

    fn info(target: &str) -> Option<String> {
        run("diskutil", &["info", target]).ok()
    }

    /// From the Rockbox disk to its volume. A mounted volume carries the
    /// card's own name, so the disk is what says Rockbox.
    pub fn look() -> Look {
        let list = run("diskutil", &["list", "external", "physical"]).unwrap_or_default();
        for (disk, parts) in disks(&list) {
            let Some(i) = info(&disk) else { continue };
            if !rockbox(&i) {
                continue;
            }
            if field(&i, "Disk Size:").map(|s| s.starts_with("0 B")).unwrap_or(false) {
                return Look::NoMedia;
            }
            // the first partition, or the disk itself if it has none
            let vol = parts.into_iter().next().unwrap_or_else(|| disk.clone());
            let vi = info(&vol).unwrap_or_default();
            return match (field(&vi, "Mounted:"), field(&vi, "Mount Point:")) {
                (Some("Yes"), Some(mp)) if !mp.is_empty() => {
                    Look::Ready(Card { mount: PathBuf::from(mp), volume: format!("/dev/{vol}"), disk: format!("/dev/{disk}") })
                }
                _ => Look::Unmounted(vol),
            };
        }
        Look::Absent
    }

    pub fn mount(vol: &str) -> Result<(), String> {
        run("diskutil", &["mount", vol]).map(|_| ())
    }

    pub fn free_space(p: &Path) -> Option<u64> {
        let c = std::ffi::CString::new(p.as_os_str().as_encoded_bytes()).ok()?;
        let mut s: libc::statvfs = unsafe { std::mem::zeroed() };
        if unsafe { libc::statvfs(c.as_ptr(), &mut s) } != 0 {
            return None;
        }
        Some(s.f_bavail as u64 * s.f_frsize as u64)
    }

    /// Unmount and mount again: the only sure way past the unified cache.
    pub fn drop_cache(card: &mut Card) -> Result<(), String> {
        for verb in ["unmount", "mount"] {
            run("diskutil", &[verb, &card.volume]).map_err(|e| err!(E530, "{e}"))?;
        }
        let i = info(&card.volume).ok_or_else(|| err!(E531, "card vanished after remount"))?;
        card.mount = PathBuf::from(field(&i, "Mount Point:").ok_or_else(|| err!(E532, "card did not remount"))?);
        Ok(())
    }

    pub fn read_uncached(path: &Path) -> std::io::Result<Vec<u8>> {
        std::fs::read(path)
    }

    pub fn eject(card: &Card) -> Result<(), String> {
        run("diskutil", &["eject", &card.disk]).map(|_| ())
    }
}

#[cfg(windows)]
mod imp {
    use super::*;

    /// (Windows mounts volumes itself; look never reports one unmounted)
    pub const AUTOMOUNT: Duration = Duration::from_secs(0);
    use std::os::windows::ffi::OsStrExt;
    use windows_sys::Win32::Foundation::{CloseHandle, GENERIC_READ, GENERIC_WRITE, HANDLE, INVALID_HANDLE_VALUE};
    use windows_sys::Win32::Storage::FileSystem::{
        CreateFileW, GetDiskFreeSpaceExW, GetDriveTypeW, GetLogicalDrives, FILE_SHARE_READ, FILE_SHARE_WRITE, OPEN_EXISTING,
    };
    use windows_sys::Win32::System::Ioctl::{
        PropertyStandardQuery, StorageDeviceProperty, FSCTL_DISMOUNT_VOLUME, FSCTL_LOCK_VOLUME, IOCTL_STORAGE_EJECT_MEDIA,
        IOCTL_STORAGE_MEDIA_REMOVAL, IOCTL_STORAGE_QUERY_PROPERTY, STORAGE_DEVICE_DESCRIPTOR, STORAGE_PROPERTY_QUERY,
    };
    use windows_sys::Win32::System::IO::DeviceIoControl;

    fn wide(s: &str) -> Vec<u16> {
        std::ffi::OsStr::new(s).encode_wide().chain(Some(0)).collect()
    }

    struct H(HANDLE);
    impl Drop for H {
        fn drop(&mut self) {
            unsafe { CloseHandle(self.0) };
        }
    }

    fn open_volume(letter: char, access: u32) -> Option<H> {
        let p = wide(&format!("\\\\.\\{letter}:"));
        let h = unsafe {
            CreateFileW(p.as_ptr(), access, FILE_SHARE_READ | FILE_SHARE_WRITE, std::ptr::null(), OPEN_EXISTING, 0, std::ptr::null_mut())
        };
        (h != INVALID_HANDLE_VALUE).then_some(H(h))
    }

    fn ioctl(h: &H, code: u32, input: &[u8], out: &mut [u8]) -> bool {
        let mut n = 0u32;
        unsafe {
            DeviceIoControl(
                h.0,
                code,
                input.as_ptr().cast(),
                input.len() as u32,
                out.as_mut_ptr().cast(),
                out.len() as u32,
                &mut n,
                std::ptr::null_mut(),
            ) != 0
        }
    }

    fn is_rockbox(letter: char) -> bool {
        let Some(h) = open_volume(letter, 0) else { return false };
        let q = STORAGE_PROPERTY_QUERY { PropertyId: StorageDeviceProperty, QueryType: PropertyStandardQuery, AdditionalParameters: [0] };
        let qb = unsafe { std::slice::from_raw_parts((&q as *const STORAGE_PROPERTY_QUERY).cast::<u8>(), std::mem::size_of_val(&q)) };
        let mut buf = vec![0u8; 1024];
        if !ioctl(&h, IOCTL_STORAGE_QUERY_PROPERTY, qb, &mut buf) {
            return false;
        }
        let d = unsafe { &*(buf.as_ptr() as *const STORAGE_DEVICE_DESCRIPTOR) };
        let s = |off: u32| -> String {
            if off == 0 || off as usize >= buf.len() {
                return String::new();
            }
            let b = &buf[off as usize..];
            String::from_utf8_lossy(&b[..b.iter().position(|&c| c == 0).unwrap_or(b.len())]).trim().to_string()
        };
        s(d.VendorIdOffset) == "Rockbox" && s(d.ProductIdOffset).starts_with("Internal Storage")
    }

    pub fn look() -> Look {
        let mask = unsafe { GetLogicalDrives() };
        for i in 2..26u32 {
            if mask & (1 << i) == 0 {
                continue;
            }
            let letter = (b'A' + i as u8) as char;
            let root = wide(&format!("{letter}:\\"));
            if unsafe { GetDriveTypeW(root.as_ptr()) } < 2 || !is_rockbox(letter) {
                continue;
            }
            let mount = PathBuf::from(format!("{letter}:\\"));
            if std::fs::read_dir(&mount).is_err() {
                return Look::NoMedia;
            }
            return Look::Ready(Card { mount, volume: letter.to_string(), disk: letter.to_string() });
        }
        Look::Absent
    }

    pub fn mount(_vol: &str) -> Result<(), String> {
        Ok(()) // Windows mounts volumes itself
    }

    pub fn free_space(p: &Path) -> Option<u64> {
        let w = wide(&p.to_string_lossy());
        let mut free = 0u64;
        let ok = unsafe { GetDiskFreeSpaceExW(w.as_ptr(), &mut free, std::ptr::null_mut(), std::ptr::null_mut()) };
        (ok != 0).then_some(free)
    }

    fn letter(card: &Card) -> char {
        card.volume.chars().next().unwrap_or('Z')
    }

    /// Lock + dismount: the next access mounts it fresh, with no cache.
    pub fn drop_cache(card: &mut Card) -> Result<(), String> {
        let h = open_volume(letter(card), GENERIC_READ | GENERIC_WRITE).ok_or_else(|| err!(E533, "cannot open the card's volume"))?;
        if !ioctl(&h, FSCTL_LOCK_VOLUME, &[], &mut []) {
            return Err(err!(E534, "the card is in use by another program"));
        }
        ioctl(&h, FSCTL_DISMOUNT_VOLUME, &[], &mut []);
        Ok(())
    }

    pub fn read_uncached(path: &Path) -> std::io::Result<Vec<u8>> {
        std::fs::read(path)
    }

    pub fn eject(card: &Card) -> Result<(), String> {
        let h = open_volume(letter(card), GENERIC_READ | GENERIC_WRITE).ok_or("cannot open the card's volume")?;
        if !ioctl(&h, FSCTL_LOCK_VOLUME, &[], &mut []) {
            return Err("the card is in use by another program".into());
        }
        ioctl(&h, FSCTL_DISMOUNT_VOLUME, &[], &mut []);
        ioctl(&h, IOCTL_STORAGE_MEDIA_REMOVAL, &[0u8], &mut []);
        ioctl(&h, IOCTL_STORAGE_EJECT_MEDIA, &[], &mut []);
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::diskutil::*;

    const LIST: &str = "/dev/disk4 (external, physical):
   #:                       TYPE NAME                    SIZE       IDENTIFIER
   0:     FDisk_partition_scheme                        *15.5 GB    disk4
   1:                 DOS_FAT_32 STICK                   15.5 GB    disk4s1

/dev/disk6 (external, physical):
   #:                       TYPE NAME                    SIZE       IDENTIFIER
   0:     FDisk_partition_scheme                        *31.7 GB    disk6
   1:                 DOS_FAT_32 MY CARD                 31.7 GB    disk6s1
   2:                 DOS_FAT_32 SECOND              1.0 GB     disk6s2

/dev/disk7 (external, physical):
   #:                       TYPE NAME                    SIZE       IDENTIFIER
   0:                            BARE                   *8.0 GB     disk7
";

    #[test]
    fn diskutil_list() {
        let d = disks(LIST);
        assert_eq!(d.len(), 3);
        assert_eq!(d[0], ("disk4".to_string(), vec!["disk4s1".to_string()]));
        assert_eq!(d[1], ("disk6".to_string(), vec!["disk6s1".to_string(), "disk6s2".to_string()]));
        // a card with no partition table is its own volume
        assert_eq!(d[2], ("disk7".to_string(), vec![]));
        assert!(disks("").is_empty());
    }

    #[test]
    fn diskutil_info() {
        let whole = "   Device Identifier:         disk6
   Device Node:               /dev/disk6
   Whole:                     Yes
   Device / Media Name:       Rockbox Internal Storage
   Disk Size:                 31.7 GB (31719424000 Bytes) (exactly 61952000 512-Byte-Units)
";
        // what is mounted names the card, not the player
        let part = "   Device Identifier:         disk6s1
   Device Node:               /dev/disk6s1
   Whole:                     No
   Part of Whole:             disk6
   Volume Name:               MY CARD
   Mounted:                   Yes
   Mount Point:               /Volumes/MY CARD
   Device / Media Name:       Untitled 1
";
        assert!(rockbox(whole));
        assert!(!rockbox(part));
        assert!(!rockbox("   Device / Media Name:       SanDisk Ultra\n"));
        assert_eq!(field(part, "Mounted:"), Some("Yes"));
        assert_eq!(field(part, "Mount Point:"), Some("/Volumes/MY CARD"));
        assert_eq!(field(whole, "Mount Point:"), None);
    }
}
