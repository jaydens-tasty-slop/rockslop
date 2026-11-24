/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Bluetooth in the apps: power from the setting (at boot and from the
 * menu), auto-connect to the most recent paired audio device (input
 * devices connect by themselves), and audio routing:
 * playback moves to PCM_SINK_BT when a headset's stream is ready, and back
 * to the built-in output, paused, when the headset goes away; playback
 * resumes if the same headset is back within RESUME_WINDOW and playback is
 * still paused. Bluetooth and the built-in output each keep their volume
 * (global_status.volume is the one in use). With AVRCP absolute volume the
 * headset plays at the Rockbox volume and its own volume changes come back
 * here (firmware/sound.c, bluetooth.h).
 *
 * RAM: the stack and its SBC encoder take ~15 KB of heap while on; the
 * playback block leaves that room. */
#include "config.h"
#include "kernel.h"
#include "audio.h"
#include "bluetooth.h"
#include "bluetooth_policy.h"
#include "misc.h"
#include "pcm_mixer.h"
#include "pcm_sink.h"
#include "playback.h"
#include "playlist.h"
#include "settings.h"
#include "sound.h"
#include "wps.h"
#include <string.h>

#define RESUME_WINDOW   (5 * 60 * HZ)
#define PAIRED_MAX      8           /* paired devices looked at */
#define PAUSE_WAIT      (HZ / 2)    /* for the pause's fade-out (HZ / 3) */

static bt_addr_t headset;   /* the device whose stream we route to */
static long resume_by;      /* the headset left mid-play: resume before
                               this tick if it is back; 0 when not */

/* Make global_status.volume the current sink's and apply it: the
 * volumes swap when the sink changed since it was last set */
static void volume_follow_sink(void)
{
    bool bt = pcm_current_sink() == PCM_SINK_BT;
    if (bt != (bool)global_status.volume_is_bt) {
        int v = global_status.volume;
        global_status.volume = global_status.volume_other;
        global_status.volume_other = v;
        global_status.volume_is_bt = bt;
    }
    sound_set_volume(global_status.volume);
}

/* Move playback to a sink; the volume is applied again, as a headset
 * with absolute volume takes it out of the PCM level */
static void route(enum pcm_sink_ids sink)
{
    mixer_switch_sink(sink);
    volume_follow_sink();
}

/* Back to the built-in output; pause first so nothing plays out loud.
 * With "volume fade" the pause only starts a fade-out: it has to end
 * before the switch, or the rest of it plays from the built-in output at
 * the headset's PCM level (full scale with absolute volume) until the
 * volume follows. If the sink takes no more PCM (a link that is gone),
 * the channel does not stop, but the fade still ends (a tick task) and
 * leaves it muted.
 * True if that paused playback. */
static bool route_builtin(void)
{
    if (pcm_current_sink() != PCM_SINK_BT)
        return false;
    int st = audio_status();
    bool pause = (st & AUDIO_STATUS_PLAY) && !(st & AUDIO_STATUS_PAUSE);
    if (pause) {
        pause_action(false);
        long end = current_tick + PAUSE_WAIT;
        while (mixer_channel_status(PCM_MIXER_CHAN_PLAYBACK) == CHANNEL_PLAYING &&
               TIME_BEFORE(current_tick, end))
            sleep(1);
    }
    route(PCM_SINK_BUILTIN);
    return pause;
}

/* Power on (then auto-connect if enabled) or off (audio back to the
 * built-in output first); false if power-on failed */
static bool power(bool on)
{
    if (!on) {
        resume_by = 0;
        route_builtin();
        bt_power(false);
        return true;
    }
    if (!bt_power(true))
        return false;
    if (!global_settings.bt_autoconnect)
        return true;
    struct bt_device devs[PAIRED_MAX];  /* most recently used first */
    int n = bt_paired_devices(devs, PAIRED_MAX);
    for (int i = 0; i < n; i++) {
        if (!BT_COD_INPUT(devs[i].cod)) {
            bt_connect(&devs[i].addr);
            break;
        }
    }
    return true;
}

static bool started;     /* bluetooth_init() has run */

void bluetooth_init(void)
{
    started = true;
    volume_follow_sink();   /* saved while a headset played */
    if (global_settings.bt_enabled)
        power(true);
}

void bluetooth_apply_setting(void)
{
    if (started && global_settings.bt_enabled != (bt_status() != BT_STATUS_OFF))
        bluetooth_set_power(global_settings.bt_enabled);
}

bool bluetooth_set_power(bool on)
{
    bool ok = power(on);
    global_settings.bt_enabled = on && ok;
    return ok;
}

void bluetooth_audio_event(long event)
{
    if (event == SYS_BT_AUDIO_CONNECTED) {
        struct bt_device dev;
        bool same = false;
        if (bt_connected_device(&dev)) {
            same = !memcmp(&dev.addr, &headset, sizeof headset);
            headset = dev.addr;
        }
        route(PCM_SINK_BT);
        if (resume_by && same && TIME_BEFORE(current_tick, resume_by) &&
            audio_status() == (AUDIO_STATUS_PLAY | AUDIO_STATUS_PAUSE))
            unpause_action(false);
        resume_by = 0;
    } else if (route_builtin()) {
        resume_by = current_tick + RESUME_WINDOW;
    }
    bt_audio_acknowledge(event);
}

/* Centibels to the nearest volume setting (firmware/sound.c's
 * sound_value_to_cb() in reverse): value = cb * num / den */
static int volume_of_cb(int cb)
{
    int min = sound_min(SOUND_VOLUME), step = sound_steps(SOUND_VOLUME);
    int num = 1, den = 1;
    for (int d = sound_numdecimals(SOUND_VOLUME); d > 1; d--)
        num *= 10;
    for (int d = sound_numdecimals(SOUND_VOLUME); d < 1; d++)
        den *= 10;
    int steps = (cb * num - min * den + step * den / 2) / (step * den);
    return min + MAX(steps, 0) * step;
}

void bluetooth_volume_event(int cb)
{
    /* Not routed to the headset: avrcp.c sends it no volume before the
     * route, so this is the PCM volume coming back after the link went,
     * or a change on a headset that plays nothing of ours */
    if (!global_status.volume_is_bt)
        return;
    global_status.volume = volume_of_cb(cb);
    setvol();
}

/* Play and Pause are states, not a toggle: decided here in the UI thread,
 * after the keys before them, whose starts and pauses finish before they
 * return (audio_play(), audio_pause() wait for the audio thread). A toggle
 * decided from audio_status() as each key arrived raced them: Play while
 * stopped, then Pause and Play during the resume, ended paused. */
void bluetooth_play_event(bool play)
{
    int status = audio_status();
    if (!play) {
        if (status == AUDIO_STATUS_PLAY)
            pause_action(true);
    } else if (status & AUDIO_STATUS_PAUSE) {
        unpause_action(true);
    } else if (!(status & AUDIO_STATUS_PLAY) && playlist_resume() != -1) {
        playlist_start(global_status.resume_index,
                       global_status.resume_elapsed,
                       global_status.resume_offset);
    }
}
