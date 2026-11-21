/* SPDX-License-Identifier: GPL-2.0-or-later */
/* The YP3's own dev shell commands and boot tasks (apps/dev_shell.c has the
 * shared ones; devlog.h): the sdpush bundle, the PMU, the FM capture path,
 * core voltage, the profiler, the watchpoint, NOR clock margins. */
#include "config.h"
#include "system.h"
#include "kernel.h"
#include "thread.h"
#include "backlight.h"
#include "button.h"
#include "file.h"
#include "dir.h"
#include "crc32.h"
#include "core_alloc.h"
#include "pcm.h"
#include "pcm-internal.h"
#include "pcm_mixer.h"
#include "powermgmt.h"
#include "tuner.h"
#include "fmradio_i2c.h"
#include "dev-yp3.h"
#include "pcm-yp3.h"
#include "pmu-yp3.h"
#include "capture-yp3.h"
#ifdef HAVE_BLUETOOTH
#include "bt-yp3.h"
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define PROF_MAX_BYTES 4096     /* `prof` sample buffer, at most */

#define BUNDLE_MAGIC 0x48535059u /* "YPSH" */

struct bundle_entry {
    uint32_t magic, path_len, data_len, crc;   /* then path, data (4-aligned)
                                                */
};


static bool file_matches(const char *path, const void *data, uint32_t len,
                         uint32_t crc)
{
    static unsigned char buf[512];
    int fd = open(path, O_RDONLY);
    if (fd < 0)
        return false;
    bool same = lseek(fd, 0, SEEK_END) == (off_t)len &&
                lseek(fd, 0, SEEK_SET) == 0;
    uint32_t c = 0xffffffff;
    for (uint32_t done = 0; same && done < len; ) {
        ssize_t n = read(fd, buf, MIN(sizeof buf, len - done));
        if (n <= 0)
            same = false;
        else {
            c = crc_32r(buf, n, c);
            done += n;
            if (!(done & 16383))
                dev_feed();
        }
    }
    close(fd);
    (void)data;
    return same && ~c == crc;
}

static void make_parents(char *path)
{
    for (char *p = path + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(path);
            *p = '/';
        }
    }
}

static void sdpush(void)
{
    const char *p = YP3_DEV_XIP(YP3_DEV_BUNDLE_OFFSET);
    const char *end = YP3_DEV_XIP(YP3_DEV_CONFIG_OFFSET);
    while (p + sizeof(struct bundle_entry) <= end) {
        dev_feed();
        const struct bundle_entry *e = (const void *)p;
        if (e->magic != BUNDLE_MAGIC)
            break;
        char path[MAX_PATH];
        const char *name = p + sizeof *e;
        const char *data = name + ((e->path_len + 3) & ~3u);
        if (e->path_len >= sizeof path || data + e->data_len > end)
            break;
        memcpy(path, name, e->path_len);
        path[e->path_len] = '\0';
        long tcrc = current_tick;
        bool bad = ~crc_32r(data, e->data_len, 0xffffffff) != e->crc;
        dev_log("sdpush %s: bundle crc %ld ms", path,
                (current_tick - tcrc) * 1000 / HZ);
        {
            static unsigned char b[512];
            long tr = current_tick;
            int fd = open(path, O_RDONLY);
            uint32_t total = 0;
            ssize_t n;
            while (fd >= 0 && (n = read(fd, b, sizeof b)) > 0) {
                total += n;
                if (!(total & 16383))
                    dev_feed();
            }
            if (fd >= 0)
                close(fd);
            dev_log("sdpush %s: plain read %lu bytes %ld ms", path,
                        (unsigned long)total, (current_tick - tr) * 1000 / HZ);
        }
        if (bad) {
            dev_log("sdpush %s: bundle CRC mismatch", path);
            break;
        }
        long tc = current_tick;
        struct dev_sd_stats st0 = dev_sd_stats;
        if (file_matches(path, data, e->data_len, e->crc)) {
            dev_log("sdpush %s: same (compare %ld ms; sd %lu calls %lu "
                    "sectors %lu us)",
                    path, (current_tick - tc) * 1000 / HZ,
                    (unsigned long)(dev_sd_stats.read_calls - st0.read_calls),
                    (unsigned long)(dev_sd_stats.read_sectors -
                                    st0.read_sectors),
                    (unsigned long)(dev_sd_stats.read_us - st0.read_us));
        } else {
            make_parents(path);
            long t0 = current_tick;
            int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            uint32_t done = 0;
            /* Runs before the UI loop feeds the watchdog: feed per chunk. */
            while (fd >= 0 && done < e->data_len) {
                uint32_t n = MIN(16384u, e->data_len - done);
                if (write(fd, data + done, n) != (ssize_t)n)
                    break;
                done += n;
                dev_feed();
            }
            int rc = fd >= 0 ? close(fd) : -1;
            long t1 = current_tick;
            bool ok = done == e->data_len && rc == 0 &&
                      file_matches(path, data, e->data_len, e->crc);
            dev_log("sdpush %s: %s (%lu bytes, write %ld ms, verify %ld ms)",
                    path, ok ? "written" : "FAILED",
                    (unsigned long)e->data_len, (t1 - t0) * 1000 / HZ,
                    (current_tick - t1) * 1000 / HZ);
            if (!ok)
                dev_log("  fd=%d written=%lu close=%d", fd,
                        (unsigned long)done, rc);
        }
        p = data + ((e->data_len + 3) & ~3u);
    }
}

/* dev `norscan stress`: XIP reads at another NOR clock. The switch, the
 * checksums and the switch back run from SRAM with IRQs masked, so a clock
 * the flash or controller can't take gives a wrong checksum instead of
 * executing garbage. */
extern uint32_t yp3_nor_clock32_set(uint32_t expected);

/* norscan stress DIV N: 32 x 1 KiB block checksums of flash at /DIV,
 * compared with the same at the normal clock, N times: how many blocks read
 * wrong (all: far past the limit; a few: at the edge; none: margin) */
#define NORSCAN_BLOCKS 32
static void ICODE_ATTR __attribute__((noinline))
    norscan_blocks(uint32_t clock, uint32_t restore, uint32_t *sums)
{
    const volatile uint32_t *src =
        (const volatile uint32_t *)SL6801_FLASH_BASE;
    yp3_nor_clock32_set(clock);
    for (unsigned b = 0; b < NORSCAN_BLOCKS; b++) {
        uint32_t sum = 0;
        for (unsigned i = 0; i < 256; i++)
            sum = (sum << 1 | sum >> 31) ^ src[b * 256 + i];
        sums[b] = sum;
    }
    yp3_nor_clock32_set(restore);
}

static void norscan_stress(unsigned div, unsigned n)
{
    const uint32_t now = *(volatile uint32_t *)CLK_SEL_2B;
    const uint32_t clock = (now & ~0xf00u) | (div - 1) << 8;
    uint32_t ref[NORSCAN_BLOCKS], sums[NORSCAN_BLOCKS];
    unsigned long bad = 0, bad_runs = 0;
    int old = disable_irq_save();
    norscan_blocks(now, now, ref);
    restore_irq(old);
    for (unsigned r = 0; r < n; r++) {
        old = disable_irq_save();
        norscan_blocks(clock, now, sums);
        restore_irq(old);
        unsigned run_bad = 0;
        for (unsigned b = 0; b < NORSCAN_BLOCKS; b++)
            run_bad += sums[b] != ref[b];
        bad += run_bad;
        bad_runs += run_bad != 0;
        if ((r & 63) == 0) {
            dev_feed();
            yield();
        }
    }
    dev_log("norscan stress /%u (%lu MHz): %u x 32 KiB, %lu bad blocks of "
            "%lu, %lu bad runs",
            div, 384ul / div, n, bad, (unsigned long)n * NORSCAN_BLOCKS,
            bad_runs);
}

static bool sh_pmu(char *arg)
{
    char *save, *i = strtok_r(arg, " ", &save);
    char *v = strtok_r(NULL, " ", &save);
    if (!i)
        return false;
    uint8_t idx = strtoul(i, NULL, 0), val = 0;
    while (!yp3_pmu_try_lock())
        sleep(1);
    bool ok = (!v || yp3_pmu_write(idx, strtoul(v, NULL, 0), 1000)) &&
              yp3_pmu_read(idx, 1000, &val);
    yp3_pmu_unlock();
    if (!ok)
        return false;
    dev_log("pmu[%02x] = %02x", idx, val);
    return true;
}

static bool sh_fm(char *arg)
{
    /* fm KHZ: tune and unmute; fm off: sleep the tuner */
    if (!strcmp(arg, "off")) {
        yp3_capture_stop();
        tuner_set(RADIO_MUTE, 1);
        tuner_set(RADIO_SLEEP, 1);
        return true;
    }
    tuner_set(RADIO_SLEEP, 0);
    tuner_set(RADIO_FREQUENCY, atoi(arg) * 1000);
    tuner_set(RADIO_MUTE, 0);
    yp3_capture_start();
    backlight_on();
    sleep(HZ / 5);
    dev_log("fm present=%d tuned=%d stereo=%d rssi=%d",
            tuner_get(RADIO_PRESENT), tuner_get(RADIO_TUNED),
            tuner_get(RADIO_STEREO), tuner_get(RADIO_RSSI));
    return true;
}

static bool sh_fmregs(char *arg)
{
    /* RDA sequential read starts at 0x0A: STC/SF/ST/READCHAN, RSSI/FM_TRUE */
    (void)arg;
    unsigned char rx[4] = { 0 };
    int r = fmradio_i2c_read(0x20, rx, 4);
    unsigned a = rx[0] << 8 | rx[1], b = rx[2] << 8 | rx[3];
    dev_log("fmregs r=%d 0A=%04x (stc=%u sf=%u st=%u chan=%u) 0B=%04x "
            "(rssi=%u true=%u)",
            r, a, a >> 14 & 1, a >> 13 & 1, a >> 10 & 1, a & 0x3ff, b,
            b >> 9, b >> 8 & 1);
    return true;
}

static bool sh_fmgain(char *arg)
{
    /* fmgain N: line-in (aux) gain code 0..31 for both channels */
    yp3_capture_set_gain(atoi(arg), atoi(arg));
    return true;
}

static bool sh_micgain(char *arg)
{
    /* micgain N: mic gain code 0..127 for the next captures */
    yp3_capture_set_mic_gain(atoi(arg));
    return true;
}

static bool sh_fmmon(char *arg)
{
    (void)arg;
    int pl, pr, al, ar;
    yp3_capture_levels(&pl, &pr, &al, &ar);
    dev_log("capture periods=%lu consumed=%lu over=%lu under=%lu peak %d/%d "
            "avg %d/%d",
            yp3_capture_stats.periods, yp3_capture_stats.consumed,
            yp3_capture_stats.overruns, yp3_capture_stats.underruns,
            pl, pr, al, ar);
    dev_log("mixer chan=%d pcm playing=%d dac periods=%lu",
            (int)mixer_channel_status(PCM_MIXER_CHAN_PLAYBACK),
            pcm_is_playing(), (unsigned long)yp3_pcm_completed_periods());
    /* since the last fmmon */
    unsigned long last = yp3_capture_stats.last_us;
    dev_log("mixer gaps: longest %lu ms, %lu over two periods, last call "
            "%lu ms ago",
            yp3_capture_stats.gap_max_us / 1000, yp3_capture_stats.late,
            last ? (*(volatile unsigned long *)SL6801_COUNTER_LO - last) /
                   1000 : 0);
    yp3_capture_stats.gap_max_us = 0;
    yp3_capture_stats.late = 0;
    return true;
}

#ifdef HAVE_BLUETOOTH
static bool sh_bt(char *arg)
{
    return bt_yp3_dev_cmd(arg);
}
#endif

static bool sh_vcore(char *arg)
{
    /* vcore [MV64 [MV192]]: core voltage per clock (0 = keep); no
     * arguments: log the state */
    char *end;
    unsigned mv64 = strtoul(arg, &end, 10);
    yp3_core_voltage_dev(mv64, strtoul(end, NULL, 10));
    return true;
}

static bool sh_prof(char *arg)
{
    /* prof start [callers] [irq N [late [US]]]: sample PCs (and stack
     * return addresses) at the tick, or at interrupt N (62: audio DMA;
     * with "late" only when the audio period is over US, default 1000,
     * microseconds late);
     * prof dump: print them */
    extern uint32_t *yp3_prof_pc;
    extern unsigned yp3_prof_size, yp3_prof_count;
    extern bool yp3_prof_on, yp3_prof_callers;
    extern unsigned yp3_prof_late;
    extern void yp3_prof_irq(int irq);
    static int prof_handle;
    static size_t prof_bytes;
    if (!strncmp(arg, "start", 5)) {
        yp3_prof_on = false;
        if (prof_handle <= 0) {
            /* what the heap has, up to PROF_MAX_BYTES (HE-AAC playback
             * leaves ~3 KB) */
            size_t avail = core_available();
            if (avail < 1024)
                return false;
            prof_bytes = MIN((size_t)PROF_MAX_BYTES, (avail - 64) & ~3u);
            prof_handle = core_alloc(prof_bytes);
            if (prof_handle <= 0)
                return false;
            core_pin(prof_handle);      /* the tick writes into it */
        }
        yp3_prof_pc = core_get_data(prof_handle);
        yp3_prof_size = prof_bytes / sizeof *yp3_prof_pc;
        yp3_prof_callers = !strncmp(arg, "start callers", 13);
        const char *late = strstr(arg, " late");
        yp3_prof_late = !late ? 0 : late[5] == ' ' ? atoi(late + 6) : 1000;
        const char *irq = strstr(arg, "irq ");
        yp3_prof_irq(irq ? atoi(irq + 4) : -1);
        yp3_prof_count = 0;
        yp3_prof_on = true;
    } else {
        if (prof_handle <= 0)
            return false;
        yp3_prof_on = false;
        yp3_prof_irq(-1);
        char line[100];
        for (unsigned i = 0; i < yp3_prof_count; i += 10) {
            int n = 0;
            for (unsigned j = i; j < i + 10 && j < yp3_prof_count; j++)
                n += snprintf(line + n, sizeof line - n, "%lx ",
                              (unsigned long)yp3_prof_pc[j]);
            line[n++] = '\n';
            dev_serial_write(line, n);
            sleep(1);
        }
        dev_log("prof: %u samples", yp3_prof_count);
        yp3_prof_size = 0;
        prof_handle = core_free(prof_handle);
    }
    return true;
}

static bool sh_watch(char *arg)
{
    /* watch HEXADDR: arm a write watchpoint; watch sp SLOT: on that
     * thread's saved sp, hits only when it leaves its stack; watch: list
     * the hits */
    if (!strncmp(arg, "sp ", 3))
        yp3_dev_watch_thread_sp(atoi(arg + 3));
    else if (*arg)
        yp3_dev_watch(strtoul(arg, NULL, 16));
    else
        for (unsigned i = 0; i < MIN(yp3_watch_count, 16u); i++)
            dev_log("watch hit %u: pc %08lx lr %08lx value %08lx thread %x",
                        i, (unsigned long)yp3_watch_hits[i].pc,
                        (unsigned long)yp3_watch_hits[i].lr,
                        (unsigned long)yp3_watch_hits[i].value,
                        yp3_watch_hits[i].thread);
    dev_log("watch: %u hits", yp3_watch_count);
    return true;
}

static bool sh_amp(char *arg)
{
    extern void yp3_audiohw_amp(bool on);
    yp3_audiohw_amp(atoi(arg) != 0);
    return true;
}

static bool sh_load(char *arg)
{
    /* load [SECS]: idle (WFI) and per-thread shares of SECS (2) s */
    extern void yp3_dev_thread_load(int secs);
    yp3_dev_thread_load(*arg ? atoi(arg) : 2);
    return true;
}

static bool sh_norscan(char *arg)
{
    if (strncmp(arg, "stress ", 7))
        return false;
    char *save, *d = strtok_r(arg + 7, " ", &save);
    char *n = strtok_r(NULL, " ", &save);
    int div = d ? atoi(d) : 0;
    if (div < 2 || div > 16)    /* divisor field [11:8] holds 1..16 */
        return false;
    norscan_stress(div, n ? atoi(n) : 100);
    return true;
}

static int profat_seconds;   /* config prof= */
static long profat_stack[DEFAULT_STACK_SIZE / sizeof(long)];

static void profat_thread(void)
{
    /* USB activity every 5 s (for states without the shell). */
    sleep(profat_seconds * HZ);
    for (int w = 0; w < 20; w++) {
        struct yp3_usb_stats u0 = yp3_usb_stats;
        unsigned long i0 = yp3_idle_us;
        sleep(5 * HZ);
        dev_log("%ds idle %lu ms usb isr %lu ms, ep1 rx %lu tx %lu irqs",
                w * 5, (yp3_idle_us - i0) / 1000,
                (unsigned long)(yp3_usb_stats.isr_us - u0.isr_us) / 1000,
                (unsigned long)(yp3_usb_stats.src[0] - u0.src[0]),
                (unsigned long)(yp3_usb_stats.src[1] - u0.src[1]));
    }
    thread_exit();
}

void dev_target_boot(void)
{
    char v[16];
    sdpush();
    if (dev_config("prof", v, sizeof v)) {
        profat_seconds = atoi(v);
        create_thread(profat_thread, profat_stack, sizeof profat_stack, 0,
                      "yp3 prof" IF_PRIO(, PRIORITY_USER_INTERFACE)
                      IF_COP(, CPU));
    }
}

static const struct dev_cmd cmds[] = {
    { "pmu",       sh_pmu,        "pmu IDX [VAL]" },
    { "fm",        sh_fm,         "fm KHZ|off" },
    { "fmregs",    sh_fmregs,     "fmregs" },
    { "fmgain",    sh_fmgain,     "fmgain N" },
    { "micgain",   sh_micgain,    NULL },
    { "fmmon",     sh_fmmon,      "fmmon" },
#ifdef HAVE_BLUETOOTH
    { "bt",        sh_bt,         "bt rf|sbc|ctrl|hci ..." },
#endif
    { "vcore",     sh_vcore,      "vcore [MV64 [MV192]]" },
    { "prof",      sh_prof,       "prof start [callers]|dump" },
    { "watch",     sh_watch,      "watch [HEXADDR|sp SLOT]" },
    { "amp",       sh_amp,        "amp 0|1" },
    { "load",      sh_load,       "load [S]" },
    { "norscan",   sh_norscan,    "norscan stress DIV [N]" },
};

const struct dev_cmd *dev_target_cmd(size_t i)
{
    return i < ARRAYLEN(cmds) ? &cmds[i] : NULL;
}
