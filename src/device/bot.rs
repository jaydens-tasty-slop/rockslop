//! USB mass storage Bulk-Only Transport framing (CBW / data / CSW), as both
//! boot ROMs speak it with vendor CDBs. On Windows a ROM on Windows' own
//! disk driver gets the same commands through SCSI pass-through instead.

use super::usb::Link;
use crate::err;
use std::time::Duration;

const CBW_SIG: u32 = 0x4342_5355;
const CSW_SIG: u32 = 0x5342_5355;

/// What the commands go over.
pub enum Pipe {
    /// raw bulk endpoints (libusb)
    Usb(Link),
    /// Windows' own USB disk driver
    #[cfg(windows)]
    Disk(super::spt::Disk),
}

pub struct Bot {
    pipe: Pipe,
    tag: u32,
    /// the Actions ROM wants tag 0 and echoes it
    zero_tag: bool,
    /// SmartLink: residue must be 0 too
    strict: bool,
}

pub enum Data<'a> {
    None,
    In(usize),
    Out(&'a [u8]),
}

impl Bot {
    pub fn new(pipe: Pipe, zero_tag: bool, strict: bool) -> Bot {
        Bot { pipe, tag: 1, zero_tag, strict }
    }

    /// Discard anything a previous session left unread.
    pub fn drain(&mut self) {
        match &mut self.pipe {
            Pipe::Usb(link) => link.drain(),
            #[cfg(windows)]
            Pipe::Disk(_) => {}
        }
    }

    pub fn serial(&self) -> Option<String> {
        match &self.pipe {
            Pipe::Usb(link) => link.serial(),
            #[cfg(windows)]
            Pipe::Disk(d) => d.serial.clone(),
        }
    }

    fn cbw(&self, cdb: &[u8], cdb_len: u8, len: u32, dir_in: bool) -> [u8; 31] {
        let mut b = [0u8; 31];
        b[0..4].copy_from_slice(&CBW_SIG.to_le_bytes());
        b[4..8].copy_from_slice(&(if self.zero_tag { 0 } else { self.tag }).to_le_bytes());
        b[8..12].copy_from_slice(&len.to_le_bytes());
        b[12] = if dir_in { 0x80 } else { 0 };
        b[13] = 0;
        b[14] = cdb_len;
        b[15..15 + cdb.len().min(16)].copy_from_slice(&cdb[..cdb.len().min(16)]);
        b
    }

    /// Full command: returns the IN data (empty for None/Out).
    pub fn exec(&mut self, cdb: &[u8], cdb_len: u8, data: Data, timeout: Duration) -> Result<Vec<u8>, String> {
        let want_tag = if self.zero_tag { 0 } else { self.tag };
        let cbw = self.cbw(cdb, cdb_len, data.len() as u32, matches!(data, Data::In(_)));
        let r = match &mut self.pipe {
            Pipe::Usb(link) => bulk(link, &cbw, data, want_tag, timeout),
            #[cfg(windows)]
            Pipe::Disk(d) => d.exec(cdb, cdb_len, data, timeout),
        };
        self.tag = self.tag.wrapping_add(1).max(1);
        let (status, residue, out) = r?;
        if status != 0 || (self.strict && residue != 0) {
            return Err(err!(E221, "command failed (status {status}, residue {residue})"));
        }
        Ok(out)
    }
}

impl Data<'_> {
    fn len(&self) -> usize {
        match self {
            Data::None => 0,
            Data::In(n) => *n,
            Data::Out(d) => d.len(),
        }
    }
}

/// CBW, data, CSW over the bulk pipes: (status, residue, IN data).
fn bulk(link: &mut Link, cbw: &[u8; 31], data: Data, want_tag: u32, timeout: Duration) -> Result<(u8, u32, Vec<u8>), String> {
    link.write(cbw, timeout)?;
    let out = match data {
        Data::Out(d) => {
            link.write(d, timeout)?;
            Vec::new()
        }
        Data::In(n) if n > 0 => link.read(n, timeout)?,
        _ => Vec::new(),
    };
    let csw = link.read(13, timeout)?;
    let sig = u32::from_le_bytes(csw[0..4].try_into().unwrap());
    let tag = u32::from_le_bytes(csw[4..8].try_into().unwrap());
    let residue = u32::from_le_bytes(csw[8..12].try_into().unwrap());
    if sig != CSW_SIG || tag != want_tag {
        return Err(err!(E220, "bad status block (sig {sig:08x} tag {tag})"));
    }
    Ok((csw[12], residue, out))
}
