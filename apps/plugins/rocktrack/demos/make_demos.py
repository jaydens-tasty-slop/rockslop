#!/usr/bin/env python3
# RockTrack demo songs. This script, the arrangements and the original songs
# "Sparkstone", "Quartz Comet", "Cinder Warden" and "Lemon Kite" are
# dedicated to the public domain (CC0 1.0); the arranged compositions are
# themselves in the public domain. See README.txt.
"""Generate RockTrack's demo songs.

    python3 make_demos.py [OUTDIR]          write every demo .xm plus greensleeves.mid
                                            (OUTDIR defaults to this script's directory)
    python3 make_demos.py --check [DIR]     re-read this script's .xm files in DIR and
                                            check them (third-party modules are skipped)

Python 3 standard library only; the output is the same bytes on every run.

The music is text, one string per voice. Tokens, separated by spaces:
    C#5:4    note C#5 lasting 4 rows (C4 = middle C = XM C-4). The length
             is sticky: a token without ":n" reuses the previous length.
    r:2      rest (keys the sounding note off)
    ~:4      tie: the previous note keeps sounding
    |        bar line, checked against the song's bar length
    v40      volume (0-64) of the following notes
    $bell    instrument of the following notes
  Suffixes on a note, rest or tie: >n key off after n rows, @n volume of
  this note only (on a tie: a volume change), +EXY an XM effect on the first
  row, *EXY the effect on every row (047 arpeggio, 308 tone portamento,
  446 vibrato ...).
Drum grids: one character per row, '.' empty, '|' bar line; the letters
come from the song's kit. Chord figures: a chord chart ("Am F E7") and a
figure played on each chord, using R root, T third, F fifth, S seventh,
O octave and A (the root with the whole chord as a 0xy arpeggio).

The instruments are RockTrack synth patches: each sample is named with its
"rt1:" tag and rendered exactly like the plugin's synth.c (DESIGN.md
"Synth"), and the volume envelope is built from ADSR the same way, so
RockTrack shows the synth controls and changing one does not make the
sound jump.
"""

import math
import os
import re
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))

# ---------------------------------------------------------------- synth --

AMP = 100           # amplitude of every synth source (signed 8-bit samples)
RATE = 16726        # C-4 with relative note +12: one-shot drums play at this rate
KEYOFF = 97

# round(100 * sin(2 pi i / 64)), as synth.c
SINE64 = [0, 10, 20, 29, 38, 47, 56, 63, 71, 77, 83, 88, 92, 96, 98, 100,
          100, 100, 98, 96, 92, 88, 83, 77, 71, 63, 56, 47, 38, 29, 20, 10]
SINE64 += [-x for x in SINE64]
TUNE_UP = [65536, 69433, 73562, 77936, 82570, 87480, 92682, 98193, 104032,
           110218, 116772, 123715, 131072]          # 2^(n/12) in Q16
DRUM_LEN = {'kick': (1600, 400), 'snare': (1200, 300), 'hat': (300, 150),
            'tom': (1600, 300)}


def cdiv(a, b):
    """C integer division (truncates toward zero)."""
    q = abs(a) // abs(b)
    return q if (a < 0) == (b < 0) else -q


class Lfsr:
    """15-bit LFSR from 1: each step returns bit 0, then shifts in bit0^bit<tap>."""

    def __init__(self):
        self.s = 1

    def step(self, tap):
        r = self.s
        self.s = (r >> 1) | (((r ^ (r >> tap)) & 1) << 14)
        return r & 1


def sine_at(phase):
    """Sine from the 64-step table, linear interpolation; phase in Q16 cycles."""
    i, f = (phase >> 10) & 63, phase & 1023
    a, b = SINE64[i], SINE64[(i + 1) & 63]
    return a + (((b - a) * f) >> 10)


def drum(src, tune, length):
    """kick/snare/hat/tom one-shots: synth.c render_drum(), integer for integer."""
    n = DRUM_LEN[src][0] + DRUM_LEN[src][1] * max(0, min(15, length))
    semis = max(-12, min(12, tune - 12))
    t = TUNE_UP[semis] if semis >= 0 else (1 << 32) // TUNE_UP[-semis]
    phase, e, prev = 0, 65536, 0
    r = 65536 - (8 if src == 'kick' else 3) * 65536 // n
    lfsr = Lfsr()
    out = []
    for i in range(n):
        env = (n - i) * 256 // n                       # linear 256..0
        f = 0
        if src == 'kick':                              # 180 -> 45 Hz
            f = ((45 * 256 + 135 * (e >> 8)) * t) >> 16
            v = cdiv(sine_at(phase) * env, 256)
        elif src == 'snare':                           # 190 Hz + noise
            tone_env = max(0, 256 - i * 1024 // n)
            noise = -AMP if lfsr.step(1) else AMP
            f = (190 * 256 * t) >> 16
            v = (cdiv(sine_at(phase) * tone_env * 6, 2560) +
                 cdiv(noise * env * 7, 2560))
        elif src == 'hat':                             # differenced noise
            noise = -AMP if lfsr.step(1) else AMP
            v = cdiv(cdiv(noise - prev, 2) * (env * env // 256), 256)
            prev = noise
        else:                                          # tom: 220 -> 110 Hz
            f = ((110 * 256 + 110 * (e >> 8)) * t) >> 16
            v = cdiv(sine_at(phase) * env, 256)
        phase = (phase + f * 256 // RATE) & 0xFFFFFFFF
        e = (e * r) >> 16
        out.append(max(-127, min(127, v)))
    return out


def synth(tag):
    """rt1 tag -> (signed 8-bit sample data, loop length; 0 = one-shot)."""
    m = re.fullmatch(r'rt1:([a-z]+)((?:,-?\d+)*)', tag)
    if not m:
        raise ValueError('bad synth tag ' + tag)
    src = m.group(1)
    args = [int(x) for x in m.group(2).split(',')[1:]]
    if src == 'pulse':
        d = max(1, min(15, args[0]))
        return [AMP if i < 4 * d else -AMP for i in range(64)], 64
    if src == 'tri':
        return [AMP - AMP * abs(2 * i - 64) // 32 for i in range(64)], 64
    if src == 'saw':
        return [2 * AMP * i // 64 - AMP for i in range(64)], 64
    if src == 'sine':
        return list(SINE64), 64
    if src == 'noise':
        lfsr, tap = Lfsr(), 6 if args[0] else 1
        return [-AMP if lfsr.step(tap) else AMP for _ in range(4096)], 4096
    if src in DRUM_LEN:
        return drum(src, args[0], args[1]), 0
    raise ValueError('unknown synth source ' + tag)


ENV_TIMES = [0, 1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192]


def adsr_envelope(a, d, s, r):
    """ADSR (0-15 each) -> XM volume envelope points and sustain point."""
    ta, td, tr = ENV_TIMES[a], ENV_TIMES[d], max(1, ENV_TIMES[r])
    pts = []

    def add(x, y):
        if pts and pts[-1][0] == x:
            pts[-1] = (x, y)            # same position: the later value wins
        else:
            pts.append((x, y))
    add(0, 0 if ta else 64)
    add(ta, 64)
    add(ta + td, s * 64 // 15)
    sus = len(pts) - 1
    add(ta + td + tr, 0)
    return pts, sus


class Inst:
    """One instrument: a synth sample, an ADSR volume envelope, pan, vibrato."""

    def __init__(self, key, name, tag, adsr, pan=128, vib=(0, 0, 0, 0)):
        if len(name) > 22 or len(tag) > 22:
            raise ValueError('name too long: ' + name)
        self.key, self.name, self.tag, self.adsr = key, name, tag, adsr
        self.pan, self.vib = pan, vib
        self.data, self.loop = synth(tag)
        self.env, self.sus = adsr_envelope(*adsr)


# ---------------------------------------------------------------- notes --

NOTE_PC = {'C': 0, 'D': 2, 'E': 4, 'F': 5, 'G': 7, 'A': 9, 'B': 11}
PC_NAME = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']


def note_num(name):
    """'C#5' -> XM note number (C-0 = 1, C-4 = 49)."""
    m = re.fullmatch(r'([A-G])([#b]?)(\d)', name)
    if not m:
        raise ValueError('bad note ' + name)
    n = (12 * int(m.group(3)) + NOTE_PC[m.group(1)] +
         {'#': 1, 'b': -1, '': 0}[m.group(2)] + 1)
    if not 1 <= n <= 96:
        raise ValueError('note out of range ' + name)
    return n


def note_name(n):
    return PC_NAME[(n - 1) % 12] + str((n - 1) // 12)


CHORD_TONES = {'': (0, 4, 7), 'm': (0, 3, 7), '7': (0, 4, 7, 10),
               'm7': (0, 3, 7, 10), 'maj7': (0, 4, 7, 11), 'dim': (0, 3, 6)}
CHORD_ARP = {'': '047', 'm': '037', '7': '04A', 'm7': '037', 'maj7': '04B',
             'dim': '036'}


def chord(name):
    """'F#m' -> (pitch class, chord tones, 0xy arpeggio)."""
    m = re.fullmatch(r'([A-G])([#b]?)(maj7|m7|m|7|dim)?', name)
    if not m:
        raise ValueError('bad chord ' + name)
    pc = (NOTE_PC[m.group(1)] + {'#': 1, 'b': -1, '': 0}[m.group(2)]) % 12
    q = m.group(3) or ''
    return pc, CHORD_TONES[q], CHORD_ARP[q]


D_MAJOR = [2, 4, 6, 7, 9, 11, 1]
A_MAJOR = [9, 11, 1, 2, 4, 6, 8]
A_MINOR = [9, 11, 0, 2, 4, 5, 7]


def scale(tonic, minor=False):
    """The pitch classes of a major or natural minor scale, tonic first."""
    steps = (0, 2, 3, 5, 7, 8, 10) if minor else (0, 2, 4, 5, 7, 9, 11)
    return [(NOTE_PC[tonic[0]] + {'#': 1, 'b': -1, '': 0}[tonic[1:]] + k) % 12
            for k in steps]


def transpose_chart(chart, semis):
    """A chord chart moved by semis semitones (sharp names)."""
    out = []
    for name in chart.split():
        pc = chord(name)[0]
        out.append(PC_NAME[(pc + semis) % 12] + re.sub(r'^[A-G][#b]?', '', name))
    return ' '.join(out)


class Ev:
    """A note ('n'), rest ('r') or tie ('~') in a voice, at a row."""

    def __init__(self, row, dur, kind, note=0, vol=None, inst=None,
                 fx1=(), fxall=(), gate=None, drum=False):
        self.row, self.dur, self.kind, self.note = row, dur, kind, note
        self.vol, self.inst, self.gate, self.drum = vol, inst, gate, drum
        self.fx1, self.fxall = list(fx1), list(fxall)
        self.span = dur             # rows the note sounds, ties included

    def copy(self, row=None):
        e = Ev(self.row if row is None else row, self.dur, self.kind,
               self.note, self.vol, self.inst, self.fx1, self.fxall,
               self.gate, self.drum)
        e.span = self.span
        return e


class V:
    """A stretch of one voice: events (rows relative to its start), length."""

    def __init__(self, evs=(), length=0):
        self.evs, self.length = list(evs), length

    def __add__(self, other):
        return V(self.evs + [e.copy(e.row + self.length) for e in other.evs],
                 self.length + other.length)

    def __mul__(self, n):
        out = V()
        for _ in range(n):
            out = out + self
        return out


def rest(rows):
    return V([Ev(0, rows, 'r')], rows)


TOKEN = re.compile(r'([A-G][#b]?\d|r|~)(?::(\d+))?(?:>(\d+))?(?:@(\d+))?'
                   r'((?:[+*][0-9A-F]{3})*)$')

# ----------------------------------------------------------------- song --


class SongError(Exception):
    pass


class Song:
    """A song being arranged: channels of cells, filled block by block."""

    def __init__(self, fname, title, chans, speed, bpm, rows, bar,
                 restart=0, timesig=(4, 4)):
        if len(title) > 20:
            raise SongError('title too long: ' + title)
        self.fname, self.title, self.chans = fname, title, chans.split()
        self.speed, self.bpm, self.rows, self.bar = speed, bpm, rows, bar
        self.restart, self.timesig = restart, timesig
        self.insts = []
        self.grid = {c: {} for c in self.chans}     # row -> [note ins vol fx par]
        self.events = {c: [] for c in self.chans}   # absolute events, for MIDI
        self.sounding = {c: False for c in self.chans}
        self.gfx = []                                # (row, effect)
        self.pos = 0

    # instruments ----------------------------------------------------
    def inst(self, key, *args, **kw):
        self.insts.append(Inst(key, *args, **kw))

    def inst_no(self, key):
        for i, ins in enumerate(self.insts):
            if ins.key == key:
                return i + 1
        raise SongError('%s: no instrument %r' % (self.title, key))

    def kit(self, **chars):
        """Drum kit: letter -> (instrument, volume[, note[, effect]])."""
        return {c: (v + ('C4', None))[:4] if len(v) == 2 else
                (v + (None,))[:4] for c, v in chars.items()}

    # voice text -----------------------------------------------------
    def notes(self, text, tr=0, inst=None, vol=None, art=0):
        """Parse voice text (see the module doc). tr transposes in semitones;
        art > 0 keys each note off art rows early (detached playing)."""
        evs, pos, dur, last = [], 0, 4, None
        for tok in text.replace('|', ' | ').split():
            if tok == '|':
                if pos % self.bar:
                    raise SongError('%s: bar line at row %d in "%s..."'
                                    % (self.title, pos, text[:50]))
                continue
            if re.fullmatch(r'v\d+', tok):
                vol = int(tok[1:])
                continue
            if tok[0] == '$':
                inst = tok[1:]
                continue
            m = TOKEN.match(tok)
            if not m:
                raise SongError('%s: bad token %r' % (self.title, tok))
            p, d, g, v, fx = m.groups()
            if d:
                dur = int(d)
            fx1 = [f[1:] for f in re.findall(r'[+*][0-9A-F]{3}', fx) if f[0] == '+']
            fxa = [f[1:] for f in re.findall(r'[+*][0-9A-F]{3}', fx) if f[0] == '*']
            if p == 'r':
                ev, last = Ev(pos, dur, 'r', fx1=fx1, fxall=fxa), None
            elif p == '~':
                ev = Ev(pos, dur, '~', vol=int(v) if v else None,
                        fx1=fx1, fxall=fxa)
                if last:
                    last.span += dur
            else:
                if inst is None:
                    raise SongError('%s: note without instrument' % self.title)
                ev = Ev(pos, dur, 'n', note_num(p) + tr,
                        int(v) if v else vol, inst, fx1, fxa,
                        int(g) if g else None)
                if not 1 <= ev.note <= 96:
                    raise SongError('%s: note out of range' % self.title)
                last = ev
            evs.append(ev)
            pos += dur
        if art:
            notes = [e for e in evs if e.kind != '~']
            for i, e in enumerate(notes):
                nxt = notes[i + 1] if i + 1 < len(notes) else None
                glide = nxt and nxt.kind == 'n' and any(
                    f[0] == '3' for f in nxt.fx1 + nxt.fxall)
                if e.kind == 'n' and e.gate is None and e.span > art and not glide:
                    e.gate = e.span - art
        return V(evs, pos)

    def drums(self, grid, kit):
        """A drum grid: one character per row."""
        evs, pos = [], 0
        for ch in grid:
            if ch == ' ':
                continue
            if ch == '|':
                if pos % self.bar:
                    raise SongError('%s: drum bar line at row %d' % (self.title, pos))
                continue
            if ch != '.':
                inst, vol, note, fx = kit[ch]
                evs.append(Ev(pos, 1, 'n', note_num(note), vol, inst,
                              [fx] if fx else [], drum=True))
            pos += 1
        return V(evs, pos)

    def fig(self, chart, figure, per, lo, **kw):
        """Play a figure on every chord of a chart; each chord lasts per rows.
        Chord roots are placed at or above note lo."""
        lo_n, out = note_num(lo), V()
        for name in chart.split():
            pc, tones, arp = chord(name)
            root = lo_n + (pc - (lo_n - 1)) % 12
            pick = {'R': root, 'T': root + tones[1], 'F': root + tones[2],
                    'O': root + 12, 'A': root,
                    'S': root + (tones[3] if len(tones) > 3 else 12)}
            toks = []
            for tok in figure.split():
                if tok[0] in pick:
                    tok = note_name(pick[tok[0]]) + tok[1:] + (
                        '*' + arp if tok[0] == 'A' else '')
                toks.append(tok)
            v = self.notes(' '.join(toks), **kw)
            if v.length != per:
                raise SongError('%s: figure is %d rows, not %d'
                                % (self.title, v.length, per))
            out = out + v
        return out

    def ramp(self, note, rows, v0, v1, fx, inst):
        """One note held for rows, its volume going v0 -> v1, fx every row."""
        toks = ['%s:1@%d*%s' % (note, v0, fx)]
        toks += ['~:1@%d*%s' % (v0 + (v1 - v0) * k // (rows - 1), fx)
                 for k in range(1, rows)]
        return self.notes(' '.join(toks), inst=inst)

    def harmony(self, v, chart, per, scale, inst, vscale):
        """A second voice under a melody: chord tones become the next chord
        tone at least a minor third below; other notes a diatonic third below."""
        chords = [chord(n) for n in chart.split()]
        out = []
        for e in v.evs:
            h = e.copy()
            if e.kind == 'n':
                pc, tones, _ = chords[e.row // per]
                ct = {(pc + t) % 12 for t in tones}
                n = e.note
                if (n - 1) % 12 in ct:
                    h.note = n - 3
                    while (h.note - 1) % 12 not in ct:
                        h.note -= 1
                else:
                    i = scale.index((n - 1) % 12) if (n - 1) % 12 in scale else None
                    if i is None:
                        h.note = n - 4 if (n - 5) % 12 in scale else n - 3
                    else:
                        h.note = n - 1
                        while (h.note - 1) % 12 != scale[(i - 2) % 7]:
                            h.note -= 1
                h.inst = inst
                h.vol = round((64 if e.vol is None else e.vol) * vscale)
            elif e.kind == '~' and e.vol is not None:
                h.vol = round(e.vol * vscale)
            out.append(h)
        return V(out, v.length)

    # placing ----------------------------------------------------------
    def _cell(self, ch, row, note=None, inst=None, vol=None, fx=None):
        c = self.grid[ch].setdefault(row, [0, 0, 0, 0, 0])
        if note is not None:
            if c[0]:
                raise SongError('%s: two notes in %s at row %d'
                                % (self.title, ch, row))
            c[0] = note
        if inst:
            c[1] = inst
        if vol is not None:
            if c[2]:
                raise SongError('%s: two volumes in %s at row %d'
                                % (self.title, ch, row))
            c[2] = 0x10 + max(0, min(64, vol))
        if fx is not None:
            t, p = int(fx[0], 16), int(fx[1:], 16)
            if (c[3] or c[4]) and (c[3], c[4]) != (t, p):
                raise SongError('%s: two effects in %s at row %d'
                                % (self.title, ch, row))
            c[3], c[4] = t, p

    def block(self, rows, **voices):
        """Append rows to the song; voices maps channel -> V of that length.
        A channel left out falls silent (its sounding note is keyed off)."""
        for ch in voices:
            if ch not in self.chans:
                raise SongError('%s: no channel %s' % (self.title, ch))
        for ch in self.chans:
            v = voices.get(ch)
            if v is None:
                if self.sounding[ch]:
                    self._cell(ch, self.pos, note=KEYOFF)
                self.sounding[ch] = False
                continue
            if v.length != rows:
                raise SongError('%s: %s is %d rows, block is %d'
                                % (self.title, ch, v.length, rows))
            self._render(ch, v, self.pos)
        self.pos += rows

    def _render(self, ch, v, base):
        sounding = self.sounding[ch]
        for e in v.evs:
            r = base + e.row
            if e.kind == 'n':
                glide = any(f[0] == '3' for f in e.fx1 + e.fxall)
                # a tone-portamento note keeps the sounding note's envelope
                self._cell(ch, r, note=e.note,
                           inst=0 if glide else self.inst_no(e.inst), vol=e.vol)
                sounding = not e.drum
                if e.gate is not None and e.gate < e.span:
                    self._cell(ch, r + e.gate, note=KEYOFF)
                    sounding = False
                self.events[ch].append(e.copy(r))
            elif e.kind == 'r':
                if sounding:
                    self._cell(ch, r, note=KEYOFF)
                sounding = False
            elif e.vol is not None:
                self._cell(ch, r, vol=e.vol)
            for f in e.fx1:
                self._cell(ch, r, fx=f)
            for f in e.fxall:
                for k in range(e.dur):
                    self._cell(ch, r + k, fx=f)
        self.sounding[ch] = sounding

    def echo(self, src, dst, start, end, delay, scale, inst):
        """Copy rows start..end of channel src into dst, delay rows later,
        volume scaled, with another instrument: a tracker echo."""
        for r in range(start, end):
            c = self.grid[src].get(r)
            t = r + delay
            if not c or t >= self.pos:
                continue
            n, i, v, f, p = c
            vol = None
            if v:
                vol = round((v - 0x10) * scale)
            elif n and n != KEYOFF:
                vol = round(64 * scale)
            keep = f in (0, 1, 2, 3, 4, 0xA) or (f == 0xE and p >> 4 in (9, 0xC, 0xD))
            self._cell(dst, t, note=n or None,
                       inst=self.inst_no(inst) if i else None, vol=vol,
                       fx='%X%02X' % (f, p) if keep and (f or p) else None)

    def pan(self, ch, start, end, pans):
        """8xx on every note of ch in rows start..end that has no effect yet,
        cycling through pans: a ping-pong echo."""
        k = 0
        for r in range(start, min(end, self.pos)):
            c = self.grid[ch].get(r)
            if c and c[0] and c[0] != KEYOFF and not (c[3] or c[4]):
                self._cell(ch, r, fx='8%02X' % pans[k % len(pans)])
                k += 1

    def fx(self, row, effect):
        """A song-wide effect (tempo, speed, break) on any free effect column."""
        self.gfx.append((row, effect))

    def tempo(self, row, bpm):
        self.fx(row, 'F%02X' % bpm)

    # output -----------------------------------------------------------
    def finish(self):
        """Place song-wide effects, cut patterns, share identical ones."""
        total = self.pos
        self.fx(0, 'F%02X' % self.speed)
        self.tempo(0, self.bpm)
        if total % self.rows:
            self.fx(total - 1, 'D00')       # the last pattern ends early
        for row, eff in self.gfx:
            for ch in reversed(self.chans):
                c = self.grid[ch].get(row)
                if not c or not (c[3] or c[4]):
                    self._cell(ch, row, fx=eff)
                    break
            else:
                raise SongError('%s: no free effect column at row %d'
                                % (self.title, row))
        self.patterns, self.order, seen = [], [], {}
        for p in range(0, total, self.rows):
            cells = tuple(tuple(self.grid[ch].get(r, (0, 0, 0, 0, 0)))
                          for r in range(p, p + self.rows) for ch in self.chans)
            if cells not in seen:
                seen[cells] = len(self.patterns)
                self.patterns.append(cells)
            self.order.append(seen[cells])
        if not 0 <= self.restart < len(self.order):
            raise SongError('%s: bad restart position' % self.title)
        return self

    def duration(self):
        """Playing time in seconds of the song once through."""
        fx = {}
        for row, eff in self.gfx:
            fx.setdefault(row, []).append(eff)
        speed, bpm, t = self.speed, self.bpm, 0.0
        for row in range(self.pos):
            for eff in fx.get(row, []):
                if eff[0] == 'F':
                    p = int(eff[1:], 16)
                    if p < 0x20:
                        speed = p
                    else:
                        bpm = p
            t += speed * 2.5 / bpm
        return t

    def xm(self):
        """The song as XM 1.04 bytes."""
        nch = len(self.chans)
        out = bytearray(b'Extended Module: ')
        out += self.title.encode('ascii').ljust(20, b'\0') + b'\x1a'
        out += b'RockTrack'.ljust(20) + struct.pack('<H', 0x0104)
        out += struct.pack('<I8H', 276, len(self.order), self.restart, nch,
                           len(self.patterns), len(self.insts), 1,
                           self.speed, self.bpm)
        out += bytes(self.order).ljust(256, b'\0')
        for cells in self.patterns:
            data = pack_pattern(cells)
            out += struct.pack('<IBHH', 9, 0, self.rows, len(data)) + data
        for ins in self.insts:
            out += instrument_bytes(ins)
        return bytes(out)


def pack_pattern(cells):
    """FT2 pattern packing: a mask byte then only the fields present."""
    out = bytearray()
    for c in cells:
        if all(c):
            out += bytes(c)
            continue
        mask = 0x80
        for k in range(5):
            if c[k]:
                mask |= 1 << k
        out.append(mask)
        out += bytes(x for x in c if x)
    return bytes(out)


def instrument_bytes(ins):
    """Instrument header (263 bytes), one sample header, delta sample data."""
    env = b''.join(struct.pack('<HH', x, y) for x, y in ins.env)
    h = struct.pack('<I', 263) + ins.name.encode('ascii').ljust(22, b'\0')
    h += struct.pack('<BHI', 0, 1, 40) + bytes(96)
    h += env.ljust(48, b'\0') + bytes(48)
    h += bytes([len(ins.env), 0, ins.sus, 0, 0, 0, 0, 0, 3, 0]) + bytes(ins.vib)
    h += struct.pack('<H', 0) + bytes(22)
    assert len(h) == 263
    data, prev = bytearray(), 0
    for s in ins.data:
        data.append((s - prev) & 0xFF)
        prev = s
    h += struct.pack('<IIIBbBBbB', len(data), 0, ins.loop, 64, 0,
                     1 if ins.loop else 0, ins.pan, 12, 0)
    h += ins.tag.encode('ascii').ljust(22, b'\0')
    return h + bytes(data)


# ----------------------------------------------------------------- MIDI --

GM_DRUM = {'kick': 36, 'snare': 38, 'hat': 42, 'tom': 45, 'noise': 49}


def vlq(n):
    b = [n & 0x7F]
    n >>= 7
    while n:
        b.append(0x80 | (n & 0x7F))
        n >>= 7
    return bytes(reversed(b))


def midi_track(events):
    """events: (tick, order, bytes); returns an MTrk chunk."""
    out, last = bytearray(), 0
    for t, _, data in sorted(events, key=lambda e: (e[0], e[1])):
        out += vlq(t - last) + data
        last = t
    out += b'\x00\xff\x2f\x00'
    return b'MTrk' + struct.pack('>I', len(out)) + out


def song_midi(song, tracks, rows_per_quarter, tpq=96):
    """Standard MIDI File type 1: a tempo track, then one track per voice.
    tracks: (channels, name, GM program 1-128 or 'drums')."""
    tpr = tpq // rows_per_quarter
    usq = lambda bpm: round(rows_per_quarter * song.speed * 2.5 / bpm * 1e6)
    meta = [(0, 0, b'\xff\x03' + vlq(len(song.title)) + song.title.encode()),
            (0, 0, b'\xff\x58\x04' + bytes([song.timesig[0],
                                             song.timesig[1].bit_length() - 1, 24, 8])),
            (0, 0, b'\xff\x51\x03' + usq(song.bpm).to_bytes(3, 'big'))]
    for row, eff in song.gfx:
        if eff[0] == 'F' and int(eff[1:], 16) >= 0x20 and row:
            meta.append((row * tpr, 0, b'\xff\x51\x03' +
                         usq(int(eff[1:], 16)).to_bytes(3, 'big')))
    chunks = [midi_track(meta)]
    keys = {i.key: i for i in song.insts}
    for n, (chans, name, prog) in enumerate(tracks):
        mch = 9 if prog == 'drums' else n
        evs = [(0, 0, b'\xff\x03' + vlq(len(name)) + name.encode())]
        if prog != 'drums':
            evs.append((0, 0, bytes([0xC0 | mch, prog - 1])))
        for ch in chans:
            for e in song.events[ch]:
                vel = max(1, min(127, round((64 if e.vol is None else e.vol) * 2)))
                if prog == 'drums':
                    src = keys[e.inst].tag[4:].split(',')[0]
                    pitches, length = [GM_DRUM[src]], 1
                else:
                    pitches = [e.note + 11]
                    arp = [f for f in e.fxall if f[0] == '0' and f != '000']
                    if arp:
                        pitches += [e.note + 11 + int(arp[0][1], 16),
                                    e.note + 11 + int(arp[0][2], 16)]
                    length = e.gate if e.gate is not None else e.span
                on, off = e.row * tpr, (e.row + length) * tpr
                for p in pitches:
                    evs.append((on, 2, bytes([0x90 | mch, p, vel])))
                    evs.append((off, 1, bytes([0x80 | mch, p, 0])))
        chunks.append(midi_track(evs))
    return (b'MThd' + struct.pack('>IHHH', 6, 1, len(chunks), tpq) +
            b''.join(chunks))


# ================================================================= MUSIC ==
# Durations are in rows. Unless a song says otherwise a row is a sixteenth
# note (speed 6, 4 rows per beat), so :4 is a crotchet, :2 a quaver.

# ---- Ode to Joy (Beethoven, Symphony No. 9, finale), in D --------------
ODE_1_8 = ('F#4:4 F#4 G4 A4 | A4 G4 F#4 E4 | D4 D4 E4 F#4 | F#4:6 E4:2 E4:8 |'
           ' F#4:4 F#4 G4 A4 | A4 G4 F#4 E4 | D4 D4 E4 F#4 | E4:6 D4:2 D4:8 |')
ODE_9_16 = ('E4:4 E4 F#4 D4 | E4 F#4:2 G4 F#4:4 D4 | E4 F#4:2 G4 F#4:4 E4 |'
            ' D4 E4 A3:8 |'
            ' F#4:4 F#4 G4 A4 | A4 G4 F#4 E4 | D4 D4 E4 F#4 | E4:6 D4:2 D4:8 |')
ODE_CH_1_8 = 'D D A A D D D A D D A A D D A D'          # half bars
ODE_CH_9_16 = 'A D A D A D E7 A D D A A D D A D'


def ode_to_joy():
    s = Song('ode_to_joy.xm', 'Ode to Joy', 'lead echo arp bass drum hat',
             speed=6, bpm=120, rows=64, bar=16)
    s.inst('lead', 'Lead', 'rt1:pulse,8', (0, 5, 11, 4))
    s.inst('echo', 'Echo', 'rt1:pulse,8', (0, 5, 11, 4), pan=200)
    s.inst('harm', 'Harmony', 'rt1:pulse,4', (0, 5, 11, 4), pan=72)
    s.inst('arp', 'Arp', 'rt1:pulse,4', (0, 5, 5, 3), pan=96)
    s.inst('bass', 'Bass', 'rt1:tri', (0, 0, 15, 2))
    s.inst('kick', 'Kick', 'rt1:kick,12,6', (0, 0, 15, 0))
    s.inst('snare', 'Snare', 'rt1:snare,12,6', (0, 0, 15, 0), pan=144)
    s.inst('hat', 'Hat', 'rt1:hat,12,2', (0, 0, 15, 0), pan=168)
    kit = s.kit(K=('kick', 56), k=('kick', 40), S=('snare', 48),
                s=('snare', 28), h=('hat', 20), H=('hat', 32))
    theme = ODE_1_8 + ODE_9_16
    chart = ODE_CH_1_8 + ' ' + ODE_CH_9_16
    # intro: four bars of chords
    intro = 'D D G G D D A A'
    s.block(64, arp=s.fig(intro, 'A:4 A:4', 8, 'A3', inst='arp', vol=20),
            bass=s.fig(intro, 'R:8', 8, 'A2', inst='bass', vol=36))
    # the theme, quietly; a snare roll leads into the band
    s.block(256, lead=s.notes(theme, tr=12, inst='lead', vol=40, art=1),
            arp=s.fig(chart, 'A:4 A:4', 8, 'A3', inst='arp', vol=18),
            bass=s.fig(chart, 'R:8', 8, 'A2', inst='bass', vol=36),
            drum=s.drums('................|' * 15 + '........s.s.sSSS|', kit))
    # again with drums, a walking bass and an echo
    a = s.pos
    s.block(256, lead=s.notes(theme, tr=12, inst='lead', vol=50, art=1),
            arp=s.fig(chart, 'A:2 A:2 A:2 A:2', 8, 'A3', inst='arp', vol=24),
            bass=s.fig(chart, 'R:4 F:4', 8, 'A2', inst='bass', vol=40),
            drum=s.drums('K...S...K...S...|' * 15 + 'K...S...S.S.SSSS|', kit),
            hat=s.drums('h.h.h.h.h.h.h.h.|' * 16, kit))
    s.echo('lead', 'echo', a, s.pos, delay=3, scale=0.4, inst='echo')
    # second half once more in thirds, loud, slowing to the last chord
    lead = s.notes(ODE_9_16, tr=12, inst='lead', vol=58, art=1)
    body = ODE_CH_9_16.rsplit(' ', 1)[0]
    s.block(128, lead=lead,
            echo=s.harmony(lead, ODE_CH_9_16, 8, D_MAJOR, 'harm', 0.6),
            arp=s.fig(body, 'A:2 A:2 A:2 A:2', 8, 'A3', inst='arp', vol=28) +
            s.fig('D', 'A:8', 8, 'A3', inst='arp', vol=32),
            bass=s.fig(body, 'R:2 O:2 R:2 O:2', 8, 'A2', inst='bass', vol=44) +
            s.fig('D', 'R:8', 8, 'A2', inst='bass', vol=48),
            drum=s.drums('K...S...K.K.S...|' * 7 + 'K...S...K.......|', kit),
            hat=s.drums('h.H.h.H.h.H.h.H.|' * 7 + 'h.H.h.H.H.......|', kit))
    for k, bpm in enumerate((108, 96, 84, 72)):
        s.tempo(s.pos - 16 + 4 * k, bpm)
    return s


# ---- Fur Elise (Beethoven, WoO 59), A section --------------------------
# 3/8: a bar is six sixteenths, one row each.
FE_PICKUP = 'r:4 E5:1 D#5 |'
FE_RH = ('E5:1 D#5 E5 B4 D5 C5 | A4:3 C4:1 E4 A4 | B4:3 E4:1 G#4 B4 |'
         ' C5:3 E4:1 E5 D#5 | E5 D#5 E5 B4 D5 C5 | A4:3 C4:1 E4 A4 |'
         ' B4:3 E4:1 C5 B4 |')                                   # bars 1-7
FE_RH_TURN = 'A4:4 E5:1 D#5 |'                                  # 1st ending
FE_LH = ('r:6 | A2:1 E3 A3:4 | E2:1 E3 G#3:4 | A2:1 E3 A3:4 |'
         ' r:6 | A2:1 E3 A3:4 | E2:1 E3 G#3:4 |')
FE_LH_TURN = 'A2:1 E3 A3:4 |'
FE_PAD = ('r:6 | A3:6*037 | E3:6*047 | A3:6*037 | r:6 | A3:6*037 |'
          ' E3:6*047 |')


def fur_elise():
    s = Song('fur_elise.xm', 'Fur Elise', 'rh lh echo pad',
             speed=6, bpm=68, rows=48, bar=6, timesig=(3, 8))
    s.inst('keys', 'Keys', 'rt1:pulse,6', (0, 11, 5, 7))
    s.inst('lh', 'Left Hand', 'rt1:tri', (0, 11, 7, 7))
    s.inst('echo', 'Echo', 'rt1:pulse,6', (0, 11, 5, 7), pan=200)
    s.inst('bell', 'Music Box', 'rt1:sine', (0, 12, 0, 8))
    s.inst('bellecho', 'Music Box Echo', 'rt1:sine', (0, 12, 0, 8), pan=200)
    s.inst('pad', 'Pad', 'rt1:saw', (6, 0, 15, 10), pan=88)
    keys = lambda vol: s.notes(FE_RH + FE_RH_TURN, inst='keys', vol=vol)
    lh = lambda vol: s.notes(FE_LH + FE_LH_TURN, inst='lh', vol=vol)
    s.block(6, rh=s.notes(FE_PICKUP, inst='keys', vol=50))
    s.block(48, rh=keys(50), lh=lh(44))                     # as written
    a = s.pos
    s.block(48, rh=keys(52), lh=lh(44))                     # with an echo
    s.echo('rh', 'echo', a, s.pos, delay=3, scale=0.35, inst='echo')
    a = s.pos                                               # music box
    s.block(48, rh=s.notes(FE_RH + ' A4:4', tr=12, inst='bell', vol=46) +
            s.notes('E5:1 D#5', inst='keys', vol=50), lh=lh(40),
            pad=s.notes(FE_PAD + ' A3:6*037 |', inst='pad', vol=12))
    s.echo('rh', 'echo', a, s.pos - 2, delay=3, scale=0.35, inst='bellecho')
    a = s.pos                                               # last time
    s.block(60, rh=s.notes(FE_RH + ' A4:18 |', inst='keys', vol=52),
            lh=s.notes(FE_LH + ' A2:1 E3 A3:16 |', inst='lh', vol=46),
            pad=s.notes(FE_PAD + ' A3:18*037 |', inst='pad', vol=10))
    s.echo('rh', 'echo', a, s.pos, delay=3, scale=0.3, inst='echo')
    s.tempo(a + 36, 62)
    s.tempo(a + 42, 56)
    s.tempo(a + 48, 48)
    return s


# ---- Canon in D (Pachelbel): the ground and the first three variations --
CANON_GROUND = 'D3:4 A2 B2 F#2 G2 D2 G2 A2 |'
CANON_V1 = 'F#5:4 E5 D5 C#5 | B4 A4 B4 C#5 |'
CANON_V2 = 'D5:4 C#5 B4 A4 | G4 F#4 G4 E4 |'
CANON_V3 = 'D4:2 F#4 A4 G4 F#4 D4 F#4 E4 | D4 B3 D4 A4 G4 B4 A4 G4 |'
CANON_CHORDS = 'D A Bm F#m G D G A'                     # one per crotchet
CANON_BASS8 = 'D3:2 D4 A2 A3 B2 B3 F#2 F#3 | G2 G3 D2 D3 G2 G3 A2 A3 |'


def canon():
    s = Song('canon_in_d.xm', 'Canon in D', 'bass v1 v2 v3 arp echo drum hat',
             speed=6, bpm=76, rows=64, bar=16)
    s.inst('cello', 'Cello', 'rt1:tri', (1, 8, 12, 6))
    s.inst('vln1', 'Violin 1', 'rt1:pulse,6', (2, 8, 11, 6), pan=96)
    s.inst('vln2', 'Violin 2', 'rt1:pulse,5', (2, 8, 11, 6), pan=160)
    s.inst('vln3', 'Violin 3', 'rt1:pulse,7', (2, 8, 11, 6))
    s.inst('harp', 'Harpsichord', 'rt1:pulse,3', (0, 9, 0, 5), pan=112)
    s.inst('lead', 'Lead', 'rt1:pulse,4', (0, 6, 11, 5), vib=(0, 24, 3, 10))
    s.inst('echo', 'Echo', 'rt1:pulse,4', (0, 6, 11, 5), pan=208)
    s.inst('bass', 'Bass', 'rt1:tri', (0, 0, 15, 2))
    s.inst('kick', 'Kick', 'rt1:kick,12,6', (0, 0, 15, 0))
    s.inst('snare', 'Snare', 'rt1:snare,12,6', (0, 0, 15, 0), pan=144)
    s.inst('hat', 'Hat', 'rt1:hat,12,2', (0, 0, 15, 0), pan=168)
    kit = s.kit(K=('kick', 52), k=('kick', 36), S=('snare', 44),
                h=('hat', 18), H=('hat', 30))
    n = lambda text, inst, vol, tr=0: s.notes(text, tr=tr, inst=inst, vol=vol)
    ground = lambda: n(CANON_GROUND, 'cello', 40)
    chords = lambda vol: s.fig(CANON_CHORDS, 'A:2 A:2', 4, 'A3', inst='harp', vol=vol)
    # the canon: each violin enters two bars after the last
    s.block(32, bass=ground())
    s.block(32, bass=ground(), v1=n(CANON_V1, 'vln1', 40))
    s.block(32, bass=ground(), v1=n(CANON_V2, 'vln1', 40), v2=n(CANON_V1, 'vln2', 38))
    s.block(32, bass=ground(), v1=n(CANON_V3, 'vln1', 40), v2=n(CANON_V2, 'vln2', 38),
            v3=n(CANON_V1, 'vln3', 36))
    s.block(32, bass=ground(), v2=n(CANON_V3, 'vln2', 38), v3=n(CANON_V2, 'vln3', 36),
            arp=chords(20))
    s.block(32, bass=ground(), v3=n(CANON_V3, 'vln3', 36), arp=chords(22),
            hat=s.drums('h...h...h...h...|h...h...h.h.hhhh|', kit))
    # the same canon as a chiptune, a little faster
    a = s.pos
    s.tempo(a, 88)
    bass8 = lambda: n(CANON_BASS8, 'bass', 44)
    drums = lambda: s.drums('K...S...K...S...|K...S...K.k.S...|', kit)
    hats = lambda g='h.H.h.H.h.H.h.H.|': s.drums(g * 2, kit)
    s.block(32, bass=bass8(), v1=n(CANON_V1, 'lead', 48), arp=chords(22),
            drum=drums(), hat=hats())
    s.block(32, bass=bass8(), v1=n(CANON_V2, 'lead', 48), v2=n(CANON_V1, 'vln2', 36),
            arp=chords(22), drum=drums(), hat=hats())
    s.block(32, bass=bass8(), v1=n(CANON_V3, 'lead', 48, 12), v2=n(CANON_V2, 'vln2', 36),
            v3=n(CANON_V1, 'vln3', 34), arp=chords(24), drum=drums(), hat=hats())
    s.block(32, bass=bass8(), v1=n(CANON_V1, 'lead', 36, 12),
            v2=n(CANON_V3, 'vln2', 40, 12), v3=n(CANON_V2, 'vln3', 36),
            arp=chords(26), drum=drums(), hat=hats('hhHhhhHhhhHhhhHh|'))
    s.block(32, bass=bass8(), v1=n(CANON_V1, 'lead', 52), v2=n(CANON_V2, 'vln2', 42),
            arp=chords(26), drum=s.drums('K...S...K...S...|K...S...K...S.S.|', kit),
            hat=hats())
    s.tempo(s.pos - 8, 80)
    s.tempo(s.pos - 4, 72)
    # final chord
    s.block(16, bass=n('D3:16 |', 'bass', 48), v1=n('D5:16 |', 'lead', 52),
            v2=n('F#4:16 |', 'vln2', 42), v3=n('A4:16 |', 'vln3', 38),
            arp=n('D4:16*047 |', 'harp', 24),
            drum=s.drums('K...............|', kit))
    s.tempo(s.pos - 16, 64)
    s.echo('v1', 'echo', a, s.pos, delay=2, scale=0.35, inst='echo')
    return s


# ---- In the Hall of the Mountain King (Grieg, Peer Gynt), main phrase ---
MK_THEME = ('B2:2 C#3 D3 E3 F#3 D3 F#3:4 | F3:2 C#3 F3:4 E3:2 C3 E3:4 |'
            ' B2:2 C#3 D3 E3 F#3 D3 F#3 B3 | A3 F#3 D3 F#3 A3:8 |')
MK_TEMPI = [92, 96, 100, 106, 112, 120, 128, 138, 148, 160, 172, 186, 200,
            216, 232, 248]                                      # per two bars


def mountain_king():
    s = Song('mountain_king.xm', 'Mountain King', 'low mid high pedal drum hat',
             speed=6, bpm=MK_TEMPI[0], rows=64, bar=16)
    s.inst('bsn', 'Bassoon', 'rt1:pulse,3', (0, 5, 8, 2))
    s.inst('str', 'Strings', 'rt1:pulse,6', (0, 5, 9, 2), pan=96)
    s.inst('picc', 'Piccolo', 'rt1:pulse,4', (0, 5, 9, 2), pan=160)
    s.inst('horn', 'Horn', 'rt1:pulse,8', (5, 0, 15, 9))
    s.inst('bass', 'Bass', 'rt1:tri', (0, 4, 10, 2))
    s.inst('kick', 'Kick', 'rt1:kick,12,6', (0, 0, 15, 0))
    s.inst('snare', 'Snare', 'rt1:snare,12,6', (0, 0, 15, 0), pan=144)
    s.inst('hat', 'Hat', 'rt1:hat,12,2', (0, 0, 15, 0), pan=168)
    s.inst('crash', 'Cymbal', 'rt1:noise,0', (0, 12, 0, 8), pan=150)
    kit = s.kit(K=('kick', 56), k=('kick', 36), S=('snare', 48), s=('snare', 26),
                h=('hat', 22), H=('hat', 32), X=('crash', 40, 'C5'))
    theme = lambda tr, inst, vol: s.notes(MK_THEME, tr=tr, inst=inst, vol=vol, art=1) * 2
    pedal = lambda vol: s.notes('B2:62 r:2 | B2:62 r:2 |', inst='horn', vol=vol)
    s.block(128, low=theme(0, 'bsn', 34), pedal=pedal(18))
    s.block(128, low=theme(0, 'bsn', 38), mid=theme(12, 'str', 40), pedal=pedal(22),
            drum=s.drums('k...s...k...s...|' * 8, kit))
    s.block(128, low=theme(0, 'bsn', 46), mid=theme(12, 'str', 48),
            high=theme(24, 'picc', 40),
            drum=s.drums('K.s.K.s.K.s.K.s.|' * 8, kit),
            hat=s.drums('h.h.h.h.h.h.h.h.|' * 8, kit))
    s.block(128, low=theme(0, 'bsn', 52), mid=theme(12, 'str', 54),
            high=theme(24, 'picc', 46), pedal=theme(-12, 'bass', 50),
            drum=s.drums('K.S.K.S.K.S.K.S.|' * 7 + 'K.S.S.S.SSSSSSSS|', kit),
            hat=s.drums(('X.hhh.hhh.hhh.hh|' + 'H.hhH.hhH.hhH.hh|' * 3) * 2, kit))
    for k, bpm in enumerate(MK_TEMPI[1:]):
        s.tempo(32 * (k + 1), bpm)
    # the end: B minor hits, F sharp, B minor
    hit = lambda b, f, inst: s.notes('%s:2 r:6 %s:2 r:6 | %s:2 r:2 %s:2 r:2 %s:8 |'
                                     ' ~:12 r:4 |' % (b, b, f, f, b), inst=inst, vol=54)
    s.tempo(s.pos, 150)
    s.block(48, low=hit('B2', 'F#2', 'bsn'), mid=hit('D4', 'C#4', 'str'),
            high=hit('F#4', 'A#4', 'picc'), pedal=hit('B1', 'F#1', 'bass'),
            drum=s.drums('K.......K.......|K...K...K.......|................|', kit),
            hat=s.drums('X.......X.......|X...X...X.......|................|', kit))
    return s


# ---- Gymnopedie No. 1 (Satie), the opening -----------------------------
# 3/4: a bar is twelve rows.
GY_BASS = 'G2:12 | D2:12 |'
GY_CHORD = ('r:4 B3:8 | r:4 A3:8 |', 'r:4 D4:8 | r:4 C#4:8 |',
            'r:4 F#4:8 | r:4 F#4:8 |')
GY_MELODY = ('r:4 F#5:4 A5:4 | G5:4 F#5:4 C#5:4 | B4:4 C#5:4 D5:4 | A4:12 |'
             ' F#4:48 |')


def gymnopedie():
    s = Song('gymnopedie.xm', 'Gymnopedie No. 1', 'bass c1 c2 c3 mel echo',
             speed=6, bpm=72, rows=48, bar=12, timesig=(3, 4))
    s.inst('bass', 'Bass', 'rt1:tri', (0, 13, 6, 10))
    s.inst('chord', 'Chord', 'rt1:sine', (3, 11, 9, 9))
    s.inst('flute', 'Flute', 'rt1:tri', (3, 8, 12, 8), vib=(0, 48, 3, 8))
    s.inst('echo', 'Echo', 'rt1:tri', (3, 8, 12, 8), pan=200)

    def accomp(pairs, vb, vc):
        """pairs of bars (G major 7, D major 7) of left hand."""
        v = dict(bass=s.notes(GY_BASS, inst='bass', vol=vb) * pairs)
        for ch, text in zip(('c1', 'c2', 'c3'), GY_CHORD):
            v[ch] = s.notes(text, inst='chord', vol=vc) * pairs
        return v
    s.block(48, **accomp(2, 42, 24))
    s.block(96, mel=s.notes(GY_MELODY, inst='flute', vol=52), **accomp(4, 42, 24))
    s.block(48, **accomp(2, 40, 22))
    a = s.pos
    s.block(96, mel=s.notes(GY_MELODY, inst='flute', vol=56), **accomp(4, 42, 24))
    s.echo('mel', 'echo', a, s.pos, delay=6, scale=0.3, inst='echo')
    s.block(24, **accomp(1, 38, 20))
    s.block(24, **accomp(1, 30, 15))
    for k, bpm in enumerate((68, 64, 58, 52)):
        s.tempo(s.pos - 48 + 12 * k, bpm)
    return s


# ---- Prelude in C (Bach, BWV 846), bars 1-31 and the final chord -------
# Each bar: the five notes of its broken chord, played as Bach does twice
# per bar: n1 (held), n2 (held), then n3 n4 n5 n3 n4 n5.
BACH = ['C4 E4 G4 C5 E5', 'C4 D4 A4 D5 F5', 'B3 D4 G4 D5 F5', 'C4 E4 G4 C5 E5',
        'C4 E4 A4 E5 A5', 'C4 D4 F#4 A4 D5', 'B3 D4 G4 D5 G5', 'B3 C4 E4 G4 C5',
        'A3 C4 E4 G4 C5', 'D3 A3 D4 F#4 C5', 'G3 B3 D4 G4 B4', 'G3 Bb3 E4 G4 C#5',
        'F3 A3 D4 A4 D5', 'F3 Ab3 D4 F4 B4', 'E3 G3 C4 G4 C5', 'E3 F3 A3 C4 F4',
        'D3 F3 A3 C4 F4', 'G2 D3 G3 B3 F4', 'C3 E3 G3 C4 E4', 'C3 G3 Bb3 C4 E4',
        'F2 F3 A3 C4 E4', 'F#2 C3 A3 C4 Eb4', 'Ab2 F3 B3 C4 D4', 'G2 F3 G3 B3 D4',
        'G2 E3 G3 C4 E4', 'G2 D3 G3 C4 F4', 'G2 D3 G3 B3 F4', 'G2 Eb3 A3 C4 F#4',
        'G2 E3 G3 C4 G4', 'G2 D3 G3 C4 F4', 'G2 D3 G3 B3 F4']
BACH_DYN = ([40] * 4 + [44] * 7 + [48] * 4 + [44] * 4 + [48, 50, 52, 54, 52] +
            [46, 48, 50, 52, 54, 58, 60])


def bach_prelude():
    s = Song('bach_prelude.xm', 'Prelude in C', 'n1 n2 fig echo',
             speed=6, bpm=70, rows=64, bar=16)
    s.inst('bass', 'Bass', 'rt1:tri', (0, 12, 8, 8))
    s.inst('harp', 'Harp', 'rt1:pulse,6', (0, 9, 5, 6))
    s.inst('echo', 'Echo', 'rt1:pulse,6', (0, 9, 5, 6), pan=200)
    n1 = n2 = fig = V()
    for notes, v in zip(BACH, BACH_DYN):
        a, b, c, d, e = notes.split()
        half = 'r:2 %s:1 %s %s %s %s %s' % (c, d, e, c, d, e)
        n1 += s.notes('%s:8 %s:8 |' % (a, a), inst='bass', vol=v - 6)
        n2 += s.notes('r:1 %s:7 r:1 %s:7 |' % (b, b), inst='harp', vol=v - 10)
        fig += s.notes(half + ' ' + half + ' |', inst='harp', vol=v)
    n1 += s.notes('C3:16 |', inst='bass', vol=50)              # final chord
    n2 += s.notes('r:1 G3:15 |', inst='harp', vol=40)
    fig += s.notes('r:2 C4:1 E4 G4 C5:11 |', inst='harp', vol=50)
    s.block(n1.length, n1=n1, n2=n2, fig=fig)
    s.echo('fig', 'echo', 0, s.pos, delay=3, scale=0.35, inst='echo')
    s.tempo(s.pos - 48, 64)
    s.tempo(s.pos - 32, 58)
    s.tempo(s.pos - 16, 50)
    return s


# ---- Greensleeves (traditional), verse and refrain ---------------------
# 3/4: a bar is twelve rows.
GS_VERSE = ('C5:8 D5:4 | E5:6 F5:2 E5:4 | D5:8 B4:4 | G4:6 A4:2 B4:4 |'
            ' C5:8 A4:4 | A4:6 G#4:2 A4:4 | B4:8 G#4:4 | E4:8 A4:4 |'
            ' C5:8 D5:4 | E5:6 F5:2 E5:4 | D5:8 B4:4 | G4:6 A4:2 B4:4 |'
            ' C5:6 B4:2 A4:4 | G#4:6 F#4:2 G#4:4 | A4:24 |')
GS_REFRAIN = ('G5:12 | G5:6 F#5:2 E5:4 | D5:8 B4:4 | G4:6 A4:2 B4:4 |'
              ' C5:8 A4:4 | A4:6 G#4:2 A4:4 | B4:8 G#4:4 | E4:12 |'
              ' G5:12 | G5:6 F#5:2 E5:4 | D5:8 B4:4 | G4:6 A4:2 B4:4 |'
              ' C5:6 B4:2 A4:4 | G#4:6 F#4:2 G#4:4 | A4:24 |')
GS_CH_VERSE = 'Am C G Em Am F E E Am C G Em Am E Am Am'
GS_CH_REFRAIN = 'C C G Em Am F E E C C G Em Am E Am Am'
GS_LUTE = 'R:2 T:2 F:2 O:2 F:2 T:2'
GS_MIDI = [(('mel',), 'Recorder', 75), (('harm',), 'Viol', 42),
           (('lute',), 'Lute', 25), (('bass',), 'Bass', 33),
           (('drum', 'hat'), 'Drums', 'drums')]


def greensleeves():
    s = Song('greensleeves.xm', 'Greensleeves', 'mel harm lute bass drum hat',
             speed=6, bpm=100, rows=48, bar=12, timesig=(3, 4))
    s.inst('rec', 'Recorder', 'rt1:tri', (1, 6, 12, 5), vib=(0, 24, 4, 10))
    s.inst('viol', 'Viol', 'rt1:pulse,6', (2, 8, 10, 6), pan=80)
    s.inst('lute', 'Lute', 'rt1:pulse,3', (0, 9, 0, 6), pan=168)
    s.inst('bass', 'Bass', 'rt1:tri', (0, 10, 8, 5))
    s.inst('tabor', 'Tabor', 'rt1:tom,7,4', (0, 0, 15, 0))
    s.inst('tick', 'Tick', 'rt1:hat,12,0', (0, 0, 15, 0), pan=176)
    kit = s.kit(T=('tabor', 40), t=('tabor', 26), h=('tick', 16), H=('tick', 24))
    lute = lambda ch, vol: s.fig(ch, GS_LUTE, 12, 'C3', inst='lute', vol=vol)
    s.block(48, mel=s.notes('r:12 | r:12 | r:12 | r:8 A4:4 |', inst='rec', vol=46),
            lute=lute('Am E Am Am', 26),
            bass=rest(24) + s.fig('Am Am', 'R:12', 12, 'E2', inst='bass', vol=36))
    s.block(192, mel=s.notes(GS_VERSE, inst='rec', vol=46, art=1),
            lute=lute(GS_CH_VERSE, 26),
            bass=s.fig(GS_CH_VERSE, 'R:8 F:4', 12, 'E2', inst='bass', vol=38))
    mel = s.notes(GS_REFRAIN, inst='rec', vol=50, art=1)
    s.block(192, mel=mel, harm=s.harmony(mel, GS_CH_REFRAIN, 12, A_MINOR, 'viol', 0.55),
            lute=lute(GS_CH_REFRAIN, 26),
            bass=s.fig(GS_CH_REFRAIN, 'R:8 F:4', 12, 'E2', inst='bass', vol=40),
            drum=s.drums('T...t...t...|' * 16, kit),
            hat=s.drums('......h.....|' * 16, kit))
    mel = s.notes(GS_REFRAIN, inst='rec', vol=54, art=1)
    s.block(192, mel=mel, harm=s.harmony(mel, GS_CH_REFRAIN, 12, A_MINOR, 'viol', 0.6),
            lute=s.fig(GS_CH_REFRAIN, 'A:2 A:2 A:2 A:2 A:2 A:2', 12, 'A3',
                       inst='lute', vol=24),
            bass=s.fig(GS_CH_REFRAIN, 'R:4 F:4 O:4', 12, 'E2', inst='bass', vol=42),
            drum=s.drums('T..tT...t.t.|' * 16, kit),
            hat=s.drums('h.h.H.h.h.h.|' * 16, kit))
    s.block(48, lute=lute('Am E Am', 24) + s.notes('A3:12 |', inst='lute', vol=26),
            bass=s.fig('Am E Am Am', 'R:12', 12, 'E2', inst='bass', vol=36),
            mel=s.notes('r:36 | A4:12 |', inst='rec', vol=44),
            harm=s.notes('r:36 | E4:12 |', inst='viol', vol=28),
            drum=s.drums('T...........|' * 3 + 'T...........|', kit))
    for k, bpm in enumerate((92, 84, 72)):
        s.tempo(s.pos - 36 + 12 * k, bpm)
    return s


# ---- Sparkstone (original, RockTrack contributors) ---------------------
# A major, 136 BPM. The hook runs down each chord in a 3-3-2 rhythm.
SP_HOOK = ('C#6:3 B5:3 A5:2 E5:3 A5:3 B5:2 | B5:3 G#5:3 E5:2 B4:3 E5:3 G#5:2 |'
           ' A5:3 G#5:3 F#5:2 C#5:3 F#5:3 A5:2 | F#5:3 E5:3 D5:2 A4:2 B4 D5 E5 |'
           ' C#6:3 B5:3 A5:2 E5:3 A5:3 B5:2 | B5:3 G#5:3 E5:2 B4:3 E5:3 G#5:2 |'
           ' A5:3 G#5:3 F#5:2 C#6:3 B5:3 A5:2 | A5:3 B5:3 C#6:2 E6:2*308 ~:6*444 |')
SP_HOOK_CH = 'A A E E F#m F#m D D A A E E F#m F#m D E'          # half bars
SP_VERSE = ('F#5:6 E5:2 C#5:4 A4:4 | D5:6 C#5:2 D5:4 F#5:4 |'
            ' E5:4 ~:8*444 C#5:4 | B4:6 C#5:2 E5:3*306 ~:5*434 |'
            ' F#5:6 E5:2 C#5:4 A4:4 | D5:6 C#5:2 D5:4 F#5:4 |'
            ' F#5:6 G#5:2 A5:4 D#5:4 | E5:8 G#5:4 B5:4 |')
SP_VERSE_CH = 'F#m F#m D D A A E E F#m F#m D D B B E E'
SP_VERSE_BARS = 'F#m D A E F#m D B E'


def sparkstone():
    s = Song('sparkstone.xm', 'Sparkstone', 'lead echo arp bass drum hat harm fx',
             speed=6, bpm=136, rows=64, bar=16, restart=3)
    s.inst('lead', 'Lead', 'rt1:pulse,4', (0, 6, 10, 5))
    s.inst('square', 'Square', 'rt1:pulse,8', (0, 6, 10, 5))
    s.inst('echo', 'Echo', 'rt1:pulse,4', (0, 6, 10, 5), pan=208)
    s.inst('harm', 'Harmony', 'rt1:pulse,8', (0, 6, 9, 5), pan=64)
    s.inst('arp', 'Arp', 'rt1:pulse,2', (0, 5, 4, 3), pan=96)
    s.inst('bass', 'Bass', 'rt1:tri', (0, 0, 15, 2))
    s.inst('pad', 'Pad', 'rt1:saw', (5, 8, 10, 9), pan=160)
    s.inst('riser', 'Riser', 'rt1:saw', (0, 0, 15, 4), pan=144)
    s.inst('kick', 'Kick', 'rt1:kick,12,6', (0, 0, 15, 0))
    s.inst('snare', 'Snare', 'rt1:snare,12,6', (0, 0, 15, 0), pan=140)
    s.inst('hat', 'Hat', 'rt1:hat,12,2', (0, 0, 15, 0), pan=168)
    s.inst('tom', 'Tom', 'rt1:tom,12,4', (0, 0, 15, 0))
    s.inst('crash', 'Crash', 'rt1:noise,0', (0, 12, 0, 8), pan=150)
    kit = s.kit(K=('kick', 60), k=('kick', 44), S=('snare', 52), s=('snare', 32),
                R=('snare', 44, 'C4', 'E93'), Q=('snare', 54, 'C4', 'E92'),
                h=('hat', 20), H=('hat', 32), X=('crash', 44, 'C5'),
                u=('tom', 46, 'F4'), t=('tom', 46, 'C4'), y=('tom', 46, 'G3'))

    def intro():
        arp = V()
        for c, vol in zip('A E F#m D'.split(), (14, 18, 22, 26)):
            arp += s.fig(c, 'A:2 ' * 8, 16, 'A3', inst='arp', vol=vol)
        s.block(64, arp=arp,
                hat=s.drums('................|h.h.h.h.h.h.h.h.|'
                            'h.h.h.h.h.h.h.h.|hhhhhhhhHHHHHHHH|', kit),
                drum=s.drums('................|................|'
                             'K...K...K...K...|K...K...S.S.RRRR|', kit),
                bass=rest(32) + s.fig('F#m D', 'R:2 O:2 ' * 4, 16, 'E2',
                                      inst='bass', vol=40),
                fx=rest(32) + s.ramp('E4', 32, 6, 36, '101', 'riser'))

    def hook(vol, harm=False, hats='h.H.h.H.h.H.h.H.|'):
        a = s.pos
        lead = s.notes(SP_HOOK, inst='lead', vol=vol, art=1)
        v = dict(lead=lead,
                 arp=s.fig(SP_HOOK_CH, 'A:2 A:2 A:2 A:2', 8, 'A3', inst='arp', vol=24),
                 bass=s.fig(SP_HOOK_CH, 'R:2 O:2 R:2 O:2', 8, 'E2', inst='bass', vol=44),
                 drum=s.drums('K...S...K.k.S...|' * 7 + 'K...S...K.S.SSSS|', kit),
                 hat=s.drums(hats * 8, kit),
                 fx=s.drums('X' + '.' * 127, kit))
        if harm:
            v['harm'] = s.harmony(lead, SP_HOOK_CH, 8, A_MAJOR, 'harm', 0.5)
        s.block(128, **v)
        s.echo('lead', 'echo', a, s.pos, delay=1, scale=0.4, inst='echo')

    def verse(inst='lead', harm=False):
        a = s.pos
        lead = s.notes(SP_VERSE, inst=inst, vol=50, art=1)
        v = dict(lead=lead,
                 arp=s.fig(SP_VERSE_CH, 'A:4 A:4', 8, 'A3', inst='arp', vol=20),
                 bass=s.fig(SP_VERSE_CH, 'R:3 R:3 O:2', 8, 'E2', inst='bass', vol=44),
                 drum=s.drums('K.....k.S.......|' * 7 + 'K.....k.S.u.t.y.|', kit),
                 hat=s.drums('h.h.h.h.h.h.h.h.|' * 8, kit))
        if harm:
            v['harm'] = s.harmony(lead, SP_VERSE_CH, 8, A_MAJOR, 'harm', 0.5)
            v['fx'] = s.drums('X' + '.' * 127, kit)
        else:
            v['harm'] = s.fig(SP_VERSE_BARS, 'T:16', 16, 'C3', inst='pad', vol=18)
        s.block(128, **v)
        s.echo('lead', 'echo', a, s.pos, delay=1, scale=0.4, inst='echo')

    def breakdown():
        a = s.pos
        s.block(64, lead=s.notes('C#6:3 B5:3 A5:2 r:8 | r:16 |'
                                 ' A5:3 G#5:3 F#5:2 r:8 | r:16 |', inst='lead', vol=44),
                arp=s.fig('A E F#m D', 'A:16', 16, 'A3', inst='pad', vol=20),
                bass=s.fig('A E F#m D', 'R:2 r:2 R:2 r:2 O:2 r:2 R:2 r:2', 16, 'E2',
                           inst='bass', vol=40),
                drum=s.drums('K...K...K...K...|' * 4, kit))
        s.echo('lead', 'echo', a, s.pos, delay=3, scale=0.45, inst='echo')

    def build():
        arp = bass = V()
        for c, vol in zip('D D E E'.split(), (20, 24, 28, 32)):
            arp += s.fig(c, 'A:2 ' * 8, 16, 'A3', inst='arp', vol=vol)
            bass += s.fig(c, 'R:2 ' * 8, 16, 'E2', inst='bass', vol=vol + 16)
        s.block(64, arp=arp, bass=bass,
                drum=s.drums('K...s...K...s...|K.s.K.s.K.s.K.s.|'
                             'SsSsSsSsSSSSSSSS|RRRRRRRRQQQQQQQQ|', kit),
                hat=s.drums('h.h.h.h.h.h.h.h.|h.h.h.h.h.h.h.h.|'
                            'hhhhhhhhhhhhhhhh|HHHHHHHHHHHHHHHH|', kit),
                fx=s.ramp('A3', 64, 4, 40, '101', 'riser'))

    intro()                                     # 0
    hook(52)                                    # 1-2
    verse()                                     # 3-4  <- the loop restarts here
    hook(54, harm=True)                         # 5-6
    breakdown()                                 # 7
    build()                                     # 8
    hook(60, harm=True, hats='hhHhhhHhhhHhhhHh|')   # 9-10
    verse('square', harm=True)                  # 11-12
    hook(60, harm=True, hats='hhHhhhHhhhHhhhHh|')   # 13-14
    return s


# ---- Quartz Comet (original, RockTrack contributors) -------------------
# E minor, 160 BPM. A pedal-note riff, a verse, a chorus of long falling
# notes, a solo with a ping-pong echo, a breakdown, a build that steps up
# from B to C sharp, and the last chorus a tone higher in F sharp minor.
QC_RIFF = ('E5:1 E5 B5:2 E5:1 E5 A5:2 E5:1 E5 G5:2 F#5 G5 |'
           ' E5:1 E5 C6:2 E5:1 E5 B5:2 E5:1 E5 G5:2 E5 G5 |'
           ' D5:1 D5 A5:2 D5:1 D5 C6:2 D5:1 D5 A5:2 F#5 A5 |'
           ' D#5:1 D#5 B5:2 D#5:1 D#5 F#5:2 D#5:1 D#5 A5:2 F#5 D#5 |')
QC_RIFF_CH = 'Em C D B'
QC_VERSE = ('B4:2 E5 G5 B5:4 A5:2 G5 F#5 | G5:4 E5 r:2 C5 E5 G5 |'
            ' A5:4 F#5:2 D5 F#5 A5 C6 B5 | B5:6 A5:2 F#5:4 D5 |'
            ' B4:2 E5 G5 B5:4 A5:2 G5 B5 | C6:4 B5:2 G5 E5:4 G5:2 C6 |'
            ' A5:2 C6 E6:4 D6:2 C6 B5 A5 | B5:2 A5 F#5 D#5 F#5 B5 C6 D#6 |')
QC_VERSE_CH = 'Em C D Bm Em C Am B'
QC_CHORUS = ('E6:2 ~:4*446 D6:6 C6:4 | D6:2 ~:4*446 C6:6 B5:2 A5 |'
             ' B5:2 ~:4*446 G5:6 E5:4 | F#5:2 G5 A5:4+306 ~:8*446 |'
             ' E6:2 ~:4*446 D6:6 C6:4 | D6:2 ~:4*446 E6:6 F#6:4 |'
             ' F#6:2 ~:4*446 D#6:6 B5:4 | E6:2+304 ~:14*446 |')
QC_CHORUS_CH = 'C D Em D C D B Em'
QC_SOLO = ('E5:1 F#5 G5 A5 B5:2 E6 D6:1 B5 A5 G5 A5:2 B5 |'
           ' C6:1 B5 G5 E5 G5:2 C6 E6:4 D6:2 C6 |'
           ' D6:1 C6 A5 F#5 A5:2 D6 F#6:2 ~:2*446 E6:2 D6 |'
           ' B5:2 D6 F#6 D6 B5:4 ~:4*208 |'
           ' G5:1 A5 B5 G5 E6:2 B5 G6:1 F#6 E6 D6 B5:2 G5 |'
           ' E6:2 C6 G5 C6 E6 G6 E6 C6 |'
           ' A5:1 C6 E6 C6 A5 E5 A5 C6 E6:2 D6 C6 B5 |'
           ' B5:2 ~:6*446 r:2 F#5:1 G5 A5 B5 C6 D#6 |')
QC_BREAK = ('E5:2 ~:4*446 D5:6+306 C5:4+306 | D5:2 ~:4*446 C5:6 B4:2 A4 |'
            ' B4:2 ~:4*446 G4:6 E4:4 | F#4:2 G4 A4:4+306 ~:8*446 |'
            ' E5:2 ~:4*446 D5:6+306 C5:4+306 | D5:2 ~:4*446 E5:6 F#5:4 |'
            ' F#5:2 ~:4*446 D#5:6 B4:4 | F#4:2 B4 D#5 F#5 B5:2 ~:6*446 |')
QC_BREAK_CH = 'C D Em D C D B B'
QC_OCTAVES = 'R:2 O R O R O R O'
QC_GALLOP = 'R:1 R O:2 R:1 R O:2 R:1 R O:2 R:1 R O:2'


def quartz_comet():
    s = Song('quartz_comet.xm', 'Quartz Comet', 'lead echo harm arp bass drum hat fx',
             speed=6, bpm=160, rows=64, bar=16)
    s.inst('lead', 'Lead', 'rt1:pulse,4', (0, 6, 11, 5))
    s.inst('solo', 'Solo', 'rt1:pulse,2', (0, 5, 11, 4))
    s.inst('square', 'Square', 'rt1:pulse,8', (0, 6, 11, 6))
    s.inst('echo', 'Echo', 'rt1:pulse,4', (0, 6, 11, 5), pan=208)
    s.inst('harm', 'Harmony', 'rt1:pulse,8', (0, 6, 10, 5), pan=64)
    s.inst('arp', 'Arp', 'rt1:pulse,3', (0, 4, 6, 3), pan=96)
    s.inst('bass', 'Bass', 'rt1:pulse,6', (0, 5, 10, 1))
    s.inst('pad', 'Pad', 'rt1:saw', (7, 8, 11, 9), pan=168)
    s.inst('kick', 'Kick', 'rt1:kick,12,4', (0, 0, 15, 0))
    s.inst('snare', 'Snare', 'rt1:snare,13,4', (0, 0, 15, 0), pan=140)
    s.inst('hat', 'Hat', 'rt1:hat,12,1', (0, 0, 15, 0), pan=168)
    s.inst('tom', 'Tom', 'rt1:tom,12,3', (0, 0, 15, 0))
    s.inst('crash', 'Crash', 'rt1:noise,0', (0, 12, 0, 8), pan=150)
    s.inst('whoosh', 'Whoosh', 'rt1:noise,1', (0, 0, 15, 5), pan=112)
    kit = s.kit(K=('kick', 58), k=('kick', 40), S=('snare', 50), s=('snare', 30),
                R=('snare', 40, 'C4', 'E93'), Q=('snare', 46, 'C4', 'E92'),
                h=('hat', 18), H=('hat', 30), O=('hat', 28, 'F3'),
                X=('crash', 38, 'C5'),
                u=('tom', 48, 'F4'), t=('tom', 48, 'C4'), y=('tom', 48, 'G3'))
    groove = 'K...S...K..KS...|'
    drive = 'K...S..kK.K.S..k|'
    crash = lambda bars: s.drums(('X' + '.' * 63) * (bars // 4), kit)

    def intro():
        arp = V()
        for c, vol in zip(QC_RIFF_CH.split(), (12, 16, 22, 28)):
            arp += s.fig(c, 'A:2 ' * 8, 16, 'A3', inst='arp', vol=vol)
        s.block(64, arp=arp,
                bass=rest(32) + s.fig('D B', QC_OCTAVES, 16, 'E2', inst='bass', vol=40),
                hat=s.drums('................|h.h.h.h.h.h.h.h.|'
                            'h.H.h.H.h.H.h.H.|hhhhhhhhHHHHHHHH|', kit),
                drum=s.drums('................|................|'
                             'K...K...K...K...|K...K...SsSsRRQQ|', kit),
                fx=rest(32) + s.ramp('B3', 32, 6, 40, '102', 'whoosh'))

    def riff(tr=0):
        chart = transpose_chart(QC_RIFF_CH, tr)
        s.block(64, lead=s.notes(QC_RIFF, tr=tr, inst='solo', vol=46),
                harm=s.notes(QC_RIFF, tr=tr - 12, inst='harm', vol=24),
                arp=s.fig(chart, 'A:4 ' * 4, 16, 'A3', inst='arp', vol=20),
                bass=s.fig(chart, QC_OCTAVES, 16, 'E2', inst='bass', vol=40),
                drum=s.drums(groove * 3 + 'K...S...u.u.t.y.|', kit),
                hat=s.drums('h.H.h.H.h.H.h.H.|' * 4, kit), fx=crash(4))

    def verse():
        a = s.pos
        s.block(128, lead=s.notes(QC_VERSE, inst='lead', vol=50, art=1),
                harm=s.fig(QC_VERSE_CH, 'T:16', 16, 'C4', inst='pad', vol=16),
                arp=s.fig(QC_VERSE_CH, 'A:2 ' * 8, 16, 'A3', inst='arp', vol=20),
                bass=s.fig(QC_VERSE_CH, QC_OCTAVES, 16, 'E2', inst='bass', vol=40),
                drum=s.drums(groove * 7 + 'K...S...K.K.SSSS|', kit),
                hat=s.drums('h.H.h.H.h.H.h.H.|' * 8, kit), fx=crash(4) + rest(64))
        s.echo('lead', 'echo', a, s.pos, delay=3, scale=0.35, inst='echo')

    def chorus(tr=0, harm=False):
        a = s.pos
        chart = transpose_chart(QC_CHORUS_CH, tr)
        lead = s.notes(QC_CHORUS, tr=tr, inst='lead', vol=54, art=1)
        s.block(128, lead=lead,
                harm=s.harmony(lead, chart, 16, scale(PC_NAME[(4 + tr) % 12], True),
                               'harm', 0.5) if harm else
                s.fig(chart, 'T:16', 16, 'C4', inst='pad', vol=18),
                arp=s.fig(chart, 'A:4 ' * 4, 16, 'A3', inst='arp', vol=22),
                bass=s.fig(chart, QC_GALLOP, 16, 'E2', inst='bass', vol=40),
                drum=s.drums(drive * 7 + 'K...S...SSu.t.y.|', kit),
                hat=s.drums('h.O.h.O.h.O.h.O.|' * 8, kit), fx=crash(8))
        s.echo('lead', 'echo', a, s.pos, delay=3, scale=0.35, inst='echo')

    def solo():
        a = s.pos
        s.block(128, lead=s.notes(QC_SOLO, inst='solo', vol=46),
                harm=s.fig(QC_VERSE_CH, 'T:16', 16, 'C4', inst='pad', vol=18),
                arp=s.fig(QC_VERSE_CH, 'A:2 ' * 8, 16, 'A3', inst='arp', vol=18),
                bass=s.fig(QC_VERSE_CH, QC_GALLOP, 16, 'E2', inst='bass', vol=40),
                drum=s.drums(drive * 7 + 'K...S...K.K.SSSS|', kit),
                hat=s.drums('hhHhhhHhhhHhhhHh|' * 8, kit), fx=crash(4) + rest(64))
        s.echo('lead', 'echo', a, s.pos, delay=3, scale=0.3, inst='echo')
        s.pan('echo', a, s.pos, (40, 216))

    def breakdown():
        a = s.pos
        s.block(128, lead=s.notes(QC_BREAK, inst='square', vol=44, art=1),
                harm=s.fig(QC_BREAK_CH, 'T:16*744', 16, 'C4', inst='pad', vol=24),
                arp=s.fig(QC_BREAK_CH, 'A:16', 16, 'A3', inst='arp', vol=18),
                bass=s.fig('C D Em D', 'R:16', 16, 'E2', inst='bass', vol=34) +
                s.fig('C D B B', QC_OCTAVES, 16, 'E2', inst='bass', vol=40),
                drum=s.drums('K...............|' * 4 + 'K.......K.......|' * 2 +
                             'K...K...K...K...|K...K...K.K.K.K.|', kit),
                hat=s.drums('....h.......h...|' * 4 + 'h.h.h.h.h.h.h.h.|' * 4, kit))
        s.echo('lead', 'echo', a, s.pos, delay=3, scale=0.45, inst='echo')

    def build():
        arp = bass = V()
        for c, vol in zip('B B C# C#'.split(), (18, 22, 26, 30)):
            arp += s.fig(c, 'A:2 ' * 8, 16, 'A3', inst='arp', vol=vol)
            bass += s.fig(c, 'R:1 R O R ' * 4, 16, 'E2', inst='bass', vol=vol + 14)
        s.block(64, arp=arp, bass=bass,
                drum=s.drums('K...K...K...K...|K...K...K...K...|'
                             'K.K.K.K.K.K.K.K.|K.K.K.K.K.......|', kit),
                hat=s.drums('s...s...s...s...|s.s.s.s.s.s.s.s.|'
                            'SSSSSSSSSSSSSSSS|RRRRRRRRQQQQQQ..|', kit),
                fx=s.ramp('B3', 64, 4, 44, '101', 'whoosh'),
                lead=rest(56) + s.notes('C#6:2 D6 E6 E#6', inst='lead', vol=50))

    def ending():
        s.block(16, lead=s.notes('F#5:4 ~:12*A01', inst='lead', vol=56),
                harm=s.notes('C#5:4 ~:12*A01', inst='harm', vol=36),
                arp=s.notes('F#4:16*037', inst='arp', vol=26),
                bass=s.notes('F#2:16', inst='bass', vol=44),
                drum=s.drums('K...............', kit), fx=s.drums('X...............', kit))

    intro()                     # 0
    riff()                      # 1
    verse()                     # 2-3
    chorus()                    # 4-5
    solo()                      # 6-7
    chorus(harm=True)           # 8-9
    breakdown()                 # 10-11
    build()                     # 12
    chorus(2, harm=True)        # 13-14  F sharp minor
    riff(2)                     # 15
    ending()                    # 16
    return s


# ---- Cinder Warden (original, RockTrack contributors) ------------------
# C minor, 138 BPM: a final boss. A chromatic bass ostinato, power-chord
# stabs, tremolo strings, a funeral bell and tom rolls; the build speeds up
# and the second phase is a semitone higher.
CW_OST = 'C2:2 C2:1 C2 C3:2 C2 Db2 C2 B1 C2 |'
CW_OST2 = 'C2:2 C2:1 C2 C3:2 C2 Eb2 D2 Db2 D2 |'
CW_STAB = ('C4:3*07C C4:3*07C C4:2*07C r:4 Db4:2*07C C4:2*07C |'
           ' C4:3*07C C4:3*07C C4:2*07C r:4 Eb4:2*07C D4:2*07C |')
CW_A = ('C5:2 C5:1 C5 G5:4 F#5:2 G5 Eb5 D5 | C5:2 C5:1 C5 Ab5:4 G5:2 Ab5 B5:4 |'
        ' C6:6 B5:2 Ab5:4 Eb5 | D5:2 F5 Ab5 B5 D6:2 ~:6*446 |'
        ' C6:2 C6:1 C6 G5:4 F#5:2 G5 Eb5 D5 | C5:2 D5 Eb5 F5 G5:4 Ab5:2 G5 |'
        ' F5:6 Ab5:2 Db6:4 C6:2 Ab5 | B5:6 C6:2 B5:2 Ab5 G5:4 |')
CW_A_CH = 'Cm Cm Ab G Cm Cm Db G'
CW_A_BASS = ((CW_OST, 0), (CW_OST2, 0), (CW_OST, -4), (CW_OST, -5),
             (CW_OST, 0), (CW_OST2, 0), (CW_OST, 1), (CW_OST, -5))
CW_A_STR = ('Eb4:32*7A6 Eb4:16*7A6 D4:16*7A6 |'
            ' Eb4:32*7A6 F4:16*7A6 D4:16*7A6 |')
CW_B = ('Eb6:2 ~:4*446 C6:2 Ab5:4 C6:4 | D6:2 ~:4*446 Bb5:2 F5:4 Bb5:4 |'
        ' G6:2 ~:6*446 F6:2 Eb6 D6 Bb5 | C6:2 ~:10*446 r:4 |'
        ' Eb6:2 ~:4*446 C6:2 Ab5:4 Eb6:4 | F6:2 ~:4*446 D6:2 Bb5:4 F6:4 |'
        ' Ab6:2 ~:4*446 G6:2 F6:4 D6:4 | B5:2 ~:14*448 |')
CW_B_CH = 'Ab Bb Eb Cm Ab Bb G G'
CW_B_STR = ('C5:16*7A6 D5:16*7A6 Eb5:16*7A6 Eb5:16*7A6 |'
            ' C5:16*7A6 D5:16*7A6 D5:32*7A6 |')
CW_BRK_CH = 'Cm G Gm F Fm Fm G G'
CW_BRK_BASS = 'C2:16 B1 Bb1 A1 Ab1:32 G1:32 |'
CW_BRK_STR = ('Eb5:16*7A6 D5:16*7A6 D5:16*7A6 C5:16*7A6 |'
              ' C5:32*7A6 B4:32*7A6 |')
CW_BRK_LEAD = ('r:64 | C6:4 ~:12*448 Ab5:4 ~:12*448 |'
               ' B5:4 ~:12*448 D6:4 ~:12*448 |')
CW_BUILD_STR = ('G#4:8*7A6 A4:8*7A6 A#4:8*7A6 B4:8*7A6'
                ' C5:8*7A6 C#5:8*7A6 D5:8*7A6 D#5:8*7A6 |')
CW_END = ('C5:2 C5:1 C5 G5:4 F#5:2 G5 Eb5 D5 | C5:2 C5:1 C5 Ab5:4 G5:2 Ab5 B5:4 |'
          ' C6:2 B5 Bb5 A5 Ab5 G5 F#5 G5 | C6:4 ~:12*A01 |')
CW_END_BASS = CW_OST + CW_OST2 + ' C3:2 B2 Bb2 A2 Ab2 G2 F#2 G1 | C2:16 |'


def cinder_warden():
    s = Song('cinder_warden.xm', 'Cinder Warden', 'lead echo str arp bass drum perc fx',
             speed=6, bpm=138, rows=64, bar=16)
    s.inst('lead', 'Lead', 'rt1:pulse,5', (0, 5, 12, 5))
    s.inst('echo', 'Echo', 'rt1:pulse,5', (0, 5, 12, 5), pan=200)
    s.inst('harm', 'Harmony', 'rt1:pulse,3', (0, 5, 11, 5), pan=72)
    s.inst('str', 'Strings', 'rt1:saw', (3, 6, 12, 8), pan=88)
    s.inst('arp', 'Arp', 'rt1:pulse,2', (0, 3, 5, 2), pan=168)
    s.inst('bell', 'Bell', 'rt1:sine', (0, 13, 0, 12), pan=150)
    s.inst('bass', 'Bass', 'rt1:saw', (0, 5, 10, 2))
    s.inst('siren', 'Siren', 'rt1:saw', (0, 0, 15, 4), pan=144)
    s.inst('kick', 'Kick', 'rt1:kick,10,4', (0, 0, 15, 0))
    s.inst('snare', 'Snare', 'rt1:snare,11,4', (0, 0, 15, 0), pan=140)
    s.inst('hat', 'Hat', 'rt1:hat,12,1', (0, 0, 15, 0), pan=168)
    s.inst('tom', 'Tom', 'rt1:tom,10,4', (0, 0, 15, 0))
    s.inst('anvil', 'Anvil', 'rt1:noise,1', (0, 5, 0, 3), pan=104)
    s.inst('crash', 'Crash', 'rt1:noise,0', (0, 12, 0, 8), pan=150)
    kit = s.kit(K=('kick', 60), k=('kick', 40), S=('snare', 50), s=('snare', 28),
                R=('snare', 40, 'C4', 'E93'), Q=('snare', 52, 'C4', 'E92'),
                h=('hat', 18), m=('anvil', 22, 'C5'), M=('anvil', 32, 'C5'),
                X=('crash', 40, 'C5'),
                u=('tom', 46, 'G4'), t=('tom', 48, 'D4'), y=('tom', 50, 'A3'),
                b=('tom', 56, 'E3'), T=('tom', 46, 'A3', 'E93'),
                v=('tom', 28, 'A3', 'E93'))
    gallop = 'KkK.S...KkK.S...|'
    clank = '..m...M...m...M.|'
    crash = lambda bars: s.drums(('X' + '.' * 63) * (bars // 4), kit)
    minor = lambda tr: scale(PC_NAME[tr % 12], True)

    def intro():
        s.block(64, fx=s.notes('C5:16 C5 C5 C5:8 B4 |', inst='bell', vol=44),
                str=s.notes('v18 C4:16*7A6 v22 Db4:16*7A6 v26 D4:16*7A6'
                            ' v30 Eb4:8*7A6 D4:8*7A6 |', inst='str'),
                bass=s.notes('C2:48 G1:16 |', inst='bass', vol=40),
                perc=s.drums('b...............|b...........vvvv|'
                             'b.......vvvvvvvv|vvvvvvvvTTTTTTTT|', kit),
                drum=s.drums('K..k............|' * 3 + 'K..k....K..k....|', kit))

    def riff(tr):
        s.block(64, lead=s.notes(CW_STAB * 2, tr=tr, inst='lead', vol=40),
                str=s.notes('G4:64*7A6', tr=tr, inst='str', vol=28),
                bass=s.notes((CW_OST + CW_OST2) * 2, tr=tr, inst='bass', vol=46),
                drum=s.drums(gallop * 3 + 'KkK.S...SSRRQQQQ|', kit),
                perc=s.drums(clank * 4, kit), fx=crash(4))

    def part_a(tr):
        a = s.pos
        bass = V()
        for text, t in CW_A_BASS:
            bass += s.notes(text, tr=tr + t, inst='bass', vol=46)
        s.block(128, lead=s.notes(CW_A, tr=tr, inst='lead', vol=50, art=1),
                str=s.notes(CW_A_STR, tr=tr, inst='str', vol=26),
                arp=s.fig(transpose_chart(CW_A_CH, tr), 'A:2 ' * 8, 16, 'C4',
                          inst='arp', vol=16),
                bass=bass,
                drum=s.drums(gallop * 7 + 'KkK.S...SSSSRRRR|', kit),
                perc=s.drums(clank * 7 + 'u.u.t.t.y.y.b.b.|', kit),
                fx=crash(4) + rest(64))
        s.echo('lead', 'echo', a, s.pos, delay=2, scale=0.35, inst='echo')

    def part_b(tr, harm=False):
        a = s.pos
        chart = transpose_chart(CW_B_CH, tr)
        lead = s.notes(CW_B, tr=tr, inst='lead', vol=48, art=1)
        v = dict(lead=lead, str=s.notes(CW_B_STR, tr=tr, inst='str', vol=26),
                 arp=s.fig(chart, 'A:4 ' * 4, 16, 'C4', inst='arp', vol=18),
                 bass=s.fig(chart, 'R:2 O ' * 4, 16, 'G1', inst='bass', vol=46),
                 drum=s.drums('K...S...K.K.S...|' * 7 + 'K...S...SSSSQQQQ|', kit),
                 perc=s.drums('h.h.h.h.h.h.h.h.|' * 7 + 'u.u.t.t.y.y.b.b.|', kit),
                 fx=crash(8))
        if harm:
            v['echo'] = s.harmony(lead, chart, 16, minor(tr), 'harm', 0.55)
        s.block(128, **v)
        if not harm:
            s.echo('lead', 'echo', a, s.pos, delay=2, scale=0.35, inst='echo')

    def breakdown():
        a = s.pos
        s.block(128, lead=s.notes(CW_BRK_LEAD, inst='lead', vol=44),
                str=s.notes(CW_BRK_STR, inst='str', vol=30),
                arp=s.fig(CW_BRK_CH, 'A:16', 16, 'C4', inst='arp', vol=14),
                bass=s.notes(CW_BRK_BASS, inst='bass', vol=40),
                drum=s.drums('K...............|' * 4 + 'K.......K.......|' * 3 +
                             'K...K...K...K...|', kit),
                perc=s.drums('b.......b.......|' * 4 + 'b..y..u.b..y..t.|' * 3 +
                             'vvvvvvvvTTTTTTTT|', kit),
                fx=s.notes('C5:32 C5:32 r:64', inst='bell', vol=40))
        s.echo('lead', 'echo', a, s.pos, delay=3, scale=0.45, inst='echo')

    def build():
        a = s.pos
        bass = V()
        for vol in (36, 40, 44, 48):
            bass += s.fig('G#', 'R:1 R O R ' * 4, 16, 'G1', inst='bass', vol=vol)
        s.block(64, lead=s.notes('G#5:16*442 ~:16*444 ~:16*446 ~:16*448', inst='lead',
                                 vol=40),
                str=s.notes(CW_BUILD_STR, inst='str', vol=30), bass=bass,
                drum=s.drums('s...s...s...s...|s.s.s.s.s.s.s.s.|'
                             'SSSSSSSSSSSSSSSS|RRRRRRRRQQQQQQQQ|', kit),
                perc=s.drums('................|................|'
                             'y.y.u.u.t.t.b.b.|vvvvvvvvTTTTTTTT|', kit),
                fx=s.ramp('G#3', 64, 4, 40, '102', 'siren'))
        for k, bpm in enumerate((140, 142, 144, 146)):
            s.tempo(a + 16 * k, bpm)

    def ending(tr):
        a = s.pos
        chart = transpose_chart('Cm Cm G Cm', tr)
        s.block(64, lead=s.notes(CW_END, tr=tr, inst='lead', vol=52),
                str=s.notes('G4:32*7A6 B4:16*7A6 C5:16 |', tr=tr, inst='str', vol=28),
                arp=s.fig(chart.rsplit(' ', 1)[0], 'A:2 ' * 8, 16, 'C4', inst='arp',
                          vol=16) +
                s.fig(chart.rsplit(' ', 1)[1], 'A:16', 16, 'C4', inst='arp', vol=20),
                bass=s.notes(CW_END_BASS, tr=tr, inst='bass', vol=48),
                drum=s.drums(gallop * 2 + 'K.K.K.K.K.K.K.K.|K...............|', kit),
                perc=s.drums(clank * 2 + 'y.y.u.u.t.t.b.b.|b...............|', kit),
                fx=s.drums('X' + '.' * 47 + 'X' + '.' * 15, kit))
        s.echo('lead', 'echo', a, s.pos, delay=2, scale=0.35, inst='echo')
        s.tempo(a + 48, 116)

    intro()                     # 0
    riff(0)                     # 1
    part_a(0)                   # 2-3
    part_b(0)                   # 4-5
    breakdown()                 # 6-7
    build()                     # 8      speeds up to 146 BPM
    riff(1)                     # 9      C sharp minor
    part_a(1)                   # 10-11
    part_b(1, harm=True)        # 12-13
    ending(1)                   # 14
    return s


# ---- Lemon Kite (original, RockTrack contributors) ---------------------
# G major with a swing: three rows to the beat, so a swung quaver is two
# rows and then one (a bar is twelve rows). A square and a flute trade
# phrases, the chorus goes in thirds, a cloudy breakdown, a triplet snare
# build and the last chorus in A major with a ping-pong echo.
LK_CALL = ('D5:2 E5:1 G5:2 B5:4 A5:2 G5:1 | A5:2 F#5:1 D#5:3 F#5:2 B5:4 | r:24 |'
           ' D5:2 E5:1 G5:2 B5:4 D6:2 B5:1 | G#5:2 E5:1 D5:3 E5:2 G#5:4 | r:24 |')
LK_RESP = ('r:24 | C6:2 B5:1 A5:2 G5:4 E5:2 G5:1 | A5:3 G5:2 E5:1 C#5:6 |'
           ' r:24 | C6:2 A5:1 E5:3 F#5:2 A5:1 C6:3 | B5:3 A5:2 G5:4 r:3 |')
LK_VERSE_CH = 'G G B7 B7 C C A7 A7 G G E7 E7 Am D7 G G'      # half bars
LK_CHORUS = ('E5:2 E5:1 G5:2 E5:1 C6:3 B5:2 A5:1 | B5:3 G5:2 D5:4 B4:3 |'
             ' C#5:2 C#5:1 E5:2 C#5:1 A5:3 G5:2 E5:1 | F#5:3 A5:2 D6:4 C6:3 |'
             ' E5:2 E5:1 G5:2 E5:1 C6:3 B5:2 A5:1 | B5:3 D6:2 B5:4 G5:3 |'
             ' A5:2 C6:1 E6:3 D6:2 C6:1 A5:3 | G5:3 D5:2 E5:1 G5:6 |')
LK_CHORUS_CH = 'C Am G G A7 A7 D7 D7 C Am G G Am D7 G G'
LK_BREAK = ('B5:6 ~:6*446 | G5:6 E5:6 | D5:3 G5:3 B5:6+306 | A5:6 ~:6*446 |'
            ' B5:6 C6:3 B5:3 | G5:6 ~:6*446 | E5:3 G5:3 C6:6+306 | D6:6 ~:6*446 |')
LK_BREAK_CH = 'Em C G D Em C Am D'                              # whole bars
LK_TAG = ('A5:2 C6:1 E6:3 D6:2 C6:1 A5:3 | G5:3 D5:2 E5:1 G5:6 |'
          ' A5:2 C6:1 E6:3 D6:2 C6:1 A5:3 | G5:2 r:3 G5:7 |')
LK_TAG_CH = 'Am D7 G G Am D7 G G'
LK_BEAT = 'K..S..K.kS..|'
LK_FILL = 'K..S..uuttyy|'


def lemon_kite():
    s = Song('lemon_kite.xm', 'Lemon Kite', 'call resp echo stab bass drum hat glock',
             speed=6, bpm=100, rows=48, bar=12, timesig=(4, 4))
    s.inst('square', 'Square', 'rt1:pulse,4', (0, 5, 11, 4), pan=88)
    s.inst('flute', 'Flute', 'rt1:tri', (1, 6, 12, 5), pan=168, vib=(0, 16, 3, 12))
    s.inst('echo', 'Echo', 'rt1:pulse,4', (0, 5, 11, 4), pan=200)
    s.inst('stab', 'Stab', 'rt1:pulse,6', (0, 5, 0, 3), pan=112)
    s.inst('bass', 'Bass', 'rt1:tri', (0, 4, 12, 2))
    s.inst('glock', 'Glock', 'rt1:sine', (0, 9, 0, 7), pan=152)
    s.inst('kick', 'Kick', 'rt1:kick,13,3', (0, 0, 15, 0))
    s.inst('snare', 'Snare', 'rt1:snare,14,2', (0, 0, 15, 0), pan=136)
    s.inst('hat', 'Hat', 'rt1:hat,13,1', (0, 0, 15, 0), pan=176)
    s.inst('tom', 'Tom', 'rt1:tom,13,2', (0, 0, 15, 0))
    s.inst('splash', 'Splash', 'rt1:noise,0', (0, 10, 0, 7), pan=160)
    kit = s.kit(K=('kick', 58), k=('kick', 40), S=('snare', 46), s=('snare', 28),
                c=('snare', 26, 'C5'), R=('snare', 38, 'C4', 'E93'),
                Q=('snare', 48, 'C4', 'E92'), h=('hat', 16), H=('hat', 28),
                X=('splash', 38, 'C5'),
                u=('tom', 42, 'G4'), t=('tom', 44, 'D4'), y=('tom', 46, 'A3'))
    hats = 'H.hH.hH.hH.h|'
    major = lambda tr: scale(PC_NAME[(7 + tr) % 12])

    def intro():
        chart = 'C C D7 D7 G G D7 D7'
        s.block(48, stab=s.fig(chart, 'r:3 A:3', 6, 'G3', inst='stab', vol=22),
                bass=s.fig(chart, 'R:3 F:3', 6, 'E2', inst='bass', vol=42),
                glock=s.notes('E5:2 G5:1 C6:3 E6:6 | F#5:2 A5:1 C6:3 F#6:6 | r:24 |',
                              inst='glock', vol=22),
                hat=s.drums(hats * 4, kit),
                drum=s.drums('............|............|K.....K.....|' + LK_FILL, kit),
                resp=s.notes('r:36 D5:3 D6:9+304', inst='flute', vol=44))

    def verse(call, resp, glock=False):
        v = dict(call=s.notes(LK_CALL, inst=call, vol=48, art=1),
                 resp=s.notes(LK_RESP, inst=resp, vol=48, art=1),
                 stab=s.fig(LK_VERSE_CH, 'r:3 A:3', 6, 'G3', inst='stab', vol=20),
                 bass=s.fig(LK_VERSE_CH, 'R:3 F:3', 6, 'E2', inst='bass', vol=42),
                 drum=s.drums(LK_BEAT * 7 + LK_FILL, kit),
                 hat=s.drums('X' + hats[1:] + hats * 7, kit))
        if glock:
            v['glock'] = s.notes(LK_RESP, tr=12, inst='glock', vol=16)
        s.block(96, **v)

    def chorus(tr=0, echo=False, glock=False):
        a = s.pos
        chart = transpose_chart(LK_CHORUS_CH, tr)
        lead = s.notes(LK_CHORUS, tr=tr, inst='square', vol=50, art=1)
        v = dict(call=lead,
                 resp=s.harmony(lead, chart, 6, major(tr), 'flute', 0.8),
                 stab=s.fig(chart, 'r:2 A:1 r:2 A:1', 6, 'G3', inst='stab', vol=20),
                 bass=s.fig(chart, 'R:2 R:1 F:3', 6, 'E2', inst='bass', vol=44),
                 drum=s.drums(LK_BEAT * 7 + LK_FILL, kit),
                 hat=s.drums(('XhhHhhHhhHhh|' + 'HhhHhhHhhHhh|' * 7) if glock else
                             'X' + hats[1:] + hats * 7, kit))
        if glock:
            v['glock'] = s.fig(chart, 'O:1 F T O F T', 6, 'G4', inst='glock', vol=14)
        s.block(96, **v)
        if echo:
            s.echo('call', 'echo', a, s.pos, delay=3, scale=0.35, inst='echo')
            s.pan('echo', a, s.pos, (40, 216))

    def breakdown():
        a = s.pos
        s.block(96, resp=s.notes(LK_BREAK, inst='flute', vol=46, art=1),
                glock=s.fig(LK_BREAK_CH, 'R:1 T F O F T ' * 2, 12, 'E4',
                            inst='glock', vol=18),
                bass=s.fig(LK_BREAK_CH, 'R:6 F:6', 12, 'E2', inst='bass', vol=36),
                stab=rest(48) + s.fig(LK_BREAK_CH.split(None, 4)[4], 'r:3 A:3 ' * 2,
                                      12, 'G3', inst='stab', vol=16),
                drum=s.drums('k..c.....c..|' * 4 + 'K..c..K..c..|' * 3 + LK_FILL, kit),
                hat=s.drums('............|' * 4 + 'h.hh.hh.hh.h|' * 4, kit))
        s.echo('resp', 'echo', a, s.pos, delay=3, scale=0.4, inst='flute')
        s.pan('echo', a, s.pos, (48, 208))

    def build():
        stab = bass = V()
        for vol in (16, 22, 28, 34):
            stab += s.fig('E7', 'A:3 ' * 4, 12, 'G3', inst='stab', vol=vol)
            bass += s.fig('E7', 'R:1 R R O R R ' * 2, 12, 'E2', inst='bass', vol=vol + 18)
        s.block(48, stab=stab, bass=bass,
                drum=s.drums('K..K..K..K..|' * 3 + 'K..K..K.....|', kit),
                hat=s.drums('s..s..s..s..|s.ss.ss.ss.s|ssssssssssss|RRRRRRQQQ...|', kit),
                resp=s.ramp('B4', 45, 10, 46, '101', 'flute') + rest(3),
                call=rest(45) + s.notes('C#5:1 D5 E5', inst='square', vol=50))

    def tag(tr):
        a = s.pos
        chart = transpose_chart(LK_TAG_CH, tr)
        lead = s.notes(LK_TAG, tr=tr, inst='square', vol=52, art=1)
        s.block(48, call=lead,
                resp=s.harmony(lead, chart, 6, major(tr), 'flute', 0.8),
                stab=s.fig(chart.rsplit(' ', 2)[0], 'r:2 A:1 r:2 A:1', 6, 'G3',
                           inst='stab', vol=22) +
                s.notes('A3:2*047 r:3 A3:7*047', inst='stab', vol=30),
                bass=s.fig(chart.rsplit(' ', 2)[0], 'R:2 R:1 F:3', 6, 'E2',
                           inst='bass', vol=44) +
                s.notes('A2:2 r:3 A2:7', inst='bass', vol=48),
                drum=s.drums('K..S..K.kS..|K..S..uuttyy|K..S..K.kS..|K....K......|', kit),
                hat=s.drums('HhhHhhHhhHhh|' * 3 + 'H....X......|', kit),
                glock=rest(41) + s.notes('A6:7', inst='glock', vol=24))
        s.echo('call', 'echo', a, s.pos, delay=3, scale=0.35, inst='echo')
        s.pan('echo', a, s.pos, (40, 216))

    intro()                             # 0
    verse('square', 'flute')            # 1-2
    chorus()                            # 3-4
    verse('flute', 'square', glock=True)    # 5-6
    chorus(echo=True)                   # 7-8
    breakdown()                         # 9-10
    build()                             # 11
    chorus(2, echo=True, glock=True)    # 12-13  A major
    tag(2)                              # 14
    return s


SONGS = [ode_to_joy, fur_elise, canon, mountain_king, gymnopedie,
         bach_prelude, greensleeves, sparkstone, quartz_comet,
         cinder_warden, lemon_kite]


# ================================================================ CHECK ==

LIMITS = dict(channels=(4, 8), patterns=24, pattern_bytes=60000,
              sample_bytes=32 * 1024, file_bytes=48 * 1024,
              rows=(32, 48, 64, 96))
FX_OK = {0, 1, 2, 3, 4, 7, 8, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF}


def read_xm(data):
    """Parse an XM file into a dict (patterns unpacked)."""
    u16 = lambda o: struct.unpack_from('<H', data, o)[0]
    u32 = lambda o: struct.unpack_from('<I', data, o)[0]
    x = dict(id=data[:17], title=data[17:37].rstrip(b'\0 '), eof=data[37],
             tracker=data[38:58], version=u16(58), hsize=u32(60))
    (x['songlen'], x['restart'], x['nch'], x['npat'], x['nins'], x['flags'],
     x['speed'], x['bpm']) = struct.unpack_from('<8H', data, 64)
    x['order'] = list(data[80:80 + x['songlen']])
    pos = 60 + x['hsize']
    x['patterns'] = []
    for _ in range(x['npat']):
        plen, ptype, rows, psize = struct.unpack_from('<IBHH', data, pos)
        pos += plen
        buf, i, cells = data[pos:pos + psize], 0, []
        for _ in range(rows * x['nch']):
            b = buf[i]
            i += 1
            if b & 0x80:
                c = [0] * 5
                for k in range(5):
                    if b & (1 << k):
                        c[k] = buf[i]
                        i += 1
            else:
                c = [b] + list(buf[i:i + 4])
                i += 4
            cells.append(c)
        if i != psize:
            raise ValueError('pattern data size mismatch')
        x['patterns'].append(dict(rows=rows, type=ptype, cells=cells))
        pos += psize
    x['insts'] = []
    for _ in range(x['nins']):
        isize = u32(pos)
        ins = dict(size=isize, name=data[pos + 4:pos + 26].rstrip(b'\0 '),
                   nsmp=u16(pos + 27))
        if ins['nsmp']:
            o = pos + 129
            ins['env'] = [struct.unpack_from('<HH', data, o + 4 * k)
                          for k in range(data[pos + 225])]
            ins['npan'] = data[pos + 226]
            ins['sus'] = data[pos + 227]
            ins['voltype'], ins['pantype'] = data[pos + 233], data[pos + 234]
            ins['vib'] = tuple(data[pos + 235:pos + 239])
            ins['fadeout'] = u16(pos + 239)
            shsize = u32(pos + 29)
        pos += isize
        smps = []
        for _ in range(ins['nsmp']):
            (ln, ls, ll, vol, fine, typ, pan, rel, _r) = struct.unpack_from(
                '<IIIBbBBbB', data, pos)
            smps.append(dict(len=ln, lstart=ls, llen=ll, vol=vol, fine=fine,
                             type=typ, pan=pan, rel=rel,
                             name=data[pos + 18:pos + 40].rstrip(b'\0 ')))
            pos += shsize
        for sm in smps:
            raw, prev, out = data[pos:pos + sm['len']], 0, []
            for b in raw:
                prev = (prev + b) & 0xFF
                out.append(prev - 256 if prev > 127 else prev)
            sm['data'] = out
            pos += sm['len']
        ins['samples'] = smps
        x['insts'].append(ins)
    x['size'], x['trailing'] = len(data), len(data) - pos
    return x


_SHAPES = set()


def adsr_shapes():
    """Every envelope adsr_envelope() can produce."""
    if not _SHAPES:
        for a in range(16):
            for d in range(16):
                for s in range(16):
                    for r in range(16):
                        pts, sus = adsr_envelope(a, d, s, r)
                        _SHAPES.add((tuple(pts), sus))
    return _SHAPES


def check_xm(path):
    """Problems found in an XM file (empty list = fine) and a summary."""
    data = open(path, 'rb').read()
    x = read_xm(data)
    errs = []

    def need(ok, msg):
        if not ok:
            errs.append(msg)
    need(x['id'] == b'Extended Module: ' and x['eof'] == 0x1A, 'XM id')
    need(x['tracker'] == b'RockTrack'.ljust(20), 'tracker name')
    need(x['version'] == 0x104 and x['hsize'] == 276, 'version/header size')
    need(x['flags'] & 1, 'linear frequency flag')
    need(x['title'] and all(32 <= c < 127 for c in x['title']), 'song name')
    need(LIMITS['channels'][0] <= x['nch'] <= LIMITS['channels'][1]
         and x['nch'] % 2 == 0, 'channel count %d' % x['nch'])
    need(x['npat'] <= LIMITS['patterns'], 'pattern count %d' % x['npat'])
    need(3 <= x['speed'] <= 6, 'speed')
    need(0 < x['songlen'] and x['restart'] < x['songlen'], 'restart position')
    need(all(o < x['npat'] for o in x['order']), 'order table')
    need(x['trailing'] == 0, 'trailing bytes')
    pat_bytes = sum(p['rows'] * x['nch'] * 5 for p in x['patterns'])
    need(pat_bytes <= LIMITS['pattern_bytes'], 'pattern bytes %d' % pat_bytes)
    for n, p in enumerate(x['patterns']):
        need(p['rows'] in LIMITS['rows'] and p['type'] == 0, 'pattern %d rows' % n)
        for c in p['cells']:
            note, ins, vol, fx, par = c
            need(note <= 96 or note == KEYOFF, 'note %d' % note)
            need(ins <= x['nins'], 'instrument %d' % ins)
            need(vol == 0 or 0x10 <= vol <= 0x50 or 0xC0 <= vol <= 0xCF,
                 'volume column %02X' % vol)
            need(fx in FX_OK and (fx != 0xE or par >> 4 in (9, 0xC, 0xD))
                 and (fx != 0xF or par), 'effect %X%02X' % (fx, par))
    smp_bytes = 0
    for n, ins in enumerate(x['insts']):
        need(ins['size'] == 263 and ins['nsmp'] == 1, 'instrument %d header' % n)
        need(ins['name'] and all(32 <= c < 127 for c in ins['name']),
             'instrument %d name' % n)
        if ins['nsmp'] != 1:
            continue
        need(ins['voltype'] == 3 and ins['pantype'] == 0 and ins['npan'] == 0
             and ins['fadeout'] == 0, 'instrument %d envelope flags' % n)
        need((tuple(ins['env']), ins['sus']) in adsr_shapes(),
             'instrument %d envelope is not ADSR' % n)
        sm = ins['samples'][0]
        smp_bytes += sm['len']
        tag = sm['name'].decode('ascii', 'replace')
        try:
            want, loop = synth(tag)
        except ValueError:
            errs.append('instrument %d sample name %r' % (n, tag))
            continue
        need(sm['data'] == want, 'instrument %d sample differs from %s' % (n, tag))
        need(sm['rel'] == 12 and sm['fine'] == 0 and sm['vol'] == 64,
             'instrument %d tuning/volume' % n)
        if loop:
            need(sm['type'] == 1 and sm['lstart'] == 0 and sm['llen'] == loop,
                 'instrument %d loop' % n)
        else:
            need(sm['type'] == 0, 'instrument %d one-shot' % n)
    need(smp_bytes <= LIMITS['sample_bytes'], 'sample bytes %d' % smp_bytes)
    need(x['size'] < LIMITS['file_bytes'], 'file size %d' % x['size'])
    summary = ('%-18s %-20s ch %d  pat %2d  ord %2d  ins %2d  patbytes %5d'
               '  smp %5d  file %5d' % (os.path.basename(path),
                                         x['title'].decode(), x['nch'], x['npat'],
                                         x['songlen'], x['nins'], pat_bytes,
                                         smp_bytes, x['size']))
    return errs, summary


# ================================================================= MAIN ==

def main(argv):
    if argv and argv[0] == '--check':
        d = argv[1] if len(argv) > 1 else HERE
        ours = {make().fname for make in SONGS}
        bad = 0
        for f in sorted(os.listdir(d)):
            if f in ours:
                errs, summary = check_xm(os.path.join(d, f))
                print(summary)
                for e in errs:
                    print('   ERROR', e)
                bad += bool(errs)
        return 1 if bad else 0
    out = argv[0] if argv else HERE
    os.makedirs(out, exist_ok=True)
    for make in SONGS:
        s = make().finish()
        data = s.xm()
        with open(os.path.join(out, s.fname), 'wb') as f:
            f.write(data)
        print('%-18s %d ch, %2d patterns, %2d positions, %5.1f s, %5d bytes'
              % (s.fname, len(s.chans), len(s.patterns), len(s.order),
                 s.duration(), len(data)))
        if s.fname == 'greensleeves.xm':
            with open(os.path.join(out, 'greensleeves.mid'), 'wb') as f:
                f.write(song_midi(s, GS_MIDI, rows_per_quarter=4))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
