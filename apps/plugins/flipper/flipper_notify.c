/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Notification sequences and the speaker. Only notes and delays do anything:
 * a sequence with notes is turned into (tone, duration) segments that a
 * square-wave generator plays on PCM_MIXER_CHAN_BEEP (the
 * mixer, not the playback stream). LEDs, vibration and the backlight have no hardware here. */
#include "plugin.h"
#include <notification/notification_messages.h>
#include <furi_hal.h>
#include "flipper_internal.h"


const NotificationSequence sequence_set_red_255 = {NULL};
const NotificationSequence sequence_set_green_255 = {NULL};
const NotificationSequence sequence_set_blue_255 = {NULL};
const NotificationSequence sequence_reset_red = {NULL};
const NotificationSequence sequence_reset_green = {NULL};
const NotificationSequence sequence_reset_blue = {NULL};
const NotificationSequence sequence_reset_rgb = {NULL};
const NotificationSequence sequence_single_vibro = {NULL};
const NotificationSequence sequence_success = {NULL};
const NotificationSequence sequence_error = {NULL};
const NotificationSequence sequence_blink_blue_10 = {NULL};
const NotificationSequence sequence_blink_white_100 = {NULL};
const NotificationSequence sequence_blink_red_10 = {NULL};
const NotificationSequence sequence_blink_green_10 = {NULL};
const NotificationSequence sequence_display_backlight_on = {NULL};
const NotificationSequence sequence_display_backlight_enforce_on = {NULL};
const NotificationSequence sequence_display_backlight_enforce_auto = {NULL};
const NotificationMessage message_blue_0;
const NotificationMessage message_blue_255;
const NotificationMessage message_green_0;
const NotificationMessage message_green_255;
const NotificationMessage message_red_0;
const NotificationMessage message_red_255;
const NotificationMessage message_sound_off = {NM_SOUND_OFF, 0};
const NotificationMessage message_vibro_on;
const NotificationMessage message_do_not_reset;
const NotificationMessage message_force_speaker_volume_setting_1f;
const NotificationMessage message_force_vibro_setting_on;
const NotificationSequence sequence_blink_stop = {NULL};
const NotificationMessage message_vibro_off;
const NotificationMessage message_display_backlight_on;
const NotificationMessage message_display_backlight_off;
const NotificationMessage message_delay_1 = {NM_DELAY, 1};
const NotificationMessage message_delay_10 = {NM_DELAY, 10};
const NotificationMessage message_delay_25 = {NM_DELAY, 25};
const NotificationMessage message_delay_50 = {NM_DELAY, 50};
const NotificationMessage message_delay_100 = {NM_DELAY, 100};
const NotificationMessage message_delay_250 = {NM_DELAY, 250};
const NotificationMessage message_delay_500 = {NM_DELAY, 500};
const NotificationMessage message_delay_1000 = {NM_DELAY, 1000};
const NotificationMessage message_note_c0 = {NM_NOTE, 16};
const NotificationMessage message_note_c1 = {NM_NOTE, 33};
const NotificationMessage message_note_c2 = {NM_NOTE, 65};
const NotificationMessage message_note_c3 = {NM_NOTE, 131};
const NotificationMessage message_note_c4 = {NM_NOTE, 262};
const NotificationMessage message_note_c5 = {NM_NOTE, 523};
const NotificationMessage message_note_c6 = {NM_NOTE, 1047};
const NotificationMessage message_note_c7 = {NM_NOTE, 2093};
const NotificationMessage message_note_c8 = {NM_NOTE, 4186};
const NotificationMessage message_note_d0 = {NM_NOTE, 18};
const NotificationMessage message_note_d1 = {NM_NOTE, 37};
const NotificationMessage message_note_d2 = {NM_NOTE, 73};
const NotificationMessage message_note_d3 = {NM_NOTE, 147};
const NotificationMessage message_note_d4 = {NM_NOTE, 294};
const NotificationMessage message_note_d5 = {NM_NOTE, 587};
const NotificationMessage message_note_d6 = {NM_NOTE, 1175};
const NotificationMessage message_note_d7 = {NM_NOTE, 2349};
const NotificationMessage message_note_d8 = {NM_NOTE, 4699};
const NotificationMessage message_note_e0 = {NM_NOTE, 21};
const NotificationMessage message_note_e1 = {NM_NOTE, 41};
const NotificationMessage message_note_e2 = {NM_NOTE, 82};
const NotificationMessage message_note_e3 = {NM_NOTE, 165};
const NotificationMessage message_note_e4 = {NM_NOTE, 330};
const NotificationMessage message_note_e5 = {NM_NOTE, 659};
const NotificationMessage message_note_e6 = {NM_NOTE, 1319};
const NotificationMessage message_note_e7 = {NM_NOTE, 2637};
const NotificationMessage message_note_e8 = {NM_NOTE, 5274};
const NotificationMessage message_note_f0 = {NM_NOTE, 22};
const NotificationMessage message_note_f1 = {NM_NOTE, 44};
const NotificationMessage message_note_f2 = {NM_NOTE, 87};
const NotificationMessage message_note_f3 = {NM_NOTE, 175};
const NotificationMessage message_note_f4 = {NM_NOTE, 349};
const NotificationMessage message_note_f5 = {NM_NOTE, 698};
const NotificationMessage message_note_f6 = {NM_NOTE, 1397};
const NotificationMessage message_note_f7 = {NM_NOTE, 2794};
const NotificationMessage message_note_f8 = {NM_NOTE, 5588};
const NotificationMessage message_note_g0 = {NM_NOTE, 24};
const NotificationMessage message_note_g1 = {NM_NOTE, 49};
const NotificationMessage message_note_g2 = {NM_NOTE, 98};
const NotificationMessage message_note_g3 = {NM_NOTE, 196};
const NotificationMessage message_note_g4 = {NM_NOTE, 392};
const NotificationMessage message_note_g5 = {NM_NOTE, 784};
const NotificationMessage message_note_g6 = {NM_NOTE, 1568};
const NotificationMessage message_note_g7 = {NM_NOTE, 3136};
const NotificationMessage message_note_g8 = {NM_NOTE, 6272};
const NotificationMessage message_note_a0 = {NM_NOTE, 28};
const NotificationMessage message_note_a1 = {NM_NOTE, 55};
const NotificationMessage message_note_a2 = {NM_NOTE, 110};
const NotificationMessage message_note_a3 = {NM_NOTE, 220};
const NotificationMessage message_note_a4 = {NM_NOTE, 440};
const NotificationMessage message_note_a5 = {NM_NOTE, 880};
const NotificationMessage message_note_a6 = {NM_NOTE, 1760};
const NotificationMessage message_note_a7 = {NM_NOTE, 3520};
const NotificationMessage message_note_a8 = {NM_NOTE, 7040};
const NotificationMessage message_note_b0 = {NM_NOTE, 31};
const NotificationMessage message_note_b1 = {NM_NOTE, 62};
const NotificationMessage message_note_b2 = {NM_NOTE, 123};
const NotificationMessage message_note_b3 = {NM_NOTE, 247};
const NotificationMessage message_note_b4 = {NM_NOTE, 494};
const NotificationMessage message_note_b5 = {NM_NOTE, 988};
const NotificationMessage message_note_b6 = {NM_NOTE, 1976};
const NotificationMessage message_note_b7 = {NM_NOTE, 3951};
const NotificationMessage message_note_b8 = {NM_NOTE, 7902};
const NotificationMessage message_note_cs0 = {NM_NOTE, 17};
const NotificationMessage message_note_cs1 = {NM_NOTE, 35};
const NotificationMessage message_note_cs2 = {NM_NOTE, 69};
const NotificationMessage message_note_cs3 = {NM_NOTE, 139};
const NotificationMessage message_note_cs4 = {NM_NOTE, 277};
const NotificationMessage message_note_cs5 = {NM_NOTE, 554};
const NotificationMessage message_note_cs6 = {NM_NOTE, 1109};
const NotificationMessage message_note_cs7 = {NM_NOTE, 2217};
const NotificationMessage message_note_cs8 = {NM_NOTE, 4435};
const NotificationMessage message_note_ds0 = {NM_NOTE, 19};
const NotificationMessage message_note_ds1 = {NM_NOTE, 39};
const NotificationMessage message_note_ds2 = {NM_NOTE, 78};
const NotificationMessage message_note_ds3 = {NM_NOTE, 156};
const NotificationMessage message_note_ds4 = {NM_NOTE, 311};
const NotificationMessage message_note_ds5 = {NM_NOTE, 622};
const NotificationMessage message_note_ds6 = {NM_NOTE, 1245};
const NotificationMessage message_note_ds7 = {NM_NOTE, 2489};
const NotificationMessage message_note_ds8 = {NM_NOTE, 4978};
const NotificationMessage message_note_fs0 = {NM_NOTE, 23};
const NotificationMessage message_note_fs1 = {NM_NOTE, 46};
const NotificationMessage message_note_fs2 = {NM_NOTE, 92};
const NotificationMessage message_note_fs3 = {NM_NOTE, 185};
const NotificationMessage message_note_fs4 = {NM_NOTE, 370};
const NotificationMessage message_note_fs5 = {NM_NOTE, 740};
const NotificationMessage message_note_fs6 = {NM_NOTE, 1480};
const NotificationMessage message_note_fs7 = {NM_NOTE, 2960};
const NotificationMessage message_note_fs8 = {NM_NOTE, 5920};
const NotificationMessage message_note_gs0 = {NM_NOTE, 26};
const NotificationMessage message_note_gs1 = {NM_NOTE, 52};
const NotificationMessage message_note_gs2 = {NM_NOTE, 104};
const NotificationMessage message_note_gs3 = {NM_NOTE, 208};
const NotificationMessage message_note_gs4 = {NM_NOTE, 415};
const NotificationMessage message_note_gs5 = {NM_NOTE, 831};
const NotificationMessage message_note_gs6 = {NM_NOTE, 1661};
const NotificationMessage message_note_gs7 = {NM_NOTE, 3322};
const NotificationMessage message_note_gs8 = {NM_NOTE, 6645};
const NotificationMessage message_note_as0 = {NM_NOTE, 29};
const NotificationMessage message_note_as1 = {NM_NOTE, 58};
const NotificationMessage message_note_as2 = {NM_NOTE, 117};
const NotificationMessage message_note_as3 = {NM_NOTE, 233};
const NotificationMessage message_note_as4 = {NM_NOTE, 466};
const NotificationMessage message_note_as5 = {NM_NOTE, 932};
const NotificationMessage message_note_as6 = {NM_NOTE, 1865};
const NotificationMessage message_note_as7 = {NM_NOTE, 3729};
const NotificationMessage message_note_as8 = {NM_NOTE, 7459};

#define RING 64                 /* queued segments (a power of two) */
#define BLOCK_FRAMES 256
#define SEQ_AMP 2500
#define MAX_BACKLOG 32

struct seg {
    uint32_t step;              /* phase increment, 2^32 per cycle; 0: silence */
    uint32_t frames;
};

/* audio_more() runs in interrupt context: integer-only, no FP state is saved
 * there (check with yp3dev/fpucheck.py rock.elf audio_more). */
static struct seg ring[RING];
static volatile uint32_t ring_head, ring_tail;  /* producer / consumer */
static struct seg cur;                          /* consumer only */
static uint32_t seq_phase;
static volatile uint32_t tone_step, tone_amp;   /* furi_hal_speaker_start */
static uint32_t tone_phase;
static uint32_t sample_rate;
static int16_t pcm[BLOCK_FRAMES * 2];
static bool chan_open, speaker_owned;

static void audio_more(const void **start, size_t *size)
{
    uint32_t tstep = tone_step, tamp = tone_amp;
    for (int i = 0; i < BLOCK_FRAMES; i++) {
        int s = 0;
        while (cur.frames == 0 && ring_tail != ring_head) {
            cur = ring[ring_tail % RING];
            ring_tail++;
        }
        if (cur.frames) {
            cur.frames--;
            if (cur.step) {
                seq_phase += cur.step;
                s += (seq_phase & 0x80000000u) ? SEQ_AMP : -SEQ_AMP;
            }
        }
        if (tstep) {
            tone_phase += tstep;
            s += (tone_phase & 0x80000000u) ? (int)tamp : -(int)tamp;
        }
        pcm[2 * i] = pcm[2 * i + 1] = s;
    }
    *start = pcm;
    *size = sizeof(pcm);
}

static void chan_start(void)
{
    static const struct mixer_play_cbs cbs = { .get_more = audio_more };
    if (chan_open)
        return;
    sample_rate = rb->mixer_get_frequency();
    ring_head = ring_tail = 0;
    cur.frames = 0;
    tone_step = 0;
    rb->mixer_channel_set_amplitude(PCM_MIXER_CHAN_BEEP, MIX_AMP_UNITY);
    rb->mixer_channel_play_data(PCM_MIXER_CHAN_BEEP, &cbs, NULL, 0);
    chan_open = true;
}

static uint32_t step_for(uint32_t hz)
{
    return (uint32_t)(((uint64_t)hz << 32) / sample_rate);
}

void flipper_sound_stop(void)
{
    if (!chan_open)
        return;
    rb->pcm_play_lock();
    tone_step = 0;
    ring_tail = ring_head;
    cur.frames = 0;
    rb->pcm_play_unlock();
}

void flipper_sound_close(void)
{
    if (chan_open)
        rb->mixer_channel_stop(PCM_MIXER_CHAN_BEEP);
    chan_open = speaker_owned = false;
}

bool furi_hal_speaker_acquire(uint32_t timeout)
{
    UNUSED(timeout);
    speaker_owned = true;
    chan_start();
    return true;
}
bool furi_hal_speaker_is_mine(void) { return speaker_owned; }
void furi_hal_speaker_set_volume(float v)
{
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;
    tone_amp = (uint32_t)(v * SEQ_AMP);
}
void furi_hal_speaker_start(float f, float v)
{
    if (!chan_open || f < 1.0f)
        return;
    furi_hal_speaker_set_volume(v);
    tone_step = step_for((uint32_t)f);
}
void furi_hal_speaker_stop(void) { tone_step = 0; }
void furi_hal_speaker_release(void)
{
    tone_step = 0;
    speaker_owned = false;
}

/* Queue the sound part of a sequence: a note sounds until the next note or
 * sound_off, delays give the durations. Not blocking, also for the _block
 * variant: the game keeps running while the notes play. */
void notification_message(NotificationApp *a, const NotificationSequence *s)
{
    struct seg tmp[RING / 2];
    int n = 0;
    uint32_t hz = 0;
    bool notes = false;
    const NotificationMessage *const *m;

    UNUSED(a);
    for (m = *s; *m; m++)
        if ((*m)->kind == NM_NOTE)
            notes = true;
    if (!notes)
        return;
    chan_start();
    for (m = *s; *m; m++) {
        switch ((*m)->kind) {
        case NM_NOTE:
            hz = (*m)->value;
            break;
        case NM_SOUND_OFF:
            hz = 0;
            break;
        case NM_DELAY:
            if (n < (int)ARRAYLEN(tmp)) {
                tmp[n].step = hz ? step_for(hz) : 0;
                tmp[n].frames = (uint32_t)(*m)->value * (sample_rate / 1000);
                n++;
            }
            break;
        }
    }
    if (!n)
        return;
    rb->pcm_play_lock();
    if (ring_head - ring_tail + n <= MAX_BACKLOG) {
        for (int i = 0; i < n; i++)
            ring[(ring_head + i) % RING] = tmp[i];
        ring_head += n;
    }
    rb->pcm_play_unlock();
}

void notification_message_block(NotificationApp *a, const NotificationSequence *s)
{
    notification_message(a, s);
}
