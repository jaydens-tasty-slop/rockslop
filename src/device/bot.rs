//! USB mass storage Bulk-Only Transport framing (CBW / data / CSW), as both
//! boot ROMs speak it with vendor CDBs.

use super::usb::Link;
use crate::err;
use std::time::Duration;

const CBW_SIG: u32 = 0x4342_5355;
const CSW_SIG: u32 = 0x5342_5355;

pub struct Bot {
    pub link: Link,
    tag: u32,
    /// the Actions ROM wants tag 0 and echoes it
    zero_tag: bool,
    /// SmartLink: residue must be 0 too
    strict: bool,
    pub timeout: Duration,
}

pub enum Data<'a> {
    None,
    In(usize),
    Out(&'a [u8]),
}

impl Bot {
    pub fn new(link: Link, zero_tag: bool, strict: bool) -> Bot {
        Bot { link, tag: 1, zero_tag, strict, timeout: Duration::from_millis(2000) }
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

    /// Send the CBW (and OUT data); don't wait for anything. For commands
    /// after which the device resets itself.
    pub fn fire(&mut self, cdb: &[u8], cdb_len: u8, data: Data) -> Result<(), String> {
        let (len, dir_in) = match &data {
            Data::None => (0, false),
            Data::In(n) => (*n as u32, true),
            Data::Out(d) => (d.len() as u32, false),
        };
        let cbw = self.cbw(cdb, cdb_len, len, dir_in);
        self.link.write(&cbw, self.timeout)?;
        if let Data::Out(d) = data {
            self.link.write(d, self.timeout)?;
        }
        Ok(())
    }

    /// Full command: returns the IN data (empty for None/Out).
    pub fn exec(&mut self, cdb: &[u8], cdb_len: u8, data: Data, timeout: Duration) -> Result<Vec<u8>, String> {
        let n_in = if let Data::In(n) = data { n } else { 0 };
        let saved = self.timeout;
        self.timeout = timeout;
        let r = (|| {
            self.fire(cdb, cdb_len, data)?;
            let out = if n_in > 0 { self.link.read(n_in, timeout)? } else { Vec::new() };
            let csw = self.link.read(13, timeout)?;
            let sig = u32::from_le_bytes(csw[0..4].try_into().unwrap());
            let tag = u32::from_le_bytes(csw[4..8].try_into().unwrap());
            let residue = u32::from_le_bytes(csw[8..12].try_into().unwrap());
            let want_tag = if self.zero_tag { 0 } else { self.tag };
            if sig != CSW_SIG || tag != want_tag {
                return Err(err!(E220, "bad status block (sig {sig:08x} tag {tag})"));
            }
            if csw[12] != 0 || (self.strict && residue != 0) {
                return Err(err!(E221, "command failed (status {}, residue {residue})", csw[12]));
            }
            Ok(out)
        })();
        self.timeout = saved;
        self.tag = self.tag.wrapping_add(1).max(1);
        r
    }
}
