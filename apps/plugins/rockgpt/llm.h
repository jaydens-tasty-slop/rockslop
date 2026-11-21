/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Tiny llama2-style transformer inference with int8 weights.
 *
 * Weights live in a file (on the SD card); as many tensors as fit stay in
 * RAM and the rest are re-read for every token. No heap, no libm, no FPU:
 * matmuls and attention dot products are integer (ARMv7E-M SIMD where
 * available), the remaining float work is small enough for soft-float.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 ****************************************************************************/
#ifndef LLM_H
#define LLM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define LLM_MAGIC   0x4d4c4c59 /* "YLLM" */
#define LLM_VERSION 2
#define LLM_CHUNK   16384  /* most bytes per streamed read: one 32-sector SD command */
#define LLM_MAX_BATCH 8
#define LLM_BOS     1
#define LLM_EOS     2

/* On-disk header, little endian, followed (at tok_off, norm_off, tensor_off)
 * by the tokenizer, all float norm weights and the int8 tensors. Each tensor
 * is rows of cols int8 values (cols % 4 == 0), each row followed by its
 * float scale; the file is padded to whole 512-byte sectors. Tensor
 * order: embedding (vocab x dim), per layer wq wk wv wo w1 w2 w3, then the
 * classifier unless it is shared with the embedding. */
struct llm_header {
    uint32_t magic, version;
    uint32_t dim, hidden_dim, n_layers, n_heads, n_kv_heads, vocab_size;
    uint32_t seq_len, flags, n_params;
    uint32_t tok_off, tok_size;   /* f32 scores[V], u8 lens[V], bytes */
    uint32_t norm_off;            /* f32: per layer att, ffn; then final */
    uint32_t tensor_off;
    float rope_theta;
    char name[32];                /* display name */
    char prompt[64];              /* reply prompt, %s = user text */
    char stop[16];                /* reply ends at this string */
};
#define LLM_SHARED_CLASSIFIER 1

/* File access. read: n bytes at offset off, false on a short read.
 * Optional asynchronous reads (all reads go through them once set): start
 * queues a read and returns its ticket, wait blocks until that ticket and
 * all older ones are done (false if one failed). yield, if set, is called
 * between chunks of work so a reader thread can keep the card busy. */
struct llm_io {
    void *ctx;
    bool (*read)(void *ctx, uint32_t off, void *buf, uint32_t n);
    uint32_t (*start)(void *ctx, uint32_t off, void *buf, uint32_t n);
    bool (*wait)(void *ctx, uint32_t ticket);
    void (*yield)(void);
};

struct llm_qt {
    int8_t *q;                    /* NULL: streamed from the file */
    uint32_t off;
    uint16_t rows, cols;
};

struct llm_chunk {                /* rows r0.. of a streamed tensor */
    uint32_t start, len;          /* sector-aligned file range */
    uint16_t r0, nr, skip;        /* skip: bytes before row r0 */
};

struct llm_mem {                  /* a free region for llm_open */
    uint8_t *p;
    size_t size;
};

struct llm_stats {
    size_t resident, streamed;    /* weight bytes */
    size_t kv, other;             /* KV cache, everything else */
    size_t mem_left;
};

struct llm {
    struct llm_header h;
    struct llm_io io;
    int seq;                      /* usable context (<= h.seq_len) */
    int kv_dim, head_size;
    /* tokenizer */
    float *scores;
    uint16_t *tok_off;            /* vocab+1 offsets into tok_str */
    char *tok_str;
    uint16_t *tok_hash;           /* open addressing, 0 = empty, id+1 */
    int hash_mask;
    /* weights */
    float *norms;                 /* layer l: [2l], [2l+1]; final: [2L] */
    struct llm_qt emb, cls, *w;   /* w: 7 per layer */
    uint8_t *sbuf[2];             /* stream slots */
    struct llm_chunk *sched;      /* streamed chunks in order of use */
    int nsched;
    uint32_t ticket[2];           /* async read pending per slot */
    unsigned k;                   /* streamed tensors used so far */
    float *freq;                  /* RoPE, head_size/2 */
    /* state */
    float *x, *xb, *xb2, *hb, *hb2, *q, *att, *logits;
    int nb, act;                  /* batch rows, activation row length */
    int8_t *xq;                   /* quantized activations [nb][act] */
    uint32_t *xp;                 /* same, SIMD pair layout */
    float xs[LLM_MAX_BATCH];      /* their scales */
    int8_t *kq, *vq;              /* KV cache [layer][seq][kv_dim] */
    float *ks;                    /* K scales [layer][seq] */
    int32_t *vsx;                 /* V scales in Q24 */
    float *sigtab;                /* sigmoid, 512 steps on [-8, 8) */
    uint16_t *exp2tab;            /* 2^(-i/256) in Q15 */
    float rc[32], rs[32];         /* RoPE cos/sin for the current token */
    int *ids;                     /* top-k scratch */
    struct llm_stats st;
    struct llm_mem mem[2];
    bool oom;                     /* a take() failed while opening */
};

/* Returns NULL on success or a short error message. max_seq caps the KV
 * cache, max_batch the positions per llm_forward_batch (each costs
 * activation RAM); the memory regions must outlive the model. With
 * LLM_NO_TOKENIZER (a draft model) llm_encode and llm_piece are unusable. */
#define LLM_NO_TOKENIZER 1
const char *llm_open(struct llm *m, const struct llm_io *io,
                     struct llm_mem *mem, int nmem, int max_seq, int max_batch,
                     unsigned flags);

/* Runs one token at position pos; returns the logits (vocab_size). */
float *llm_forward(struct llm *m, int token, int pos);

/* Runs n <= max_batch tokens at positions pos0.. in one pass over the
 * weights. Returns n rows of logits if all, else one row for the last. */
float *llm_forward_batch(struct llm *m, const int *tokens, int n, int pos0,
                         bool all);

/* Encodes UTF-8 text (BPE with byte fallback, sentencepiece style dummy
 * prefix); returns the number of tokens written, <= max. */
int llm_encode(struct llm *m, const char *text, bool bos, int *tokens, int max);

/* Text of token tok following prev (NUL terminated, valid until the next
 * call). */
const char *llm_piece(struct llm *m, int prev, int tok);

/* Speculative decoding: draft d (fully resident, same vocabulary) proposes
 * up to k tokens, target t checks them in one pass (both need max_batch >
 * k). Both have seen every position before pos; last is the token at pos.
 * Writes 1..k+1 next tokens to out, distributed as sampling t alone at
 * temp (greedy if temp <= 0); returns how many. */
int llm_speculate(struct llm *t, struct llm *d, int last, int pos, int k,
                  float temp, uint32_t *rng, int *out);

/* Temperature + top-k sampling; temp <= 0 is greedy. */
int llm_sample(struct llm *m, float *logits, float temp, int topk,
               uint32_t *rng);

#endif
