//! YP3 (SmartLink SL6801) boot ROM recovery mode, USB 301a:2800.
//!
//! The ROM speaks USB mass storage BOT with vendor CDBs (0xC0 | cmd);
//! protocol from smtlink_dump.c (Smartlink firmware dumper). Everything
//! that touches flash goes through the rockbox-yp3 service payload
//! (payloads/yp3/svc.c, loaded at 0x806000) exactly as yp3dev/yp3.py does:
//! the ROM's own flash commands hang in the watchdog-return ROM state.

use super::bot::{Bot, Data};
use super::usb::Link;
use crate::err;
use std::time::Duration;

pub const VID: u16 = 0x301a;
pub const PID_ROM: u16 = 0x2800;
pub const PID_STOCK: u16 = 0x2801;
pub const PID_ROCKBOX: u16 = 0x0001; // VID 0x1209

pub const FLASH_SIZE: u32 = 2 * 1024 * 1024;
pub const SECTOR: u32 = 4096;
/// The payload refuses writes outside this (bootloader + partition table
/// below, settings/calibration above).
pub const WRITABLE: (u32, u32) = (0xE000, 0x1FC000);
pub const CPUID: u32 = 0x410F_C241;
/// P25Q16H (docs/hardware.md); RDID bytes 85 60 15
pub const JEDEC: [u8; 3] = [0x85, 0x60, 0x15];
pub const CHARGE_LIMIT_MV: u32 = 4180;

const CMD_WRITERAM: u8 = 9;
const CMD_RUNRAM: u8 = 10;
const LOAD: u32 = 0x806000;
const ENTRY: u32 = 0x806001;
const ARGS: u32 = 0x806c00;
const RESULT: u32 = 0x806c40;
const STAGE: u32 = 0x807000;
const SVC_MAGIC: u32 = 0x5653_3359;
const REPLY_MAGIC: u32 = 0x5052_3359;
// ROM reply buffer: descriptor 0x800de8 + 8; the tiny reader sits after it
const TINY_AT: u32 = 0x800de8 + 8 + 64;

const SVC: &[u8] = include_bytes!("../../payloads/yp3/svc.bin");

/// smtlink_dump.c read_mem for the 6801: copy `len` bytes from the address
/// in the CDB to the reply buffer (memcpy at ROM 0xb2a).
const TINY: [u16; 10] = [0xf8d4, 0x2210, 0xf8d4, 0x1219, 0x80e2, 0xf640, 0x332b, 0xf104, 0x0008, 0x4718];

#[derive(Clone, Copy, Debug, PartialEq)]
#[repr(u32)]
pub enum Op {
    Info = 0,
    Read32 = 1,
    Write32 = 2,
    PmuRead = 3,
    PmuWrite = 4,
    Boot = 5,
    ResetToRom = 6,
    FlashCrc = 7,
    FlashSector = 8,
    FlashRead = 9,
    Jedec = 10,
}

const STATUS: [&str; 12] = [
    "ok",
    "bad_args",
    "spi_busy",
    "spi_timeout",
    "flash_state",
    "wel",
    "wip_timeout",
    "verify",
    "stage_crc",
    "pmu",
    "pmu_flags",
    "unchanged",
];

pub struct Yp3 {
    bot: Bot,
    tiny_loaded: bool,
    svc_loaded: bool,
    pub serial: Option<String>,
}

fn cdb(cmd: u8, len: u32, addr: u32) -> [u8; 16] {
    let mut c = [0u8; 16];
    c[0] = 0xc0;
    c[1] = cmd;
    c[2..6].copy_from_slice(&len.to_le_bytes());
    c[6..10].copy_from_slice(&addr.to_le_bytes());
    c
}

pub fn crc32(d: &[u8]) -> u32 {
    crc32fast::hash(d)
}

/// A sector write the payload reported as failed (the simulator's failure
/// demo raises it too).
pub fn sector_failed(addr: u32, why: &str) -> String {
    err!(E315, "sector 0x{addr:06x}: {why}")
}

impl Yp3 {
    pub fn open() -> Result<Yp3, String> {
        let link = Link::open(VID, PID_ROM)?;
        let serial = link.serial();
        let mut y = Yp3 { bot: Bot::new(link, false, true), tiny_loaded: false, svc_loaded: false, serial };
        y.bot.link.drain();
        Ok(y)
    }

    /// SCSI INQUIRY: vendor/product string ("SMTLINK DEVICE 2.00").
    pub fn inquiry(&mut self) -> Result<String, String> {
        let mut c = [0u8; 16];
        c[0] = 0x12;
        c[4] = 0x24;
        let d = self.bot.exec(&c, 6, Data::In(0x24), Duration::from_secs(2))?;
        Ok(String::from_utf8_lossy(&d[8..]).trim_matches(|c: char| c == '\0' || c.is_whitespace()).to_string())
    }

    pub fn write_mem(&mut self, addr: u32, data: &[u8]) -> Result<(), String> {
        for (i, chunk) in data.chunks(1024).enumerate() {
            let a = addr + (i * 1024) as u32;
            self.bot.exec(&cdb(CMD_WRITERAM, chunk.len() as u32, a), 16, Data::Out(chunk), Duration::from_secs(2))?;
        }
        Ok(())
    }

    /// RAM read, 64 bytes per ROM transaction.
    pub fn read_mem(&mut self, addr: u32, n: usize) -> Result<Vec<u8>, String> {
        if !self.tiny_loaded {
            let code: Vec<u8> = TINY.iter().flat_map(|h| h.to_le_bytes()).collect();
            self.write_mem(TINY_AT, &code)?;
            self.tiny_loaded = true;
        }
        let mut out = Vec::with_capacity(n);
        while out.len() < n {
            let k = (n - out.len()).min(64);
            let a = addr + out.len() as u32;
            let d = self.bot.exec(&cdb(CMD_RUNRAM, a, TINY_AT | 1), 16, Data::In(k), Duration::from_secs(2))?;
            out.extend_from_slice(&d);
        }
        Ok(out)
    }

    fn load_svc(&mut self) -> Result<(), String> {
        if !self.svc_loaded {
            self.write_mem(LOAD, SVC)?;
            let back = self.read_mem(LOAD, SVC.len())?;
            if back != SVC {
                return Err(err!(E230, "payload did not read back from RAM"));
            }
            self.svc_loaded = true;
        }
        Ok(())
    }

    /// Run one payload op; (status, the 16 reply words).
    pub fn call(&mut self, op: Op, a: [u32; 3], stage: Option<&[u8]>, timeout: Duration) -> Result<(&'static str, [u32; 16]), String> {
        self.load_svc()?;
        let mut args = Vec::with_capacity(128);
        for w in [SVC_MAGIC, op as u32, a[0], a[1], a[2]] {
            args.extend_from_slice(&w.to_le_bytes());
        }
        args.resize(128, 0); // also clears RESULT
        self.write_mem(ARGS, &args)?;
        if let Some(s) = stage {
            self.write_mem(STAGE, s)?;
        }
        let mut raw = self.bot.exec(&cdb(CMD_RUNRAM, 0, ENTRY), 16, Data::In(64), timeout)?;
        let words = |r: &[u8]| -> [u32; 16] {
            let mut w = [0u32; 16];
            for i in 0..16 {
                w[i] = u32::from_le_bytes(r[i * 4..i * 4 + 4].try_into().unwrap());
            }
            w
        };
        let mut w = words(&raw);
        if w[0] != REPLY_MAGIC || w[1] != op as u32 {
            // the RESULT mirror is authoritative (yp3dev reads it)
            raw = self.read_mem(RESULT, 64)?;
            w = words(&raw);
            if w[0] != REPLY_MAGIC || w[1] != op as u32 {
                return Err(err!(E231, "{op:?}: bad payload reply"));
            }
        }
        let st = STATUS.get(w[2] as usize).copied().unwrap_or("unknown");
        Ok((st, w))
    }

    pub fn ok(&mut self, op: Op, a: [u32; 3]) -> Result<[u32; 16], String> {
        let (st, w) = self.call(op, a, None, Duration::from_secs(5))?;
        if st != "ok" {
            return Err(err!(E232, "{op:?}: {st}"));
        }
        Ok(w)
    }

    pub fn info(&mut self) -> Result<[u32; 16], String> {
        self.ok(Op::Info, [0; 3])
    }

    pub fn jedec(&mut self) -> Result<[u8; 3], String> {
        let w = self.ok(Op::Jedec, [0; 3])?;
        let b = w[3].to_le_bytes();
        Ok([b[0], b[1], b[2]])
    }

    pub fn pmu_read(&mut self, idx: u32) -> Result<u32, String> {
        Ok(self.ok(Op::PmuRead, [idx, 0, 0])?[3])
    }

    pub fn pmu_write(&mut self, idx: u32, v: u32) -> Result<(), String> {
        self.ok(Op::PmuWrite, [idx, v, 0]).map(|_| ())
    }

    /// Battery mV (stock method) and charge enable; charging is switched
    /// off at CHARGE_LIMIT_MV, since nothing in ROM USB stops it.
    pub fn battery_guard(&mut self) -> Result<(u32, bool, bool), String> {
        let r47 = self.pmu_read(0x47)?;
        self.pmu_write(0x47, r47 | 0x10)?;
        std::thread::sleep(Duration::from_millis(50));
        let raw = self.pmu_read(0x2e)?;
        self.pmu_write(0x47, r47 & !0x10)?;
        let mv = (2800.0 + 14.173 * (raw & 127) as f32) as u32;
        let charge = self.pmu_read(0x17)?;
        let mut stopped = false;
        if charge & 4 != 0 && mv >= CHARGE_LIMIT_MV {
            self.pmu_write(0x17, charge & !4)?;
            stopped = true;
        }
        Ok((mv, charge & 4 != 0 && !stopped, stopped))
    }

    /// Stop charging regardless (before leaving the player in ROM USB).
    pub fn charge_off(&mut self) -> Result<(), String> {
        let charge = self.pmu_read(0x17)?;
        if charge & 4 != 0 {
            self.pmu_write(0x17, charge & !4)?;
        }
        Ok(())
    }

    /// CRC32 of `count` sectors from `start` (computed on the player).
    pub fn flash_crcs(&mut self, start: u32, count: u32) -> Result<Vec<u32>, String> {
        let mut out = Vec::new();
        let mut s = start;
        while s < start + count {
            let n = (start + count - s).min(256);
            let (st, _) = self.call(Op::FlashCrc, [s * SECTOR, n, 0], None, Duration::from_secs(8))?;
            if st != "ok" {
                return Err(err!(E320, "flash CRC: {st}"));
            }
            let raw = self.read_mem(STAGE, n as usize * 4)?;
            out.extend(raw.chunks(4).map(|c| u32::from_le_bytes(c.try_into().unwrap())));
            s += n;
        }
        Ok(out)
    }

    pub fn read_sector(&mut self, addr: u32) -> Result<Vec<u8>, String> {
        self.ok(Op::FlashRead, [addr, SECTOR, 0])?;
        self.read_mem(STAGE, SECTOR as usize)
    }

    /// Erase + program + verify one sector on the player. Ok(false) if it
    /// already matched.
    pub fn write_sector(&mut self, addr: u32, data: &[u8]) -> Result<bool, String> {
        assert_eq!(data.len(), SECTOR as usize);
        if addr < WRITABLE.0 || addr >= WRITABLE.1 {
            return Err(err!(E310, "refusing to write protected sector 0x{addr:06x}"));
        }
        let (st, _) = self.call(Op::FlashSector, [addr, crc32(data), 0], Some(data), Duration::from_secs(8))?;
        match st {
            "ok" => Ok(true),
            "unchanged" => Ok(false),
            e => Err(sector_failed(addr, e)),
        }
    }

    /// Reset into the flashed image (clears the recovery reason). The
    /// player drops off USB, so no reply is expected.
    pub fn boot(&mut self) -> Result<(), String> {
        self.load_svc()?;
        let mut args = Vec::new();
        for w in [SVC_MAGIC, Op::Boot as u32, 0, 0, 0] {
            args.extend_from_slice(&w.to_le_bytes());
        }
        args.resize(128, 0);
        self.write_mem(ARGS, &args)?;
        // A reply means the PMU flags looked wrong and it did not reset.
        match self.bot.exec(&cdb(CMD_RUNRAM, 0, ENTRY), 16, Data::In(64), Duration::from_millis(2500)) {
            Ok(r) => {
                let st = u32::from_le_bytes(r[8..12].try_into().unwrap());
                Err(err!(E330, "reset refused: {}", STATUS.get(st as usize).unwrap_or(&"?")))
            }
            Err(_) => Ok(()),
        }
    }
}
