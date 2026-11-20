//! Raw bulk pipes to a USB interface (libusb via rusb, statically linked).
//! Reads are a byte stream: one bulk read may return a data phase and the
//! status that follows it together (both ROMs do this).

use crate::err;
use rusb::{Context, DeviceHandle, Direction, TransferType, UsbContext};
use std::time::{Duration, Instant};

pub struct Link {
    h: DeviceHandle<Context>,
    iface: u8,
    ep_in: u8,
    ep_out: u8,
    mps: usize,
    pending: Vec<u8>,
}

thread_local! {
    static CTX: Context = Context::new().expect("libusb init");
}

pub fn ctx() -> Context {
    CTX.with(|c| c.clone())
}

fn ours(vid: u16) -> bool {
    matches!(vid, 0x301a | 0x10d6 | 0x1209)
}

/// Every attached (vid, pid), with whether we could open it.
pub fn list() -> Vec<(u16, u16, bool)> {
    let mut out: Vec<(u16, u16, bool)> = match ctx().devices() {
        Ok(devs) => devs
            .iter()
            .filter_map(|d| {
                let desc = d.device_descriptor().ok()?;
                let (v, p) = (desc.vendor_id(), desc.product_id());
                ours(v).then(|| (v, p, d.open().is_ok() || via_disk(v, p)))
            })
            .collect(),
        Err(_) => Vec::new(),
    };
    for (v, p) in driverless() {
        if !out.iter().any(|o| (o.0, o.1) == (v, p)) {
            out.push((v, p, false));
        }
    }
    out
}

pub fn present(vid: u16, pid: u16) -> bool {
    list().iter().any(|(v, p, _)| *v == vid && *p == pid)
}

/// Windows: a download mode on Windows' own disk driver, which libusb
/// can't open, is reached through it instead (spt.rs).
#[cfg(windows)]
fn via_disk(vid: u16, pid: u16) -> bool {
    super::spt::reachable(vid, pid)
}

#[cfg(not(windows))]
fn via_disk(_: u16, _: u16) -> bool {
    false
}

/// Windows: libusb leaves out a device Windows has no driver for (Rockbox
/// charging only); the system's own list of USB devices has it.
fn driverless() -> Vec<(u16, u16)> {
    windows_ids().iter().filter_map(|id| device_ids(id)).filter(|(v, _)| ours(*v)).collect()
}

/// Windows: the instance IDs of the present USB devices.
#[cfg(windows)]
pub fn windows_ids() -> Vec<String> {
    use windows_sys::Win32::Devices::DeviceAndDriverInstallation::{
        CM_Get_Device_ID_ListW, CM_Get_Device_ID_List_SizeW, CM_GETIDLIST_FILTER_ENUMERATOR, CM_GETIDLIST_FILTER_PRESENT,
        CR_SUCCESS,
    };
    let filter: Vec<u16> = "USB".encode_utf16().chain([0]).collect();
    let flags = CM_GETIDLIST_FILTER_ENUMERATOR | CM_GETIDLIST_FILTER_PRESENT;
    let mut len = 0u32;
    if unsafe { CM_Get_Device_ID_List_SizeW(&mut len, filter.as_ptr(), flags) } != CR_SUCCESS {
        return Vec::new();
    }
    let mut buf = vec![0u16; len as usize];
    if unsafe { CM_Get_Device_ID_ListW(filter.as_ptr(), buf.as_mut_ptr(), len, flags) } != CR_SUCCESS {
        return Vec::new();
    }
    String::from_utf16_lossy(&buf).split('\0').filter(|s| !s.is_empty()).map(str::to_string).collect()
}

#[cfg(not(windows))]
fn windows_ids() -> Vec<String> {
    Vec::new()
}

/// A Windows USB device instance ID, `USB\VID_1209&PID_0001\...`: its
/// (vid, pid). Not an interface of a composite device (`...&MI_00\...`).
pub fn device_ids(id: &str) -> Option<(u16, u16)> {
    let hw = id.to_ascii_uppercase();
    let hw = hw.strip_prefix("USB\\")?.split('\\').next()?;
    let mut parts = hw.split('&');
    let vid = u16::from_str_radix(parts.next()?.strip_prefix("VID_")?, 16).ok()?;
    let pid = u16::from_str_radix(parts.next()?.strip_prefix("PID_")?, 16).ok()?;
    parts.next().is_none().then_some((vid, pid))
}

#[cfg(test)]
mod tests {
    #[test]
    fn windows_device_ids() {
        assert_eq!(super::device_ids(r"USB\VID_1209&PID_0001\5&E57B0DF&0&7"), Some((0x1209, 0x0001)));
        assert_eq!(super::device_ids(r"USB\VID_301A&PID_2800\20201111000001"), Some((0x301a, 0x2800)));
        assert_eq!(super::device_ids(r"usb\vid_10d6&pid_10d6\x"), Some((0x10d6, 0x10d6)));
        assert_eq!(super::device_ids(r"USB\VID_1209&PID_0001&MI_00\6&65CFE5F&0&0000"), None);
        assert_eq!(super::device_ids(r"USB\ROOT_HUB30\4&1"), None);
        assert_eq!(super::device_ids(""), None);
    }
}

/// Wait until (vid, pid) shows up (and stays up for ~0.3 s).
pub fn wait_for(vid: u16, pid: u16, timeout: Duration, mut tick: impl FnMut(f32)) -> bool {
    let t0 = Instant::now();
    let mut stable = 0;
    while t0.elapsed() < timeout {
        if list().iter().any(|(v, p, ok)| *v == vid && *p == pid && *ok) {
            stable += 1;
            if stable >= 3 {
                return true;
            }
        } else {
            stable = 0;
        }
        tick(t0.elapsed().as_secs_f32());
        std::thread::sleep(Duration::from_millis(100));
    }
    false
}

fn usb_err(what: &str, e: rusb::Error) -> String {
    // Windows opens only a device on the WinUSB driver: one on any other
    // driver, or none, is "not supported"
    let denied = e == rusb::Error::Access || (cfg!(windows) && e == rusb::Error::NotSupported);
    match e {
        _ if denied => err!(
            E212,
            "{what}: permission denied. {}",
            if cfg!(target_os = "linux") {
                "Install the udev rule (see README) or run as root."
            } else if cfg!(target_os = "macos") {
                "macOS needs root to take the device from its mass storage driver: run from Terminal with sudo."
            } else {
                "The device needs the WinUSB driver (Zadig)."
            }
        ),
        rusb::Error::Busy => err!(E213, "{what}: the device is busy (another tool is using it?)"),
        rusb::Error::NoDevice => err!(E214, "{what}: the device went away"),
        rusb::Error::Timeout => err!(E215, "{what}: timed out"),
        e => err!(E216, "{what}: {e}"),
    }
}

impl Link {
    /// Open (vid, pid), detach any kernel driver and claim the first
    /// interface that has a bulk IN/OUT pair (or the given endpoints).
    pub fn open(vid: u16, pid: u16) -> Result<Link, String> {
        let dev = ctx()
            .devices()
            .map_err(|e| usb_err("usb", e))?
            .iter()
            .find(|d| d.device_descriptor().map(|x| x.vendor_id() == vid && x.product_id() == pid).unwrap_or(false))
            .ok_or_else(|| err!(E210, "{vid:04x}:{pid:04x} not connected"))?;
        let cfg = dev.active_config_descriptor().or_else(|_| dev.config_descriptor(0)).map_err(|e| usb_err("descriptor", e))?;
        let mut found = None;
        for intf in cfg.interfaces() {
            for alt in intf.descriptors() {
                let mut i = None;
                let mut o = None;
                let mut mps = 64;
                for ep in alt.endpoint_descriptors() {
                    if ep.transfer_type() == TransferType::Bulk {
                        match ep.direction() {
                            Direction::In => {
                                i = Some(ep.address());
                                mps = ep.max_packet_size() as usize;
                            }
                            Direction::Out => o = Some(ep.address()),
                        }
                    }
                }
                if let (Some(i), Some(o)) = (i, o) {
                    found = Some((alt.interface_number(), i, o, mps));
                    break;
                }
            }
            if found.is_some() {
                break;
            }
        }
        let (iface, ep_in, ep_out, mps) = found.ok_or_else(|| err!(E211, "no bulk endpoints"))?;
        let h = dev.open().map_err(|e| usb_err("open", e))?;
        if rusb::supports_detach_kernel_driver() {
            if h.kernel_driver_active(iface).unwrap_or(false) {
                h.detach_kernel_driver(iface).map_err(|e| usb_err("detach kernel driver", e))?;
            }
        }
        h.claim_interface(iface).map_err(|e| usb_err("claim", e))?;
        Ok(Link { h, iface, ep_in, ep_out, mps: mps.max(64), pending: Vec::new() })
    }

    pub fn serial(&self) -> Option<String> {
        let d = self.h.device().device_descriptor().ok()?;
        self.h.read_serial_number_string_ascii(&d).ok()
    }

    pub fn write(&mut self, data: &[u8], timeout: Duration) -> Result<(), String> {
        let n = self.h.write_bulk(self.ep_out, data, timeout).map_err(|e| usb_err("usb write", e))?;
        if n != data.len() {
            return Err(err!(E217, "short write {n}/{}", data.len()));
        }
        Ok(())
    }

    /// Exactly n bytes from the IN stream.
    pub fn read(&mut self, n: usize, timeout: Duration) -> Result<Vec<u8>, String> {
        let deadline = Instant::now() + timeout;
        while self.pending.len() < n {
            let left = deadline.saturating_duration_since(Instant::now());
            if left.is_zero() {
                return Err(usb_err("usb read", rusb::Error::Timeout));
            }
            let mut buf = vec![0u8; (n - self.pending.len() + 13).div_ceil(self.mps) * self.mps + self.mps * 8];
            match self.h.read_bulk(self.ep_in, &mut buf, left) {
                Ok(0) => return Err(err!(E218, "usb read: empty")),
                Ok(k) => self.pending.extend_from_slice(&buf[..k]),
                Err(e) => return Err(usb_err("usb read", e)),
            }
        }
        let rest = self.pending.split_off(n);
        Ok(std::mem::replace(&mut self.pending, rest))
    }

    /// Discard anything a previous session left unread.
    pub fn drain(&mut self) {
        self.pending.clear();
        let mut buf = vec![0u8; 4096];
        while let Ok(k) = self.h.read_bulk(self.ep_in, &mut buf, Duration::from_millis(50)) {
            if k == 0 {
                break;
            }
        }
    }
}

impl Drop for Link {
    fn drop(&mut self) {
        let _ = self.h.release_interface(self.iface);
    }
}
