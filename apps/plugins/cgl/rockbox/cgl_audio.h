/* Crisp Game Lib for Rockbox: square-wave synth on PCM_MIXER_CHAN_BEEP.
 * Built without the FPU: audio_more() runs in interrupt context. */
#ifndef CGL_AUDIO_H
#define CGL_AUDIO_H

#include <stdint.h>

void cgl_audio_open(void);
void cgl_audio_close(void);
uint32_t cgl_audio_rate(void);           /* mixer samples per second */
uint32_t cgl_audio_clock(void);          /* samples played since open */
/* start/end in clock samples; step: phase increment, 2^32 per cycle */
void cgl_audio_tone(uint32_t start, uint32_t end, uint32_t step);
void cgl_audio_stop_tones(void);

#endif
