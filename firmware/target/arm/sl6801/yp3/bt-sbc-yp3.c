/* SPDX-License-Identifier: GPL-2.0-or-later */
/* bt_sbc_* (firmware/export/bt_sbc.h) on the boot ROM's SBC encoder.
 * The ROM codec is float code (VFP), reached through
 * Thumb function pointers.
 *
 * Memory: ROM open allocates three blocks through ROM malloc 0x11044, whose
 * hook slot we fill for the duration of the call. The blocks come from one
 * pinned arena that bt_sbc_reserve() takes, sized for the largest
 * configuration, and bt_sbc_release() frees.
 *
 * FPU: the core is soft-float with FP access off and no FP state stacking
 * (crt0.S). bt_sbc_encode() turns access on around the ROM call. If it was
 * already on, a plugin's main thread owns the FPU (plugin_fpu.h) and may
 * have been interrupted: s0-s31 and FPSCR are saved and restored. */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "config.h"
#include "system.h"
#include "core_alloc.h"
#include "core_alloc_top.h"
#include "bluetooth.h"
#include "bt_sbc.h"
#include "bt-yp3.h"
#include "dev-yp3.h"
#include "mmio-sl6801.h"
#ifdef YP3_DEV
#include <stdio.h>
#include <stdlib.h>
#include "file.h"
#include "kernel.h"
#include "fixedpoint.h"
#endif

#ifdef HAVE_BLUETOOTH_SBC_ENCODER
/* ---- boot ROM SBC codec (ROM functions are Thumb: call with bit 0 set) - */
/* void *open(const struct rom_sbc_params *p): context, or NULL when an
 * allocation failed (ROM 0x1dbfc). */
#define ROM_SBC_OPEN            0x0001dbfc
/* int encode(void *ctx, const int16_t *pcm[channels], int *samples,
 *            uint8_t **frame, int *frame_bytes) (ROM 0x1e358): pcm is
 * planar, one pointer per channel, advanced past the samples taken;
 * *samples (per channel) is reduced by the samples taken. It stops after
 * at most one frame: *frame then points into the context's frame buffer
 * and *frame_bytes is its length (0 while a frame is still incomplete).
 * Returns 0, or -1 when frame packing failed (ROM 0x1daea refuses a
 * bitpool above the mode's limit). */
#define ROM_SBC_ENCODE          0x0001e358
/* What open allocates (ROM 0x1dc00, 0x1dc34, 0x1dc8e): the context (fixed
 * 0x490 bytes; it holds 16 blocks x 2 channels x 8 subbands of int32
 * subband samples from +0x90), the frame buffer (frame length) and the
 * analysis history (40 bytes per channel and subband). */
#define ROM_SBC_CTX_BYTES       0x490u
#define ROM_SBC_HISTORY_BYTES(ch, sb) (40u * (ch) * (sb))
/* ROM allocator hooks (ROM BSS, zeroed by ROM reset). ROM malloc 0x11044
 * tail-calls the malloc slot with r0 = size when it is non-zero; otherwise
 * it bump-allocates from a ROM arena that is empty after reset (NULL). The
 * encoder relies on zeroed memory from the allocator (counters at
 * +0x0a/+0x0b, the analysis history). ROM free 0x110c0
 * tail-calls the free slot, or does nothing when it is zero; open calls it
 * (via 0x1dbda) when its second or third allocation fails. The other slots
 * (0x8009a8 raw alloc, 0x8009b0/b4 largest block, 0x8009c8 free space) are
 * not used by the codec. */
#define ROM_MALLOC_HOOK         0x008009ac
#define ROM_FREE_HOOK           0x008009b8
/* ROM malloc rounds every request up to a word (ROM 0x11020). */
#define ROM_ALLOC_ALIGN         4u
/* The ROM's bitpool limit per subband (ROM 0x1db30): mono and dual channel
 * code each channel on its own, stereo and joint stereo both together. */
#define ROM_SBC_BITPOOL_MONO    16
#define ROM_SBC_BITPOOL_STEREO  32

/* SBC (A2DP spec, Appendix B) */
#define SBC_HEADER_BYTES        4   /* sync, parameters, bitpool, CRC */
#define SBC_SCALE_FACTOR_BITS   4   /* per subband and channel */
#define SBC_MAX_CHANNELS        2
#define SBC_MAX_SUBBANDS        8
#define A2DP_BITPOOL_MIN        2
#define A2DP_BITPOOL_MAX        250u

/* The 7-byte parameter block open reads (ROM 0x1dc0c..0x1dc6a). The codes
 * are the SBC header's: open derives blocks = (code + 1) * 4, subbands 4 or
 * 8 and the channel count (1 for mono) from them. */
struct rom_sbc_params {
    uint8_t freq;       /* enum bt_sbc_freq (header bits 7:6) */
    uint8_t blocks;     /* blocks / 4 - 1 */
    uint8_t subbands;   /* 0: 4, 1: 8 */
    uint8_t mode;       /* enum bt_sbc_mode */
    uint8_t alloc;      /* enum bt_sbc_alloc */
    uint8_t bitpool;
    uint8_t msbc;       /* non-zero: mSBC (16 kHz mono, bitpool 26); 0 */
};

typedef void *rom_sbc_open_fn(const struct rom_sbc_params *p);
typedef int rom_sbc_encode_fn(void *ctx, const int16_t **pcm, int *samples,
                              uint8_t **frame, int *frame_bytes);
#define ROM_CALL(type, addr)    ((type *)((uintptr_t)(addr) | 1))

/* ---- FPU access (ARMv7-M; crt0.S turns it off at boot) ---- */
#define CPACR_CP10_CP11         0x00f00000u /* bits 23:20: full FP access */
/* FPSCR for the ROM: round to nearest, no flush-to-zero, no default NaN
 * (the architectural defaults; the encoder never writes FPSCR). */
#define ROM_FPSCR               0u

/* FP state of an interrupted FPU owner. The ROM is AAPCS code (it saves
 * s16-s31 itself and this path only uses s9-s15); all of s0-s31 are saved
 * anyway, so nothing depends on that. Kept at the end of the arena, not on
 * the interrupt stack. */
struct fp_save {
    uint32_t s[32];
    uint32_t fpscr;
};

static int arena;               /* bt_sbc_reserve()'s block, 0: none */

static struct {
    void *ctx;                  /* ROM context inside the arena, NULL: closed */
    struct fp_save *fp;         /* after the ROM's blocks in the arena */
    uint8_t *next;              /* arena allocation pointer (open only) */
    size_t left;
    uint8_t channels, subbands, blocks;
    uint16_t frame_bytes;
} enc;

/* ROM malloc hook while open runs: hands out the zeroed arena in order. */
static void *rom_malloc(size_t size)
{
    size = ALIGN_UP(size, ROM_ALLOC_ALIGN);
    if (size > enc.left)
        return NULL;
    void *p = enc.next;
    enc.next += size;
    enc.left -= size;
    return p;
}

/* ROM free hook while open runs: the arena is released as a whole. */
static void rom_free(void *p)
{
    (void)p;
}

/* SBC frame length in bytes (A2DP spec 12.9; ROM 0x1db9c computes the
 * same for open): header, scale factors, then the samples (bitpool bits
 * per block and channel; stereo modes share one bitpool and joint stereo
 * adds one join bit per subband), rounded up to a byte. */
static size_t frame_length(const struct bt_sbc_params *p, unsigned channels)
{
    size_t n = SBC_HEADER_BYTES +
               SBC_SCALE_FACTOR_BITS * p->subbands * channels / 8;
    if (p->mode == BT_SBC_MONO || p->mode == BT_SBC_DUAL)
        return n + (p->blocks * channels * p->bitpool + 7) / 8;
    return n + ((p->mode == BT_SBC_JOINT ? p->subbands : 0) +
                p->blocks * p->bitpool + 7) / 8;
}

/* The highest bitpool for a mode and subband count: the ROM's and A2DP's */
static unsigned bitpool_max(unsigned mode, unsigned subbands)
{
    return MIN((mode <= BT_SBC_DUAL ? ROM_SBC_BITPOOL_MONO :
                ROM_SBC_BITPOOL_STEREO) * subbands, A2DP_BITPOOL_MAX);
}

/* The parameters bt_sbc.h allows, within those bitpool limits */
static bool params_valid(const struct bt_sbc_params *p)
{
    return p->freq <= BT_SBC_48000 && p->mode <= BT_SBC_JOINT &&
           (p->blocks == 4 || p->blocks == 8 || p->blocks == 12 ||
            p->blocks == 16) &&
           (p->subbands == 4 || p->subbands == 8) &&
           p->alloc <= BT_SBC_SNR && p->bitpool >= A2DP_BITPOOL_MIN &&
           p->bitpool <= bitpool_max(p->mode, p->subbands);
}

/* What open allocates for a frame length, plus the FP save area */
static size_t arena_size(size_t frame, unsigned channels, unsigned subbands)
{
    return ROM_SBC_CTX_BYTES + ALIGN_UP(frame, ROM_ALLOC_ALIGN) +
           ROM_SBC_HISTORY_BYTES(channels, subbands) + sizeof(struct fp_save);
}

bool bt_sbc_reserve(void)
{
    if (arena > 0)
        return true;
    /* The largest: 16 blocks, 8 subbands, two channels, each mode at its
     * highest bitpool */
    size_t frame = 0;
    for (unsigned mode = BT_SBC_MONO; mode <= BT_SBC_JOINT; mode++) {
        const struct bt_sbc_params p = {
            .mode = mode, .blocks = 16, .subbands = 8,
            .bitpool = bitpool_max(mode, 8),
        };
        frame = MAX(frame, frame_length(&p, mode == BT_SBC_MONO ? 1 : 2));
    }
    int handle = core_alloc_top(arena_size(frame, 2, 8));
    if (handle <= 0)
        return false;
    core_pin(handle);           /* the context holds pointers into it */
    arena = handle;
    return true;
}

void bt_sbc_release(void)
{
    bt_sbc_close();
    if (arena > 0)
        arena = core_free(arena);
}

bool bt_sbc_open(const struct bt_sbc_params *params)
{
    bt_sbc_close();
    if (!params_valid(params) || arena <= 0)
        return false;
    unsigned channels = params->mode == BT_SBC_MONO ? 1 : 2;
    size_t frame = frame_length(params, channels);
    size_t size = arena_size(frame, channels, params->subbands);
    enc.next = core_get_data(arena);
    enc.left = size;
    memset(enc.next, 0, size);

    const struct rom_sbc_params rp = {
        .freq = params->freq,
        .blocks = params->blocks / 4 - 1,
        .subbands = params->subbands == 8,
        .mode = params->mode,
        .alloc = params->alloc,
        .bitpool = params->bitpool,
        .msbc = 0,
    };
    /* Hooks only around the call, IRQs masked, so no other ROM malloc user
     * can reach the arena; open itself uses no FP and only sets fields. */
    int irq = disable_irq_save();
    uint32_t old_malloc = REG(ROM_MALLOC_HOOK), old_free = REG(ROM_FREE_HOOK);
    REG(ROM_MALLOC_HOOK) = (uintptr_t)rom_malloc;
    REG(ROM_FREE_HOOK) = (uintptr_t)rom_free;
    void *ctx = ROM_CALL(rom_sbc_open_fn, ROM_SBC_OPEN)(&rp);
    REG(ROM_MALLOC_HOOK) = old_malloc;
    REG(ROM_FREE_HOOK) = old_free;
    restore_irq(irq);

    if (!ctx || enc.left != sizeof(struct fp_save)) {
        dev_log("sbc open: ctx %p, %u of %u arena bytes unused",
                    ctx, (unsigned)enc.left, (unsigned)size);
        memset(&enc, 0, sizeof enc);
        return false;
    }
    enc.ctx = ctx;
    enc.fp = (struct fp_save *)enc.next;
    enc.channels = channels;
    enc.subbands = params->subbands;
    enc.blocks = params->blocks;
    enc.frame_bytes = frame;
    return true;
}

size_t bt_sbc_frame_bytes(void)
{
    return enc.ctx ? enc.frame_bytes : 0;
}

size_t bt_sbc_frame_samples(void)
{
    return enc.ctx ? (size_t)enc.blocks * enc.subbands : 0;
}

/* Returns the CPACR value to restore. VFP instructions run only after
 * access is on (yp3dev/fpucheck.py's rule for CPACR switchers). */
static uint32_t fpu_begin(struct fp_save *save)
{
    uint32_t cpacr = REG(SCB_CPACR);
    if (!(cpacr & CPACR_CP10_CP11)) {
        REG(SCB_CPACR) = cpacr | CPACR_CP10_CP11;
        __asm__ volatile("dsb\n\tisb" ::: "memory");
    } else {
        __asm__ volatile(".fpu fpv4-sp-d16\n\t"
                         "vstmia %1, {s0-s31}\n\t"
                         "vmrs %0, fpscr\n\t"
                         ".fpu softvfp"
                         : "=r"(save->fpscr) : "r"(save->s) : "memory");
    }
    __asm__ volatile(".fpu fpv4-sp-d16\n\t"
                     "vmsr fpscr, %0\n\t"
                     ".fpu softvfp" :: "r"(ROM_FPSCR));
    return cpacr;
}

static void fpu_end(const struct fp_save *save, uint32_t cpacr)
{
    if (cpacr & CPACR_CP10_CP11) {
        __asm__ volatile(".fpu fpv4-sp-d16\n\t"
                         "vldmia %0, {s0-s31}\n\t"
                         "vmsr fpscr, %1\n\t"
                         ".fpu softvfp"
                         :: "r"(save->s), "r"(save->fpscr) : "memory");
    } else {
        REG(SCB_CPACR) = cpacr;
        __asm__ volatile("dsb\n\tisb" ::: "memory");
    }
}

/* One block (subbands samples per channel) per ROM call, deinterleaved
 * into a small buffer: the ROM wants planar input, and a whole frame
 * would need 512 bytes of stack in interrupt context. The frame comes
 * out of the call that completes the last block. */
size_t bt_sbc_encode(const int16_t *pcm, uint8_t *out)
{
    if (!enc.ctx)
        return 0;
    rom_sbc_encode_fn *encode = ROM_CALL(rom_sbc_encode_fn, ROM_SBC_ENCODE);
    const unsigned ch = enc.channels, sb = enc.subbands;
    size_t written = 0;
    uint32_t cpacr = fpu_begin(enc.fp);
    for (unsigned blk = 0; blk < enc.blocks; blk++) {
        int16_t block[SBC_MAX_CHANNELS][SBC_MAX_SUBBANDS];
        const int16_t *planes[SBC_MAX_CHANNELS] = { block[0], block[1] };
        for (unsigned i = 0; i < sb; i++)
            for (unsigned c = 0; c < ch; c++)
                block[c][i] = *pcm++;
        int samples = sb, frame_bytes = 0;
        uint8_t *frame = NULL;
        if (encode(enc.ctx, planes, &samples, &frame, &frame_bytes) < 0 ||
            samples != 0)
            break;
        if (frame_bytes > 0) {
            if (blk == enc.blocks - 1u && frame_bytes == enc.frame_bytes) {
                memcpy(out, frame, frame_bytes);
                written = frame_bytes;
            }
            break;
        }
    }
    fpu_end(enc.fp, cpacr);
    return written;
}

/* Not supported: the ROM context fixes the bitpool at open, and its frame
 * buffer is sized for it. A new bitpool needs a new bt_sbc_open(). */
bool bt_sbc_set_bitpool(unsigned bitpool)
{
    (void)bitpool;
    return false;
}

void bt_sbc_close(void)
{
    /* ROM close (0x1dbda) would only hand the three blocks to ROM free;
     * the arena stays reserved */
    memset(&enc, 0, sizeof enc);
}
#endif /* HAVE_BLUETOOTH_SBC_ENCODER */

#ifdef YP3_DEV
/* Dev shell "bt sbc ...": A2DP's usual high-quality setting (44.1 kHz
 * joint stereo, 16 blocks, 8 subbands, loudness, bitpool 53: 119-byte
 * frames, 328 kbit/s), fed with a sine (left -6 dBFS, right -12 dBFS so
 * a channel swap shows). */
#define DEV_RATE                44100u
#define DEV_FRAME_SAMPLES       128u            /* 16 blocks x 8 subbands */
#define DEV_FRAME_US            (DEV_FRAME_SAMPLES * 1000000u / DEV_RATE)
#define DEV_FRAME_MAX           128u            /* 119 bytes needed */
#define DEV_FILE_BATCH          4u              /* frames per SD write */
#define DEV_YIELD_FRAMES        32u             /* let the UI feed the wdog */
#define DEV_MAX_SECONDS         600u
#define DEV_TONE_HZ             1000            /* default tone */
#define DEV_TEST_FRAMES         100             /* default test length */
#define DEV_HEX_BYTES           16              /* frame bytes test logs */

static const struct bt_sbc_params dev_params = {
    .freq = BT_SBC_44100, .mode = BT_SBC_JOINT, .blocks = 16, .subbands = 8,
    .alloc = BT_SBC_LOUDNESS, .bitpool = 53,
};

/* The encoder for a test, in memory of its own: one encoder at a time,
 * so not while the stack is on */
static bool dev_open(const char *what)
{
    if (bt_status() != BT_STATUS_OFF) {
        dev_log("%s: Bluetooth is on", what);
        return false;
    }
    if (bt_sbc_reserve() && bt_sbc_open(&dev_params))
        return true;
    bt_sbc_release();
    dev_log("%s: open failed", what);
    return false;
}

static char *next_arg(char **p)
{
    char *a = *p;
    while (*a == ' ')
        a++;
    if (!*a)
        return NULL;
    char *end = strchr(a, ' ');
    if (end)
        *end++ = '\0';
    *p = end ? end : a + strlen(a);
    return a;
}

struct dev_tone {
    uint32_t phase, step;
};

static bool dev_tone_init(struct dev_tone *t, const char *hz)
{
    unsigned f = hz ? strtoul(hz, NULL, 0) : DEV_TONE_HZ;
    if (!f || f >= DEV_RATE / 2)
        return false;
    t->phase = 0;
    t->step = ((uint64_t)f << 32) / DEV_RATE;
    return true;
}

static void dev_tone_frame(struct dev_tone *t, int16_t *pcm)
{
    for (unsigned i = 0; i < DEV_FRAME_SAMPLES; i++, t->phase += t->step) {
        long s = fp_sincos(t->phase, NULL) >> 17;   /* Q31 -> +-16384 */
        pcm[2 * i] = s;
        pcm[2 * i + 1] = s / 2;
    }
}

static uint32_t now_us(void)
{
    return REG(SL6801_COUNTER_LO);
}

/* bt sbc test [HZ [FRAMES]] */
static bool dev_test(char *arg)
{
    char *hz = next_arg(&arg), *count = next_arg(&arg);
    unsigned frames = count ? strtoul(count, NULL, 0) : DEV_TEST_FRAMES;
    struct dev_tone tone;
    if (!dev_tone_init(&tone, hz) || !frames)
        return false;
    if (!dev_open("sbc test"))
        return true;
    int16_t pcm[2 * DEV_FRAME_SAMPLES];
    uint8_t out[DEV_FRAME_MAX];
    uint32_t total = 0, worst = 0;
    unsigned bad = 0;
    for (unsigned f = 0; f < frames; f++) {
        dev_tone_frame(&tone, pcm);
        uint32_t t0 = now_us();
        size_t n = bt_sbc_encode(pcm, out);
        uint32_t dt = now_us() - t0;
        total += dt;
        worst = MAX(worst, dt);
        if (n != bt_sbc_frame_bytes())
            bad++;
        if (f == 0) {
            char hex[3 * DEV_HEX_BYTES + 1];
            for (unsigned i = 0; i < DEV_HEX_BYTES; i++)
                snprintf(hex + 3 * i, 4, " %02x", out[i]);
            dev_log("sbc test: frame 0 (%u bytes):%s", (unsigned)n, hex);
        }
        if (f % DEV_YIELD_FRAMES == DEV_YIELD_FRAMES - 1)
            yield();
    }
    /* One frame lasts 128 / 44.1 kHz = 2902 us. */
    uint32_t avg = total / frames;
    dev_log("sbc test: %u frames, %u bytes/frame, %u bad, %lu us avg, "
                "%lu us max per frame (%lu.%lu%% of real time)",
                frames, (unsigned)bt_sbc_frame_bytes(), bad,
                (unsigned long)avg, (unsigned long)worst,
                (unsigned long)(avg * 1000 / DEV_FRAME_US / 10),
                (unsigned long)(avg * 1000 / DEV_FRAME_US % 10));
    bt_sbc_release();
    return true;
}

/* bt sbc file PATH SECONDS [HZ]: raw SBC frames, for the host
 * (ffmpeg -f sbc -i x.sbc x.wav, or sbcdec). */
static bool dev_file(char *arg)
{
    char *path = next_arg(&arg), *secs = next_arg(&arg), *hz = next_arg(&arg);
    unsigned seconds = secs ? strtoul(secs, NULL, 0) : 0;
    struct dev_tone tone;
    if (!path || !seconds || seconds > DEV_MAX_SECONDS ||
        !dev_tone_init(&tone, hz))
        return false;
    if (!dev_open("sbc file"))
        return true;
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        dev_log("sbc file: cannot create %s", path);
        bt_sbc_release();
        return true;
    }
    unsigned frames = seconds * DEV_RATE / DEV_FRAME_SAMPLES, bad = 0;
    size_t fb = bt_sbc_frame_bytes(), used = 0, bytes = 0;
    uint8_t batch[DEV_FILE_BATCH * DEV_FRAME_MAX];
    int16_t pcm[2 * DEV_FRAME_SAMPLES];
    for (unsigned f = 0; f < frames; f++) {
        dev_tone_frame(&tone, pcm);
        if (bt_sbc_encode(pcm, batch + used) != fb) {
            bad++;
            continue;
        }
        used += fb;
        if (used + fb > sizeof batch || f == frames - 1) {
            if (write(fd, batch, used) != (ssize_t)used) {
                dev_log("sbc file: write failed after %u bytes",
                            (unsigned)bytes);
                break;
            }
            bytes += used;
            used = 0;
        }
        if (f % DEV_YIELD_FRAMES == DEV_YIELD_FRAMES - 1)
            yield();
    }
    close(fd);
    bt_sbc_release();
    dev_log("sbc file %s: %u frames, %u bytes, %u bad",
                path, frames, (unsigned)bytes, bad);
    return true;
}

bool bt_sbc_yp3_dev(char *arg)
{
    char *cmd = next_arg(&arg);
    if (cmd && !strcmp(cmd, "test"))
        return dev_test(arg);
    if (cmd && !strcmp(cmd, "file"))
        return dev_file(arg);
    return false;
}
#endif
