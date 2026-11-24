/***************************************************************************
 * Crisp Game Lib Portable for Rockbox: square-wave voices scheduled in
 * mixer samples on PCM_MIXER_CHAN_BEEP (the mixer, not the playback stream).
 * Rockbox port: MIT, see ../LICENSE. Built without the FPU (cgl.make): the
 * mixer callback runs in interrupt context, where FP state is not saved.
 ***************************************************************************/
#include "plugin.h"
#include "cgl_audio.h"

#define TONE_COUNT 48
#define BLOCK_FRAMES 256
#define MAX_VOICES 3
#define TONE_AMP 2500

struct tone {
    uint32_t start, end; /* sample clock */
    uint32_t step;       /* phase increment, 2^32 per cycle */
    uint32_t phase;
};

static struct tone tones[TONE_COUNT];
static int tone_next;
static volatile uint32_t clock_samples;
static uint32_t sample_rate;
static int16_t pcm[BLOCK_FRAMES * 2];
static bool audio_open;

static void audio_more(const void **start, size_t *size)
{
    uint32_t t0 = clock_samples, t1 = t0 + BLOCK_FRAMES;
    struct tone *v[MAX_VOICES];
    int nv = 0;
    for (int i = 0; i < TONE_COUNT && nv < MAX_VOICES; i++) {
        if (tones[i].step && tones[i].start < t1 && tones[i].end > t0)
            v[nv++] = &tones[i];
    }
    for (int i = 0; i < BLOCK_FRAMES; i++) {
        int s = 0;
        for (int k = 0; k < nv; k++) {
            v[k]->phase += v[k]->step;
            s += (v[k]->phase & 0x80000000u) ? TONE_AMP : -TONE_AMP;
        }
        pcm[2 * i] = pcm[2 * i + 1] = s / 2;
    }
    clock_samples = t1;
    *start = pcm;
    *size = sizeof(pcm);
}

void cgl_audio_open(void)
{
    static const struct mixer_play_cbs cbs = { .get_more = audio_more };
    sample_rate = rb->mixer_get_frequency();
    rb->memset(tones, 0, sizeof(tones));
    tone_next = 0;
    clock_samples = 0;
    rb->mixer_channel_set_amplitude(PCM_MIXER_CHAN_BEEP, MIX_AMP_UNITY);
    rb->mixer_channel_play_data(PCM_MIXER_CHAN_BEEP, &cbs, NULL, 0);
    audio_open = true;
}

void cgl_audio_close(void)
{
    if (audio_open)
        rb->mixer_channel_stop(PCM_MIXER_CHAN_BEEP);
    audio_open = false;
}

uint32_t cgl_audio_rate(void)
{
    return sample_rate;
}

uint32_t cgl_audio_clock(void)
{
    return clock_samples;
}

void cgl_audio_tone(uint32_t start, uint32_t end, uint32_t step)
{
    if (!audio_open)
        return;
    struct tone t = { .start = start, .end = end, .step = step, .phase = 0 };
    rb->pcm_play_lock();
    tones[tone_next] = t;
    tone_next = (tone_next + 1) % TONE_COUNT;
    rb->pcm_play_unlock();
}

void cgl_audio_stop_tones(void)
{
    rb->pcm_play_lock();
    rb->memset(tones, 0, sizeof(tones));
    rb->pcm_play_unlock();
}
