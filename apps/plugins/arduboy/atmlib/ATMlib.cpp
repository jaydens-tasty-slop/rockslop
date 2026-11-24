/* ATMlib (Arduboy Tracker Music) for the Rockbox Arduboy layer.
 *
 * The play routine (ATM_playroutine, song commands and effects) is upstream
 * ATMlib.cpp, (c) 2016 - 2018 TEAM a.r.g., Davey Taylor, Joeri Gantois,
 * BSD-3-Clause (LICENSE.ATMlib), with these changes: the four oscillators
 * that the AVR timer-4 ISR mixed at 31250 Hz (in assembly) are rendered by
 * ab_tunes_fill() at the mixer rate; play()/stop() only post a request that
 * the PCM callback carries out; the signed effect fields are int8_t (char is
 * unsigned on ARM); 16-bit song fields are read byte by byte (they are not
 * aligned); read_vle() advances the song pointer. */
#include "ATMlib.h"

#define AVR_RATE 31250      /* samples/s of the original synthesiser */

typedef struct {
  uint8_t  vol;
  uint16_t freq;
  uint16_t phase;
} osc_t;

static byte trackCount;
static byte tickRate;
static const byte *trackList;
static const byte *trackBase;
static uint16_t cia;

static byte ChannelActiveMute = 0b11110000;
//                                ||||||||
//                                |||||||└->  0  channel 0 is muted (0 = false / 1 = true)
//                                ||||||└-->  1  channel 1 is muted (0 = false / 1 = true)
//                                |||||└--->  2  channel 2 is muted (0 = false / 1 = true)
//                                ||||└---->  3  channel 3 is muted (0 = false / 1 = true)
//                                |||└----->  4  channel 0 is Active (0 = false / 1 = true)
//                                ||└------>  5  channel 1 is Active (0 = false / 1 = true)
//                                |└------->  6  channel 2 is Active (0 = false / 1 = true)
//                                └-------->  7  channel 3 is Active (0 = false / 1 = true)

static osc_t osc[4];

static const word noteTable[64] PROGMEM = {
  0,
  262,  277,  294,  311,  330,  349,  370,  392,  415,  440,  466,  494,
  523,  554,  587,  622,  659,  698,  740,  784,  831,  880,  932,  988,
  1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976,
  2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729, 3951,
  4186, 4435, 4699, 4978, 5274, 5588, 5920, 6272, 6645, 7040, 7459, 7902,
  8372, 8870, 9397,
};


struct ch_t {
  const byte *ptr;
  byte note;

  // Nesting
  word stackPointer[7];
  byte stackCounter[7];
  byte stackTrack[7]; // note 1
  byte stackIndex;
  byte repeatPoint;

  // Looping
  word delay;
  byte counter;
  byte track;

  // External FX
  word freq;
  byte vol;

  // Volume & Frequency slide FX
  int8_t volFreSlide;
  byte volFreConfig;
  byte volFreCount;

  // Arpeggio or Note Cut FX
  byte arpNotes;       // notes: base, base+[7:4], base+[7:4]+[3:0], if FF => note cut ON
  byte arpTiming;      // [7] = reserved, [6] = not third note ,[5] = retrigger, [4:0] = tick count
  byte arpCount;

  // Retrig FX
  byte reConfig;       // [7:2] = , [1:0] = speed // used for the noise channel
  byte reCount;        // also using this as a buffer for volume retrig on all channels

  // Transposition FX
  int8_t transConfig;

  // Tremolo or Vibrato FX
  byte treviDepth;
  byte treviConfig;
  byte treviCount;

  // Glissando FX
  int8_t glisConfig;
  byte glisCount;

};

static ch_t channel[4];

/* Requests from the game (main thread) to the PCM callback, which owns all
 * of the state above. */
static const byte *volatile reqSong;
static volatile bool reqStop, paused;
static bool playing;

static uint16_t read_vle(const byte **pp) {
  word q = 0;
  byte d;
  do {
    q <<= 7;
    d = pgm_read_byte((*pp)++);
    q |= (d & 0x7F);
  } while (d & 0x80);
  return q;
}

static inline const byte *getTrackPointer(byte track) {
  const byte *p = trackList + 2 * track;
  return trackBase + (pgm_read_byte(p) | (pgm_read_byte(p + 1) << 8));
}

static void load_song(const byte *song) {
  // cleanUp stuff first
  memset(channel, 0, sizeof(channel));
  memset(osc, 0, sizeof(osc));
  ChannelActiveMute = 0b11110000;

  // Sets tick rate
  tickRate = 25;
  cia = 15625 / tickRate;

  osc[3].freq = 0x0001; // Seed LFSR
  channel[3].freq = 0x0001; // xFX

  // Read track count
  trackCount = pgm_read_byte(song++);
  // Store track list pointer
  trackList = song;
  // Store track pointer
  trackBase = (song += (trackCount << 1)) + 4;
  // Fetch starting points for each track
  for (unsigned n = 0; n < 4; n++) {
    channel[n].ptr = getTrackPointer(pgm_read_byte(song++));
  }
  playing = true;
}

void ATMsynth::play(const byte *song) {
  reqStop = false;
  paused = false;
  reqSong = song;
  ab_audio_start();
}

// Stop playing, unload melody
void ATMsynth::stop() {
  reqSong = 0;
  reqStop = true;
}

// Start grinding samples or Pause playback
void ATMsynth::playPause() {
  paused = !paused;
  if (!paused)
    ab_audio_start();
}

// Toggle mute on/off on a channel, so it can be used for sound effects
// So you have to call it before and after the sound effect
void ATMsynth::muteChannel(byte ch) {
  ChannelActiveMute += (1 << 0 );
}

void ATMsynth::unMuteChannel(byte ch) {
  ChannelActiveMute &= (~(1 << 0 ));
}


static void ATM_playroutine() {
  ch_t *ch;

  // for every channel start working
  for (byte n = 0; n < 4; n++)
  {
    ch = &channel[n];

    // Noise retriggering
    if (ch->reConfig) {
      if (ch->reCount >= (ch->reConfig & 0x03)) {
        osc[n].freq = pgm_read_word(&noteTable[ch->reConfig >> 2]);
        ch->reCount = 0;
      }
      else ch->reCount++;
    }


    //Apply Glissando
    if (ch->glisConfig) {
      if (ch->glisCount >= (ch->glisConfig & 0x7F)) {
        if (ch->glisConfig & 0x80) ch->note -= 1;
        else ch->note += 1;
        if (ch->note < 1) ch->note = 1;
        else if (ch->note > 63) ch->note = 63;
        ch->freq = pgm_read_word(&noteTable[ch->note]);
        ch->glisCount = 0;
      }
      else ch->glisCount++;
    }


    // Apply volume/frequency slides
    if (ch->volFreSlide) {
      if (!ch->volFreCount) {
        int16_t vf = ((ch->volFreConfig & 0x40) ? ch->freq : ch->vol);
        vf += (ch->volFreSlide);
        if (!(ch->volFreConfig & 0x80)) {
          if (vf < 0) vf = 0;
          else if (ch->volFreConfig & 0x40) if (vf > 9397) vf = 9397;
            else if (!(ch->volFreConfig & 0x40)) if (vf > 63) vf = 63;
        }
        (ch->volFreConfig & 0x40) ? ch->freq = vf : ch->vol = vf;
      }
      if (ch->volFreCount++ >= (ch->volFreConfig & 0x3F)) ch->volFreCount = 0;
    }


    // Apply Arpeggio or Note Cut
    if (ch->arpNotes && ch->note) {
      if ((ch->arpCount & 0x1F) < (ch->arpTiming & 0x1F)) ch->arpCount++;
      else {
        if ((ch->arpCount & 0xE0) == 0x00) ch->arpCount = 0x20;
        else if ((ch->arpCount & 0xE0) == 0x20 && !(ch->arpTiming & 0x40) && (ch->arpNotes != 0xFF)) ch->arpCount = 0x40;
        else ch->arpCount = 0x00;
        byte arpNote = ch->note;
        if ((ch->arpCount & 0xE0) != 0x00) {
          if (ch->arpNotes == 0xFF) arpNote = 0;
          else arpNote += (ch->arpNotes >> 4);
        }
        if ((ch->arpCount & 0xE0) == 0x40) arpNote += (ch->arpNotes & 0x0F);
        ch->freq = pgm_read_word(&noteTable[(arpNote + ch->transConfig) & 63]);
      }
    }


    // Apply Tremolo or Vibrato
    if (ch->treviDepth) {
      int16_t vt = ((ch->treviConfig & 0x40) ? ch->freq : ch->vol);
      vt = (ch->treviCount & 0x80) ? (vt + ch->treviDepth) : (vt - ch->treviDepth);
      if (vt < 0) vt = 0;
      else if (ch->treviConfig & 0x40) if (vt > 9397) vt = 9397;
        else if (!(ch->treviConfig & 0x40)) if (vt > 63) vt = 63;
      (ch->treviConfig & 0x40) ? ch->freq = vt : ch->vol = vt;
      if ((ch->treviCount & 0x1F) < (ch->treviConfig & 0x1F)) ch->treviCount++;
      else {
        if (ch->treviCount & 0x80) ch->treviCount = 0;
        else ch->treviCount = 0x80;
      }
    }


    if (ch->delay) {
      if (ch->delay != 0xFFFF) ch->delay--;
    }
    else {
      do {
        byte cmd = pgm_read_byte(ch->ptr++);
        if (cmd < 64) {
          // 0 … 63 : NOTE ON/OFF
          if ((ch->note = cmd)) ch->note += ch->transConfig;
          ch->freq = pgm_read_word(&noteTable[ch->note & 63]);
          if (!ch->volFreConfig) ch->vol = ch->reCount;
          if (ch->arpTiming & 0x20) ch->arpCount = 0; // ARP retriggering
        }
        else if (cmd < 160) {
          // 64 … 159 : SETUP FX
          switch (cmd - 64) {
            case 0: // Set volume
              ch->vol = pgm_read_byte(ch->ptr++);
              ch->reCount = ch->vol;
              break;
            case 1: case 4: // Slide volume/frequency ON
              ch->volFreSlide = pgm_read_byte(ch->ptr++);
              ch->volFreConfig = (cmd - 64) == 1 ? 0x00 : 0x40;
              break;
            case 2: case 5: // Slide volume/frequency ON advanced
              ch->volFreSlide = pgm_read_byte(ch->ptr++);
              ch->volFreConfig = pgm_read_byte(ch->ptr++);
              break;
            case 3: case 6: // Slide volume/frequency OFF (same as 0x01 0x00)
              ch->volFreSlide = 0;
              break;
            case 7: // Set Arpeggio
              ch->arpNotes = pgm_read_byte(ch->ptr++);    // 0x40 + 0x03
              ch->arpTiming = pgm_read_byte(ch->ptr++);   // 0x40 (no third note) + 0x20 (toggle retrigger) + amount
              break;
            case 8: // Arpeggio OFF
              ch->arpNotes = 0;
              break;
            case 9: // Set Retriggering (noise)
              ch->reConfig = pgm_read_byte(ch->ptr++);    // RETRIG: point = 1 (*4), speed = 0 (0 = fastest, 1 = faster , 2 = fast)
              break;
            case 10: // Retriggering (noise) OFF
              ch->reConfig = 0;
              break;
            case 11: // ADD Transposition
              ch->transConfig += (int8_t)pgm_read_byte(ch->ptr++);
              break;
            case 12: // SET Transposition
              ch->transConfig = pgm_read_byte(ch->ptr++);
              break;
            case 13: // Transposition OFF
              ch->transConfig = 0;
              break;
            case 14: case 16: // SET Tremolo/Vibrato
              ch->treviDepth = pgm_read_byte(ch->ptr++);
              ch->treviConfig = pgm_read_byte(ch->ptr++) + ((cmd - 64) == 14 ? 0x00 : 0x40);
              break;
            case 15: case 17: // Tremolo/Vibrato OFF
              ch->treviDepth = 0;
              break;
            case 18: // Glissando
              ch->glisConfig = pgm_read_byte(ch->ptr++);
              break;
            case 19: // Glissando OFF
              ch->glisConfig = 0;
              break;
            case 20: // SET Note Cut
              ch->arpNotes = 0xFF;                        // 0xFF use Note Cut
              ch->arpTiming = pgm_read_byte(ch->ptr++);   // tick amount
              break;
            case 21: // Note Cut OFF
              ch->arpNotes = 0;
              break;
            case 92: // ADD tempo
              tickRate += pgm_read_byte(ch->ptr++);
              cia = 15625 / tickRate;
              break;
            case 93: // SET tempo
              tickRate = pgm_read_byte(ch->ptr++);
              cia = 15625 / tickRate;
              break;
            case 94: // Goto advanced
              for (byte i = 0; i < 4; i++) channel[i].repeatPoint = pgm_read_byte(ch->ptr++);
              break;
            case 95: // Stop channel
              ChannelActiveMute = ChannelActiveMute ^ (1 << (n + 4));
              ch->vol = 0;
              ch->delay = 0xFFFF;
              break;
          }
        } else if (cmd < 224) {
          // 160 … 223 : DELAY
          ch->delay = cmd - 159;
        } else if (cmd == 224) {
          // 224: LONG DELAY
          ch->delay = read_vle(&ch->ptr) + 65;
        } else if (cmd < 252) {
          // 225 … 251 : RESERVED
        } else if (cmd == 252 || cmd == 253) {
          // 252 (253) : CALL (REPEATEDLY)
          byte new_counter = cmd == 252 ? 0 : pgm_read_byte(ch->ptr++);
          byte new_track = pgm_read_byte(ch->ptr++);

          if (new_track != ch->track) {
            // Stack PUSH
            ch->stackCounter[ch->stackIndex] = ch->counter;
            ch->stackTrack[ch->stackIndex] = ch->track; // note 1
            ch->stackPointer[ch->stackIndex] = ch->ptr - trackBase;
            ch->stackIndex++;
            ch->track = new_track;
          }

          ch->counter = new_counter;
          ch->ptr = getTrackPointer(ch->track);
        } else if (cmd == 254) {
          // 254 : RETURN
          if (ch->counter > 0 || ch->stackIndex == 0) {
            // Repeat track
            if (ch->counter) ch->counter--;
            ch->ptr = getTrackPointer(ch->track);
          } else {
            // Check stack depth
            if (ch->stackIndex == 0) {
              // Stop the channel
              ch->delay = 0xFFFF;
            } else {
              // Stack POP
              ch->stackIndex--;
              ch->ptr = ch->stackPointer[ch->stackIndex] + trackBase;
              ch->counter = ch->stackCounter[ch->stackIndex];
              ch->track = ch->stackTrack[ch->stackIndex]; // note 1
            }
          }
        } else if (cmd == 255) {
          // 255 : EMBEDDED DATA
          ch->ptr += read_vle(&ch->ptr);
        }
      } while (ch->delay == 0);

      if (ch->delay != 0xFFFF) ch->delay--;
    }

    if (!(ChannelActiveMute & (1 << n))) {
      if (n == 3) {
        // Half volume, no frequency for noise channel
        osc[n].vol = ch->vol >> 1;
      } else {
        osc[n].freq = ch->freq;
        osc[n].vol = ch->vol;
      }
    }
    // if all channels are inactive, stop playing or check for repeat

    if (!(ChannelActiveMute & 0xF0))
    {
      byte repeatSong = 0;
      for (byte j = 0; j < 4; j++) repeatSong += channel[j].repeatPoint;
      if (repeatSong) {
        for (byte k = 0; k < 4; k++) {
          channel[k].ptr = getTrackPointer(channel[k].repeatPoint);
          channel[k].delay = 0;
        }
        ChannelActiveMute = 0b11110000;
      }
      else
      {
        memset(channel, 0, sizeof(channel));
        playing = false;
        return;
      }
    }
  }
}

/* ---- PCM source (replaces the timer-4 ISR) ------------------------------ */

static uint32_t phase[3];   /* osc[].phase in 16.16 */
static uint32_t avr_acc;    /* AVR samples elapsed, in 1/rate units */
static uint32_t tick_acc;   /* AVR samples since the last play routine */

/* One AVR sample of the noise LFSR (osc[3].freq is its state). */
static inline void lfsr_step(void)
{
  uint16_t r = osc[3].freq << 1;
  if (r & 0x8000) r ^= 1;
  if (r & 0x4000) r ^= 1;
  osc[3].freq = r;
}

extern "C" int ab_tunes_fill(int16_t *buf, int frames, int rate)
{
  if (reqStop) {
    reqStop = false;
    playing = false;
  }
  const byte *song = reqSong;
  if (song) {
    reqSong = 0;
    load_song(song);
    tick_acc = 0;
  }
  if (!playing || paused) {
    memset(buf, 0, frames * sizeof(int16_t));
    return 0;
  }
  /* Phases run at AVR_RATE: advance them by freq * AVR_RATE / rate. */
  uint32_t scale = ((uint32_t)AVR_RATE << 16) / (uint32_t)rate;
  bool audible = ab_audio_enabled();
  for (int i = 0; i < frames; i++) {
    avr_acc += AVR_RATE;
    while (avr_acc >= (uint32_t)rate) {
      avr_acc -= rate;
      lfsr_step();
      if (++tick_acc >= cia) {
        tick_acc = 0;
        ATM_playroutine();
        if (!playing) {
          memset(buf + i, 0, (frames - i) * sizeof(int16_t));
          return 1;
        }
      }
    }
    int s = 0;
    for (int n = 0; n < 3; n++) {
      phase[n] += osc[n].freq * scale;
      osc[n].phase = phase[n] >> 16;
    }
    /* channel 2: triangle */
    uint8_t hi = osc[2].phase >> 8;
    int8_t tri = (int8_t)((uint8_t)(((hi & 0x80) ? ~hi : hi) << 1) - 128);
    s += (tri * osc[2].vol) >> 7;
    /* channel 0: 25 % pulse */
    hi = osc[0].phase >> 8;
    s += ((hi & 0xc0) == 0xc0) ? -osc[0].vol : osc[0].vol;
    /* channel 1: square */
    s += (osc[1].phase & 0x8000) ? -osc[1].vol : osc[1].vol;
    /* channel 3: noise */
    s += (osc[3].freq & 0x8000) ? -osc[3].vol : osc[3].vol;
    buf[i] = audible ? s * 40 : 0;
  }
  return 1;
}
