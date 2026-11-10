//! Sound: tracker modules while a job runs and after it, shuffled, each
//! crossfading into a different one when it ends (rendered live by
//! xmrsplayer, so the binary carries kilobytes, not megabytes of audio);
//! a voice line at the end, picked at random; and a tape-stop when a job
//! fails. Before the music starts, the interface has sounds of its own
//! (the music takes over from them). A follower on the kick drum gives the
//! visuals something to pulse with. Silent, never fatal, when there is no
//! audio device.

use cpal::traits::{DeviceTrait, HostTrait, StreamTrait};
use std::f32::consts::TAU;
use std::io::Read;
use std::sync::atomic::{AtomicBool, AtomicU32, AtomicUsize, Ordering};
use std::sync::{Arc, OnceLock};
use xmrs::core::module::Module;
use xmrsplayer::prelude::XmrsPlayer;

const MUSIC_GAIN: f32 = 0.9;
const VOICE_GAIN: f32 = 1.0;
/// music level under the voice
const DUCK: f32 = 0.35;
/// seconds one track takes to hand over to the next
const XFADE: f32 = 3.0;

/// A track: credited in CREDITS.md (a test checks), shown when it starts.
pub struct Track {
    pub title: &'static str,
    pub artist: &'static str,
    pub url: &'static str,
    pub licence: &'static str,
    /// the module, xz-compressed
    data: &'static [u8],
    /// where to start handing over to the next track, in seconds: before the
    /// song wraps round to its start (or, for one, trails off into silence)
    end: f32,
    /// evens out loudness between tracks (to an RMS of ~0.15 over the
    /// whole track, measured with `every_track_loads_and_plays`)
    gain: f32,
}

macro_rules! module {
    ($f:literal) => {
        include_bytes!(concat!("../assets/audio/music/", $f, ".xz"))
    };
}

/// a module's page on The Mod Archive
macro_rules! ma {
    ($id:literal) => {
        concat!("https://modarchive.org/index.php?request=view_by_moduleid&query=", $id)
    };
}

pub const TRACKS: &[Track] = &[
    Track {
        title: "Kicking It Powerful",
        artist: "null1024",
        url: ma!("170242"),
        licence: "CC BY 4.0",
        data: module!("kicking_it_powerful.xm"),
        end: 265.5,
        gain: 1.0,
    },
    Track {
        title: "Keygen Wraith",
        artist: "Drozerix",
        url: ma!("207854"),
        licence: "Public domain",
        data: module!("keygen_wraith.xm"),
        end: 93.3 - XFADE,
        gain: 0.93,
    },
    Track {
        title: "m4's chipgabber",
        artist: "M4kachu",
        url: ma!("212725"),
        licence: "CC BY 4.0",
        data: module!("m4s_chipgabber.it"),
        end: 138.08 - XFADE,
        gain: 1.4,
    },
    Track {
        title: "Glitch Abuse",
        artist: "usrfriendly",
        url: ma!("163775"),
        licence: "CC0 1.0",
        data: module!("glitch_abuse.xm"),
        end: 77.3 - XFADE,
        gain: 1.8,
    },
    Track {
        title: "Kick the beat",
        artist: "Andreas Viklund (TSEC)",
        url: ma!("34320"),
        licence: "CC BY 4.0",
        data: module!("kick_the_beat.xm"),
        end: 317.5 - XFADE,
        gain: 0.6,
    },
    Track {
        title: "Gate 303",
        artist: "Andreas Viklund",
        url: ma!("34313"),
        licence: "CC BY 4.0",
        data: module!("gate_303.xm"),
        end: 231.9 - XFADE,
        gain: 1.1,
    },
    Track {
        title: "Neon Techno",
        artist: "Drozerix",
        url: ma!("178172"),
        licence: "Public domain",
        data: module!("neon_techno.mod"),
        end: 212.3 - XFADE,
        gain: 0.64,
    },
    Track {
        title: "Bassline Techno",
        artist: "Katie Cadet",
        url: ma!("175335"),
        licence: "Public domain",
        data: module!("bassline_techno.xm"),
        end: 101.8 - XFADE,
        gain: 0.97,
    },
];

/// The voice lines ("access granted"); one plays when a job completes.
const VOICES: &[&[u8]] = &[
    include_bytes!("../assets/audio/voice/sonoticai_male.wav"),
    include_bytes!("../assets/audio/voice/sonoticai_female.wav"),
    include_bytes!("../assets/audio/voice/johnnyray_british.wav"),
];

/// The interface's sounds.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum Sfx {
    Hover,
    Click,
    Confirm,
    Transition,
    Boot,
    Link,
    Error,
    Drop,
}

/// Each `Sfx`'s sound (from Freesound, CC0; credited in CREDITS.md) and
/// level, in the enum's order. The levels even out loudness (about -17 dBFS
/// over the loudest 50 ms; the hover tick well under that).
const SFX: [(&[u8], f32); 8] = [
    (include_bytes!("../assets/audio/sfx/hover.wav"), 0.13),
    (include_bytes!("../assets/audio/sfx/click.wav"), 0.2),
    (include_bytes!("../assets/audio/sfx/confirm.wav"), 0.6),
    (include_bytes!("../assets/audio/sfx/transition.wav"), 0.3),
    (include_bytes!("../assets/audio/sfx/boot.wav"), 0.55),
    (include_bytes!("../assets/audio/sfx/link.wav"), 0.25),
    (include_bytes!("../assets/audio/sfx/error.wav"), 0.5),
    (include_bytes!("../assets/audio/sfx/drop.wav"), 0.16),
];

#[derive(Default)]
struct Ctl {
    music: AtomicBool,
    /// bumped to start the music again (on a new track)
    restart: AtomicU32,
    /// bumped to play a voice line
    voice: AtomicU32,
    /// bumped to play each sound, and whenever any of them is
    sfx: [AtomicU32; SFX.len()],
    sfx_any: AtomicU32,
    stop_tape: AtomicBool,
    muted: AtomicBool,
    /// kick-drum pulse, 0..1, as f32 bits
    pulse: AtomicU32,
    /// the track playing (index + 1, 0 for none), and a count of track starts
    track: AtomicU32,
    starts: AtomicU32,
}

/// The modules, unpacked and parsed on a thread of their own the first time
/// they're wanted (the audio callback must never wait on that).
struct Library {
    slots: Vec<OnceLock<Option<&'static Module>>>,
    /// bit i: track i is wanted
    want: AtomicUsize,
    loader: OnceLock<std::thread::Thread>,
}

impl Library {
    fn new() -> Arc<Library> {
        Arc::new(Library { slots: TRACKS.iter().map(|_| OnceLock::new()).collect(), want: AtomicUsize::new(0), loader: OnceLock::new() })
    }

    fn get(&self, i: usize) -> Option<&'static Module> {
        self.slots[i].get().copied().flatten()
    }

    /// Unpack and parse track `i` (once; the module lives as long as the app).
    fn load(&self, i: usize) -> Option<&'static Module> {
        *self.slots[i].get_or_init(|| {
            let mut raw = Vec::new();
            lzma_rust2::XzReader::new(TRACKS[i].data, true).read_to_end(&mut raw).ok()?;
            match Module::load(&raw) {
                Ok(m) => Some(Box::leak(Box::new(m))),
                Err(e) => {
                    eprintln!("audio: {}: {e:?}", TRACKS[i].title);
                    None
                }
            }
        })
    }

    /// Ask the loader thread for track `i` (safe from the audio callback).
    fn request(&self, i: usize) {
        self.want.fetch_or(1 << i, Ordering::Relaxed);
        if let Some(t) = self.loader.get() {
            t.unpark();
        }
    }

    fn start_loader(self: &Arc<Self>) {
        let lib = self.clone();
        let h = std::thread::Builder::new().name("audio-loader".into()).spawn(move || loop {
            let w = lib.want.swap(0, Ordering::Relaxed);
            for i in 0..TRACKS.len() {
                if w & (1 << i) != 0 {
                    lib.load(i);
                }
            }
            std::thread::park();
        });
        if let Ok(h) = h {
            let _ = self.loader.set(h.thread().clone());
            // a request made before the handle was set
            h.thread().unpark();
        }
    }
}

/// Shuffled play order: every track once, then a new order that doesn't
/// start with the track just played.
struct Shuffle {
    rng: u64,
    bag: Vec<usize>,
    last: Option<usize>,
}

impl Shuffle {
    fn new(seed: u64) -> Shuffle {
        Shuffle { rng: seed | 1, bag: Vec::new(), last: None }
    }

    fn rand(&mut self, n: usize) -> usize {
        // xorshift64*
        self.rng ^= self.rng >> 12;
        self.rng ^= self.rng << 25;
        self.rng ^= self.rng >> 27;
        (self.rng.wrapping_mul(0x2545_F491_4F6C_DD1D) >> 33) as usize % n.max(1)
    }

    /// The track after the next one is known in advance, so it can load.
    fn peek(&mut self) -> usize {
        if self.bag.is_empty() {
            let mut b: Vec<usize> = (0..TRACKS.len()).collect();
            for i in (1..b.len()).rev() {
                let j = self.rand(i + 1);
                b.swap(i, j);
            }
            // the bag is popped from the back
            if b.len() > 1 && b.last() == self.last.as_ref() {
                let n = b.len();
                b.swap(0, n - 1);
            }
            self.bag = b;
        }
        *self.bag.last().unwrap()
    }

    fn next(&mut self) -> usize {
        let i = self.peek();
        self.bag.pop();
        self.last = Some(i);
        i
    }
}

fn seed() -> u64 {
    std::time::SystemTime::now().duration_since(std::time::UNIX_EPOCH).map(|d| d.as_nanos() as u64).unwrap_or(0x5EED)
}

pub struct Audio {
    ctl: Arc<Ctl>,
    _stream: Option<cpal::Stream>,
}

/// Mono PCM from a WAV file (8-bit unsigned or 16-bit signed).
fn wav_mono(bytes: &[u8]) -> Option<(Vec<f32>, u32)> {
    if bytes.len() < 12 || &bytes[0..4] != b"RIFF" || &bytes[8..12] != b"WAVE" {
        return None;
    }
    let (mut ch, mut rate, mut bits) = (0u16, 0u32, 0u16);
    let mut i = 12;
    while i + 8 <= bytes.len() {
        let id = &bytes[i..i + 4];
        let len = u32::from_le_bytes(bytes[i + 4..i + 8].try_into().ok()?) as usize;
        let body = bytes.get(i + 8..(i + 8 + len).min(bytes.len()))?;
        if id == b"fmt " && body.len() >= 16 {
            ch = u16::from_le_bytes([body[2], body[3]]);
            rate = u32::from_le_bytes(body[4..8].try_into().ok()?);
            bits = u16::from_le_bytes([body[14], body[15]]);
        } else if id == b"data" && ch > 0 {
            let frame = (bits as usize / 8) * ch as usize;
            let out = body
                .chunks_exact(frame)
                .map(|f| {
                    let s = |k: usize| -> f32 {
                        match bits {
                            8 => (f[k] as f32 - 128.0) / 128.0,
                            _ => i16::from_le_bytes([f[2 * k], f[2 * k + 1]]) as f32 / 32768.0,
                        }
                    };
                    (0..ch as usize).map(s).sum::<f32>() / ch as f32
                })
                .collect();
            return Some((out, rate));
        }
        i += 8 + len + (len & 1);
    }
    None
}

/// Linear resample (the voice is lo-fi on purpose).
fn resample(v: &[f32], from: u32, to: u32) -> Vec<f32> {
    if from == to || v.is_empty() {
        return v.to_vec();
    }
    let n = (v.len() as u64 * to as u64 / from as u64) as usize;
    (0..n)
        .map(|i| {
            let p = i as f64 * from as f64 / to as f64;
            let k = p as usize;
            let f = (p - k as f64) as f32;
            let a = v[k.min(v.len() - 1)];
            let b = v[(k + 1).min(v.len() - 1)];
            a + (b - a) * f
        })
        .collect()
}

/// One module playing.
struct Deck {
    track: usize,
    player: XmrsPlayer<'static>,
    /// frames played
    pos: u64,
}

/// Everything the audio callback owns.
struct Mixer {
    ctl: Arc<Ctl>,
    lib: Arc<Library>,
    order: Shuffle,
    /// the track playing, and the one fading out under it
    deck: Option<Deck>,
    out: Option<Deck>,
    /// crossfade progress 0..1, and the outgoing track's low-pass state
    xf: f32,
    xf_lp: (f32, f32),
    rate: u32,
    restart_seen: u32,
    voice_seen: u32,
    voices: Vec<Vec<f32>>,
    voice: usize,
    voice_pos: usize,
    /// the interface's sounds, and the ones playing (which, how far in)
    sfx: Vec<Vec<f32>>,
    sfx_seen: [u32; SFX.len()],
    sfx_any_seen: u32,
    shots: Vec<(usize, usize)>,
    gain: f32,
    duck: f32,
    /// tape speed (1 = normal) and the resampler around the players
    speed: f32,
    cur: (f32, f32),
    next: (f32, f32),
    frac: f32,
    /// beat follower: low-passed music, fast and slow envelopes
    lp: f32,
    fast: f32,
    slow: f32,
    tick: u32,
}

impl Mixer {
    fn new(ctl: Arc<Ctl>, lib: Arc<Library>, rate: u32, voices: Vec<Vec<f32>>, sfx: Vec<Vec<f32>>, seed: u64) -> Mixer {
        let mut order = Shuffle::new(seed);
        // the first track loads while the user picks a player
        lib.request(order.peek());
        Mixer {
            ctl,
            lib,
            order,
            deck: None,
            out: None,
            xf: 1.0,
            xf_lp: (0.0, 0.0),
            rate,
            restart_seen: 0,
            voice_seen: 0,
            voices,
            voice: 0,
            voice_pos: usize::MAX,
            sfx,
            sfx_seen: [0; SFX.len()],
            sfx_any_seen: 0,
            shots: Vec::new(),
            gain: 0.0,
            duck: 1.0,
            speed: 1.0,
            cur: (0.0, 0.0),
            next: (0.0, 0.0),
            frac: 0.0,
            lp: 0.0,
            fast: 0.0,
            slow: 0.0,
            tick: 0,
        }
    }

    /// Track the kick: low-pass the music around 150 Hz, follow its level
    /// fast and slow, and publish how far the fast one sits above the slow.
    fn follow(&mut self, m: f32, muted: bool) {
        let r = self.rate as f32;
        self.lp += (m - self.lp) * (1.0 - (-TAU * 150.0 / r).exp());
        let a = self.lp.abs();
        let k = if a > self.fast { 1.0 / (0.004 * r) } else { 1.0 / (0.12 * r) };
        self.fast += (a - self.fast) * k;
        self.slow += (self.fast - self.slow) / (1.5 * r);
        self.tick += 1;
        if self.tick % 128 == 0 {
            let p = if muted { 0.0 } else { ((self.fast - self.slow * 1.15) / (self.slow + 0.02)).clamp(0.0, 1.0) };
            self.ctl.pulse.store(p.to_bits(), Ordering::Relaxed);
        }
    }

    /// Start the next track in the shuffle, if it has loaded (if not, the
    /// current one carries on and this is tried again).
    fn advance(&mut self, fade: bool) -> bool {
        let i = self.order.peek();
        let Some(m) = self.lib.get(i) else {
            self.lib.request(i);
            return false;
        };
        self.order.next();
        self.lib.request(self.order.peek());
        let mut player = XmrsPlayer::new(m, self.rate, 0);
        player.set_max_loop_count(0);
        let d = Deck { track: i, player, pos: 0 };
        self.out = if fade { self.deck.take() } else { None };
        self.xf = if self.out.is_some() { 0.0 } else { 1.0 };
        self.xf_lp = (0.0, 0.0);
        self.deck = Some(d);
        self.ctl.track.store(i as u32 + 1, Ordering::Relaxed);
        self.ctl.starts.fetch_add(1, Ordering::Relaxed);
        true
    }

    fn deck_frame(d: &mut Deck) -> Option<(f32, f32)> {
        d.pos += 1;
        match (d.player.next(), d.player.next()) {
            (Some(l), Some(r)) => Some((l as f32 / 32768.0 * TRACKS[d.track].gain, r as f32 / 32768.0 * TRACKS[d.track].gain)),
            _ => None,
        }
    }

    /// One frame of music at the output rate: the current track, the old one
    /// fading out (and darkening) under it during a handover.
    fn pull(&mut self) -> (f32, f32) {
        if self.deck.is_none() && !self.advance(false) {
            return (0.0, 0.0);
        }
        let d = self.deck.as_mut().unwrap();
        let ended = d.pos as f32 >= TRACKS[d.track].end * self.rate as f32;
        let frame = Self::deck_frame(d);
        let mut s = frame.unwrap_or_default();
        if frame.is_none() {
            // stopped short of its length: the next one straight away
            self.deck = None;
            self.advance(false);
        } else if ended && self.out.is_none() {
            // (if the next one hasn't loaded, this one plays on a little)
            self.advance(true);
        }
        if self.out.is_some() {
            let dt = 1.0 / self.rate as f32;
            self.xf = (self.xf + dt / XFADE).min(1.0);
            let o = self.out.as_mut().and_then(Self::deck_frame).unwrap_or_default();
            // the outgoing track's low-pass closes from ~16 kHz to ~200 Hz
            let fc = 16000.0 * (200.0f32 / 16000.0).powf(self.xf);
            let k = 1.0 - (-TAU * fc * dt).exp();
            self.xf_lp.0 += (o.0 - self.xf_lp.0) * k;
            self.xf_lp.1 += (o.1 - self.xf_lp.1) * k;
            // equal power
            let (gi, go) = ((self.xf * std::f32::consts::FRAC_PI_2).sin(), (self.xf * std::f32::consts::FRAC_PI_2).cos());
            s = (s.0 * gi + self.xf_lp.0 * go, s.1 * gi + self.xf_lp.1 * go);
            if self.xf >= 1.0 {
                self.out = None;
            }
        }
        s
    }

    fn frame(&mut self) -> (f32, f32) {
        let (r, v, on, tape, muted) = {
            let c = &self.ctl;
            (
                c.restart.load(Ordering::Relaxed),
                c.voice.load(Ordering::Relaxed),
                c.music.load(Ordering::Relaxed),
                c.stop_tape.load(Ordering::Relaxed),
                c.muted.load(Ordering::Relaxed),
            )
        };
        if r != self.restart_seen {
            self.restart_seen = r;
            self.deck = None;
            self.out = None;
            self.speed = 1.0;
            self.gain = 0.0;
            self.ctl.track.store(0, Ordering::Relaxed);
        }
        if v != self.voice_seen {
            self.voice_seen = v;
            if self.voices.len() > 1 {
                // a different line from last time
                self.voice = (self.voice + 1 + self.order.rand(self.voices.len() - 1)) % self.voices.len();
            }
            self.voice_pos = 0;
        }
        let any = self.ctl.sfx_any.load(Ordering::Acquire);
        if any != self.sfx_any_seen {
            self.sfx_any_seen = any;
            for i in 0..SFX.len() {
                let n = self.ctl.sfx[i].load(Ordering::Relaxed);
                if n != self.sfx_seen[i] {
                    self.sfx_seen[i] = n;
                    // a sound starts over rather than piling up on itself
                    self.shots.retain(|s| s.0 != i);
                    if self.shots.len() < 6 {
                        self.shots.push((i, 0));
                    }
                }
            }
        }
        let dt = 1.0 / self.rate as f32;
        // fades: in fast, out slower; tape stop winds the speed down
        let target = if on { 1.0 } else { 0.0 };
        let rate = if on { 3.0 } else { 0.8 };
        self.gain += (target - self.gain) * (rate * dt).min(1.0);
        if !on && self.gain < 0.0005 && self.deck.is_some() {
            // faded out: the next start is a new track
            self.deck = None;
            self.out = None;
            self.ctl.track.store(0, Ordering::Relaxed);
        }
        if tape {
            self.speed = (self.speed - dt / 1.4).max(0.0);
        }
        let line_len = self.voices.get(self.voice).map(Vec::len).unwrap_or(0);
        let speaking = self.voice_pos < line_len;
        let dk = if speaking { DUCK } else { 1.0 };
        self.duck += (dk - self.duck) * (8.0 * dt).min(1.0);
        let mut out = (0.0, 0.0);
        if self.gain > 0.0005 && self.speed > 0.0 {
            let s = (self.cur.0 + (self.next.0 - self.cur.0) * self.frac, self.cur.1 + (self.next.1 - self.cur.1) * self.frac);
            self.frac += self.speed;
            while self.frac >= 1.0 {
                self.frac -= 1.0;
                self.cur = self.next;
                self.next = self.pull();
            }
            let g = self.gain * self.duck * MUSIC_GAIN * self.speed.sqrt();
            out = (s.0 * g, s.1 * g);
        }
        if speaking {
            let x = self.voices[self.voice][self.voice_pos] * VOICE_GAIN;
            self.voice_pos += 1;
            out = (out.0 + x, out.1 + x);
        }
        self.follow(out.0 + out.1, muted);
        // the interface's sounds, as far as the music leaves room for them
        if !self.shots.is_empty() {
            let room = 1.0 - (self.gain * self.speed).clamp(0.0, 1.0);
            let mut fx = 0.0;
            let bank = &self.sfx;
            self.shots.retain_mut(|(i, pos)| match bank[*i].get(*pos) {
                Some(x) => {
                    fx += x * SFX[*i].1;
                    *pos += 1;
                    true
                }
                None => false,
            });
            out = (out.0 + fx * room, out.1 + fx * room);
        }
        if muted {
            return (0.0, 0.0);
        }
        // soft clip
        (out.0.tanh(), out.1.tanh())
    }
}

/// Output devices to try, best first. On Linux, ALSA's "default" is often
/// plain dmix on the sound card, which can't open while PipeWire or
/// PulseAudio holds the card; their own ALSA devices can, so they go first.
/// (cpal lists "default", "pipewire", "pulse", then other plugins and cards;
/// stop before those, as opening them can be slow or noisy.)
fn outputs(host: &cpal::Host) -> Vec<cpal::Device> {
    let mut v = Vec::new();
    if cfg!(target_os = "linux") {
        if let Ok(devs) = host.output_devices() {
            for d in devs {
                match d.name().as_deref() {
                    Ok("default") => {}
                    Ok("pipewire" | "pulse") => v.push(d),
                    _ => break,
                }
            }
        }
    }
    v.extend(host.default_output_device());
    v
}

/// Start the mixer on `dev`, or None if the device won't take a stream.
fn open(dev: &cpal::Device, ctl: &Arc<Ctl>, lib: &Arc<Library>, voices: &[(Vec<f32>, u32)], sfx: &[(Vec<f32>, u32)]) -> Option<cpal::Stream> {
    let cfg = dev.default_output_config().ok()?;
    let rate = cfg.sample_rate().0;
    let chans = cfg.channels() as usize;
    let at_rate = |v: &[(Vec<f32>, u32)]| v.iter().map(|(v, r)| resample(v, (*r).max(1), rate)).collect();
    let mut mix = Mixer::new(ctl.clone(), lib.clone(), rate, at_rate(voices), at_rate(sfx), seed());
    let err = |e| eprintln!("audio: {e}");
    macro_rules! build {
        ($t:ty, $conv:expr) => {
            dev.build_output_stream(
                &cfg.clone().into(),
                move |data: &mut [$t], _| {
                    for f in data.chunks_mut(chans) {
                        let (l, r) = mix.frame();
                        for (k, s) in f.iter_mut().enumerate() {
                            *s = $conv(if k % 2 == 0 { l } else { r });
                        }
                    }
                },
                err,
                None,
            )
        };
    }
    let s = match cfg.sample_format() {
        cpal::SampleFormat::F32 => build!(f32, |x: f32| x),
        cpal::SampleFormat::I16 => build!(i16, |x: f32| (x * 32767.0) as i16),
        cpal::SampleFormat::U16 => build!(u16, |x: f32| ((x * 32767.0) as i32 + 32768) as u16),
        cpal::SampleFormat::I32 => build!(i32, |x: f32| (x as f64 * 2147483647.0) as i32),
        _ => return None,
    }
    .ok()?;
    s.play().ok()?;
    Some(s)
}

impl Audio {
    /// A silent stand-in (screenshots, no device).
    pub fn silent() -> Audio {
        Audio { ctl: Arc::new(Ctl::default()), _stream: None }
    }

    pub fn new() -> Audio {
        let ctl = Arc::new(Ctl::default());
        let lib = Library::new();
        lib.start_loader();
        let voices: Vec<(Vec<f32>, u32)> = VOICES.iter().filter_map(|w| wav_mono(w)).collect();
        let sfx: Vec<(Vec<f32>, u32)> = SFX.iter().map(|(w, _)| wav_mono(w).unwrap_or((Vec::new(), 1))).collect();
        let host = cpal::default_host();
        let stream = outputs(&host).into_iter().find_map(|dev| open(&dev, &ctl, &lib, &voices, &sfx));
        if stream.is_none() {
            eprintln!("audio: no output device would open; running silent");
        }
        Audio { ctl, _stream: stream }
    }

    /// Music on (a new track if it was off or wound down) or off (fades).
    pub fn music(&self, on: bool) {
        let was = self.ctl.music.swap(on, Ordering::Relaxed);
        if on && (!was || self.ctl.stop_tape.load(Ordering::Relaxed)) {
            self.ctl.stop_tape.store(false, Ordering::Relaxed);
            self.ctl.restart.fetch_add(1, Ordering::Relaxed);
        }
    }

    /// Wind the music down like a tape deck losing power.
    pub fn tape_stop(&self) {
        self.ctl.stop_tape.store(true, Ordering::Relaxed);
    }

    /// Say a voice line (a different one from last time).
    pub fn voice(&self) {
        self.ctl.voice.fetch_add(1, Ordering::Relaxed);
    }

    /// Play one of the interface's sounds (heard only while the music isn't).
    pub fn sfx(&self, s: Sfx) {
        self.ctl.sfx[s as usize].fetch_add(1, Ordering::Relaxed);
        // after the sound's own count, so the mixer sees that too
        self.ctl.sfx_any.fetch_add(1, Ordering::Release);
    }

    pub fn toggle_mute(&self) -> bool {
        let m = !self.ctl.muted.load(Ordering::Relaxed);
        self.ctl.muted.store(m, Ordering::Relaxed);
        m
    }

    pub fn muted(&self) -> bool {
        self.ctl.muted.load(Ordering::Relaxed)
    }

    /// Kick-drum pulse, 0..1 (0 when muted or silent).
    pub fn pulse(&self) -> f32 {
        f32::from_bits(self.ctl.pulse.load(Ordering::Relaxed))
    }

    pub fn live(&self) -> bool {
        self._stream.is_some()
    }

    /// The track playing, with a number that changes each time one starts.
    pub fn now_playing(&self) -> Option<(u32, &'static Track)> {
        let i = self.ctl.track.load(Ordering::Relaxed) as usize;
        let n = self.ctl.starts.load(Ordering::Relaxed);
        i.checked_sub(1).and_then(|i| TRACKS.get(i)).map(|t| (n, t))
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    const RATE: u32 = 22050;

    fn rms(m: &mut Mixer, secs: f32) -> (f32, f32) {
        let n = (secs * RATE as f32) as usize;
        let (mut e, mut pulse) = (0.0f32, 0.0f32);
        for _ in 0..n {
            let (l, r) = m.frame();
            e += l * l + r * r;
            pulse = pulse.max(f32::from_bits(m.ctl.pulse.load(Ordering::Relaxed)));
        }
        ((e / (2 * n) as f32).sqrt(), pulse)
    }

    fn mixer() -> (Audio, Mixer) {
        let voices: Vec<Vec<f32>> = VOICES
            .iter()
            .map(|w| {
                let (v, r) = wav_mono(w).unwrap();
                assert!(v.len() > r as usize, "voice line shorter than a second");
                resample(&v, r, RATE)
            })
            .collect();
        let sfx = SFX.iter().map(|(w, _)| wav_mono(w).map(|(v, r)| resample(&v, r, RATE)).unwrap()).collect();
        let a = Audio::silent();
        let lib = Library::new();
        let m = Mixer::new(a.ctl.clone(), lib.clone(), RATE, voices, sfx, 42);
        // no loader thread here: load what the mixer asked for
        let w = lib.want.swap(0, Ordering::Relaxed);
        (0..TRACKS.len()).filter(|i| w & (1 << i) != 0).for_each(|i| assert!(lib.load(i).is_some()));
        (a, m)
    }

    #[test]
    fn music_voice_and_tape_stop() {
        let (a, mut m) = mixer();
        assert_eq!(rms(&mut m, 0.2).0, 0.0, "silent before the music starts");
        assert!(a.now_playing().is_none());
        a.music(true);
        let (on, pulse) = rms(&mut m, 4.0);
        assert!(on > 0.05, "music too quiet: {on}");
        assert!(pulse > 0.3, "no beat pulse: {pulse}");
        let (n, first) = a.now_playing().expect("a track is playing");
        a.voice();
        let speaking = rms(&mut m, 1.0).0;
        assert!(speaking > 0.05, "voice inaudible: {speaking}");
        a.toggle_mute();
        rms(&mut m, 0.01); // the pulse is published every 128 frames
        assert_eq!(rms(&mut m, 0.1), (0.0, 0.0), "mute is silent and stops the pulse");
        a.toggle_mute();
        a.tape_stop();
        rms(&mut m, 1.5);
        assert_eq!(rms(&mut m, 0.5).0, 0.0, "tape stop ends in silence");
        a.music(true);
        // the mixer asked for the next track when the first started
        let w = m.lib.want.swap(0, Ordering::Relaxed);
        (0..TRACKS.len()).filter(|i| w & (1 << i) != 0).for_each(|i| assert!(m.lib.load(i).is_some()));
        assert!(rms(&mut m, 2.0).0 > 0.05, "music restarts after a tape stop");
        let (n2, second) = a.now_playing().unwrap();
        assert!(n2 > n && second.title != first.title, "a restart plays a different track");
        a.music(false);
        rms(&mut m, 6.0);
        assert!(rms(&mut m, 0.5).0 < 0.01, "music fades out");
    }

    #[test]
    fn interface_sounds_only_before_the_music() {
        for (i, (w, _)) in SFX.iter().enumerate() {
            let (v, r) = wav_mono(w).unwrap_or_else(|| panic!("sound {i} doesn't load"));
            let secs = v.len() as f32 / r as f32;
            assert!((0.02..2.0).contains(&secs), "sound {i} is {secs} s");
        }
        let (a, mut m) = mixer();
        a.sfx(Sfx::Confirm);
        assert!(rms(&mut m, 0.3).0 > 0.02, "a sound plays before the music");
        // two mixers, one with sounds fired at it: the same once the music runs
        let (b, mut quiet) = mixer();
        a.music(true);
        b.music(true);
        rms(&mut m, 4.0);
        rms(&mut quiet, 4.0);
        a.sfx(Sfx::Error);
        a.sfx(Sfx::Click);
        for _ in 0..RATE / 2 {
            let (x, y) = (m.frame(), quiet.frame());
            assert!((x.0 - y.0).abs() < 1e-3, "sounds play over the music");
        }
    }

    #[test]
    fn shuffle_plays_everything_without_repeats() {
        let mut s = Shuffle::new(7);
        let mut prev = None;
        for _round in 0..20 {
            let mut seen = vec![false; TRACKS.len()];
            for _ in 0..TRACKS.len() {
                let i = s.next();
                assert_ne!(Some(i), prev, "same track twice in a row");
                assert!(!seen[i], "a track came round twice in one pass");
                seen[i] = true;
                prev = Some(i);
            }
        }
    }

    #[test]
    fn hands_over_to_a_different_track() {
        let (a, mut m) = mixer();
        a.music(true);
        rms(&mut m, 0.5);
        let (n, first) = a.now_playing().unwrap();
        // skip to just before its end
        let d = m.deck.as_mut().unwrap();
        d.pos = (TRACKS[d.track].end * RATE as f32) as u64 - RATE as u64 / 2;
        let w = m.lib.want.swap(0, Ordering::Relaxed);
        (0..TRACKS.len()).filter(|i| w & (1 << i) != 0).for_each(|i| assert!(m.lib.load(i).is_some()));
        let (lvl, _) = rms(&mut m, XFADE + 1.0);
        let (n2, next) = a.now_playing().unwrap();
        assert!(n2 == n + 1 && next.title != first.title, "{} then {}", first.title, next.title);
        assert!(lvl > 0.05, "silence in the handover");
        assert!(m.out.is_none(), "the old track is gone after the crossfade");
    }

    #[test]
    fn every_track_loads_and_plays() {
        for (i, t) in TRACKS.iter().enumerate() {
            let lib = Library::new();
            let m = lib.load(i).unwrap_or_else(|| panic!("{} won't load", t.title));
            let mut p = XmrsPlayer::new(m, RATE, 0);
            let mut e = 0.0f64;
            // (over t.end seconds in a release build to measure the gains)
            let n = RATE as usize * 20;
            for _ in 0..n * 2 {
                let x = p.next().unwrap_or(0) as f64 / 32768.0;
                e += x * x;
            }
            let rms = (e / (2 * n) as f64).sqrt() * t.gain as f64;
            eprintln!("{:24} rms {rms:.3}", t.title);
            assert!(rms > 0.03, "{} is silent", t.title);
        }
    }

    #[test]
    fn tracks_are_credited() {
        let credits = include_str!("../CREDITS.md");
        for t in TRACKS {
            assert!(credits.contains(t.title) && credits.contains(t.url), "{} is not in CREDITS.md", t.title);
        }
    }
}
