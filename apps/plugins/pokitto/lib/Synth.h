/* SPDX-License-Identifier: GPL-2.0-or-later */
/* PokittoLib synth API as far as the games use it: setOSC() drives one
 * synthesised tone (the engine sound); the song player is not supported. */
#pragma once
#include "Pokitto.h"

struct OSC { int unused; };
extern OSC osc1, osc2, osc3;

extern float samplespertick;
extern bool track1on, track2on, track3on;
#define POK_AUD_FREQ PROJ_AUD_FREQ_DEFAULT

static inline void emptySong() {}
static inline void emptyBlocks() {}
static inline void initStreams(int) {}

/* setOSC(osc, on, wave, loop, echo, adsr, note, volume, attack, decay,
 * sustain, release, maxbend, bendrate, arpmode, overdrive, kick) */
void setOSC(OSC *osc, uint8_t on, uint8_t wave, uint8_t loop, uint8_t echo,
            uint8_t adsr, uint8_t note, uint16_t volume, uint16_t attack,
            uint16_t decay, uint16_t sustain, uint16_t release, int16_t maxbend,
            int16_t bendrate, uint8_t arpmode, uint8_t overdrive, uint8_t kick);

extern bool playing;

/* PokittoDisk (the SD card, below the game's data dir): one open file for
 * reading and a directory listing. */
#define FILE_MODE_READONLY 0
uint8_t fileOpen(const char *path, int mode);   /* 0 on success */
uint16_t fileReadBytes(uint8_t *buf, uint16_t n);
void fileClose();
static inline void pokInitSD() {}
char *getFirstDirEntry(const char *path);       /* "name" */
char *getNextDirEntry();                        /* "/name/" */
