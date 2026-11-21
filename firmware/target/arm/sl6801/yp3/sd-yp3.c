/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * SL6801 (YP3) SD host driver: cold card discovery, then synchronous
 * transfers on a 4-bit bus by descriptor DMA, up to 32 blocks per command
 * (CMD18/CMD25 with auto-stop). Buffers that are not word-aligned go
 * through a small aligned bounce buffer.
 * Polled, no interrupts.
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include "config.h"
#include "dev-yp3.h"
#include "system.h"
#include "kernel.h"
#include "mutex.h"
#include "sd.h"
#include "sdmmc.h"
#include "storage.h"
#include "gpio-yp3.h"
#include "mmio-sl6801.h"

/* ------------------------------------------------------------------ */
/* SD host controller at 0x40003000. Facts
 * come from stock firmware and our own testing. Many bit layouts (EVENT,
 * STATUS, command flags, the DMA block) also match the Synopsys
 * DesignWare MMC host, though the low register offsets differ; "DW:" notes give that role as a hint only. */

/* +0x00 control. */
#define SD_CTRL       0x40003000u
#define SD_CTRL_RESET_ALL  0x7u        /* bits 2:0 reset, set together at cold
                                          reset (DW: controller/FIFO/DMA) */
#define SD_CTRL_BIT2       (1u << 2)   /* self-clears when the reset is done
                                          (all of 2:0 clear; we poll only
                                          bit 2) */
#define SD_CTRL_DMA        (1u << 25)  /* DMA handshake enable; must be
                                          clear while idle */
#define SD_CTRL_MODE_MASK  0xc0000000u /* bits 31:30: 1 at reset; */
#define SD_CTRL_MODE_1     0x40000000u /* meaning of the values unknown */

/* +0x04 command, +0x08 argument. Writing a command word with START set
 * issues it; START reads 1 until the controller has taken the command. */
#define SD_CMD        0x40003004u
#define SD_ARG        0x40003008u
#define CMD_START      0x80000000u
/* START | bit 21 (no index) is issued after a controller reconfiguration;
 * we issue it only after the cold reset, not after select_data_clock().
 * Role unknown (DW: update clock registers only, nothing sent to the
 * card). */
#define SD_CMD_BIT21_ONLY  0x00200000u

/* Command word low bits ("packed" commands, values from stock firmware).
 * Bit roles read off which commands carry them: */
#define SD_CMD_INDEX(n)    ((uint32_t)(n))  /* bits 5:0: command number */
#define SD_CMDF_RESP       (1u << 6)   /* every command with a response */
#define SD_CMDF_LONG       (1u << 7)   /* 136-bit R2: CMD2, CMD9 only */
#define SD_CMDF_BIT8       (1u << 8)   /* most commands, not CMD0/ACMD6
                                          (DW: check response CRC) */
#define SD_CMDF_DATA       (1u << 9)   /* data commands CMD17/18/24/25 */
#define SD_CMDF_BIT10      (1u << 10)  /* writes and most non-data commands,
                                          not reads (DW: direction = write) */
#define SD_CMDF_AUTOSTOP   (1u << 12)  /* controller sends CMD12 itself after
                                          the last block */
#define SD_CMDF_BIT13      (1u << 13)  /* all commands except CMD12
                                          (DW: wait for previous data) */
#define SD_CMDF_BIT15      (1u << 15)  /* CMD0 only (DW: send the power-up
                                          init clocks first) */
/* Flag set for ordinary (non-data) commands with a response. */
#define SD_CMDF_STD        (SD_CMDF_RESP | SD_CMDF_BIT8 | SD_CMDF_BIT10 | \
                            SD_CMDF_BIT13)
#define SD_CMDF_READ       (SD_CMDF_RESP | SD_CMDF_BIT8 | SD_CMDF_DATA | \
                            SD_CMDF_BIT13)
#define SD_CMDF_WRITE      (SD_CMDF_READ | SD_CMDF_BIT10)

#define CMD0_PACKED   (SD_CMD_INDEX(0) | SD_CMDF_BIT10 | SD_CMDF_BIT13 | \
                       SD_CMDF_BIT15)                          /* 0xa400 */
#define CMD2_PACKED   (SD_CMD_INDEX(2) | SD_CMDF_STD | \
                       SD_CMDF_LONG)               /* 0x25c2 */
#define CMD3_PACKED   (SD_CMD_INDEX(3) | SD_CMDF_STD)          /* 0x2543 */
#define ACMD6_PACKED  (SD_CMD_INDEX(6) | SD_CMDF_RESP | SD_CMDF_BIT10 | \
                       SD_CMDF_BIT13)              /* 0x2446 */
#define CMD7_PACKED   (SD_CMD_INDEX(7) | SD_CMDF_STD)          /* 0x2547 */
#define CMD8_PACKED   (SD_CMD_INDEX(8) | SD_CMDF_STD)          /* 0x2548 */
#define CMD9_PACKED   (SD_CMD_INDEX(9) | SD_CMDF_STD | \
                       SD_CMDF_LONG)               /* 0x25c9 */
#define CMD13_PACKED  (SD_CMD_INDEX(13) | SD_CMDF_STD)         /* 0x254d */
#define CMD16_PACKED  (SD_CMD_INDEX(16) | SD_CMDF_STD)         /* 0x2550 */
#define CMD17_PACKED  (SD_CMD_INDEX(17) | SD_CMDF_READ)        /* 0x2351 */
#define CMD24_PACKED  (SD_CMD_INDEX(24) | SD_CMDF_WRITE)       /* 0x2758 */
#define CMD25_AUTOSTOP_PACKED \
        (SD_CMD_INDEX(25) | SD_CMDF_WRITE | SD_CMDF_AUTOSTOP)  /* 0x3759 */
#define ACMD41_PACKED (SD_CMD_INDEX(41) | SD_CMDF_STD)         /* 0x2569 */
#define CMD55_PACKED  (SD_CMD_INDEX(55) | SD_CMDF_STD)         /* 0x2577 */

/* +0x0c block length, +0x10 total byte count of the data phase, +0x14
 * timeout. Stock firmware writes 0xffffff40 (DW: data timeout 31:8, response
 * timeout 7:0); we write all ones. */
#define SD_BLEN       0x4000300cu
#define SD_BCNT       0x40003010u
#define SD_TIMEOUT    0x40003014u
#define SD_TIMEOUT_MAX 0xffffffffu

/* +0x18 bus width: bit 0 = 4-bit data bus (set after ACMD6). */
#define SD_WIDTH      0x40003018u
#define SD_WIDTH_4BIT  (1u << 0)

/* +0x34 events, write 1 to clear. */
#define SD_EVENT      0x40003034u
#define SD_EVENT_CMD_DONE   (1u << 2)   /* command finished (response
                                           received, if one was expected;
                                           CMD0 waits on it too) */
#define SD_EVENT_DATA_DONE  (1u << 3)   /* data phase finished */
#define SD_EVENT_BIT8       (1u << 8)   /* error; DW: response
                                           timeout */
#define SD_EVENT_BIT9       (1u << 9)   /* error; DW: data read
                                           timeout */
#define SD_EVENT_AUTOSTOP_DONE (1u << 14) /* auto-stop CMD12 done */
#define SD_EVENT_BIT31      (1u << 31)  /* role unknown; see below */
#define EVENT_ERROR    (SD_EVENT_BIT8 | SD_EVENT_BIT9)
/* Acknowledge-everything value: bits 1, 2, 6-13 and 15. */
#define SD_EVENT_ACK_ALL    0xbfc6u
/* After a data transfer every pending bit is acknowledged except these
 * two. */
#define SD_EVENT_KEEP       (SD_EVENT_BIT31 | SD_EVENT_CMD_DONE)

/* Error bits of EVENT (F-0002, F-0003): everything in the acknowledge-all
 * value except command done. An R3 reply has no CRC and an all-ones index,
 * so bits 1 and 6 are set on good replies; it checks the rest only. */
#define SD_EVENT_ERRORS     0xbfc2u
#define SD_EVENT_ERRORS_R3  0xbf80u

/* +0x38..+0x44 response words, most significant first: a short response
 * is RESP0, a 136-bit one runs from +0x44 (bits 127:96) down to +0x38. */
#define SD_RESP0      0x40003038u

/* +0x48 status. Bits 16:11 hold the index of the last response. */
#define SD_STATUS     0x40003048u
#define SD_STATUS_RESP_INDEX(v) (((v) >> 11) & 0x3fu)

/* +0x80..+0x90: the block's own descriptor DMA. */
#define SD_DMA_CTRL   0x40003080u
#define SD_DMA_CTRL_ENABLE  (1u << 7)   /* cleared to stop */
#define SD_DMA_CTRL_ON      0x83u       /* enable, plus bits 1:0 (F-0010;
                                           DW: soft reset + fixed burst) */
#define SD_DMA_START  0x40003084u       /* write 1: fetch the descriptor */
#define SD_DMA_STATUS 0x4000308cu       /* write 1 to clear */
#define SD_DMA_DESC   0x40003088u       /* first descriptor (F-0010) */
#define SD_DMA_IEN    0x40003090u       /* 3 (F-0010; DW: TX/RX done) */
#define SD_DMA_IEN_TXRX 3u
#define SD_DMA_STATUS_TX_DONE (1u << 0)
#define SD_DMA_STATUS_RX_DONE (1u << 1)
#define SD_DMA_STATUS_BIT13   (1u << 13) /* stays set after a transfer, to
                                            be ignored (DW IDSTS: 16:13
                                            read-only DMA FSM state) */

/* +0x100: bit 1 set by the cold setup and around DMA writes, clear
 * otherwise. Role unknown (DW: busy-clear interrupt). */
#define SD_REG_100    0x40003100u
#define SD_REG_100_BIT1 (1u << 1)

/* Clock selector 0x11 (card clock) and clock gate 36 (the controller). */
#define SD_CLOCK_SEL   CLK_SEL_11
/* Selector 0x11 fields: bit 4 clear = source 10 (configured as
 * 24 MHz, unmeasured); divider field 19:16 plus bit 8, 7 + bit 8 = /64. */
#define SD_CLK_SOURCE_BIT  (1u << 4)
#define SD_CLK_DIV_MASK    ((15u << 16) | (1u << 8))
#define SD_CLK_DIV_64      ((7u << 16) | (1u << 8))


/* Card-side values from the SD Physical Layer spec. R1 card status: */
#define R1_CARD_ERROR  0xfdfd8008u  /* bits 31:26, 24:18, 16, 15 and 3:
                                       error flags (lock is separate; bit 18
                                       is reserved in the current spec) */
#define R1_CARD_LOCKED (1u << 25)
#define R6_ERROR       0xe000u      /* R6 low bits: COM_CRC, ILLEGAL, ERROR
                                       (F-0008) */
#define R1_READY       (1u << 8)    /* READY_FOR_DATA */
#define R1_STATE_MASK  (0xfu << 9)  /* CURRENT_STATE */
#define R1_STATE_TRAN  (4u << 9)    /* transfer state */
#define R1_STATE(r)    (((r) >> 9) & 15u)
#define R1_STATE_TRAN_VALUE 4u
/* CMD8: voltage 2.7-3.6 V (bits 11:8 = 1), check pattern 0xaa; the card
 * echoes both in the low 12 bits. */
#define CMD8_ARG       0x1aau
#define CMD8_ECHO_MASK 0xfffu
/* ACMD41: HCS (we take SDHC, bit 30) and the 2.7-3.6 V window (23:15). The
 * OCR reply has bit 31 set once power-up is done, bit 30 = CCS (SDHC). */
#define ACMD41_ARG     0x40ff8000u
#define OCR_READY      (1u << 31)
#define OCR_CCS_SHIFT  30
#define ACMD6_BUS_4BIT 2u           /* ACMD6 argument: 4-bit bus */
#define RCA_SHIFT      16           /* RCA sits in argument bits 31:16 */
#define BYTE_ADDR_SHIFT 9           /* SDSC cards take byte addresses */
/* CSD_STRUCTURE (bits 127:126): 0 = v1 (SDSC), 1 = v2 (SDHC/SDXC). */
#define CSD_V1 0u
#define CSD_V2 1u

/* Poll budgets. Data transfers get a large allowance (shared by the whole
 * transfer, a quarter per phase); all other operations use the small one. */
#define POLL_LIMIT      65536u
#define DATA_POLL_LIMIT 4194304u
#define PHASE_LIMIT(op) ((op)->limit / 4u)

/* Cold-path loop budgets: register reads (or ACMD41 tries), not time; only
 * a guard against a dead controller or card. */
#define RST_BUDGET        1024u
#define CMD_BUDGET        1024u
#define OCR_BUDGET        1000u     /* CMD55+ACMD41 rounds until power-up */
#define CLOCK_DELAY_BUDGET 65536u

/* Times in counter ticks (1 us). */
#define GATE_OFF_US          5000u
#define WRITE_READY_DELAY_US 100u   /* between "card busy" CMD13 polls */
#define WRITE_BUSY_POLLS     10000u /* x >= 100 us: give up after >= 1 s */

enum {
    SD_E_ARG = -1, SD_E_TIMEOUT = -2, SD_E_STATUS = -3, SD_E_RESPONSE = -4,
    SD_E_R1 = -5, SD_E_FIFO = -6, SD_E_STATE = -7, SD_E_UNSUPPORTED = -8,
};

enum sd_state { STATE_UNINIT, STATE_READY, STATE_FAILED };

static struct mutex sd_mutex;
static tCardInfo card_info;
static enum sd_state sd_state;
static bool sd_busy;
static long last_activity;
static uint32_t card_sectors, card_rca;
static bool card_sector_addressed;

/* ------------------------------------------------------------------ */
/* Bounded polling of the data-path controller state.                  */

struct op {
    uint32_t polls;
    uint32_t limit;
};

static inline __attribute__((always_inline))
uint32_t read_op(struct op *op, uint32_t address)
{
    if (op->polls >= op->limit) {
        op->polls = op->limit + 1u;
        return 0xffffffffu;
    }
    ++op->polls;
    return REG(address);
}

static int wait_command(struct op *op)
{
    for (uint32_t i = 0; i < PHASE_LIMIT(op); ++i) {
        uint32_t v = read_op(op, SD_CMD);
        if (op->polls > op->limit)
            return SD_E_TIMEOUT;
        if ((v & CMD_START) == 0u)
            return 0;
    }
    return SD_E_TIMEOUT;
}

static int wait_response(struct op *op, uint32_t expected_index,
                         bool require_tran, uint32_t *response)
{
    for (uint32_t i = 0; i < PHASE_LIMIT(op); ++i) {
        uint32_t event = read_op(op, SD_EVENT);
        if (op->polls > op->limit)
            return SD_E_TIMEOUT;
        if (event & EVENT_ERROR)
            return SD_E_STATUS;
        if (event & SD_EVENT_CMD_DONE) {
            uint32_t status = read_op(op, SD_STATUS);
            if (op->polls > op->limit)
                return SD_E_TIMEOUT;
            if (SD_STATUS_RESP_INDEX(status) != expected_index)
                return SD_E_RESPONSE;
            *response = read_op(op, SD_RESP0);
            if (op->polls > op->limit)
                return SD_E_TIMEOUT;
            if ((*response & (R1_CARD_ERROR | R1_CARD_LOCKED)) != 0u)
                return SD_E_R1;
            if (require_tran && ((*response & R1_READY) == 0u ||
                (*response & R1_STATE_MASK) != R1_STATE_TRAN))
                return SD_E_R1;
            return 0;
        }
    }
    return SD_E_TIMEOUT;
}

/* Submit a command and collect its R1; ACK the response in EVENT. */
static int send_command(struct op *op, uint32_t command, uint32_t argument,
                        uint32_t expected_index, bool require_tran,
                        uint32_t *response)
{
    int rc = wait_command(op);
    if (rc)
        return rc;
    REG(SD_ARG) = argument;
    REG(SD_CMD) = CMD_START | command;
    rc = wait_command(op);
    if (rc)
        return rc;
    rc = wait_response(op, expected_index, require_tran, response);
    if (rc)
        return rc;
    REG(SD_EVENT) = EVENT_ERROR | SD_EVENT_CMD_DONE;
    return 0;
}

/* Controller quiescent: no command, no event, DMA and interrupts off. */
static inline __attribute__((always_inline)) bool controller_idle(void)
{
    /* DMA status bit 13 stays set after a descriptor DMA; ignore it. */
    return (REG(SD_CMD) & CMD_START) == 0u &&
           (REG(SD_DMA_CTRL) & SD_DMA_CTRL_ENABLE) == 0u &&
           (REG(SD_DMA_STATUS) & ~SD_DMA_STATUS_BIT13) == 0u &&
           REG(SD_EVENT) == 0u && REG(SD_REG_100) == 0u;
}

/* ------------------------------------------------------------------ */
/* DMA transfer (CMD17/CMD24, or CMD18/CMD25 with auto-stop) through the
 * SD block's own descriptor DMA. */

#define CMD18_AUTOSTOP_PACKED \
        (SD_CMD_INDEX(18) | SD_CMDF_READ | SD_CMDF_AUTOSTOP)   /* 0x3352 */
/* Stock firmware moves at most 8 blocks per command, one 4 KiB
 * descriptor; we chain descriptors of up to DESC_BLOCKS each. Measured (dev `sdtest 1024
 * 32`, 2026-10-18): SD time per MiB written 404 ms at 8 blocks, 177 at 32,
 * 161 at 64 (card busy polls 417 -> 121 -> 105); reads 229 -> 160 -> 156. */
#define DESC_BLOCKS     8u
#define MAX_DESCS       8u      /* up to 64 blocks (32 KiB) per command */
#define DMA_MAX_BLOCKS  32u     /* blocks per DMA command */
/* Descriptor: { flags, byte count, buffer, next descriptor }. Flags are
 * 0x10 (role unknown; DW: DES0 bit 4 CH, word 3 is a chain
 * pointer) and sets bit 31 to hand the descriptor to the DMA
 * (owner). Ours chains to itself. */
#define SD_DESC_FLAGS 0x10u
#define SD_DESC_OWN   0x80000000u
#define DMA_TIMEOUT_US 1000000u /* yielding wait: give up after 1 s; a
                                   non-yielding wait is bounded by polls */

static uint32_t dma_desc[MAX_DESCS][4] __attribute__((aligned(16)));

/* The DMA moves whole words, so a buffer that is not word-aligned (file
 * data after an odd-sized header, e.g. AIFF's 54 bytes) is copied through
 * here. Measured (dev `sdtest 2048 16 odd`, 2026-10-18): the PIO this
 * replaced wrote 125 KB/s, too slow for stereo recording (176 KB/s). */
#define BOUNCE_BLOCKS 4u
static uint32_t bounce[BOUNCE_BLOCKS * SD_BLOCK_SIZE / 4u];

static void dma_stop(void)
{
    REG(SD_CTRL) &= ~SD_CTRL_DMA;
    REG(SD_DMA_CTRL) &= ~SD_DMA_CTRL_ENABLE;
    uint32_t status = REG(SD_DMA_STATUS);
    if (status)
        REG(SD_DMA_STATUS) = status;
}

/* 100 us between "card busy" CMD13 polls. Programming can take tens of ms
 * (FAT/resume writes at every track change); threads yield meanwhile so the
 * codec keeps the small PCM ring filled. Interrupt handlers and early boot
 * spin. */
static int write_ready_delay(void)
{
    uint32_t start = sl6801_us();
    bool can_yield = irq_enabled() && is_thread_context();
    for (unsigned poll = 0; poll < POLL_LIMIT; ++poll) {
        if ((uint32_t)(sl6801_us() - start) >= WRITE_READY_DELAY_US)
            return 0;
        if (can_yield)
            yield();
    }
    return SD_E_TIMEOUT;
}

/* Ready the SD block's descriptor DMA for one data command over the chain
 * in dma_desc (F-0010, F-0013). Only the writes a polled driver needs
 * (F-0014): DMA control with bits 1:0 (without them the first multi-block
 * write times out) and the DMA status enables (without them a read is
 * never seen to finish). CTRL bit 2, +0x2c, +0x4c, CTRL bit 4 and +0x100
 * bit 1 were each dropped with no change. The descriptors are handed over
 * and the DMA started once the command is accepted. */
static void sd_dma_program(bool write, unsigned n)
{
    (void)write;
    (void)n;
    REG(SD_DMA_CTRL) |= SD_DMA_CTRL_ON;
    REG(SD_DMA_IEN) = SD_DMA_IEN_TXRX;
    REG(SD_DMA_DESC) = (uint32_t)dma_desc[0];
    REG(SD_CTRL) |= SD_CTRL_DMA;
}

/* One DMA transfer of n (1..DESC_BLOCKS * MAX_DESCS) sectors. Writes use
 * CMD24, or CMD25 with the controller's auto-stop, then poll CMD13 until
 * the card is ready. */
static int dma_transfer(uint32_t lba, uint8_t *buf, unsigned n, bool write)
{
    struct op op = { 0u, DATA_POLL_LIMIT };
    uint32_t argument, response = 0u, command, index;
    int rc;

    if (!controller_idle() || (REG(SD_CTRL) & SD_CTRL_DMA))
        return SD_E_STATE;
    argument = card_sector_addressed ? lba : lba << BYTE_ADDR_SHIFT;
    if (write) {
        command = n == 1 ? CMD24_PACKED : CMD25_AUTOSTOP_PACKED;
        index = n == 1 ? 24u : 25u;
    } else {
        command = n == 1 ? CMD17_PACKED : CMD18_AUTOSTOP_PACKED;
        index = n == 1 ? 17u : 18u;
    }
    REG(SD_TIMEOUT) = SD_TIMEOUT_MAX;
    REG(SD_BLEN) = SD_BLOCK_SIZE;
    REG(SD_BCNT) = SD_BLOCK_SIZE * n;
    /* Descriptors of up to DESC_BLOCKS each, each pointing to the next;
     * the last points to itself. */
    unsigned ndesc = (n + DESC_BLOCKS - 1) / DESC_BLOCKS;
    for (unsigned k = 0; k < ndesc; k++) {
        unsigned blocks = MIN(n - k * DESC_BLOCKS, DESC_BLOCKS);
        dma_desc[k][0] = SD_DESC_FLAGS;
        dma_desc[k][1] = SD_BLOCK_SIZE * blocks;
        dma_desc[k][2] = (uint32_t)(buf + k * DESC_BLOCKS * SD_BLOCK_SIZE);
        dma_desc[k][3] = (uint32_t)dma_desc[k + 1 < ndesc ? k + 1 : k];
    }
    sd_dma_program(write, n);
    rc = send_command(&op, command, argument, index, false, &response);
    if (rc) {
        dma_stop();
        REG(SD_REG_100) &= ~SD_REG_100_BIT1;
        return rc;
    }
    /* Hand the descriptor to the DMA only once the command is accepted;
     * dsb makes the store visible before the start. */
    for (unsigned k = 0; k < ndesc; k++)
        dma_desc[k][0] |= SD_DESC_OWN;
    __asm__ volatile("dsb" ::: "memory");
    REG(SD_DMA_START) = 1u;
    /* Done when the DMA reports RX/TX done and EVENT has data done (and
     * auto-stop done), in either order. */
    uint32_t done = n == 1 ? SD_EVENT_DATA_DONE :
                    SD_EVENT_DATA_DONE | SD_EVENT_AUTOSTOP_DONE, seen = 0;
    uint32_t dma_done = write ? SD_DMA_STATUS_TX_DONE : SD_DMA_STATUS_RX_DONE;
    bool moved = false;
    /* Polled, so let other threads run meanwhile as an interrupt-driven
     * driver would: the audio thread's metadata reads at a track change
     * otherwise kept the codec off the CPU for >185 ms (one PCM ring). */
    bool can_yield = irq_enabled() && is_thread_context();
    uint32_t t0 = sl6801_us();
    for (op.polls = 0; op.polls < op.limit; op.polls++) {
        if (can_yield && op.polls) {
            if ((uint32_t)(sl6801_us() - t0) > DMA_TIMEOUT_US)
                break;                  /* timeout below */
            yield();
        }
        uint32_t event = REG(SD_EVENT);
        if (event & EVENT_ERROR) {
            rc = SD_E_STATUS;
            break;
        }
        seen |= event & done;
        uint32_t status = REG(SD_DMA_STATUS);
        if (status)
            REG(SD_DMA_STATUS) = status;    /* write one to clear */
        if (status & dma_done)
            moved = true;
        if (moved && seen == done)
            break;
    }
    if (!rc && !(moved && seen == done))
        rc = SD_E_TIMEOUT;
    dma_stop();
    REG(SD_REG_100) &= ~SD_REG_100_BIT1;
    if (rc)
        return rc;
    REG(SD_EVENT) = REG(SD_EVENT) & ~SD_EVENT_KEEP;
    if (write) {
        /* Card busy programming: CMD13 until READY + TRAN. */
        unsigned attempt;
        for (attempt = 0; attempt < WRITE_BUSY_POLLS; ++attempt) {
            op.polls = 0u;
            rc = send_command(&op, CMD13_PACKED, card_rca << RCA_SHIFT, 13u,
                              false, &response);
            if (rc)
                return rc;
            if ((response & R1_READY) &&
                (response & R1_STATE_MASK) == R1_STATE_TRAN)
                break;
#ifdef YP3_DEV
            dev_sd_stats.write_busy_polls++;
#endif
            rc = write_ready_delay();
            if (rc)
                return rc;
        }
        if (attempt == WRITE_BUSY_POLLS)
            return SD_E_TIMEOUT;        /* card still busy */
    }
    return controller_idle() ? 0 : SD_E_STATE;
}

/* ------------------------------------------------------------------ */
/* Data clock: selector 0x11 from divider 64 to divisor 1.             */

/* 100 NOPs plus loop overhead (from stock firmware; need not established). */
static __attribute__((naked, noinline)) void data_clock_settle(void)
{
    __asm__ volatile("movs r3,#0\n"
                     "1: cmp r3,#100\n"
                     "bne 2f\n"
                     "bx lr\n"
                     "2: nop\n"
                     "adds r3,#1\n"
                     "b 1b");
}

static int select_data_clock(void)
{
    uint32_t before = REG(SD_CLOCK_SEL), value;
    /* Expect what the cold setup wrote: source 10, divider 64, gate on. */
    if ((before & (SD_CLK_DIV_MASK | SD_CLK_SOURCE_BIT | CLK_SEL_ENABLE)) !=
        (SD_CLK_DIV_64 | CLK_SEL_ENABLE))
        return SD_E_STATE;
    /* Change the divider only with the clock stopped. */
    REG(SD_CLOCK_SEL) = REG(SD_CLOCK_SEL) & ~CLK_SEL_ENABLE;   /* gate off */
    value = REG(SD_CLOCK_SEL);
    if (value != (before & ~CLK_SEL_ENABLE))
        return SD_E_STATE;
    REG(SD_CLOCK_SEL) = value & ~SD_CLK_DIV_MASK;              /* divisor 1 */
    data_clock_settle();
    value = REG(SD_CLOCK_SEL);
    if (value != (before & ~(SD_CLK_DIV_MASK | CLK_SEL_ENABLE)))
        return SD_E_STATE;
    REG(SD_CLOCK_SEL) = value | CLK_SEL_ENABLE;                /* gate on */
    return REG(SD_CLOCK_SEL) == (before & ~SD_CLK_DIV_MASK) ? 0 : SD_E_STATE;
}

/* ------------------------------------------------------------------ */
/* Cold card discovery.                                                */


static void setup_clock_and_pins(void)
{
    static const uint32_t pins[6] = {   
        PIN_SD_3_4, PIN_SD_3_6, PIN_SD_3_7, PIN_SD_3_8, PIN_SD_3_9, PIN_SD_3_5
    };
    uint32_t v = REG(SD_CLOCK_SEL);
    /* Selector 0x11: source 10 (bit4 clear), divider 64, gate on. */
    v &= ~(SD_CLK_DIV_MASK | SD_CLK_SOURCE_BIT);
    v |= SD_CLK_DIV_64 | CLK_SEL_ENABLE;
    REG(SD_CLOCK_SEL) = v;
    /* Selector 0x24 on (what it clocks is unconfirmed). */
    v = REG(CLK_SEL_24);
    REG(CLK_SEL_24) = v | CLK_SEL_ENABLE;
    for (unsigned i = 0; i < 6u; i++)
        yp3_pin_config(PIN_OWNER_SD, pins[i]);
}

/* Gate 36 off, wait 5 ms (1000 counter ticks per ms), gate 36 on, check
 * ACK. */
static int controller_gate36_cycle(void)
{
    uint32_t start, n;

    sl6801_clock_gate_disable(CLK_GATE_ID_SD);

    start = sl6801_us();
    for (n = 0; n < CLOCK_DELAY_BUDGET; n++)
        if ((uint32_t)(sl6801_us() - start) >= GATE_OFF_US)
            break;
    if (n == CLOCK_DELAY_BUDGET)
        return SD_E_TIMEOUT;

    return sl6801_clock_gate_enable(CLK_GATE_ID_SD) ? 0 : SD_E_TIMEOUT;
}

static bool wait_cmd_idle(uint32_t limit)
{
    for (uint32_t n = 0; n < limit; n++)
        if ((REG(SD_CMD) & CMD_START) == 0)
            return true;
    return false;
}

static int controller_reset(void)
{
    uint32_t v, n;
    REG(SD_CTRL) = REG(SD_CTRL) | SD_CTRL_RESET_ALL;
    for (n = 0; n < RST_BUDGET; n++)
        if ((REG(SD_CTRL) & SD_CTRL_BIT2) == 0u)
            break;
    if (n == RST_BUDGET)
        return SD_E_TIMEOUT;
    /* Cold-reset cleanup: DMA handshake and DMA engine off. */
    REG(SD_CTRL) = REG(SD_CTRL) & ~SD_CTRL_DMA;
    REG(SD_DMA_CTRL) = REG(SD_DMA_CTRL) & ~SD_DMA_CTRL_ENABLE;
    /* Then the mode value and the bit-21 command. */
    v = REG(SD_CTRL);
    REG(SD_CTRL) = (v & ~SD_CTRL_MODE_MASK) | SD_CTRL_MODE_1;
    REG(SD_CMD) = CMD_START | SD_CMD_BIT21_ONLY;
    if (!wait_cmd_idle(CMD_BUDGET))
        return SD_E_TIMEOUT;
    if (REG(SD_EVENT) & (EVENT_ERROR | SD_EVENT_CMD_DONE))
        return SD_E_STATUS;
    REG(SD_REG_100) = REG(SD_REG_100) | SD_REG_100_BIT1;
    REG(SD_TIMEOUT) = SD_TIMEOUT_MAX;
    return 0;
}

static uint32_t packed_cmd(uint32_t id)
{
    switch (id) {
    case 0:  return CMD0_PACKED;
    case 2:  return CMD2_PACKED;
    case 3:  return CMD3_PACKED;
    case 6:  return ACMD6_PACKED;
    case 7:  return CMD7_PACKED;
    case 8:  return CMD8_PACKED;
    case 9:  return CMD9_PACKED;
    case 13: return CMD13_PACKED;
    case 16: return CMD16_PACKED;
    case 41: return ACMD41_PACKED;
    case 55: return CMD55_PACKED;
    default: return 0;
    }
}

/* Check a cold-path command's completion. last is the EVENT value seen
 * with command done; the response words are still unacknowledged. */
static int sd_check_response(uint32_t id, uint32_t last)
{
    uint32_t r;

    if (id == 41u)                      /* R3: no CRC, no index (F-0003) */
        return (last & SD_EVENT_ERRORS_R3) ? SD_E_STATUS : 0;
    if (last & SD_EVENT_ERRORS)         /* F-0002 */
        return SD_E_STATUS;
    if (id == 0u || id == 2u || id == 9u)  /* none, or R2: no index */
        return 0;
    /* R1, R1b, R6, R7 echo the command index (F-0005). */
    if (SD_STATUS_RESP_INDEX(REG(SD_STATUS)) != id)
        return SD_E_RESPONSE;
    r = REG(SD_RESP0);                  /* F-0006 */
    if (id == 3u)                       /* R6 (F-0008) */
        return (r & R6_ERROR) ? SD_E_R1 : 0;
    if (id == 8u)                       /* R7: caller checks the echo */
        return 0;
    return (r & R1_CARD_ERROR) ? SD_E_R1 : 0;   /* F-0007 */
}

static int cold_command(uint32_t id, uint32_t argument, bool response,
                        uint32_t *r0)
{
    uint32_t packed = packed_cmd(id), v, last = 0;
    int status;
    if (!packed)
        return SD_E_UNSUPPORTED;
    if (!wait_cmd_idle(CMD_BUDGET))
        return SD_E_TIMEOUT;
    REG(SD_ARG) = argument;
    REG(SD_CMD) = packed | CMD_START;
    if (!wait_cmd_idle(CMD_BUDGET))
        return SD_E_TIMEOUT;
    if (!response && id != 0u)
        return 0;
    for (v = 0; v < CMD_BUDGET; v++) {
        last = REG(SD_EVENT);
        if (last & SD_EVENT_CMD_DONE)
            break;
    }
    if (v == CMD_BUDGET)
        return SD_E_TIMEOUT;
    status = sd_check_response(id, last);
#ifdef YP3_DEV
    if (status)
        dev_log("sd cmd%lu: rc %d event %08lx status %08lx resp %08lx",
                (unsigned long)id, status, (unsigned long)last,
                (unsigned long)REG(SD_STATUS), (unsigned long)REG(SD_RESP0));
#endif
    REG(SD_EVENT) = SD_EVENT_ACK_ALL;
    if (status)
        return status;
    if (response)
        *r0 = REG(SD_RESP0);
    return 0;
}

/* Extract CSD bits hi..hi-width+1 (spec numbering: w[0] holds 127:96). */
static uint32_t csd_bits(const uint32_t w[4], unsigned hi, unsigned width)
{
    uint32_t value = 0;
    for (unsigned b = 0; b < width; b++) {
        unsigned bit = hi - b, wi = (127u - bit) / 32u, shift = bit & 31u;
        value = (value << 1) | ((w[wi] >> shift) & 1u);
    }
    return value;
}

static int csd_capacity(const uint32_t csd[4], uint32_t *sectors)
{
    uint32_t structure = csd_bits(csd, 127, 2), size, read_len, mult, exp;
    if (structure == CSD_V2) {
        /* C_SIZE (69:48); capacity = (C_SIZE + 1) x 512 KiB. All ones
         * would overflow 32-bit sector numbers. */
        size = csd_bits(csd, 69, 22);
        if (size >= 0x3fffffu)
            return SD_E_UNSUPPORTED;
        *sectors = (size + 1u) * 1024u;
        return 0;
    }
    if (structure != CSD_V1)
        return SD_E_UNSUPPORTED;
    /* v1: bytes = (C_SIZE + 1) << (C_SIZE_MULT + 2 + READ_BL_LEN), so
     * sectors = (C_SIZE + 1) << (C_SIZE_MULT + READ_BL_LEN - 7). */
    size = csd_bits(csd, 73, 12);       /* C_SIZE */
    read_len = csd_bits(csd, 83, 4);    /* READ_BL_LEN */
    mult = csd_bits(csd, 49, 3);        /* C_SIZE_MULT */
    exp = mult + read_len;
    if (exp >= 7u) {
        exp -= 7u;
        if (exp >= 20u || size + 1u > (0xffffffffu >> exp))
            return SD_E_UNSUPPORTED;
        *sectors = (size + 1u) << exp;
    } else
        *sectors = (size + 1u) >> (7u - exp);
    return *sectors ? 0 : SD_E_UNSUPPORTED;
}

/* Clock/pins, gate36 cycle, controller reset, CMD0/8/55+41/2/3/9/7 (/16),
 * CMD13. Fills the card info; performs no data transfer. */
static int cold_prepare(void)
{
    uint32_t response = 0, ocrtry, i, rca, ccs, ocr = 0, csd[4], sectors;
    int status;

    setup_clock_and_pins();
    REG(SD_WIDTH) &= ~SD_WIDTH_4BIT;    /* 1-bit until ACMD6 */
    status = controller_gate36_cycle();
    if (status) return status;
    status = controller_reset();
    if (status) return status;
    status = cold_command(0, 0, false, &response);
    if (status) return status;
    status = cold_command(8, CMD8_ARG, true, &response);
    if (status) return status;
    if ((response & CMD8_ECHO_MASK) != CMD8_ARG)
        return SD_E_RESPONSE;
    for (ocrtry = 0; ocrtry < OCR_BUDGET; ocrtry++) {
        status = cold_command(55, 0, true, &response);
        if (status) return status;
        status = cold_command(41, ACMD41_ARG, true, &response);
        if (status) return status;
        ocr = response;
        if (response & OCR_READY)
            break;
    }
    if (ocrtry == OCR_BUDGET)
        return SD_E_TIMEOUT;
    ccs = (response >> OCR_CCS_SHIFT) & 1u;
    status = cold_command(2, 0, true, &response);
    if (status) return status;
    status = cold_command(3, 0, true, &response);
    if (status) return status;
    rca = response >> RCA_SHIFT;   /* R6: new RCA in bits 31:16 */
    if (!rca)
        return SD_E_RESPONSE;
    status = cold_command(9, rca << RCA_SHIFT, true, &response);
    if (status) return status;
    /* The controller exposes CSD word 0 at the highest response address. */
    for (i = 0; i < 4u; i++)
        csd[i] = REG(SD_RESP0 + (3u - i) * 4u);
    status = csd_capacity(csd, &sectors);
    if (status) return status;
    if (ccs && csd_bits(csd, 127, 2) != CSD_V2)
        return SD_E_RESPONSE;
    status = cold_command(7, rca << RCA_SHIFT, true, &response);
    if (status) return status;
    if (!ccs) {
        status = cold_command(16, SD_BLOCK_SIZE, true, &response);
        if (status) return status;
    }
    /* 4-bit bus: ACMD6(2), then the host width bit. */
    status = cold_command(55, rca << RCA_SHIFT, true, &response);
    if (status) return status;
    status = cold_command(6, ACMD6_BUS_4BIT, true, &response);
    if (status) return status;
    REG(SD_WIDTH) |= SD_WIDTH_4BIT;
    status = cold_command(13, rca << RCA_SHIFT, true, &response);
    if (status) return status;
    if ((response & R1_READY) == 0u ||
        R1_STATE(response) != R1_STATE_TRAN_VALUE)
        return SD_E_RESPONSE;

    card_sector_addressed = ccs;
    card_rca = rca;
    card_sectors = sectors;
    card_info.ocr = ocr;
    card_info.rca = rca;
    card_info.numblocks = sectors;
    for (i = 0; i < 4u; i++)
        card_info.csd[i] = csd[i];
    card_info.sd2plus = ((csd[0] >> 30) & 3u) >= CSD_V2;  /* CSD_STRUCTURE */
    return 0;
}

/* Cold setup leaves +0x100 = bit 1; clear it, move to the full data clock
 * and verify the card with a fresh CMD13. */
static int bind_card(void)
{
    struct op op = { 0u, POLL_LIMIT };
    uint32_t response = 0u;
    int rc;

    if (REG(SD_REG_100) != SD_REG_100_BIT1)
        return SD_E_STATE;
    REG(SD_REG_100) = 0u;
    if (REG(SD_REG_100) != 0u)
        return SD_E_STATE;
    if (!controller_idle() || (REG(SD_CTRL) & SD_CTRL_DMA) != 0u)
        return SD_E_STATE;
    rc = select_data_clock();
    if (rc)
        return rc;
    rc = send_command(&op, CMD13_PACKED, card_rca << RCA_SHIFT, 13u, true,
                      &response);
    if (rc)
        return rc;
    return controller_idle() ? 0 : SD_E_STATE;
}

/* ------------------------------------------------------------------ */
/* Rockbox storage API.                                                */

int sd_init(void)
{
    if (sd_state == STATE_READY)
        return 0;
    if (sd_state == STATE_FAILED)
        return -EIO;
    mutex_init(&sd_mutex);
    int rc = cold_prepare();
    if (rc == 0)
        rc = bind_card();
    if (rc != 0) {
#ifdef YP3_DEV
        dev_log("sd init: rc %d", rc);
#endif
        sd_state = STATE_FAILED;
        return -EIO;
    }
    card_info.initialized = 1;
    card_info.blocksize = SD_BLOCK_SIZE;
    card_info.current_bank = 0;
    last_activity = current_tick;
    sd_state = STATE_READY;
    return 0;
}

static bool request_ok(sector_t start, int count, const void *buffer)
{
    if (!buffer || count <= 0 || (unsigned)count > FAT_MAX_TRANSFER_SIZE)
        return false;
    return start < card_sectors && (uint64_t)count <= card_sectors - start;
}

int sd_read_sectors(IF_MD(int drive,) sector_t start, int count, void *buffer)
{
    uint8_t *out = buffer;
    int rc = 0;
#ifdef CONFIG_STORAGE_MULTI
    (void)drive;
#endif
    if (sd_state != STATE_READY)
        return -ENODEV;
    if (!request_ok(start, count, buffer))
        return -EINVAL;
    mutex_lock(&sd_mutex);
    if (sd_state != STATE_READY) {
        mutex_unlock(&sd_mutex);
        return -ENODEV;
    }
    sd_busy = true;
#ifdef YP3_DEV
    uint32_t t0 = sl6801_us();
#endif
    for (int i = 0; i < count && rc == 0; ) {
        unsigned n;
        if ((uintptr_t)out & 3u) {
            n = MIN((unsigned)(count - i), BOUNCE_BLOCKS);
            rc = dma_transfer((uint32_t)start + i, (uint8_t *)bounce, n,
                              false);
            if (rc == 0)
                memcpy(out, bounce, n * SD_BLOCK_SIZE);
        } else {
            n = MIN((unsigned)(count - i), DMA_MAX_BLOCKS);
            rc = dma_transfer((uint32_t)start + i, out, n, false);
        }
#ifdef YP3_DEV
        if (rc)
            dev_log("sd read lba %lu n %u: rc %d event %08lx dma %08lx/%08lx",
                        (unsigned long)start + i, n, rc,
                        (unsigned long)REG(SD_EVENT),
                        (unsigned long)REG(SD_DMA_CTRL),
                        (unsigned long)REG(SD_DMA_STATUS));
#endif
        i += n;
        out += n * SD_BLOCK_SIZE;
    }
#ifdef YP3_DEV
    dev_sd_stats.read_calls++;
    dev_sd_stats.read_sectors += count;
    dev_sd_stats.read_us += sl6801_us() - t0;
#endif
    if (rc == 0)
        last_activity = current_tick;
    sd_busy = false;
    if (rc) {
        /* A failed transfer leaves the controller in an unknown state. */
        sd_state = STATE_FAILED;
        card_info.initialized = 0;
    }
    mutex_unlock(&sd_mutex);
    return rc ? -EIO : 0;
}

int sd_write_sectors(IF_MD(int drive,) sector_t start, int count,
                     const void *buffer)
{
    const uint8_t *in = buffer;
    int rc = 0;
#ifdef CONFIG_STORAGE_MULTI
    (void)drive;
#endif
    if (sd_state != STATE_READY)
        return -ENODEV;
    if (!request_ok(start, count, buffer))
        return -EINVAL;
    mutex_lock(&sd_mutex);
    if (sd_state != STATE_READY) {
        mutex_unlock(&sd_mutex);
        return -ENODEV;
    }
    sd_busy = true;
#ifdef YP3_DEV
    uint32_t t0 = sl6801_us();
#endif
    for (int i = 0; i < count && rc == 0; ) {
        unsigned n;
        if ((uintptr_t)in & 3u) {
            n = MIN((unsigned)(count - i), BOUNCE_BLOCKS);
            memcpy(bounce, in, n * SD_BLOCK_SIZE);
            rc = dma_transfer((uint32_t)start + i, (uint8_t *)bounce, n, true);
        } else {
            n = MIN((unsigned)(count - i), DMA_MAX_BLOCKS);
            rc = dma_transfer((uint32_t)start + i, (uint8_t *)in, n, true);
        }
#ifdef YP3_DEV
        if (rc)
            dev_log("sd write lba %lu n %u: rc %d event %08lx index %08lx",
                        (unsigned long)start + i, n, rc,
                        (unsigned long)REG(SD_EVENT),
                        (unsigned long)REG(SD_STATUS));
#endif
        i += n;
        in += n * SD_BLOCK_SIZE;
    }
#ifdef YP3_DEV
    dev_sd_stats.write_sectors += count;
    dev_sd_stats.write_us += sl6801_us() - t0;
#endif
    if (rc == 0)
        last_activity = current_tick;
    sd_busy = false;
    if (rc) {
        sd_state = STATE_FAILED;
        card_info.initialized = 0;
    }
    mutex_unlock(&sd_mutex);
    return rc ? -EIO : 0;
}

bool sd_disk_is_active(void)
{
    return sd_state == STATE_READY && sd_busy;
}

#ifdef HAVE_HOTSWAP
bool sd_removable(IF_MD_NONVOID(int drive))
{
#ifdef CONFIG_STORAGE_MULTI
    (void)drive;
#endif
    return false;
}

bool sd_present(IF_MD_NONVOID(int drive))
{
#ifdef CONFIG_STORAGE_MULTI
    if (drive != 0)
        return false;
#endif
    return sd_state == STATE_READY;
}
#endif

long sd_last_disk_activity(void)
{
    return sd_state == STATE_READY ? last_activity : -1;
}

int sd_event(long id, intptr_t data)
{
    if (sd_state != STATE_READY)
        return 0;
    return storage_event_default_handler(id, data, last_activity, STORAGE_SD);
}

#ifdef CONFIG_STORAGE_MULTI
int sd_num_drives(int first_drive)
{
    (void)first_drive;
    return sd_state == STATE_READY ? 1 : 0;
}
#endif

int sd_spinup_time(void) { return 0; }
void sd_enable(bool on) { (void)on; }
void sd_sleepnow(void) {}
int sd_soft_reset(void) { return -EOPNOTSUPP; }

void sd_close(void)
{
    /* No retirement sequence exists; just stop accepting requests. */
    if (sd_state == STATE_READY) {
        mutex_lock(&sd_mutex);
        sd_state = STATE_FAILED;
        card_info.initialized = 0;
        mutex_unlock(&sd_mutex);
    }
    sd_state = STATE_FAILED;
}

tCardInfo *card_get_info_target(int card_no)
{
    if (card_no != 0 || sd_state != STATE_READY)
        return NULL;
    return &card_info;
}
