# RockTrack design

RockTrack is a pocket tracker for Rockbox: a song editor and chip synth that
fits a 5-button player with a 128x160 screen, and scales up to bigger
screens and richer keypads. Songs are FastTracker 2 `.xm` files.

## Name

Candidates: **RockTrack** (Rockbox + tracker; says what it is, sits with
rockblox/rockpaint/rocklife, and "track" is also a song), RockDAW (promises
audio tracks, recording and mixing it does not have), Rocktave
(rock + octave: cute, says less), Strata (channels as rock layers: pretty,
says nothing about music), Bedrock, Quarry. Picked: **RockTrack**.

## Goals

1. Make a song in minutes without reading a manual: sound comes out of
   the first key press, and the screen always says what the keys do.
2. Information-dense: the editing screen shows as much of the song as fits
   in legible pixels, and highlights what matters (beats, bars, playing
   row, instruments by colour).
3. Portable files: what you make opens in MilkyTracker, OpenMPT, libxmp,
   VLC, Rockbox's own mikmod plugin; what others make (XM, MOD, MIDI) opens
   here.
4. Any screen, any keypad: no coordinate or key is hard-coded to the YP3;
   the minimum is four directions and one select button.
5. Light: runs in the YP3's 127 KB plugin arena plus the audio buffer
   (~120-140 KB), integer-only audio in the mixer callback.

## File format: XM

| Format | Why not / why |
|---|---|
| MIDI | no sounds: playback depends on the synth; a performance, not a tracker document. Imported (and exported) as a bridge to DAWs and to the huge public-domain MIDI corpus |
| MOD | 4 channels (xCHN variants rare), no envelopes, Amiga periods. Imported |
| Own format | nobody else can open it |
| **XM** | up to 32 channels, instruments with volume/panning envelopes, auto-vibrato, fadeout, volume column, 1-256 rows per pattern, 8/16-bit samples, linear frequencies. Supported everywhere for 25+ years. A chip instrument is a 64-byte looped sample, so songs are a few KB |

RockTrack writes XM 1.04 with the linear frequency table. It loads any XM
and keeps what it does not edit (multi-sample instruments, panning
envelopes, extra samples) through a load/save round trip.

**Synth parameters ride in the sample name.** A synth instrument's sample
is named `rt1:<source>[,a[,b]]` (22 chars max), e.g. `rt1:pulse,4`,
`rt1:kick,12,6`. Another tracker sees a normal sample with a funny name;
RockTrack shows the synth controls again. The sample data is always the
authority: loading never regenerates; only changing a synth control does.

## Input: the three rules (5-key baseline)

The YP3 has Up, Down, Left, Right and Select (●). Everything works with
those five; bigger keypads add shortcuts (see Keymaps).

1. **Arrows move.**
2. **Hold ● and press arrows to change what is under the cursor.**
   ↑↓ big steps, ←→ small steps (note: octave / semitone; number: 16 / 1;
   hex pair: high / low digit). You hear the note as it changes.
3. **Tap ● for the obvious thing; double-tap ● to play/stop; hold ● alone
   for the menu.**

These match the YP3's own keymap (Select acts, hold Select is the screen's
menu, Left is back in lists) and the LSDJ/M8 "edit key + direction"
grammar handheld musicians already know.

Gesture timing (all in input.c, one place):
- tap: ● released < 300 ms after press, no arrow while held.
- double tap: second tap starts < 250 ms after the first ends. The first
  tap's edit is undone, so a double tap never changes the song.
- hold: ● released after >= 300 ms with no arrow: opens the menu grid.
  If an arrow arrives first, it is an edit and the menu never opens.
- After 300 ms of ● held still, a **chord card** fades in next to the
  cursor: a small cross showing what each arrow will do here and
  "release: menu". The UI teaches itself at the moment of hesitation.
- Arrow repeat accelerates (cursor and value edits).

**Pens.** Each column type (note, instrument, volume, effect) has a pen:
the last value placed or removed there. Tapping an empty field stamps the
pen; tapping a filled field lifts its value into the pen and clears it.
So a stray tap is undone by tapping again, and "copy this note over
there" is tap, move, tap. ●+arrows on an empty field starts from the pen.
Entering a note also writes the instrument pen.

**Note off** sits below C-0: hold ● and keep pressing ↓ (octaves) or ←.
It is also a tile in the menu grid's pattern row.

## Screens

```
 Pattern            Song                Instrument          Menu grid (hold ●)
┌────────────────┐ ┌────────────────┐ ┌────────────────┐ ┌────────────────┐
│▶03:05 r12 125  │ │Song "Canon"    │ │◂ 03 Lead     ▸ │ │Song Patt Inst Set│
│ 1▮ 2▮ 3  4▮ 5  │ │BPM 125 Spd 6   │ │ ╭─╮ ╭─╮        │ │Play Loop From Stp│
│0C C-4 02 40 037│ │00 00 ▔▁▁▔ ▁▔▁▔ │ │ │ ╰─╯ ╰─  wave │ │Undo Redo Copy Pst│
│0D ··· ·· ·· ···│ │01 01 ▁▔▁▁ ▔▁▔▔ │ │Wave  Pulse 25% │ │Mark Tsp+ Tsp- ...│
│0E E-4 ·· ·· ···│ │02 01 ...       │ │Atk 02 Dec 08   │ │Save Open Expt Qut│
│... fisheye ... │ │                │ │Sus 20 Rel 10   │ │                  │
│● toggle ●↕ oct │ │● open ●↔ pat   │ │● play C-4      │ │Play pattern loop │
└────────────────┘ └────────────────┘ └────────────────┘ └────────────────┘
```

### Pattern (the composer)

- **Continuous timeline.** The view scrolls through the whole song: rows
  of the previous and next order positions show dimmed past a boundary
  line labelled with position and pattern number, and the cursor moves
  across. Holding ↓ scrolls the song like a piano roll.
- **Centred cursor row** (FT2 style); the playing row is a tinted band.
- **Fisheye channels.** The cursor's channel opens to all its fields
  (note, instrument, volume, effect type, effect value); other channels
  get whatever detail fits, at least the note. Layout is computed from the
  screen width and font, so 128 px shows 6 channels (one open), 320 px
  shows 8 fully open.
- ←→ move field by field; holding ←/→ (repeat) jumps channel to channel.
- Colours: notes in their instrument's colour; beat rows (every 4) and bar
  rows (every 16) lightly tinted; empty fields as dim dots; note off as
  `OFF`; effects muted violet; volume teal; cursor field inverted amber.
- **Header:** transport, position:pattern, row, BPM, current instrument
  pen (name, colour). Channel strip: number, live level meter, mute/solo.
- **Hint line:** what ● does here plus the value in words:
  `037 Arpeggio +3 +7`, `F7D Tempo 125 BPM`, `C-4 Lead (02)`.
- Follow: while playing, the view follows the playing row unless an arrow
  was pressed in the last 2 s; then it stays on the cursor.
- **Gutter** (row numbers): Left from channel 1. There ●+↑↓ = previous/
  next order position, tap ● = play from this row.

### Song (arrangement)

- One line per order position: position, pattern number, then one lane
  per channel with a sparkline of that pattern's notes (pitch over time,
  instrument colour): the song's shape at a glance.
- ●+←→ / ●+↑↓ change the pattern number (1 / 16); tap ● opens it in the
  Pattern screen. Menu row: insert, delete, clone (copy into a new
  pattern), new empty pattern.
- Header fields (cursor moves up into them): title, BPM, speed, channel
  count, pattern length for new patterns, restart position.

### Instrument

- Title `◂ 03 Lead ▸`: ●+←→ previous/next instrument; tap ● renames
  (keyboard).
- Waveform picture (one cycle, or the whole one-shot) and the envelope
  curve, both redrawn as values change; the last auditioned note.
- Controls (cursor ↑↓, ●+arrows change, tap ● auditions C-4 or the
  pattern's last note):
  - **Source:** Pulse (duty 1-15/16), Triangle, Saw, Sine, Noise
    (white/metal), Wave (draw 32 steps with ●+↑↓ per step), Kick, Snare,
    Hat, Tom (tune, length), Sample (loaded WAV or from a song file).
  - **Envelope:** Attack, Decay, Sustain, Release (0-15 each) written as
    an XM volume envelope; a loaded envelope that is not ADSR-shaped shows
    as "custom" and is kept until changed.
  - Volume, Pan, Transpose (relative note), Finetune.
  - Vibrato: depth, speed, delay (XM auto-vibrato).
- Double-tap ● plays the pattern, so a sound is tuned in context; changes
  apply live.

### Menu grid (hold ●)

A 4-column grid of tiles; arrows move (wrapping), tap ● runs, hold ● or
tap the current screen's tile closes. Spatially stable, so hands learn it:

| | 1 | 2 | 3 | 4 |
|---|---|---|---|---|
| Screens | Song | Pattern | Instr | Setup |
| Play | Song | Loop pattern | From cursor | Stop |
| Edit | Undo | Redo | Copy | Paste |
| Context (per screen) | e.g. Mark, Transpose +, Transpose -, More... | | | |
| File | Save | Open | Export | Quit |

The line under the grid explains the focused tile in words. "More..."
lists the rest (interpolate, insert/delete row, note off, clear pattern,
pattern length, double/halve, channels, mute/solo...).

**Mark** starts a block at the cursor; arrows extend it; tap ● opens the
block actions (copy, cut, clear, transpose ±1/±12, interpolate, set
instrument).

### Start menu and setup

Native Rockbox menus (consistent with every other plugin):
Resume (the autosave, if any), New song (template), Open, Demo songs,
Setup, Help, About, Quit. Setup: text size (auto/small/large), follow
playback, edit step, interpolation (crisp/smooth), row highlight, hex or
decimal row numbers, key repeat speed, default song folder.

## Sound engine

- XM playback engine written for the editor (engine.c), integer only, run
  from the mixer's get-more callback (interrupt context; no FPU, no
  allocation). Linear and Amiga frequency tables (Amiga for MOD).
- Effects: 0-F (arpeggio, slides, tone porta, vibrato, tremolo, panning,
  sample offset, volume slide, position jump, set volume, pattern break,
  speed/tempo), E1-EE (fine slides, glissando, vibrato control, finetune,
  pattern loop, retrigger, fine volume, note cut, note delay, pattern
  delay), G, H, K, L, P, R, T, X; volume column (set, slides, vibrato,
  panning, tone porta).
- Instruments: volume and panning envelopes (sustain, loop), fadeout,
  auto-vibrato, multi-sample keymaps; samples 8/16-bit, forward and
  ping-pong loops.
- Two preview voices outside the song channels for auditions, so editing
  never disturbs playback.
- Short volume ramps on note start/stop (no clicks); linear interpolation
  or none ("crisp", the authentic chip edge); stereo; master gain scaled
  by channel count; per-channel peak levels for the meters.
- The UI edits pattern cells in place (single-byte writes the callback can
  race harmlessly); structural changes (lengths, order list, sample
  buffers) take the PCM lock.

### Synth (synth.c): samples from controls

All tonal sources are 64-sample loops (`rt1` relative note +12, finetune
0: C-4 plays at 16726 Hz = 261.3 Hz), amplitude A = 100, signed 8-bit,
i = 0..63:

| Source | Sample |
|---|---|
| `pulse,d` (d 1-15) | i < 4d ? A : -A |
| `tri` | A - A·abs(2i - 64) / 32 |
| `saw` | 2A·i / 64 - A |
| `sine` | round(A·sin(2πi/64)) |
| `noise,m` | 4096-sample loop; 15-bit LFSR from 1, one step per sample, feedback bit0 ^ bit1 (m=0, white) or bit0 ^ bit6 (m=1, metal); bit0 set gives -A, else A |
| `wave` | 32 drawn steps (0-15), each two samples: (step - 7.5)·A/7.5; the steps are recovered from the data |
| `kick,t,b` `snare,t,b` `hat,t,b` `tom,t,b` | one-shot at 16726 Hz (no loop), tune t 0-24 (12 = normal, semitones), length b 0-15 |

Drums: kick = sine sweeping exponentially 180→45 Hz (times 2^((t-12)/12)),
length 1600+400b samples, linear decay; snare = 190 Hz sine (fast decay)
plus white noise (slower), 1200+300b; hat = white noise first difference
(bright), 300+150b, fast decay; tom = sine 220→110 Hz, 1600+300b.

ADSR to XM envelope (ticks; one envelope step per tick):
times[16] = 0,1,2,3,4,6,8,12,16,24,32,48,64,96,128,192; sustain level
s·64/15. Points: (0, attack ? 0 : 64), (ta, 64), (ta+td, S) = sustain
point, (ta+td+max(1,tr), 0); points with equal x merge. Fadeout 0.

## Memory

- Code + static data in the plugin arena (target < 80 KB on YP3).
- Song pool: the rest of the arena (`plugin_get_buffer`) plus the audio
  buffer (`plugin_get_audio_buffer`; music is already stopped on YP3).
  A small first-fit allocator with coalescing: patterns (rows x channels x
  5 bytes, unpacked for editing), samples, instrument headers.
- Budget on YP3 (~180 KB pool): 32 patterns of 64 rows x 8 channels take
  80 KB; chip samples are 64 B; drums 2-7 KB; a loaded WAV is cut to fit.
  The header shows free memory when it gets low; operations that would
  not fit say so instead of failing silently.
- Undo: ring of ~1000 entries (cell, order, instrument field), grouped per
  gesture.

## Screens of any size

- Glyph fonts are built in (fixed width, so the grid lines up): 3x5 in a
  4x6 cell, 5x7 in a 6x9 cell, each also drawn 2x. "Auto" picks the
  largest that shows >= 18 rows and >= 4 channels with the fisheye.
  Menus and help use the user's Rockbox font through the normal APIs.
- Every position comes from a layout pass over LCD_WIDTH/LCD_HEIGHT; no
  pixel constants outside layout.c.
- Colour: a palette of semantic colours (bg, beat, bar, text, dim, accent,
  play, 16 instrument hues). Greyscale and mono map them to the available
  shades, with inversion for the cursor.
- Keymaps (keymaps.h): per CONFIG_KEYPAD, the five core keys plus optional
  extras: Play (play/stop), Back/Menu (menu grid), Rec (toggle mark),
  on the YP3 its back chord Up+Left (to the start menu, as Quit),
  scroll wheel (value change while held or cursor rows). A target without
  four directions + select gets a mapping by hand (iPods: wheel = up/down,
  Menu/Play keys). Touch: later (tap cell = cursor, drag = value).
- Enabled in SUBDIRS for the YP3 first; other targets after a sim check
  (the code builds for them with `-DROCKTRACK_ANY_TARGET`).

## Files

- Songs: `/RockTrack/*.xm` (created on first save). Demos:
  `/.rockbox/rocks/apps/rocktrack/` (start menu: Demo songs).
- Nothing is ever written in the demo folder. A demo or an import (.mod,
  .mid) has no file of its own: its first Save asks for a name in
  `/RockTrack`. Saving or exporting over an existing file always asks
  "Replace the file?", except re-saving the song's own file.
- Autosave to `/.rockbox/rocktrack/autosave.xm` on quit and before
  risky operations; the start menu offers Resume.
- Open: `.xm`, `.mod` (M.K., xCHN, FLT4), `.mid` (SMF 0/1: notes quantised
  to rows, simultaneous notes spread over channels, General MIDI programs
  mapped to chip presets, channel 10 to the drum synth).
- Export: `.wav` (render; it then plays in the Rockbox music player),
  `.mid` (notes, instruments as programs, tempo).
- Viewer: `.xm`, `.mod`, `.mid` open with RockTrack from the file browser
  ("Open with...").
- **SoundFont (.sf2) instruments** (later): pick a preset from an .sf2 file;
  its zones become an XM multi-sample instrument (keymap, loop, root key,
  volume envelope approximated), streamed from the file one sample at a
  time and cut to fit memory. The song stays a normal XM. A whole GM
  soundfont as a live synth does not fit the YP3.

## Demo songs

Arranged for RockTrack (arrangements CC0), compositions in the public
domain, plus original chiptunes (CC0); generated by `demos/make_demos.py`
(needs only Python 3), checked by rendering with libopenmpt. Third-party
chiptune modules keep their own licences (`demos/THIRD_PARTY.txt`). Each
fits the YP3 pool with room to edit.

## Phases

1. Core: song model, XM load/save, engine and output, pattern and song
   screens with the full input grammar, menu grid, instrument screen with
   synth and ADSR, start menu, help/about, templates, autosave/resume,
   setup, demo songs.
2. Undo/redo, marks and block operations, MOD and MIDI import, WAV export.
3. MIDI export, WAV sample load, wave drawing, SoundFont instruments,
   other targets' keymaps.
