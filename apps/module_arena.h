/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef MODULE_ARENA_H
#define MODULE_ARENA_H
#include <stdbool.h>
#include <stddef.h>
#include "config.h"
/* All callers remain in resident core text. An active image owns all bytes,
 * except the PCM ring's (TARGET_ARENA_PCM_SIZE) beside a codec. */
#ifdef HAVE_SHARED_CODEC_PLUGIN_BUFFER
bool module_arena_acquire_codec(void);
void module_arena_release_codec(void);
bool module_arena_codec_active(void);
/* Request blocks new codecs before the audio thread is synchronously stopped. */
bool module_arena_request_plugin(void);
bool module_arena_acquire_plugin(void);
void module_arena_release_plugin(void);
/* The owner bits, for the dev shell's health line */
unsigned int module_arena_state(void);
#else
/* Separate codec and plugin buffers: nothing to arbitrate */
static inline bool module_arena_acquire_codec(void) { return true; }
static inline void module_arena_release_codec(void) {}
static inline bool module_arena_codec_active(void) { return false; }
static inline bool module_arena_request_plugin(void) { return true; }
static inline bool module_arena_acquire_plugin(void) { return true; }
static inline void module_arena_release_plugin(void) {}
static inline unsigned int module_arena_state(void) { return 0; }
#endif
#ifdef TARGET_ARENA_PCM_SIZE
/* The PCM ring in the arena's top while small codecs play (playback.c) */
bool module_arena_acquire_ring(void);
void module_arena_release_ring(void);
/* codecs.c: keep the arena's top SIZE bytes from codecs and return their
 * start (NULL if the loaded codec reaches into them); 0 gives them back */
void *codec_arena_reserve_top(size_t size);
/* codecs.c: arena bytes (image + bss) the codec for AFMT needs, from its
 * file's header; 0 if unknown */
size_t codec_arena_need(int afmt);
#endif
#endif
