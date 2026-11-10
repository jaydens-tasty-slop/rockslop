//! TS17 (Actions "Lark", chip 0x2351) USB download mode (ADFU, 10d6:10d6),
//! ported from rockbox-yp3 ts17dev/adfu.py, norprobe.py and stock.py. The
//! ADFU command set was worked out from Ilya Kurdyukov's actions_flash and
//! cross-checked against Nicholas FitzRoy-Dale's atj2127decrypt (CREDITS.md).
//!
//! The ROM's switch command registers a hook its USB loop calls
//! continuously; we register payloads/ts17/hook.S once, which runs the
//! ops gadget (payloads/ts17/ops.c) when armed: a list of register reads,
//! writes, polls and calls into the ROM's own SPI-NOR routines.

use super::bot::{Bot, Data};
use super::usb::Link;
use crate::err;
use std::time::{Duration, Instant};

pub const VID: u16 = 0x10d6;
pub const PID_ADFU: u16 = 0x10d6;
pub const PID_STOCK: u16 = 0xb00b;
pub const PID_ROCKBOX: u16 = 0x0002; // VID 0x1209

pub const FLASH_SIZE: u32 = 0x40_0000;
pub const SECTOR: u32 = 0x1000;
/// fw0_sys: where the Rockbox core goes. Below is the boot chain and
/// fw0_rec (the way back); above, fw0_sdfs and nvram hold stock's
/// Bluetooth patch/table and MAC, which Rockbox reads.
pub const SYS: u32 = 0x14000;
pub const WRITABLE: (u32, u32) = (0x14000, 0x1BB000);
pub const CHIP: u16 = 0x2351;

/// A partition's (offset, size) in the flash's partition table: `table`
/// starts at its `ACPT` magic (NOR 0x1000, descrambled). The header gives
/// the entry count (u16 at 8) and size (u16 at 10, 24); each entry is an
/// 8-byte name, four type/flag bytes, then offset and size (u32 LE).
pub fn partition(table: &[u8], name: &str) -> Option<(u32, u32)> {
    if table.get(..4)? != b"ACPT" {
        return None;
    }
    let u16_at = |o: usize| table.get(o..o + 2).map(|b| u16::from_le_bytes([b[0], b[1]]) as usize);
    let (count, size) = (u16_at(8)?, u16_at(10)?);
    if size != 24 {
        return None;
    }
    let u32_at = |e: &[u8], o: usize| u32::from_le_bytes([e[o], e[o + 1], e[o + 2], e[o + 3]]);
    table.get(16..16 + count * size)?.chunks_exact(size).find_map(|e| {
        let n = &e[..8];
        let n = &n[..n.iter().position(|&b| b == 0).unwrap_or(8)];
        (n == name.as_bytes()).then(|| (u32_at(e, 12), u32_at(e, 16)))
    })
}
pub const CPUID: u32 = 0x410F_C241;

const CMD_READRAM: u32 = 0x93;
const CMD_WRITERAM: u32 = 0x13;
const CMD_SWITCH: u32 = 0x20;
const ROM_BLOCK: usize = 0x200;

const HOOK_CODE: u32 = 0x0101_99c0;
const HOOK_CTL: u32 = 0x0101_9980;
const HOOK_ARM: u32 = 0xa4a4_c0de;
const HOOK_DONE: u32 = 0xd0d0_d0d0;
const GADGET_CODE: u32 = 0x0101_8000;
const GADGET_ARGS: u32 = 0x0101_8800;
const GADGET_BUF: u32 = 0x0104_0000;
const CALL_ARGS: u32 = 0x0101_9c00;
const FAULT_TABLE: u32 = 0x0101_a000;
const COPY_BUF: u32 = 0x0104_8000;
const CHUNK: u32 = 0x8000;

const NOR_CTL: u32 = 0x4002_8000;
const CACHE_CTL: u32 = 0x4001_4000;
const CACHE_INV: u32 = 0x4001_4004;
const REMAP2: u32 = 0x4001_0310;
const WINDOW: u32 = 0x1200_0000;
const RANDOMIZER: u32 = 0x2000_1000;
const WDT_CTL: u32 = 0x4000_c020;
const ADFU_MAGIC_REG: u32 = 0x4000_c03c;
const SPIN_10US: u32 = 2000;

const ROM_NOR_READ: u32 = 0x4219;
const ROM_NOR_WRITE: u32 = 0x42dd;
const ROM_NOR_ERASE: u32 = 0x4135;

const CHG_CTL: u32 = 0x4000_4100;
const CHG_STOP: u32 = 1 << 19;
const CHG_CV_MASK: u32 = 0x1f << 5;
const ADC_PEND: u32 = 0x4000_4208;
const BAT_ADC: u32 = 0x4000_4214;
const CHARGE_STOP_MV: u32 = 4000;

const HOOK: &[u8] = include_bytes!("../../payloads/ts17/hook.bin");
const OPS: &[u8] = include_bytes!("../../payloads/ts17/ops.bin");

#[derive(Clone, Copy)]
#[repr(u32)]
enum K {
    W32 = 1,
    R32 = 2,
    Poll = 5,
    Delay = 6,
    Rmw = 7,
    Copy = 10,
    Call = 11,
}

#[derive(Default)]
pub struct Ops {
    ops: Vec<[u32; 4]>,
    calls: Vec<[u32; 4]>,
}

impl Ops {
    fn add(&mut self, k: K, addr: u32, val: u32, mask: u32) -> &mut Self {
        self.ops.push([k as u32, addr, val, mask]);
        self
    }
    pub fn w32(&mut self, a: u32, v: u32) -> &mut Self {
        self.add(K::W32, a, v, 0)
    }
    pub fn r32(&mut self, a: u32) -> &mut Self {
        self.add(K::R32, a, 0, 0)
    }
    pub fn poll(&mut self, a: u32, mask: u32, val: u32) -> &mut Self {
        self.add(K::Poll, a, val, mask)
    }
    pub fn delay(&mut self, n: u32) -> &mut Self {
        self.add(K::Delay, 0, n, 0)
    }
    pub fn rmw(&mut self, a: u32, mask: u32, val: u32) -> &mut Self {
        self.add(K::Rmw, a, val, mask)
    }
    pub fn copy(&mut self, src: u32, dst: u32, bytes: u32) -> &mut Self {
        self.add(K::Copy, src, dst, bytes / 4)
    }
    pub fn call(&mut self, f: u32, args: [u32; 4]) -> &mut Self {
        let slot = CALL_ARGS + 16 * self.calls.len() as u32;
        self.calls.push(args);
        self.add(K::Call, f, slot, 0)
    }
    fn invalidate(&mut self) -> &mut Self {
        self.w32(CACHE_INV, 1).poll(CACHE_INV, 1, 0)
    }
}

pub struct Ts17 {
    bot: Bot,
}

fn adfu_cdb(cmd: u32, len: u32, addr: u32) -> [u8; 16] {
    let mut c = [0u8; 16];
    c[0] = 0xcd;
    c[1..5].copy_from_slice(&cmd.to_le_bytes());
    c[5..9].copy_from_slice(&len.to_le_bytes());
    c[9..13].copy_from_slice(&addr.to_le_bytes());
    c
}

/// Stock firmware's USB disk (10d6:b00b): vendor 0xcc identifies it,
/// 0xcb reboots it into download mode (stock.py).
pub fn kick_stock() -> Result<String, String> {
    let link = Link::open(VID, PID_STOCK)?;
    let mut bot = Bot::new(link, true, false);
    bot.link.drain();
    let ident = bot.exec(&[0xcc, 0, 0, 0, 0, 0], 6, Data::In(11), Duration::from_secs(2))?;
    let ident = String::from_utf8_lossy(&ident).to_string();
    if ident != "ACTIONSUSBD" {
        return Err(err!(E250, "unexpected stock identity {ident:?}; not touching it"));
    }
    bot.exec(&[0xcb, 0, 0, 0, 0, 0], 6, Data::In(2), Duration::from_secs(2))?;
    Ok(ident)
}

impl Ts17 {
    pub fn open() -> Result<Ts17, String> {
        let link = Link::open(VID, PID_ADFU)?;
        let mut t = Ts17 { bot: Bot::new(link, true, false) };
        t.bot.link.drain();
        Ok(t)
    }

    /// ROM identify: "\0CADFUD" + chip id.
    pub fn identify(&mut self) -> Result<u16, String> {
        let mut c = [0u8; 16];
        c[0] = 0xcc;
        c[7] = 0x12;
        let d = self.bot.exec(&c, 16, Data::In(0x12), Duration::from_secs(2))?;
        if &d[..7] != b"\0CADFUD" {
            return Err(err!(E240, "not an Actions ADFU ROM ({:02x?})", &d[..8]));
        }
        Ok(u16::from_be_bytes([d[7], d[8]]))
    }

    fn read_mem(&mut self, addr: u32, n: usize, patience: Duration) -> Result<Vec<u8>, String> {
        let mut out = Vec::with_capacity(n);
        while out.len() < n {
            let k = (n - out.len()).min(ROM_BLOCK);
            let a = addr + out.len() as u32;
            let d = self.bot.exec(&adfu_cdb(CMD_READRAM, k as u32, a), 16, Data::In(k), patience)?;
            out.extend_from_slice(&d);
        }
        Ok(out)
    }

    fn write_mem(&mut self, addr: u32, data: &[u8]) -> Result<(), String> {
        for (i, chunk) in data.chunks(ROM_BLOCK).enumerate() {
            let a = addr + (i * ROM_BLOCK) as u32;
            self.bot.exec(&adfu_cdb(CMD_WRITERAM, chunk.len() as u32, a), 16, Data::Out(chunk), Duration::from_secs(2))?;
        }
        Ok(())
    }

    fn switch(&mut self, addr: u32) {
        // The ROM may or may not complete this cleanly; either is fine.
        let _ = self.bot.exec(&adfu_cdb(CMD_SWITCH, 0, addr), 16, Data::None, Duration::from_millis(1500));
    }

    /// Load the hook (unless already resident) and the ops gadget.
    pub fn install(&mut self) -> Result<(u32, u32), String> {
        let t = Duration::from_secs(2);
        if self.read_mem(HOOK_CODE, HOOK.len(), t)? != HOOK {
            self.write_mem(HOOK_CTL, &[0u8; 12])?;
            self.write_mem(HOOK_CODE, HOOK)?;
            if self.read_mem(HOOK_CODE, HOOK.len(), t)? != HOOK {
                return Err(err!(E241, "hook did not read back"));
            }
        }
        self.switch(HOOK_CODE);
        self.write_mem(GADGET_CODE, OPS)?;
        if self.read_mem(GADGET_CODE, OPS.len(), t)? != OPS {
            return Err(err!(E242, "gadget did not read back"));
        }
        let (_, st) = self.run(&Ops::default())?;
        if st[3] != FAULT_TABLE {
            return Err(err!(E243, "gadget fault table not active (VTOR {:08x}); refusing to continue", st[3]));
        }
        Ok((st[2], st[3]))
    }

    fn call(&mut self, wait: bool, timeout: Duration) -> Result<(), String> {
        let mut fn_done = Vec::new();
        fn_done.extend_from_slice(&GADGET_CODE.to_le_bytes());
        fn_done.extend_from_slice(&0u32.to_le_bytes());
        self.write_mem(HOOK_CTL + 4, &fn_done)?;
        self.write_mem(HOOK_CTL, &HOOK_ARM.to_le_bytes())?; // arm last
        if !wait {
            return Ok(());
        }
        let t0 = Instant::now();
        loop {
            let d = self.read_mem(HOOK_CTL + 8, 4, timeout)?;
            if u32::from_le_bytes(d[..4].try_into().unwrap()) == HOOK_DONE {
                return Ok(());
            }
            if t0.elapsed() > timeout {
                return Err(err!(E244, "gadget did not return"));
            }
        }
    }

    fn send_ops(&mut self, ops: &Ops) -> Result<(), String> {
        if ops.ops.len() > 256 || ops.calls.len() > 64 {
            return Err(err!(E902, "op list too long"));
        }
        let mut blob = Vec::with_capacity(16 + ops.ops.len() * 16);
        blob.extend_from_slice(&(ops.ops.len() as u32).to_le_bytes());
        blob.extend_from_slice(&[0u8; 12]);
        for o in &ops.ops {
            for w in o {
                blob.extend_from_slice(&w.to_le_bytes());
            }
        }
        self.write_mem(GADGET_ARGS, &blob)?;
        if !ops.calls.is_empty() {
            let c: Vec<u8> = ops.calls.iter().flat_map(|a| a.iter().flat_map(|w| w.to_le_bytes())).collect();
            self.write_mem(CALL_ARGS, &c)?;
        }
        Ok(())
    }

    /// Run an op list; (results, status words).
    pub fn run(&mut self, ops: &Ops) -> Result<(Vec<u32>, [u32; 16]), String> {
        self.send_ops(ops)?;
        self.call(true, Duration::from_secs(30))?;
        let head = self.read_mem(GADGET_BUF, 64, Duration::from_secs(2))?;
        let mut st = [0u32; 16];
        for i in 0..16 {
            st[i] = u32::from_le_bytes(head[i * 4..i * 4 + 4].try_into().unwrap());
        }
        if st[1] != 0 {
            return Err(err!(E245, "gadget faulted {} time(s): CFSR {:08x} BFAR {:08x}", st[1], st[4], st[6]));
        }
        let n = st[0] as usize;
        let vals = if n > 0 {
            let raw = self.read_mem(GADGET_BUF + 64, 4 * n, Duration::from_secs(2))?;
            raw.chunks(4).map(|c| u32::from_le_bytes(c.try_into().unwrap())).collect()
        } else {
            Vec::new()
        };
        Ok((vals, st))
    }

    pub fn read32(&mut self, a: u32) -> Result<u32, String> {
        let mut o = Ops::default();
        o.r32(a);
        Ok(self.run(&o)?.0[0])
    }

    /// Lower the CV ceiling, read the battery, stop charging at >= 4.0 V
    /// (norprobe.charger_guard). Returns (mV or None, charging, stopped).
    pub fn charger_guard(&mut self) -> Result<(Option<u32>, bool, bool), String> {
        let mut o = Ops::default();
        o.rmw(CHG_CTL, CHG_CV_MASK, 0).delay(3000).r32(CHG_CTL);
        o.w32(ADC_PEND, 2).poll(ADC_PEND, 2, 2).r32(BAT_ADC);
        let (v, _) = self.run(&o)?;
        let (ctl, spins, bat) = (v[0], v[1], v[2]);
        let fresh = spins != 0xffff_ffff;
        let mv = (bat & 0x3fff) * 300 / 1024;
        let charging = ctl & CHG_STOP == 0;
        let mut stopped = false;
        if charging && (mv >= CHARGE_STOP_MV || !fresh) {
            let mut o = Ops::default();
            o.rmw(CHG_CTL, 0, CHG_STOP);
            self.run(&o)?;
            stopped = true;
        }
        Ok((if fresh { Some(mv) } else { None }, charging && !stopped, stopped))
    }

    /// mbrec's cache + NOR controller setup; raw = randomizer off (we read
    /// and write stored cells).
    pub fn nor_setup(&mut self, raw: bool) -> Result<(), String> {
        let mut o = Ops::default();
        o.w32(0x4000_1020, 0x102);
        o.w32(CACHE_CTL, 0);
        o.rmw(0x4000_1004, 0, 1 << 8).delay(SPIN_10US);
        o.rmw(0x4000_0000, 1 << 8, 0).delay(SPIN_10US / 10);
        o.rmw(0x4000_0000, 0, 1 << 8).delay(SPIN_10US);
        o.w32(CACHE_CTL, 0x21);
        o.invalidate();
        o.rmw(NOR_CTL, 0, 0x0030_0000);
        o.rmw(NOR_CTL, RANDOMIZER, if raw { 0 } else { RANDOMIZER });
        o.rmw(NOR_CTL, 0x100, 0);
        self.run(&o).map(|_| ())
    }

    /// 32 KiB of NOR at `off` through the XIP remap window.
    pub fn read_chunk(&mut self, off: u32) -> Result<Vec<u8>, String> {
        let mut o = Ops::default();
        o.w32(REMAP2, 0).w32(REMAP2 + 4, 0);
        o.invalidate();
        o.w32(REMAP2, WINDOW | 0xf).w32(REMAP2 + 4, off);
        o.invalidate();
        o.copy(WINDOW, COPY_BUF, CHUNK);
        self.run(&o)?;
        self.read_mem(COPY_BUF, CHUNK as usize, Duration::from_secs(2))
    }

    pub fn chunk_size() -> u32 {
        CHUNK
    }

    /// Stored cells of one sector, through the ROM's SPI-NOR read.
    pub fn rom_read(&mut self, addr: u32, n: u32) -> Result<Vec<u8>, String> {
        let mut o = Ops::default();
        o.call(ROM_NOR_READ, [0, addr, COPY_BUF, n]);
        self.run(&o)?;
        self.read_mem(COPY_BUF, n as usize, Duration::from_secs(2))
    }

    /// Erase, program (unless blank) and verify one 4 KiB sector of cells.
    pub fn write_sector(&mut self, addr: u32, cells: &[u8]) -> Result<(), String> {
        if addr < WRITABLE.0 || addr >= WRITABLE.1 || addr % SECTOR != 0 || cells.len() != SECTOR as usize {
            return Err(err!(E311, "refusing to write sector 0x{addr:06x}"));
        }
        let mut o = Ops::default();
        o.call(ROM_NOR_ERASE, [0, addr, SECTOR, 0]);
        let (v, _) = self.run(&o)?;
        if v.first().copied().unwrap_or(1) != 0 {
            return Err(err!(E316, "erase 0x{addr:06x} failed ({:#x})", v[0]));
        }
        if cells.iter().any(|&b| b != 0xff) {
            self.write_mem(COPY_BUF, cells)?;
            let mut o = Ops::default();
            o.call(ROM_NOR_WRITE, [0, addr, COPY_BUF, SECTOR]);
            let (v, _) = self.run(&o)?;
            if v.first().copied().unwrap_or(1) != 0 {
                return Err(err!(E317, "program 0x{addr:06x} failed ({:#x})", v[0]));
            }
        }
        if self.rom_read(addr, SECTOR)? != cells {
            return Err(err!(E318, "verify failed at 0x{addr:06x}"));
        }
        Ok(())
    }

    /// Watchdog reset; clearing the ADFU magic in the same gadget call
    /// boots the flashed firmware cold (norprobe reboot stock).
    pub fn boot(&mut self) -> Result<(), String> {
        let mut o = Ops::default();
        o.w32(ADFU_MAGIC_REG, 0).w32(WDT_CTL, 0x5f).delay(0xffff_ffff);
        self.send_ops(&o)?;
        self.call(false, Duration::ZERO)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    /// The stock TS17's partition table, as far as fw0_sys (header and the
    /// first six entries, from a descrambled dump).
    fn stock_table() -> Vec<u8> {
        let mut t = b"ACPT\x01\x01\xe8\x02\x0f\x00\x18\x00\x00\x00\x00\x00".to_vec();
        for (name, kind, off, size) in [
            ("fw0_boot", [1, 1, 0, 2], 0x0, 0x1000),
            ("fw0_para", [6, 2, 0, 2], 0x1000, 0x1000),
            ("fw1_boot", [1, 1, 1, 2], 0x2000, 0x1000),
            ("fw1_para", [6, 2, 1, 2], 0x3000, 0x1000),
            ("fw0_rec", [3, 3, 0, 2], 0x4000, 0x10000),
            ("fw0_sys", [2, 4, 0, 2], 0x14000, 0x1a7000),
        ] {
            let mut n = [0u8; 8];
            n[..name.len()].copy_from_slice(name.as_bytes());
            t.extend_from_slice(&n);
            t.extend_from_slice(&kind);
            for v in [off, size, off] {
                t.extend_from_slice(&(v as u32).to_le_bytes());
            }
        }
        t.resize(16 + 15 * 24, 0);
        t
    }

    #[test]
    fn fw0_sys_is_where_we_write() {
        let t = stock_table();
        let (off, size) = partition(&t, "fw0_sys").unwrap();
        assert_eq!((off, off + size), WRITABLE);
        assert_eq!(partition(&t, "fw0_rec"), Some((0x4000, 0x10000)));
        assert_eq!(partition(&t, "fw0_sy"), None);
        let mut moved = t.clone();
        moved[16 + 5 * 24 + 12] = 0x10; // fw0_sys at 0x14010
        assert_ne!(partition(&moved, "fw0_sys").map(|(o, s)| (o, o + s)), Some(WRITABLE));
        assert_eq!(partition(&t[1..], "fw0_sys"), None, "no magic");
    }
}
