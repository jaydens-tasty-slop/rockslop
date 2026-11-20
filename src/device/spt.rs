//! Windows: a boot ROM's vendor SCSI commands through Windows' own USB disk
//! driver, so download mode needs no WinUSB driver (Zadig). Windows lets a
//! normal user open a removable disk's volume for read/write (the disk
//! itself only an administrator), and pass-through sent to the volume goes
//! to the disk.

use super::bot::Data;
use crate::err;
use std::time::{Duration, Instant};
use windows_sys::Win32::Devices::DeviceAndDriverInstallation::{
    CM_Get_Child, CM_Get_Device_IDW, CM_Get_Device_Interface_ListW, CM_Get_Device_Interface_List_SizeW, CM_Get_Sibling,
    CM_Locate_DevNodeW, CM_GET_DEVICE_INTERFACE_LIST_PRESENT, CM_LOCATE_DEVNODE_NORMAL, CR_SUCCESS,
};
use windows_sys::Win32::Foundation::{CloseHandle, GetLastError, GENERIC_READ, GENERIC_WRITE, HANDLE, INVALID_HANDLE_VALUE};
use windows_sys::Win32::Storage::FileSystem::{
    CreateFileW, FindFirstVolumeW, FindNextVolumeW, FindVolumeClose, FILE_SHARE_READ, FILE_SHARE_WRITE, OPEN_EXISTING,
};
use windows_sys::Win32::Storage::IscsiDisc::{IOCTL_SCSI_PASS_THROUGH, SCSI_PASS_THROUGH};
use windows_sys::Win32::System::Ioctl::{
    FSCTL_DISMOUNT_VOLUME, FSCTL_LOCK_VOLUME, GUID_DEVINTERFACE_DISK, IOCTL_STORAGE_GET_DEVICE_NUMBER, STORAGE_DEVICE_NUMBER,
};
use windows_sys::Win32::System::IO::DeviceIoControl;

/// The download modes that are reached this way.
const VIA_DISK: [(u16, u16); 1] = [(super::yp3::VID, super::yp3::PID_ROM)];

const SENSE: usize = 32;

struct H(HANDLE);

impl Drop for H {
    fn drop(&mut self) {
        unsafe { CloseHandle(self.0) };
    }
}

fn wide(s: &str) -> Vec<u16> {
    s.encode_utf16().chain([0]).collect()
}

fn utf16(b: &[u16]) -> String {
    String::from_utf16_lossy(&b[..b.iter().position(|&c| c == 0).unwrap_or(b.len())])
}

fn open(path: &str, access: u32) -> Option<H> {
    let p = wide(path);
    let h = unsafe {
        CreateFileW(p.as_ptr(), access, FILE_SHARE_READ | FILE_SHARE_WRITE, std::ptr::null(), OPEN_EXISTING, 0, std::ptr::null_mut())
    };
    (h != INVALID_HANDLE_VALUE).then_some(H(h))
}

/// (device type, number) of a disk or volume: a volume on a disk has the
/// disk's. Needs no access rights.
fn device_number(path: &str) -> Option<(u32, u32)> {
    let h = open(path, 0)?;
    let mut n = STORAGE_DEVICE_NUMBER { DeviceType: 0, DeviceNumber: 0, PartitionNumber: 0 };
    let mut got = 0u32;
    let ok = unsafe {
        DeviceIoControl(
            h.0,
            IOCTL_STORAGE_GET_DEVICE_NUMBER,
            std::ptr::null(),
            0,
            (&mut n as *mut STORAGE_DEVICE_NUMBER).cast(),
            std::mem::size_of::<STORAGE_DEVICE_NUMBER>() as u32,
            &mut got,
            std::ptr::null_mut(),
        )
    };
    (ok != 0).then_some((n.DeviceType, n.DeviceNumber))
}

fn children(dev: u32) -> Vec<u32> {
    let mut out = Vec::new();
    let mut c = 0u32;
    if unsafe { CM_Get_Child(&mut c, dev, 0) } != CR_SUCCESS {
        return out;
    }
    loop {
        out.push(c);
        let mut next = 0u32;
        if unsafe { CM_Get_Sibling(&mut next, c, 0) } != CR_SUCCESS {
            return out;
        }
        c = next;
    }
}

fn device_id(dev: u32) -> Option<String> {
    let mut b = [0u16; 512];
    (unsafe { CM_Get_Device_IDW(dev, b.as_mut_ptr(), b.len() as u32, 0) } == CR_SUCCESS).then(|| utf16(&b))
}

fn disk_paths(id: &str) -> Vec<String> {
    let w = wide(id);
    let mut len = 0u32;
    if unsafe { CM_Get_Device_Interface_List_SizeW(&mut len, &GUID_DEVINTERFACE_DISK, w.as_ptr(), CM_GET_DEVICE_INTERFACE_LIST_PRESENT) }
        != CR_SUCCESS
    {
        return Vec::new();
    }
    let mut b = vec![0u16; len as usize];
    if unsafe {
        CM_Get_Device_Interface_ListW(&GUID_DEVINTERFACE_DISK, w.as_ptr(), b.as_mut_ptr(), len, CM_GET_DEVICE_INTERFACE_LIST_PRESENT)
    } != CR_SUCCESS
    {
        return Vec::new();
    }
    String::from_utf16_lossy(&b).split('\0').filter(|s| !s.is_empty()).map(str::to_string).collect()
}

/// The present USB device (vid, pid): its disk's (type, number) and its
/// serial number.
fn disk_of(vid: u16, pid: u16) -> Option<((u32, u32), Option<String>)> {
    let id = super::usb::windows_ids().into_iter().find(|id| super::usb::device_ids(id) == Some((vid, pid)))?;
    let serial = id.rsplit('\\').next().filter(|s| !s.is_empty() && !s.contains('&')).map(str::to_string);
    let mut dev = 0u32;
    if unsafe { CM_Locate_DevNodeW(&mut dev, wide(&id).as_ptr(), CM_LOCATE_DEVNODE_NORMAL) } != CR_SUCCESS {
        return None;
    }
    // the disk is the USB device's child (a grandchild on a composite device)
    let near = children(dev);
    let all = near.iter().copied().chain(near.iter().flat_map(|&c| children(c)));
    let n = all.filter_map(device_id).flat_map(|d| disk_paths(&d)).find_map(|p| device_number(&p))?;
    Some((n, serial))
}

/// The volume (`\\?\Volume{...}`) on the disk (type, number).
fn volume_on(disk: (u32, u32)) -> Option<String> {
    let mut b = [0u16; 64];
    let f = unsafe { FindFirstVolumeW(b.as_mut_ptr(), b.len() as u32) };
    if f == INVALID_HANDLE_VALUE {
        return None;
    }
    let mut found = None;
    loop {
        let v = utf16(&b).trim_end_matches('\\').to_string();
        if device_number(&v) == Some(disk) {
            found = Some(v);
            break;
        }
        if unsafe { FindNextVolumeW(f, b.as_mut_ptr(), b.len() as u32) } == 0 {
            break;
        }
    }
    unsafe { FindVolumeClose(f) };
    found
}

fn fsctl(h: &H, code: u32) -> bool {
    let mut got = 0u32;
    unsafe { DeviceIoControl(h.0, code, std::ptr::null(), 0, std::ptr::null_mut(), 0, &mut got, std::ptr::null_mut()) != 0 }
}

/// Keep Windows off the disk while the boot ROM runs our code: a freshly
/// plugged player's volume is being mounted and looked at (Explorer,
/// AutoPlay, Defender), and a command of ours failed in the middle of that.
/// Lock it (that fails while another program has it open, so try a few
/// seconds) and dismount it; the lock goes with the handle.
fn take(h: &H) {
    let t0 = Instant::now();
    while t0.elapsed() < Duration::from_secs(5) {
        if fsctl(h, FSCTL_LOCK_VOLUME) {
            fsctl(h, FSCTL_DISMOUNT_VOLUME);
            return;
        }
        std::thread::sleep(Duration::from_millis(200));
    }
}

/// Whether (vid, pid) is a download mode on Windows' own disk driver, with
/// a volume to send commands through.
pub fn reachable(vid: u16, pid: u16) -> bool {
    VIA_DISK.contains(&(vid, pid)) && disk_of(vid, pid).and_then(|(n, _)| volume_on(n)).is_some()
}

pub struct Disk {
    h: H,
    pub serial: Option<String>,
}

impl Disk {
    pub fn open(vid: u16, pid: u16) -> Result<Disk, String> {
        let (vol, serial) = disk_of(vid, pid)
            .and_then(|(n, serial)| Some((volume_on(n)?, serial)))
            .ok_or_else(|| err!(E222, "Windows shows no disk for the player in download mode"))?;
        let h = open(&vol, GENERIC_READ | GENERIC_WRITE)
            .ok_or_else(|| err!(E223, "Windows refused to open the player's disk (error {})", unsafe { GetLastError() }))?;
        take(&h);
        Ok(Disk { h, serial })
    }

    /// One command: (SCSI status, bytes not moved, IN data).
    pub fn exec(&mut self, cdb: &[u8], cdb_len: u8, data: Data, timeout: Duration) -> Result<(u8, u32, Vec<u8>), String> {
        let (dir, len, out) = match data {
            Data::None => (2u8, 0, None), // SCSI_IOCTL_DATA_UNSPECIFIED
            Data::In(n) => (1, n, None),  // SCSI_IOCTL_DATA_IN
            Data::Out(d) => (0, d.len(), Some(d)),
        };
        let head = std::mem::size_of::<SCSI_PASS_THROUGH>();
        let at = (head + SENSE).next_multiple_of(8);
        let mut c = [0u8; 16];
        c[..cdb.len().min(16)].copy_from_slice(&cdb[..cdb.len().min(16)]);
        let spt = SCSI_PASS_THROUGH {
            Length: head as u16,
            CdbLength: cdb_len,
            SenseInfoLength: SENSE as u8,
            DataIn: dir,
            DataTransferLength: len as u32,
            TimeOutValue: (timeout.as_millis().div_ceil(1000) as u32).max(1),
            DataBufferOffset: at,
            SenseInfoOffset: head as u32,
            Cdb: c,
            ..Default::default()
        };
        let mut b = vec![0u8; at + len];
        unsafe { std::ptr::write_unaligned(b.as_mut_ptr().cast::<SCSI_PASS_THROUGH>(), spt) };
        if let Some(d) = out {
            b[at..].copy_from_slice(d);
        }
        let mut got = 0u32;
        let ok = unsafe {
            DeviceIoControl(
                self.h.0,
                IOCTL_SCSI_PASS_THROUGH,
                b.as_ptr().cast(),
                b.len() as u32,
                b.as_mut_ptr().cast(),
                b.len() as u32,
                &mut got,
                std::ptr::null_mut(),
            )
        };
        if ok == 0 {
            return Err(err!(E224, "a command through Windows' disk driver failed (Windows error {})", unsafe { GetLastError() }));
        }
        let r = unsafe { std::ptr::read_unaligned(b.as_ptr().cast::<SCSI_PASS_THROUGH>()) };
        let moved = (r.DataTransferLength as usize).min(len);
        let data = if dir == 1 { b[at..at + moved].to_vec() } else { Vec::new() };
        Ok((r.ScsiStatus, (len - moved) as u32, data))
    }
}
