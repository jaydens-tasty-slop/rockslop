/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef BT_SBC_H
#define BT_SBC_H
/* SBC encoder for the A2DP source (A2DP spec, Appendix B). A target with an
 * encoder in ROM or hardware defines HAVE_BLUETOOTH_SBC_ENCODER and
 * implements these; otherwise the portable encoder in firmware/bluetooth/
 * does. One encoder instance at a time. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Values as coded in the SBC frame header */
enum bt_sbc_freq { BT_SBC_16000, BT_SBC_32000, BT_SBC_44100, BT_SBC_48000 };
enum bt_sbc_mode { BT_SBC_MONO, BT_SBC_DUAL, BT_SBC_STEREO, BT_SBC_JOINT };
enum bt_sbc_alloc { BT_SBC_LOUDNESS, BT_SBC_SNR };

/* A2DP's bitpool range; the mode limits it further (bt_sbc_params) */
#define BT_SBC_BITPOOL_MIN  2
#define BT_SBC_BITPOOL_MAX  250

struct bt_sbc_params {
    uint8_t freq;       /* enum bt_sbc_freq */
    uint8_t mode;       /* enum bt_sbc_mode */
    uint8_t blocks;     /* 4, 8, 12 or 16 */
    uint8_t subbands;   /* 4 or 8 */
    uint8_t alloc;      /* enum bt_sbc_alloc */
    uint8_t bitpool;    /* BT_SBC_BITPOOL_MIN..MAX, and at most 16 * subbands
                         * (mono, dual channel) or 32 * subbands (stereo,
                         * joint stereo) */
};

/* The encoder's memory, taken when the stack powers on and kept until it
 * powers off: a headset that connects while something else holds the heap
 * (the recording screen) still gets its stream. False if no memory;
 * release closes the encoder first. */
bool bt_sbc_reserve(void);
void bt_sbc_release(void);

/* Set up the encoder in the reserved memory, or set it up again (encoding
 * stopped meanwhile). Returns false for unsupported parameters, or if
 * nothing is reserved. */
bool bt_sbc_open(const struct bt_sbc_params *params);

/* Bytes in one encoded frame, and PCM samples per channel it consumes
 * (blocks * subbands). Valid after a successful bt_sbc_open(). */
size_t bt_sbc_frame_bytes(void);
size_t bt_sbc_frame_samples(void);

/* Encode one frame from interleaved 16-bit PCM (one channel for mono,
 * otherwise two) into out (bt_sbc_frame_bytes() bytes). Callable from
 * interrupt context. Returns the bytes written, 0 on error. */
size_t bt_sbc_encode(const int16_t *pcm, uint8_t *out);

/* Change the bitpool for the following frames (A2DP lets the source vary
 * it within the configured range); bt_sbc_frame_bytes() follows. Callable
 * from interrupt context between frames. Returns false if the bitpool is
 * invalid or the encoder cannot change it. */
bool bt_sbc_set_bitpool(unsigned bitpool);

/* Stop encoding; the memory stays reserved */
void bt_sbc_close(void);

#endif /* BT_SBC_H */
