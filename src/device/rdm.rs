//! TS17 NOR data randomizer (Actions Lark NOR_CTL bit 12), ported from
//! rockbox-yp3 `ts17dev/rdm.py`: the model fitted to two NOR dumps. Reads
//! with the randomizer on descramble; writes store bytes as given, so an
//! image is encoded before it is written. XIP lines restart every 32 bytes.

const K0: [u8; 36] = [
    0xce, 0xb9, 0xb0, 0xb3, 0x86, 0x6b, 0xd0, 0x75, 0x9d, 0x1e, 0xda, 0x96, 0xbc, 0x5c, 0xeb, 0xb5, 0x1f, 0x5a, 0x63, 0x13, 0x70, 0x06,
    0xdc, 0x28, 0xc1, 0xc6, 0x98, 0x24, 0x6c, 0xa8, 0xc3, 0xc2, 0x82, 0xa1, 0x94, 0x6c,
];
const G0: [u8; 36] = [
    0x00, 0x02, 0x87, 0x8b, 0x10, 0x60, 0x06, 0x0d, 0x1e, 0x7f, 0xbc, 0x7c, 0xbf, 0xfc, 0x3e, 0xf9, 0x15, 0x2a, 0x93, 0xa0, 0x63, 0x45,
    0xaa, 0x71, 0x42, 0xc6, 0x2c, 0xbf, 0xff, 0x1f, 0x4d, 0x88, 0x92, 0x47, 0x5a, 0xc2,
];
const TAPS: [usize; 14] = [1, 4, 5, 6, 8, 10, 12, 13, 22, 26, 27, 28, 30, 31];

pub const XIP_RUN: usize = 32;
pub const BOOT_RUN: usize = 512;

fn extend(seed: &[u8; 36]) -> [u8; BOOT_RUN] {
    let mut x = [0u8; BOOT_RUN];
    x[..36].copy_from_slice(seed);
    for i in 36..BOOT_RUN {
        x[i] = TAPS.iter().fold(0, |v, &e| v ^ x[i - e]);
    }
    x
}

pub struct Rdm {
    k: [u8; BOOT_RUN],
    g: [u8; BOOT_RUN],
}

impl Rdm {
    pub fn new() -> Rdm {
        Rdm { k: extend(&K0), g: extend(&G0) }
    }

    fn transform(&self, data: &[u8], run: usize, encode: bool) -> Vec<u8> {
        assert!(data.len() % run == 0, "length must be a multiple of the run");
        let mut out = vec![0u8; data.len()];
        let mut mask = vec![0u8; run];
        for (src, dst) in data.chunks(run).zip(out.chunks_mut(run)) {
            mask.copy_from_slice(&self.k[..run]);
            for b in 0..run {
                dst[b] = src[b] ^ mask[b];
                let cell = if encode { dst[b] } else { src[b] };
                if (cell & 0x3c).count_ones() & 1 == 1 {
                    for j in b + 1..run {
                        mask[j] ^= self.g[j - b];
                    }
                }
            }
        }
        out
    }

    /// Plain bytes -> the cells to program, so a randomized read returns plain.
    pub fn encode(&self, plain: &[u8], run: usize) -> Vec<u8> {
        self.transform(plain, run, true)
    }

    /// Stored cells -> what a randomized read returns.
    pub fn decode(&self, cells: &[u8], run: usize) -> Vec<u8> {
        self.transform(cells, run, false)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    fn h(s: &str) -> Vec<u8> {
        (0..s.len() / 2).map(|i| u8::from_str_radix(&s[2 * i..2 * i + 2], 16).unwrap()).collect()
    }
    #[test]
    fn vectors() {
        let r = Rdm::new();
        assert_eq!(&r.k[504..512], &h("9c8ffe4fbaf307c8")[..]);
        assert_eq!(&r.g[504..512], &h("95b9c2847e8b2046")[..]);
        // vectors from the Python model (ts17dev/rdm.py)
        assert_eq!(r.encode(&[0u8; 32], 32), h("ceb9b2340ffc3b617508b64ac72a02a5bcbba7fdaddf8d2c49f292b70f2b4034"));
        let seq: Vec<u8> = (0..32).collect();
        assert_eq!(r.encode(&seq, 32), h("ceb8b0370bfbb86ae4f4d6d32fabd41908252c9bb6b02ef68a7aeec06846f9a5"));
        assert_eq!(r.encode(&[0xff; 32], 32), h("31464dcbf003c49e8af749b538d5fd5a43445802522072d3b60d6d48f0d4bfcb"));
        let e = r.encode(&seq, 32);
        assert_eq!(r.decode(&e, 32), seq);
    }
}
