/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Bluetooth radio (bt-yp3.h): the BT clocks, access to the RF block and
 * its register file, a reader for the PSMP settings store and the radio
 * calibration cache that the Bluetooth stack keeps between opens.
 *
 * The RF block (RF_BASE: digital part at +0, analog part at +0x1000) has
 * no documentation; registers are named by offset. The analog part holds
 * an indirect register file of BT_RF_CELLS cells, whose fields are listed
 * in bt_rf_fields[] (bt-rf-tables-yp3.c, with the static cell values, a
 * quarter sine wave and seven digital-block words).
 *
 * Helpers: bounded waits on the 1 MHz counter, register and register-file
 * access, rf_run() (an interpreter for tables of register steps), sine and
 * cosine from the quarter wave, and soft-float maths for measurements.
 * Radio bring-up, calibration and programming: F-0082..F-0093. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "system.h"
#include "kernel.h"
#include "crc32.h"
#include "bt_drv.h"
#include "bt-yp3.h"
#include "dev-yp3.h"
#include "mmio-sl6801.h"
#ifdef YP3_DEV
#include <stdio.h>
#include "core_alloc.h"
#endif

#define COUNTER     REG(SL6801_COUNTER_LO)

/* ---- Clocks ---- */

/* ---- Trim bank (TRIM_*, sl6801.h; word n at +0x20 + 4n).
 * audiohw-yp3.c and power-yp3.c enable the bank with helpers of their own,
 * which differ in detail. */
#define TRIM3_RF_VALID      (1u << 31)  /* word 3: RF trim valid */
#define TRIM1_RF_SHIFT      14          /* word 1 [17:14]: RF trim */
#define TRIM_RF_MASK        0xfu
#define TRIM_RF_DEFAULT     4u          /* not valid (from stock firmware) */

/* ---- Stock settings store (PSMP) ----
 * Flash 0x1fc000..0x1fffff. A log of records: a 12-byte header (magic
 * 55 aa, status, CRC-32 little endian, value length u16, id u16, name
 * length u8), then name and value. A changed setting is appended with the
 * same id and the old record's status bit 0 cleared. The CRC is CRC-32
 * without inversion over name, value, then the header with its CRC bytes
 * as 0xff. */
#define PSMP_BASE           0x00dfc000  /* XIP view of flash 0x1fc000 */
#define PSMP_SIZE           0x4000
#define PSMP_HEADER         12
#define PSMP_MAGIC0         0x55        /* header byte 0 */
#define PSMP_MAGIC1         0xaa        /* header byte 1 */
#define PSMP_STATUS         2           /* header offsets */
#define PSMP_CRC            3
#define PSMP_LEN            7
#define PSMP_ID             9
#define PSMP_NAME_LEN       11
#define PSMP_STATUS_VALID   0x01

/* ---- PMU port 0x90 (power-sequencer block, sl6801.h PMU_BASE) ----
 * [11:4] takes the PSMP setting "freq_drift". From the name, a crystal
 * trim (unconfirmed). */
#define PMU_XTAL            (PMU_BASE + 0x90)

/* ---- Exchange memory, used as sample RAM ---- */
#define EM_BASE             BT_EM_BASE
#define EM_ENTRY_BYTES      4           /* tone: cos, sin; capture: I, Q */
#define SAMPLE_SIGN         0x800       /* captured samples are 12-bit */
#define SAMPLE_SIGN_EXTEND  0xf000

/* ---- RF block: analog part (RF_BASE + 0x1000) ---- */
#define RF_1000             (RF_BASE + 0x1000)
/* Register file: [8:0] cell, bit 11 write, bit 16 start (reads back set
 * while busy); data through RF_REGFILE_DATA. [31:20] holds four 3-bit
 * fields of unknown role. */
#define RF_REGFILE_CTRL     (RF_BASE + 0x1004)
#define RF_REGFILE_DATA     (RF_BASE + 0x1008)
#define REGFILE_CELL_MASK   0x1ffu
#define REGFILE_WRITE       (1u << 11)
#define REGFILE_BUSY        (1u << 16)
#define RF_DUMP_DIGITAL     0x320       /* dev dump sizes */
#define RF_DUMP_ANALOG      0x0f0

/* Register-file fields 26..29 (bt_rf_fields[]) hold the per-channel
 * calibration results, cells 244..329, one cell per channel. */
#define FIELD_CHCAL_FIRST   26
#define FIELD_CHCAL_LAST    29
#define CHANNELS            86          /* 2400..2485 MHz */
/* The I/Q calibration result: RF_BASE + 0x10a4 [28:24] */
#define IQ_VALUES           32

/* ---- Waits ----
 * The settle-wait unit is about 350 us (from stock firmware, estimated);
 * we wait 500 us. The waits are settle times, so longer is safe. */
#define RF_UNIT_US          500u
#define RF_YIELD_US         1000u       /* yield in waits at least this long */
#define RF_POLL_US          20000u      /* bound on busy polls */
#define RF_WAIT_LOOPS       100000000u  /* bound if the counter stalled */

#ifdef YP3_DEV
static uint32_t rf_log_t;   /* counter at the previous RF_LOG */
static uint32_t rf_log_t0;  /* and at the start of bt_rf_yp3_init() */
#define RF_LOG(fmt, ...) do { \
        uint32_t now_ = COUNTER; \
        dev_log("bt rf: " fmt " (%lu ms, %lu ms in all)", ##__VA_ARGS__, \
                    (unsigned long)(now_ - rf_log_t) / 1000, \
                    (unsigned long)(now_ - rf_log_t0) / 1000); \
        rf_log_t = now_; \
    } while (0)
#else
#define RF_LOG(...) do { } while (0)
#endif

/* Wait on the 1 MHz counter; long waits yield when called from a thread. */
static void rf_wait_us(uint32_t us)
{
    bool can_yield = us >= RF_YIELD_US && irq_enabled() &&
                     get_interrupt_number() == 0;
    uint32_t start = COUNTER;
    for (uint32_t i = 0; i < RF_WAIT_LOOPS && COUNTER - start < us; i++)
        if (can_yield)
            yield();
}

/* Settle wait of `units` RF_UNIT_US */
static void rf_delay(unsigned units)
{
    rf_wait_us(units * RF_UNIT_US);
}

/* Wait until the busy bits in reg read back clear. */
static bool rf_wait_clear(uint32_t reg, uint32_t busy)
{
    uint32_t start = COUNTER;
    for (uint32_t i = 0; REG(reg) & busy; i++) {
        if (COUNTER - start > RF_POLL_US || i >= RF_WAIT_LOOPS) {
            dev_log("bt rf: %08lx busy %08lx stuck at %08lx",
                        (unsigned long)reg, (unsigned long)busy,
                        (unsigned long)REG(reg));
            return false;
        }
    }
    return true;
}

/* One read-modify-write store. */
static void rf_set(uint32_t reg, uint32_t clear, uint32_t set)
{
    REG(reg) = (REG(reg) & ~clear) | set;
}

/* Field write in two stores: clear the field, then OR the value in. */
static void rf_field(uint32_t reg, uint32_t mask, uint32_t value)
{
    REG(reg) &= ~mask;
    REG(reg) |= value;
}

/* Latch a value by hand: manual bit on, value field, then the start bit,
 * which the hardware clears when done. The manual bit stays on. */
static bool rf_strobe(uint32_t reg, uint32_t manual, uint32_t mask,
                                  uint32_t value, uint32_t start)
{
    rf_set(reg, 0, manual);
    rf_field(reg, mask, value);
    rf_set(reg, 0, start);
    return rf_wait_clear(reg, start);
}

/* ---- RF register file ---- */

/* Select a cell, set the direction (write: REGFILE_WRITE or 0) and run the
 * access. Data goes through RF_REGFILE_DATA. */
static bool regfile_access(unsigned cell, uint32_t write)
{
    rf_field(RF_REGFILE_CTRL, REGFILE_CELL_MASK, cell);
    rf_set(RF_REGFILE_CTRL, write ? 0 : REGFILE_WRITE, write);
    rf_set(RF_REGFILE_CTRL, 0, REGFILE_BUSY);
    return rf_wait_clear(RF_REGFILE_CTRL, REGFILE_BUSY);
}

static bool regfile_write(unsigned cell, uint32_t value)
{
    REG(RF_REGFILE_DATA) = value;
    return regfile_access(cell, REGFILE_WRITE);
}

static bool regfile_read(unsigned cell, uint32_t *value)
{
    if (!regfile_access(cell, 0))
        return false;
    *value = REG(RF_REGFILE_DATA);
    return true;
}

/* Entry i of register-file field id (bt_rf_fields[]) */
static bool field_read(unsigned id, unsigned i, uint32_t *value)
{
    const struct bt_rf_field *f = &bt_rf_fields[id];
    uint32_t v;
    if (!regfile_read(f->cell + i, &v))
        return false;
    *value = (v >> f->shift) & f->mask;
    return true;
}

static bool field_write(unsigned id, unsigned i, int32_t value)
{
    const struct bt_rf_field *f = &bt_rf_fields[id];
    uint32_t v;
    if (!regfile_read(f->cell + i, &v))
        return false;
    v = (v & ~(f->mask << f->shift)) | ((uint32_t)value & f->mask) << f->shift;
    return regfile_write(f->cell + i, v);
}

/* Value of a static field's entry i, or the cell bits for a value. */
static uint32_t field_static(unsigned id, unsigned i)
{
    const struct bt_rf_field *f = &bt_rf_fields[id];
    return (bt_rf_cells[f->cell + i] >> f->shift) & f->mask;
}

static uint32_t field_bits(unsigned id, uint32_t value)
{
    const struct bt_rf_field *f = &bt_rf_fields[id];
    return (value & f->mask) << f->shift;
}

/* Write the whole register file: cells 0..243 from bt_rf_cells, 244..329
 * from the per-channel calibration results. */
static bool load_cells(const uint32_t *channel)
{
    for (unsigned c = 0; c < BT_RF_CELLS; c++)
        if (!regfile_write(c, c < BT_RF_STATIC_CELLS ? bt_rf_cells[c] :
                                  channel[c - BT_RF_STATIC_CELLS]))
            return false;
    return true;
}

/* ---- Register scripts ----
 * Fixed sequences of register accesses, kept as tables of steps that
 * rf_run() executes. Each step is one call of the helpers above, so a table
 * makes exactly the accesses the calls would. A step's register is an
 * offset from RF_BASE. */
enum rf_op {
    OP_RMW,         /* rf_set(reg, mask, value): one store */
    OP_FIELD,       /* rf_field(reg, mask, value): two stores */
    OP_WRITE,       /* store value */
    OP_SET_EACH,    /* set the mask bits one store each, highest first */
    OP_CLEAR_EACH,  /* clear them the same way */
    OP_WAIT,        /* wait for the mask bits to read back clear */
    OP_DELAY,       /* rf_delay(value) */
    OP_ENTRY0,      /* register-file field `field`, entry 0 = value */
};

struct rf_step {
    uint8_t op;
    uint8_t field;
    uint16_t reg;
    uint32_t mask;
    uint32_t value;
};

#define STEP(op, reg, mask, val)    { op, 0, (reg) - RF_BASE, mask, val }
#define STEP_RMW(reg, clear, set)   STEP(OP_RMW, reg, clear, set)
#define STEP_SET(reg, bits)         STEP(OP_RMW, reg, 0, bits)
#define STEP_CLEAR(reg, bits)       STEP(OP_RMW, reg, bits, 0)
#define STEP_FIELD(reg, mask, val)  STEP(OP_FIELD, reg, mask, val)
#define STEP_WRITE(reg, val)        STEP(OP_WRITE, reg, 0, val)
#define STEP_SET_EACH(reg, bits)    STEP(OP_SET_EACH, reg, bits, 0)
#define STEP_CLEAR_EACH(reg, bits)  STEP(OP_CLEAR_EACH, reg, bits, 0)
#define STEP_WAIT(reg, bits)        STEP(OP_WAIT, reg, bits, 0)
#define STEP_DELAY(units)           { OP_DELAY, 0, 0, 0, units }
#define STEP_ENTRY0(id, val)        { OP_ENTRY0, id, 0, 0, (uint32_t)(val) }
/* Set a start bit and wait until the hardware clears it */
#define STEP_LATCH(reg, start)      STEP_SET(reg, start), STEP_WAIT(reg, start)
/* rf_strobe() */
#define STEP_STROBE(reg, manual, mask, val, start) \
    STEP_SET(reg, manual), STEP_FIELD(reg, mask, val), STEP_LATCH(reg, start)

/* False when a wait or a register-file access timed out */
static UNUSED_ATTR bool rf_run(const struct rf_step *steps, size_t count)
{
    for (const struct rf_step *s = steps; s < steps + count; s++) {
        uint32_t reg = RF_BASE + s->reg;
        switch (s->op) {
        case OP_RMW:
            rf_set(reg, s->mask, s->value);
            break;
        case OP_FIELD:
            rf_field(reg, s->mask, s->value);
            break;
        case OP_WRITE:
            REG(reg) = s->value;
            break;
        case OP_SET_EACH:
        case OP_CLEAR_EACH:
            for (int bit = 31; bit >= 0; bit--) {
                uint32_t b = 1u << bit;
                if (s->mask & b)
                    rf_set(reg, s->op == OP_CLEAR_EACH ? b : 0,
                           s->op == OP_SET_EACH ? b : 0);
            }
            break;
        case OP_WAIT:
            if (!rf_wait_clear(reg, s->mask))
                return false;
            break;
        case OP_DELAY:
            rf_delay(s->value);
            break;
        case OP_ENTRY0:
            if (!field_write(s->field, 0, s->value))
                return false;
            break;
        }
    }
    return true;
}

#define RF_RUN(steps)       rf_run(steps, ARRAYLEN(steps))

/* ---- Clocks and trim ---- */

/* Selector 0x1d and gate 34 off, clock 0x29 on, gate (waiting for its
 * acknowledge) and selector on again, one settle unit. The off/on toggle
 * resets the RF block (see bt_yp3_clocks_on()). */
static bool rf_clocks_on(void)
{
    rf_set(CLK_SEL_1D, CLK_SEL_ENABLE, 0);
    sl6801_clock_gate_disable(CLK_GATE_ID_BT);
    int old = disable_irq_save();
    rf_set(CLK_ROOT, 0, CLK_ROOT_CLK29);
    restore_irq(old);
    if (!sl6801_clock_gate_enable(CLK_GATE_ID_BT))
        return false;
    rf_set(CLK_SEL_1D, 0, CLK_SEL_ENABLE);
    rf_delay(1);
    return true;
}

/* Gate 34 requested and acknowledged, selector 0x1d enabled */
static bool rf_clocks_running(void)
{
    return (REG(CLK_GATE_REQ(CLK_GATE_ID_BT)) & CLK_GATE_BIT(CLK_GATE_ID_BT)) &&
           (REG(CLK_GATE_ACK(CLK_GATE_ID_BT)) & CLK_GATE_BIT(CLK_GATE_ID_BT)) &&
           (REG(CLK_SEL_1D) & CLK_SEL_ENABLE);
}

/* eFuse RF trim: trim word 1 [17:14] when word 3 marks it valid, else
 * TRIM_RF_DEFAULT. Enables the trim bank first. */
static bool rf_trim(uint32_t *trim)
{
    if (!sl6801_clock_gate_enable(CLK_GATE_ID_TRIM))
        return false;
    int old = disable_irq_save();
    rf_set(CLK_SEL_1B, 0, CLK_SEL_ENABLE);
    restore_irq(old);
    if (REG(TRIM_WORD_2C) & TRIM3_RF_VALID)
        *trim = (REG(TRIM_WORD_24) >> TRIM1_RF_SHIFT) & TRIM_RF_MASK;
    else
        *trim = TRIM_RF_DEFAULT;
    return true;
}

/* ---- Measurement helpers ---- */

/* Entry i of a 1024-point sine (4 * BT_RF_SINE_POINTS) from the quarter
 * wave */
static int16_t rf_sin(unsigned i)
{
    unsigned quarter = (i / BT_RF_SINE_POINTS) & 3, r = i % BT_RF_SINE_POINTS;
    int v = bt_rf_sine[quarter & 1 ? BT_RF_SINE_POINTS - 1 - r : r];
    return quarter & 2 ? -v : v;
}

static int16_t rf_cos(unsigned i)
{
    return rf_sin(i + BT_RF_SINE_POINTS);       /* a quarter period on */
}

/* Sample i of channel q (0 = I, 1 = Q) of a capture at EM offset 0:
 * 12-bit signed, so bit 11 is extended over bits 15:12. */
static int16_t capture_sample(unsigned i, unsigned q)
{
    uint16_t h = REG16(EM_BASE + EM_ENTRY_BYTES * i + 2 * q);
    return (int16_t)(h & SAMPLE_SIGN ? h | SAMPLE_SIGN_EXTEND : h);
}

/* log10 of an integer (the core has no libm). ln m = 2 atanh((m - 1) /
 * (m + 1)) for m in [0.7, 1.42), series to s^23 (error < 1e-16 relative). */
static double rf_log10(int32_t x)
{
    static const double ln2 = 0.69314718055994530942;
    static const double log10_e = 0.43429448190325182765;
    static const double sqrt2 = 1.41421356237309504880;
    if (x <= 0)
        return x ? __builtin_nan("") : -__builtin_inf();
    int k = 31 - __builtin_clz((uint32_t)x);
    double m = (double)x / (double)(1u << k);
    if (m > sqrt2) {
        m *= 0.5;
        k++;
    }
    double s = (m - 1.0) / (m + 1.0), s2 = s * s, t = 0.0;
    for (int n = 23; n >= 3; n -= 2)
        t = (t + 1.0 / n) * s2;
    return (k * ln2 + 2.0 * s * (1.0 + t)) * log10_e;
}

/* ---- Settings store ---- */

struct psmp_rec {
    const uint8_t *name;
    const uint8_t *value;
    size_t name_len;
    size_t len;                 /* of the value */
    unsigned id;
    bool valid;                 /* status bit 0 */
    uint32_t crc;
};

/* Decode the record at p: false if p holds no record header or the record
 * runs past the end of the store. The next record follows the value. */
static bool psmp_decode(const uint8_t *p, struct psmp_rec *r)
{
    const uint8_t *start = (const uint8_t *)PSMP_BASE;
    const uint8_t *end = start + PSMP_SIZE;
    if (p < start || end - p < PSMP_HEADER || p[0] != PSMP_MAGIC0 ||
        p[1] != PSMP_MAGIC1)
        return false;
    r->name_len = p[PSMP_NAME_LEN];
    r->len = p[PSMP_LEN] | p[PSMP_LEN + 1] << 8;
    r->id = p[PSMP_ID] | p[PSMP_ID + 1] << 8;
    r->valid = p[PSMP_STATUS] & PSMP_STATUS_VALID;
    r->crc = p[PSMP_CRC] | p[PSMP_CRC + 1] << 8 | p[PSMP_CRC + 2] << 16 |
             (uint32_t)p[PSMP_CRC + 3] << 24;
    r->name = p + PSMP_HEADER;
    r->value = r->name + r->name_len;
    return (size_t)(end - r->name) >= r->name_len + r->len;
}

/* The CRC of the record at p matches its name, value and header */
static bool psmp_crc_ok(const uint8_t *p, const struct psmp_rec *r)
{
    uint8_t header[PSMP_HEADER];
    memcpy(header, p, PSMP_HEADER);
    memset(header + PSMP_CRC, 0xff, sizeof r->crc);
    uint32_t sum = crc_32r(r->name, r->name_len, 0);
    sum = crc_32r(r->value, r->len, sum);
    return crc_32r(header, PSMP_HEADER, sum) == r->crc;
}

/* Walk the store from its base: records follow one another until a slot
 * holds no header. A name may have several records (rewrites append); the
 * current one is taken to be the last valid record whose CRC checks. */
static const uint8_t *psmp_find(const char *name)
{
    const uint8_t *p = (const uint8_t *)PSMP_BASE, *found = NULL;
    size_t n = strlen(name);
    struct psmp_rec r;
    while (psmp_decode(p, &r)) {
        if (r.valid && r.name_len == n && !memcmp(r.name, name, n) &&
            psmp_crc_ok(p, &r))
            found = p;
        p = r.value + r.len;
    }
    return found;
}

#ifdef YP3_DEV
/* Every record of the store, for "bt rf psmp". */
static void psmp_list(void)
{
    const uint8_t *p = (const uint8_t *)PSMP_BASE;
    struct psmp_rec r;
    unsigned n = 0;
    while (psmp_decode(p, &r)) {
        char name[33];
        size_t k = MIN(r.name_len, sizeof name - 1);
        memcpy(name, r.name, k);
        name[k] = '\0';
        uint8_t v[4] = { 0 };
        memcpy(v, r.value, MIN(r.len, sizeof v));
        dev_log("psmp +%04x id %02x %s len %u valid %d crc %s value "
                "%02x %02x %02x %02x", (unsigned)(p - (const uint8_t *)PSMP_BASE),
                r.id, name, (unsigned)r.len, r.valid,
                psmp_crc_ok(p, &r) ? "ok" : "BAD", v[0], v[1], v[2], v[3]);
        p = r.value + r.len;
        n++;
    }
    dev_log("psmp: %u records, end +%04x (byte %02x)", n,
            (unsigned)(p - (const uint8_t *)PSMP_BASE), *p);
}
#endif

int bt_yp3_psmp_read(const char *name, void *buf, size_t size)
{
    struct psmp_rec r;
    const uint8_t *p = psmp_find(name);
    if (!p || !psmp_decode(p, &r) || r.len > size || !psmp_crc_ok(p, &r))
        return -1;
    memcpy(buf, r.value, r.len);
    return r.len;
}

/* ---- Calibration cache ----
 * The calibration results: the per-channel register-file cells 244..329
 * and the I/Q value. With a valid struct rf_cal from the last calibration,
 * bt_rf_yp3_init() skips the measurements. The stack keeps it in a file
 * and lends the buffer for each open (bt_drv_cal_attach, bt_drv.h). The key
 * ties it to the RF tables, the format and the unit (PSMP freq_drift and
 * bt_addr), so other firmware or another player calibrates afresh. */
#define RF_CAL_MAGIC        0x43465242u /* "BRFC" */
#define RF_CAL_VERSION      1           /* bump when the measurements change */
#define BDADDR_BYTES        6
#define FREQ_DRIFT_NONE     UINT32_MAX  /* no freq_drift setting */

struct rf_cal {
    uint32_t magic;
    uint16_t version;
    uint16_t size;                      /* sizeof(struct rf_cal) */
    uint32_t key;                       /* rf_cal_key() */
    uint32_t channel[CHANNELS];         /* cells 244..329 (fields 26..29) */
    uint8_t iq;                         /* RF_BASE + 0x10a4 [28:24] */
    uint8_t reserved[3];
    uint32_t crc;                       /* of everything before it */
};
_Static_assert(sizeof(struct rf_cal) <= BT_DRV_CAL_MAX, "calibration blob");

static struct rf_cal *rf_cal;   /* the lent buffer, or NULL */
static bool rf_cal_held;        /* it holds a blob of the right size */
static bool rf_cal_fresh;       /* measured into it, not reported yet */

/* The PSMP setting freq_drift (4 bytes, little endian), or FREQ_DRIFT_NONE */
static uint32_t rf_freq_drift(void)
{
    uint8_t drift[4] = { 0 };
    if (bt_yp3_psmp_read("freq_drift", drift, sizeof drift) <= 0)
        return FREQ_DRIFT_NONE;
    return drift[0] | drift[1] << 8 | drift[2] << 16 |
           (uint32_t)drift[3] << 24;
}

static uint32_t rf_cal_crc(const struct rf_cal *c)
{
    return crc_32r(c, offsetof(struct rf_cal, crc), 0xffffffff);
}

/* What the measurements depend on, besides the hardware */
static uint32_t rf_cal_key(uint32_t freq_drift)
{
    uint8_t bdaddr[BDADDR_BYTES] = { 0 };
    bt_yp3_psmp_read("bt_addr", bdaddr, sizeof bdaddr);
    uint32_t key = crc_32r(bt_rf_fields, sizeof bt_rf_fields, RF_CAL_VERSION);
    key = crc_32r(bt_rf_cells, sizeof bt_rf_cells, key);
    key = crc_32r(bt_rf_sine, sizeof bt_rf_sine, key);
    key = crc_32r(bt_rf_ramp, sizeof bt_rf_ramp, key);
    key = crc_32r(&freq_drift, sizeof freq_drift, key);
    return crc_32r(bdaddr, sizeof bdaddr, key);
}

/* c is intact, for this key, and its values fit their fields */
static bool rf_cal_valid(const struct rf_cal *c, uint32_t key)
{
    uint32_t bits = 0;
    if (c->magic != RF_CAL_MAGIC || c->version != RF_CAL_VERSION ||
        c->size != sizeof *c || c->key != key || c->crc != rf_cal_crc(c) ||
        c->iq >= IQ_VALUES)
        return false;
    for (unsigned id = FIELD_CHCAL_FIRST; id <= FIELD_CHCAL_LAST; id++)
        bits |= field_bits(id, UINT32_MAX);
    for (unsigned i = 0; i < CHANNELS; i++)
        if (c->channel[i] & ~bits)
            return false;
    return true;
}

static void rf_cal_seal(struct rf_cal *c, uint32_t key)
{
    c->magic = RF_CAL_MAGIC;
    c->version = RF_CAL_VERSION;
    c->size = sizeof *c;
    c->key = key;
    memset(c->reserved, 0, sizeof c->reserved);
    c->crc = rf_cal_crc(c);
}

void bt_drv_cal_attach(void *buf, size_t size, size_t len)
{
    bool fits = buf && !((uintptr_t)buf & 3) && size >= sizeof *rf_cal;
    rf_cal = fits ? (struct rf_cal *)buf : NULL;
    rf_cal_held = fits && len == sizeof *rf_cal;    /* checked when used */
    rf_cal_fresh = false;
}

size_t bt_drv_cal_fresh(void)
{
    size_t n = rf_cal_fresh ? sizeof *rf_cal : 0;
    rf_cal_fresh = false;
    return n;
}

/* ---- Initialisation ---- */

/* RF_CTRL (F-0083): bus-clock divider minus 1 in [26:24], bus MHz over the
 * divider in [21:16]; bit 26 doubles as calibration mode while setting up */
#define RF_CTRL             RF_BASE
#define CTRL_CAL_MODE       (1u << 26)
#define CTRL_TIMEBASE_MASK  (0x7u << 24 | 0x3fu << 16)
#define CTRL_TIMEBASE       (1u << 24 | 16u << 16)  /* 32 MHz / 2 (F-0090) */
#define B(hi, lo)           ((0xffffffffu >> (31 - (hi))) & ~((1u << (lo)) - 1))
#define V(lo, v)            ((uint32_t)(v) << (lo))

/* Hand latches (F-0086) */
#define LATCH_A             (RF_BASE + 0x1080)
#define LATCH_A_VALUE       B(14, 8)
#define LATCH_A_START       (1u << 16)
#define LATCH_A_MANUAL      (1u << 17)
#define LATCH_B             (RF_BASE + 0x10ac)
#define LATCH_B_VALUE       B(4, 0)
#define LATCH_B_START       (1u << 8)
#define LATCH_B_MANUAL      (1u << 9)
#define SYNTH               (RF_BASE + 0x10c0)
#define SYNTH_CHANNEL       B(6, 0)
#define SYNTH_START         (1u << 8)
#define SYNTH_MANUAL        (1u << 9)
#define TUNE_PULSE          (RF_BASE + 0x10b4)
/* Per-channel tune results, read after each calibration tune: A in
 * RF_10C4 [6:0], B1 in RF_10CC [19:12] */
#define TUNE_RESULT_A       (RF_BASE + 0x10c4)
#define TUNE_RESULT_A_MASK  0x7fu
#define TUNE_RESULT_B       (RF_BASE + 0x10cc)
#define TUNE_RESULT_B_SHIFT 12
#define TUNE_RESULT_B_MASK  0xffu
#define RF_10BC_FINAL       0x1e1d0303u
#define RF_10BC_CAL         0x1e1d1303u     /* per-channel calibration only */
#define SYNTH_SETTLE_UNITS  50              /* F-0087, time based */
#define FINAL_CHANNEL       0x29            /* 2441 MHz */

/* Frequency drift defaults (F-0091) */
#define FREQ_DRIFT_DEFAULT  0x53
#define FREQ_DRIFT_TOO_BIG  0x4c

/* Register-file fields used here (F-0085, F-0087) */
#define FIELD_SYNTH_N       24
#define FIELD_SYNTH_FRAC    25
#define CELL_SYNTH          158
#define CAL_N_OFFSET        7               /* second pass: N + 7 (F-0088) */

#ifdef YP3_DEV
static bool rf_dump_first_tune;
static unsigned rf_tune_count;
static void rf_dump(uint32_t base, uint32_t size);
#endif

/* Tune the synthesizer (F-0087): latch channel, pulse, settle */
static bool rf_tune(unsigned channel, bool per_channel_cal)
{
    if (!rf_strobe(SYNTH, SYNTH_MANUAL, SYNTH_CHANNEL, channel, SYNTH_START))
        return false;
#ifdef YP3_DEV
    if (per_channel_cal && rf_dump_first_tune) {   /* F-0137 */
        rf_dump_first_tune = false;
        rf_dump(RF_1000, 0x100);
        rf_dump(RF_BASE, 0x20);
        rf_dump(RF_BASE + 0x200, 0x100);
        rf_dump(0x40080060, 0x20);      /* clock gates (F-0152) */
        rf_dump(0x400800c0, 0x40);      /* selectors 0x14..0x23 */
        rf_dump(0x40085080, 0x40);      /* PMU sequencer ports */
    }
#endif
#ifdef YP3_DEV
    if (per_channel_cal && (rf_tune_count < 3 || rf_tune_count == CHANNELS))
        dev_log("bt rf: tune %u at latch 10bc %08lx", rf_tune_count,
                (unsigned long)REG(RF_BASE + 0x10bc));
    if (per_channel_cal)
        rf_tune_count++;
#endif
    rf_set(TUNE_PULSE, 0, 1);
    rf_delay(1);
    rf_set(TUNE_PULSE, 1, 0);
    if (per_channel_cal) {
        rf_delay(1);
        REG(RF_BASE + 0x10bc) = RF_10BC_CAL;
    }
    rf_delay(SYNTH_SETTLE_UNITS);
    return true;
}

/* F-0090 steps 1..3: calibration mode on, trim, register-file side bits.
 * The block was just reset by rf_clocks_on(). */
static bool rf_prepare(void)
{
    uint32_t trim;
    if (!rf_trim(&trim))
        return false;
    rf_set(RF_CTRL, 0, CTRL_CAL_MODE);
    rf_set(RF_BASE + 0x1020, 1u << 0, 0);
    rf_set(RF_BASE + 0x1084, 0, 1u << 19);
    rf_set(RF_BASE + 0x1040, 0, 1u << 5 | V(16, trim));
    rf_set(LATCH_B, 0, 1u << 16);
    rf_set(RF_REGFILE_CTRL, B(31, 20), V(26, 1));
    return true;
}

/* Measurement-mode power-up after a fresh clock-on (F-0132, order as
 * listed there, GUESS). The register file is loaded only by rf_apply():
 * the measurement tunes by cell 158 alone (F-0139, F-0141). */
static const struct rf_step rf_meas_power[] = {
    STEP_SET(RF_CTRL, CTRL_CAL_MODE),
    STEP_CLEAR(RF_BASE + 0x1020, 1u << 0),
    STEP_SET(RF_BASE + 0x102c, 1u << 2),
    STEP_SET(RF_BASE + 0x1040, 1u << 5),
    STEP_SET(RF_BASE + 0x1064, 1u << 19),
    STEP_SET(RF_BASE + 0x1064, 1u << 18),
    STEP_SET(RF_BASE + 0x1064, 1u << 20),
    STEP_SET(LATCH_A, LATCH_A_MANUAL),
    STEP_SET(RF_BASE + 0x1084, 1u << 18),
    STEP_SET(RF_BASE + 0x1084, 1u << 19),
    STEP_SET(RF_BASE + 0x10a0, 1u << 31 | 1u << 27),
    STEP_SET(LATCH_B, LATCH_B_MANUAL),
    STEP_SET(LATCH_B, 1u << 16),
    STEP_SET(RF_BASE + 0x10b0, 1u << 24),
    STEP_SET(RF_BASE + 0x10b8, 1u << 28),
    STEP_SET(SYNTH, SYNTH_MANUAL),
    STEP_CLEAR(RF_REGFILE_CTRL, B(31, 20)),
    STEP_CLEAR(RF_BASE + 0x0200, 1u << 10 | 1u << 5),
};

/* Preparation before a group of tunes (F-0132) */
static const struct rf_step rf_meas_prepare[] = {
    STEP_CLEAR(RF_BASE + 0x1040, 1u << 2),
    STEP_RMW(RF_BASE + 0x1040, B(19, 16), V(16, 5)),
    STEP_SET(RF_BASE + 0x1040, 1u << 5),
    STEP_DELAY(1),
    STEP_SET(RF_BASE + 0x102c, 1u << 2),
    STEP_SET(RF_BASE + 0x102c, 1u << 0),
    STEP_DELAY(1),
    STEP_SET(RF_BASE + 0x10b0, 1u << 24),
    STEP_SET(RF_BASE + 0x10b8, 1u << 28),
    STEP_SET_EACH(TUNE_PULSE, B(15, 1)),
    STEP_DELAY(10),
};
#define PMU_XTAL_MEASURE    0x4c    /* during the measurements (F-0132) */

static bool rf_bringup(void)
{
#ifdef YP3_DEV
    {   /* F-0162: register file before any write */
        static const unsigned cells[] = { 0, 158, 244, 287 };
        uint32_t v[4] = { 0 };
        uint32_t ctrl = REG(RF_REGFILE_CTRL);
        for (unsigned k = 0; k < 4; k++)
            regfile_read(cells[k], &v[k]);
        dev_log("bt rf: at clock-on 1004 %08lx cells 0 %08lx 158 %08lx "
                "244 %08lx 287 %08lx", (unsigned long)ctrl,
                (unsigned long)v[0], (unsigned long)v[1],
                (unsigned long)v[2], (unsigned long)v[3]);
    }
#endif
    if (!RF_RUN(rf_meas_power))     /* clocks: bt_rf_yp3_init() */
        return false;
    rf_set(PMU_XTAL, B(11, 4), V(4, PMU_XTAL_MEASURE));
    return RF_RUN(rf_meas_prepare);
}

/* ---- I/Q value (F-0088 part 2) ----
 * RF_10A4 [28:24] sets the corner of a low-pass filter in the RF block's
 * loop-back: higher values narrow it, monotonic, about 0.21 dB more loss
 * at 1.07 MHz per step (F-0174, response map). The block plays a complex
 * tone from exchange memory (RF_100C) and captures 128 I/Q samples
 * (RF_1010) at about 11.4 Msamples/s (F-0174, F-0180).
 * Our choice of value: a Bluetooth BR channel's GFSK signal (1 Msym/s,
 * modulation index about 0.3, Core spec Vol 2 Part A) keeps nearly all its
 * energy within +-0.5 MHz of the carrier; a narrower filter rejects more
 * of the adjacent channels at +-1 MHz. So: the narrowest value whose loss
 * at 0.53 MHz (bin 6) against 0.18 MHz (bin 2) is at most 1 dB. Not yet
 * checked against link quality (weak-link RX tally, F-0175). */
#define RF_TONE             (RF_BASE + 0x100c)
#define RF_CAPTURE          (RF_BASE + 0x1010)
#define EM_RUN              (1u << 31)  /* RF_100C/RF_1010: on; capture: busy */
#define EM_CAPTURE_DIR      (1u << 15)
#define EM_FIRST(off)       ((uint32_t)(off) << 16)     /* [30:16] */
#define EM_LAST(off)        ((uint32_t)(off))   /* [14:0], from the start (F-0180) */
#define RF_1000_TONE        (1u << 14)  /* set before tone playback */
#define IQ_REG              (RF_BASE + 0x10a4)
#define IQ_SHIFT            24
#define IQ_CHANNEL          84          /* 2484 MHz (F-0087) */
#define TONE_EM             0x3000
#define TONE_POINTS         1024
#define TONE_SETTLE_UNITS   101
#define IQ_SAMPLES          128
#define IQ_REF_STEP         16          /* bin 2, 0.18 MHz */
#define IQ_TEST_STEP        48          /* bin 6, 0.53 MHz: the channel edge */
#define IQ_EDGE_LOSS_DB     1.0         /* most loss allowed at the edge */
#define IQ_CAPTURES         4           /* averaged per value */
#define IQ_LEVEL_MIN_DBFS   (-40.0)     /* below: no tone came back */
/* Checks that the loop carried the tone: the reference, below every
 * corner, stays flat over the values (F-0174 map), and the edge loss
 * grows from the widest to the narrowest value. Ours. */
#define IQ_REF_SPREAD_DB    1.5
#define IQ_EDGE_RISE_DB     0.5
#define FIELD_TONE_A        6           /* entry 0 = -32 for the tone */
#define FIELD_TONE_B        3           /* entry 0 = 1 */
/* This unit's measured value (F-0131, F-0174), when the measurement fails */
#define IQ_FALLBACK         12

/* Fill the tone table, advancing `step` of 1024 sine points per sample
 * (a whole number of periods), and start playback. */
static void iq_tone(unsigned step)
{
    rf_set(RF_TONE, EM_RUN, 0);
    for (unsigned k = 0; k < TONE_POINTS; k++) {
        unsigned ph = (k * step) % TONE_POINTS;
        REG16(EM_BASE + TONE_EM + EM_ENTRY_BYTES * k) = rf_cos(ph);
        REG16(EM_BASE + TONE_EM + EM_ENTRY_BYTES * k + 2) = rf_sin(ph);
    }
    REG(RF_TONE) = EM_RUN | EM_FIRST(TONE_EM) |
        EM_LAST(EM_ENTRY_BYTES * (TONE_POINTS - 1));
    rf_delay(TONE_SETTLE_UNITS);
}

/* 10 log10 of a 64-bit power */
static double iq_db(uint64_t p)
{
    int k = 0;
    while (p > INT32_MAX) {
        p >>= 2;
        k++;
    }
    return 10.0 * rf_log10((int32_t)p) + k * 6.020599913279624;
}

/* dB from a tone's DFT power to its amplitude re the converter's full
 * scale (12 bits): |X| = A * N/2 * 1023 for amplitude A, 1023 the table's
 * peak; 20 log10(64 * 1023 * 2048) */
#define IQ_DBFS_OFFSET      162.548

/* Capture 128 samples; level of `bin` in dBFS, mean of the two channels */
static bool iq_capture(unsigned bin, double *dbfs)
{
    REG(RF_CAPTURE) = EM_RUN | EM_CAPTURE_DIR | EM_FIRST(0) |
        EM_LAST(EM_ENTRY_BYTES * (IQ_SAMPLES - 1));
    if (!rf_wait_clear(RF_CAPTURE, EM_RUN))
        return false;
    double sum = 0.0;
    for (unsigned q = 0; q < 2; q++) {
        int64_t re = 0, im = 0;
        for (unsigned n = 0; n < IQ_SAMPLES; n++) {
            int32_t x = capture_sample(n, q);
            unsigned ph = (n * bin * (TONE_POINTS / IQ_SAMPLES)) % TONE_POINTS;
            re += (int64_t)x * rf_cos(ph);
            im -= (int64_t)x * rf_sin(ph);
        }
        uint64_t p = (uint64_t)(re * re) + (uint64_t)(im * im);
        sum += (p ? iq_db(p) : 0.0) - IQ_DBFS_OFFSET;
    }
    *dbfs = sum / 2;
    return true;
}

/* Mean of IQ_CAPTURES captures */
static bool iq_level(unsigned bin, double *dbfs)
{
    double sum = 0.0, d;
    for (unsigned k = 0; k < IQ_CAPTURES; k++) {
        if (!iq_capture(bin, &d))
            return false;
        sum += d;
    }
    *dbfs = sum / IQ_CAPTURES;
    return true;
}

/* Field 3 entry 0: 1 while a tone loads, 7 at the captures (F-0182) */
static bool iq_field3(int value)
{
    return field_write(FIELD_TONE_B, 0, value) &&
           rf_strobe(LATCH_B, LATCH_B_MANUAL, 0, 0, LATCH_B_START);
}

/* Load a tone: field 3 = 1, table, playback on, settle (F-0088) */
static bool iq_load(unsigned step)
{
    if (!iq_field3(1))
        return false;
    iq_tone(step);
    return true;
}

/* The level at every RF_10A4 value for the tone already playing, one
 * unit after each change */
static bool iq_sweep(unsigned step, double db[IQ_VALUES])
{
    if (!iq_field3(7))
        return false;
    for (unsigned v = 0; v < IQ_VALUES; v++) {
        rf_set(IQ_REG, B(28, 24), V(IQ_SHIFT, v));
        rf_delay(1);
        if (!iq_level(step / (TONE_POINTS / IQ_SAMPLES), &db[v]))
            return false;
    }
    return true;
}

/* The loop-back steps below that run, one bit each; dev builds can change
 * them (`bt rf iqw MASK`) and add bit 6 (gate 35 at the reference
 * capture). */
#define IQ_WORDS_ALL        0x3f
#ifdef YP3_DEV
static unsigned iq_words = IQ_WORDS_ALL;
#else
static const unsigned iq_words = IQ_WORDS_ALL;
#endif

/* Commit register-file entries with a hand latch start (F-0086) */
static bool latch_a_start(void)
{
    rf_set(LATCH_A, 0, LATCH_A_START);
    return rf_wait_clear(LATCH_A, LATCH_A_START);
}

static bool field_zero_a(const uint8_t *ids, unsigned n)
{
    for (unsigned k = 0; k < n; k++)
        if (!field_write(ids[k], 0, 0))
            return false;
    return latch_a_start();
}

/* Loop-back set-up for the I/Q measurement (F-0182; the order is the
 * earlier driver's, need not measured): at the 2442 MHz preparation tune,
 * the receive chain enables (RF_1064), the RF_1084 hold-and-release
 * sequence (GUESS: offset settling), fields 12..16, 22, 23 entry 0 cleared
 * with RF_1060's two 5-bit fields, and the fixed sweep values. */
#define IQ_PREP_CHANNEL     42          /* 2442 MHz (F-0087) */
static bool iq_loopback_setup(void)
{
    static const uint8_t g1[] = { 14, 15 }, g2[] = { 23, 22 },
                         g3[] = { 12, 13, 16 };
    if (!rf_tune(IQ_PREP_CHANNEL, false))
        return false;
    if (iq_words & 1) {                 /* RF_1064 */
        uint32_t f0 = 0;
        rf_set(RF_BASE + 0x1064, 1u << 7, 0);
        if (!field_read(0, 0, &f0))
            return false;
        rf_set(RF_BASE + 0x1064, 0, 1u << 19 | 1u << 16 | 1u << 6 | 1u << 5);
        rf_delay(1);
        rf_set(RF_BASE + 0x1064, 0, 1u << 19 | B(4, 0));
        rf_delay(1);
        if (f0 & B(7, 4))
            rf_set(RF_BASE + 0x1064, 0, 1u << 7);
    }
    if (iq_words & 2) {                 /* RF_1084 group 0 */
        uint32_t r = RF_BASE + 0x1084;
        rf_set(r, 0, 1u << 18);
        rf_set(r, 0, 1u << 5);
        rf_set(r, 1u << 8 | 1u << 7 | 1u << 4, 0);
        rf_set(r, 0, 1u << 3 | 1u << 2);
        rf_set(r, 1u << 1, 0);
        rf_delay(4);
        rf_set(r, 0, 1u << 8);
        rf_set(r, 0, 1u << 7);
        rf_delay(1);
        rf_set(r, 0, 1u << 4);
        rf_set(r, 1u << 3 | 1u << 2, 0);
        rf_delay(1);
    }
    if (iq_words & 4) {                 /* RF_1060, fields 12..16, 22, 23 */
        rf_set(RF_BASE + 0x1060, B(30, 26) | B(14, 10), V(26, 0x0c) | V(10, 0x0c));
        rf_set(LATCH_A, B(2, 0), 4);
        if (!field_zero_a(g1, sizeof g1) || !field_zero_a(g2, sizeof g2) ||
            !field_zero_a(g3, sizeof g3))
            return false;
    }
    if (iq_words & 8) {                 /* fixed sweep values */
        rf_set(RF_BASE + 0x1084, B(31, 28), V(28, 6));
        rf_set(RF_BASE + 0x1088, B(31, 28) | B(13, 8), V(28, 6) | V(11, 5) | V(8, 5));
    }
    return true;
}

/* In measurement mode after the channel sweeps (F-0148, F-0182, F-0186) */
static bool rf_measure_iq(const uint32_t *channel, uint8_t *iq)
{
    static double ref[IQ_VALUES], test[IQ_VALUES];
    /* a fresh block for this group, as for the channels (F-0132), on the
     * static register file (F-0182, F-0186) */
    if (!rf_clocks_on() || !rf_bringup() || !load_cells(channel))
        return false;
#ifdef YP3_DEV
    /* Dev mask bit 6: gate 35 (USB) on at the reference capture
     * (F-0189 (c)) */
    if ((iq_words & 64) && !sl6801_clock_gate_enable(CLK_GATE_ID_USB))
        return false;
#endif
    /* receive path words at the capture (F-0180) */
    REG(RF_BASE + 0x10a0) = 0xb86b0101;
    rf_set(IQ_REG, B(31, 29), V(29, 7));
    REG(RF_BASE + 0x10a8) = 0x00000070;
    /* RF_0120 bits 12:0 come up at 500, which leaves the capture about
     * 32 dB low; they must be 0 for the I/Q captures, with bits 19:17 = 3
     * in the same store (F-0199) */
    rf_set(RF_BASE + 0x120, B(19, 17) | B(12, 0), V(17, 3));
    rf_set(RF_1000, 0, RF_1000_TONE);
    if (!field_write(FIELD_TONE_A, 0, -32) ||
        !rf_strobe(LATCH_B, LATCH_B_MANUAL, 0, 0, LATCH_B_START) ||
        !iq_loopback_setup())
        return false;
    /* field 0 entry 0 = 0x73, after the RF_1064 step reads it (F-0182) */
    if (!field_write(0, 0, 0x73) ||
        !rf_strobe(LATCH_B, LATCH_B_MANUAL, 0, 0, LATCH_B_START) ||
        !rf_tune(IQ_CHANNEL, false) || !iq_load(IQ_REF_STEP))
        return false;
#ifdef YP3_DEV
    rf_dump(RF_1000, 0x100);
    rf_dump(RF_BASE + 0x100, 0x40);  /* RF digital (F-0195 (b)) */
    rf_dump(RF_BASE + 0x300, 0x20);
    rf_dump(0x40020000, 0x100);     /* BT core block (F-0195 (c)) */
#endif
    bool ok = iq_sweep(IQ_REF_STEP, ref) && iq_load(IQ_TEST_STEP) &&
              iq_sweep(IQ_TEST_STEP, test);
    rf_set(RF_TONE, EM_RUN, 0);
    rf_set(RF_1000, RF_1000_TONE, 0);
    if (!ok)
        return false;
    double lo = ref[0], hi = ref[0];
    for (unsigned v = 1; v < IQ_VALUES; v++) {
        lo = ref[v] < lo ? ref[v] : lo;
        hi = ref[v] > hi ? ref[v] : hi;
    }
    /* narrowest value with the edge loss within IQ_EDGE_LOSS_DB */
    unsigned best = IQ_VALUES;
    for (unsigned v = 0; v < IQ_VALUES; v++)
        if (ref[v] - test[v] <= IQ_EDGE_LOSS_DB)
            best = v;
    bool carried = lo > IQ_LEVEL_MIN_DBFS && hi - lo <= IQ_REF_SPREAD_DB &&
        (ref[IQ_VALUES - 1] - test[IQ_VALUES - 1]) - (ref[0] - test[0]) >=
            IQ_EDGE_RISE_DB;
#ifdef YP3_DEV
    for (unsigned v = 0; v < IQ_VALUES; v += 4)
        dev_log("bt rf: iq %2u..%2u ref cdBFS/loss cdB %d/%d %d/%d %d/%d %d/%d", v,
                v + 3, (int)(ref[v] * 100), (int)((ref[v] - test[v]) * 100),
                (int)(ref[v + 1] * 100), (int)((ref[v + 1] - test[v + 1]) * 100),
                (int)(ref[v + 2] * 100), (int)((ref[v + 2] - test[v + 2]) * 100),
                (int)(ref[v + 3] * 100), (int)((ref[v + 3] - test[v + 3]) * 100));
#endif
    if (best == IQ_VALUES || !carried) {
#ifdef YP3_DEV
        dev_log("bt rf: iq reference %d..%d cdBFS: no usable tone",
                (int)(lo * 100), (int)(hi * 100));
#endif
        return false;
    }
    *iq = best;
    return true;
}

/* Fresh calibration (F-0088): part 1, for each channel tune by cell 158
 * and read the two results, at N and at N + 7; part 2, the I/Q value. */
static bool rf_measure(struct rf_cal *c)
{
#ifdef YP3_DEV
    rf_dump_first_tune = false;
    rf_tune_count = 0;
#endif
    /* Two sweeps over the channels, at N then at N + 7 (F-0088): tuning
     * N and N + 7 per channel left B1 unsettled on every channel (B1 0x21
     * on 82 of 86, F-0136). */
    for (unsigned pass = 0; pass < 2; pass++) {
        for (unsigned i = 0; i < CHANNELS; i++) {
            /* cell 158 = this channel's static N and fraction (cell
             * 158 + i), N + 7 in the second sweep: one whole-word write */
            uint32_t n = field_static(FIELD_SYNTH_N, i) + pass * CAL_N_OFFSET;
            uint32_t word = (bt_rf_cells[CELL_SYNTH + i] &
                             ~field_bits(FIELD_SYNTH_N, UINT32_MAX)) |
                            field_bits(FIELD_SYNTH_N, n);
            if (!regfile_write(CELL_SYNTH, word))
                return false;
#ifdef YP3_DEV
            if (pass == 0 && i < 2) {   /* F-0138: read it back */
                uint32_t back = 0;
                bool ok = regfile_read(CELL_SYNTH, &back);
                dev_log("bt rf: ch %u cell 158 wrote %08lx read %08lx%s", i,
                        (unsigned long)word, (unsigned long)back,
                        ok ? "" : " (read timed out)");
            }
#endif
            if (!rf_tune(0, true))
                return false;
            uint32_t raw_a = REG(TUNE_RESULT_A);
            uint32_t raw_b = REG(TUNE_RESULT_B);
            uint32_t a = raw_a & TUNE_RESULT_A_MASK;
            uint32_t b = (raw_b >> TUNE_RESULT_B_SHIFT) & TUNE_RESULT_B_MASK;
#ifdef YP3_DEV
            if (pass == 0 && i < 8)     /* compare with F-0142 */
                dev_log("bt rf: ch %u 10bc %08lx 10c4 %08lx 10c8 %08lx "
                        "10cc %08lx", i,
                        (unsigned long)REG(RF_BASE + 0x10bc),
                        (unsigned long)raw_a,
                        (unsigned long)REG(RF_BASE + 0x10c8),
                        (unsigned long)raw_b);
#endif
            uint32_t v = (b | a << 8) << (16 * pass);
            c->channel[i] = pass ? c->channel[i] | v : v;
        }
    }
    /* back to the static synthesizer entry */
    if (!regfile_write(CELL_SYNTH, bt_rf_cells[CELL_SYNTH]))
        return false;
    RF_LOG("measured 86 channels");
    if (rf_measure_iq(c->channel, &c->iq)) {
        RF_LOG("iq %u measured", c->iq);
    } else {
        c->iq = IQ_FALLBACK;
        RF_LOG("iq measurement failed: %u (F-0131)", c->iq);
    }
    return true;
}

static const uint32_t rf_ramp_regs = RF_BASE + 0x298;

/* F-0090, all steps, from the block's reset state: the clocks are toggled
 * again first (F-0082), so measuring before does not matter. */
static bool rf_apply(const struct rf_cal *c, uint32_t freq_drift)
{
    if (freq_drift == FREQ_DRIFT_NONE)
        freq_drift = FREQ_DRIFT_DEFAULT;
    else if (freq_drift > 0xff)
        freq_drift = FREQ_DRIFT_TOO_BIG;

    /* 1..4 */
    if (!rf_clocks_on() || !rf_prepare() || !load_cells(c->channel))
        return false;
    /* 5 */
    rf_set(RF_BASE + 0x10a0, B(30, 28) | 1u << 24 | 1u << 18, V(28, 3));
    rf_set(IQ_REG, B(31, 24) | B(17, 12) | B(9, 8) | B(6, 4),
           V(29, 7) | V(24, c->iq) | V(8, 3) | V(4, 4) | 1u << 11);
    rf_set(RF_BASE + 0x10a8, B(9, 4), V(4, 1) | V(6, 1));
    if (REG(RF_BASE + 0x120) & B(12, 0))
        rf_set(RF_BASE + 0x10a8, 0, 1u << 8);
    rf_set(RF_BASE + 0x10a8, B(11, 10), V(10, 1));
    /* 6 */
    rf_set(RF_BASE + 0x1060, B(25, 24) | B(17, 16) | B(14, 10) | B(30, 26) |
           B(2, 0), V(24, 1) | V(16, 2) | V(10, 13) | V(26, 13) | V(0, 7));
    rf_set(LATCH_A, B(2, 0), V(0, 4));
    /* 7 */
    rf_set(RF_BASE + 0x1088, B(31, 28), V(28, 6));
    rf_set(RF_BASE + 0x1084, B(31, 28), V(28, 6));
    rf_set(RF_BASE + 0x1088, B(13, 8), V(8, 5) | V(11, 5));
    rf_set(RF_BASE + 0x1020, 1u << 21, 0);
    rf_set(RF_BASE + 0x1020, 0, 1u << 21);
    rf_set(RF_BASE + 0x1024, B(31, 28), 0);
    /* 8 */
    if (!rf_tune(FINAL_CHANNEL, false))
        return false;
    rf_set(SYNTH, SYNTH_MANUAL, 0);
    rf_set(RF_BASE + 0x10e4, B(29, 21), V(21, 7));
    /* 9 */
    rf_set(PMU_XTAL, B(11, 4), V(4, freq_drift));
    rf_set(RF_BASE + 0x10b8, B(30, 29), 0);
    REG(RF_BASE + 0x10bc) = RF_10BC_FINAL;
    if (!rf_strobe(LATCH_A, LATCH_A_MANUAL, LATCH_A_VALUE, V(8, 0x1a),
                   LATCH_A_START))
        return false;
    rf_set(LATCH_A, LATCH_A_MANUAL, 0);
    if (!rf_strobe(LATCH_B, LATCH_B_MANUAL, LATCH_B_VALUE, 0x12,
                   LATCH_B_START))
        return false;
    rf_set(LATCH_B, LATCH_B_MANUAL, 0);
    rf_set(RF_BASE + 0x1040, B(14, 8), 0);
    rf_set(LATCH_A, B(30, 20), V(20, 0x60) | V(28, 3));
    rf_set(RF_BASE + 0x10a0, B(2, 0) | B(15, 8), V(8, 0x60));
    rf_set(LATCH_B, B(31, 18), V(18, 5) | V(25, 5));
    rf_set(RF_BASE + 0x10b0, B(6, 0) | B(14, 8) | B(22, 16) | B(30, 25),
           V(8, 0x31) | V(16, 3) | V(25, 0x1d));
    rf_set(RF_CTRL, CTRL_CAL_MODE, 0);
    rf_delay(100);
    /* 10: the digital part. The timebase in one store (measured required) */
    rf_set(RF_CTRL, CTRL_TIMEBASE_MASK, CTRL_TIMEBASE);
    for (unsigned i = 0; i < BT_RF_RAMP_WORDS; i++)
        REG(rf_ramp_regs + 4 * i) = bt_rf_ramp[i];
    REG(RF_BASE + 0x01c) = 1;
    REG(RF_BASE + 0x100) = 0x80;
    REG(RF_BASE + 0x120) = 0x800703e8;
    REG(RF_BASE + 0x2f0) = 0x0022c3e8;
    REG(RF_BASE + 0x270) = 0x9a805e1b;
    REG(RF_BASE + 0x274) = 0x00f00341;
    REG(RF_BASE + 0x280) = 0x0eede000;
    REG(RF_BASE + 0x20c) = 0x18601111;
    rf_set(RF_BASE + 0x290, 0x3f3f00, 0x393f00);
    REG(RF_BASE + 0x21c) = 0x000cb88a;
    REG(RF_BASE + 0x2c0) = 0x600c6020;
    REG(RF_BASE + 0x2c4) = 0x01303779;
    REG(RF_BASE + 0x30c) = 0x00373d73;
    REG(RF_BASE + 0x108) = 0x14360422;
    REG(RF_BASE + 0x2c8) = 0x031cc1f4;
    REG(RF_BASE + 0x2b8) = 0x001350c0;
    REG(RF_BASE + 0x2f8) = 0x00013765;
    REG(RF_BASE + 0x30c) = 0x00353b73;
    rf_set(RF_BASE + 0x228, B(13, 8), V(8, 0x1c));
    rf_set(RF_BASE + 0x2f4, B(13, 8), V(8, 0x22));
    rf_set(RF_BASE + 0x2fc, B(17, 12), V(12, 0x2c));
    rf_set(RF_BASE + 0x200, 1u << 15, 1u << 13);
    return true;
}

/* ---- Interface ---- */

/* Enable just the BT clocks (gate 34, selector 0x1d, clock 0x29) without the
 * RF calibration, so bt-ctrl can bring the controller core and exchange
 * memory up (they stall on the bus otherwise). bt_rf_yp3_off() gates them.
 * The off/on toggle of rf_clocks_on() resets the RF block: run after
 * bt_rf_yp3_init(), none of its values survived (dump 2026-10-21), and
 * RF_BASE + 0 [21:16] (bus MHz / divider) fell back to 24, a default meant
 * for a 48 MHz bus (ours is 32 MHz): the controller timebase ran at 2/3
 * speed and the radio was silent. So leave running clocks alone. */
bool bt_yp3_clocks_on(void)
{
    return rf_clocks_running() || rf_clocks_on();
}

/* Nothing lent: measure into a copy on the stack and keep nothing (in its
 * own frame, so a lent buffer costs no stack) */
static NO_INLINE bool rf_init_unlent(uint32_t freq_drift)
{
    struct rf_cal scratch;
    return rf_measure(&scratch) && rf_apply(&scratch, freq_drift);
}

/* Measure, or reuse a valid lent calibration, then program */
bool bt_rf_yp3_init(void)
{
    uint32_t freq_drift = rf_freq_drift();
    uint32_t key = rf_cal_key(freq_drift);
    bool cached = rf_cal && rf_cal_held && rf_cal_valid(rf_cal, key);
    bool ok;

#ifdef YP3_DEV
    rf_log_t = rf_log_t0 = COUNTER;
#endif
    RF_LOG("freq_drift %#lx, %s calibration", (unsigned long)freq_drift,
           cached ? "cached" : "full");
    if (!rf_clocks_on() || !rf_bringup()) {
        ok = false;
    } else if (!rf_cal) {
        ok = rf_init_unlent(freq_drift);
    } else {
        if (!cached)
            rf_cal_held = false;    /* overwritten now */
        ok = (cached || rf_measure(rf_cal)) && rf_apply(rf_cal, freq_drift);
        if (ok && !cached) {
            rf_cal_seal(rf_cal, key);
            rf_cal_held = rf_cal_fresh = true;
        }
    }
    if (!ok) {
        bt_rf_yp3_off();
        return false;
    }
    RF_LOG("done");
    return true;
}

/* Reverse of rf_clocks_on(). Clock 0x29 stays on (it is on from cold
 * boot). */
void bt_rf_yp3_off(void)
{
    rf_set(CLK_SEL_1D, CLK_SEL_ENABLE, 0);
    sl6801_clock_gate_disable(CLK_GATE_ID_BT);
}

#ifdef YP3_DEV
static bool rf_clocked(void)
{
    if (rf_clocks_running())
        return true;
    dev_log("bt rf: clocks off (bt rf init first)");
    return false;
}

static void rf_dump(uint32_t base, uint32_t size)
{
    for (uint32_t off = 0; off < size; off += 32)
        dev_log("bt rf %08lx: %08lx %08lx %08lx %08lx %08lx %08lx %08lx "
                "%08lx",
                    (unsigned long)(base + off),
                    (unsigned long)REG(base + off),
                    (unsigned long)REG(base + off + 4),
                    (unsigned long)REG(base + off + 8),
                    (unsigned long)REG(base + off + 12),
                    (unsigned long)REG(base + off + 16),
                    (unsigned long)REG(base + off + 20),
                    (unsigned long)REG(base + off + 24),
                    (unsigned long)REG(base + off + 28));
}

/* With the stack off, "bt rf init" lends the driver a buffer of its own
 * (allocated at the first one, freed by "bt rf cal clear"), so a second
 * init runs from the first one's calibration: compare "bt rf regs" and
 * "bt rf cells" after each. */
static int dev_cal_handle;
static bool dev_cal_held;

static void dev_init(void)
{
    bool lend = !rf_cal;
    if (lend && dev_cal_handle <= 0) {
        dev_cal_handle = core_alloc(sizeof(struct rf_cal));
        if (dev_cal_handle > 0)
            core_pin(dev_cal_handle);
        dev_cal_held = false;
    }
    if (lend && dev_cal_handle > 0)
        bt_drv_cal_attach(core_get_data(dev_cal_handle), sizeof(struct rf_cal),
                          dev_cal_held ? sizeof(struct rf_cal) : 0);
    uint32_t start = COUNTER;
    bool ok = bt_rf_yp3_init();
    dev_log("bt rf init: %s in %lu ms", ok ? "ok" : "FAILED",
                (unsigned long)(COUNTER - start) / 1000);
    if (lend) {
        dev_cal_held = rf_cal_held;
        bt_drv_cal_attach(NULL, 0, 0);
    }
}

/* bt rf init | off | regs | cells | cal [clear] | psmp NAME */
bool bt_rf_yp3_dev(char *arg)
{
    if (!strcmp(arg, "cal")) {
        const struct rf_cal *c = rf_cal;
        if (!c && dev_cal_handle > 0)
            c = (const struct rf_cal *)core_get_data(dev_cal_handle);
        uint32_t key = rf_cal_key(rf_freq_drift());
        if (c)
            dev_log("bt rf cal (%s buffer): %s, iq %u, key %08lx (here %08lx)",
                        rf_cal ? "stack" : "dev",
                        rf_cal_valid(c, key) ? "valid" :
                        c->magic == RF_CAL_MAGIC ? "invalid" : "none",
                        c->iq, (unsigned long)c->key, (unsigned long)key);
        else
            dev_log("bt rf cal: no buffer (key here %08lx)",
                        (unsigned long)key);
    } else if (!strcmp(arg, "cal clear")) {
        if (dev_cal_handle > 0)
            core_free(dev_cal_handle);  /* the next init measures */
        dev_cal_handle = 0;
        dev_cal_held = false;
        if (rf_cal)
            rf_cal_held = false;
        dev_log("bt rf cal cleared");
    } else if (!strncmp(arg, "iqw ", 4)) {
        iq_words = atoi(arg + 4);   /* decimal */
        dev_log("bt rf iq words %02x", iq_words);
    } else if (!strcmp(arg, "psmp")) {
        psmp_list();
    } else if (!strncmp(arg, "psmp ", 5)) {
        const uint8_t *p = psmp_find(arg + 5);
        struct psmp_rec r;
        if (p && psmp_decode(p, &r))
            dev_log("psmp %s: flash %06lx, %u bytes: %02x %02x %02x %02x",
                    arg + 5, (unsigned long)(p - (const uint8_t *)PSMP_BASE
                                             + 0x1fc000),
                    (unsigned)r.len, r.value[0], r.value[1],
                    r.len > 2 ? r.value[2] : 0, r.len > 3 ? r.value[3] : 0);
        else
            dev_log("psmp %s: not found", arg + 5);
    } else if (!strcmp(arg, "init")) {
        dev_init();
    } else if (!strcmp(arg, "off")) {
        bt_rf_yp3_off();
        dev_log("bt rf off");
    } else if (!strcmp(arg, "regs")) {
        if (rf_clocked()) {
            rf_dump(RF_BASE, RF_DUMP_DIGITAL);
            rf_dump(RF_1000, RF_DUMP_ANALOG);
            dev_log("bt rf pmu90 %08lx", (unsigned long)REG(PMU_XTAL));
        }
    } else if (!strcmp(arg, "cells") || !strcmp(arg, "cells cal")) {
        unsigned cells_from = arg[5] ? 240 : 0;    /* "cal": 240.. only */
        uint32_t v[8];
        for (unsigned c = cells_from; rf_clocked() && c < BT_RF_CELLS; c += 8) {
            unsigned n = BT_RF_CELLS - c < 8 ? BT_RF_CELLS - c : 8;
            memset(v, 0, sizeof v);
            for (unsigned i = 0; i < n; i++)
                if (!regfile_read(c + i, &v[i]))
                    return true;
            dev_log("bt rf cell %3u: %08lx %08lx %08lx %08lx %08lx %08lx "
                    "%08lx %08lx",
                        c, (unsigned long)v[0], (unsigned long)v[1],
                        (unsigned long)v[2], (unsigned long)v[3],
                        (unsigned long)v[4], (unsigned long)v[5],
                        (unsigned long)v[6], (unsigned long)v[7]);
        }
    } else if (!strncmp(arg, "psmp ", 5)) {
        uint8_t v[64];
        char hex[2 * sizeof v + 1];
        int len = bt_yp3_psmp_read(arg + 5, v, sizeof v);
        for (int i = 0; i < len; i++)
            snprintf(hex + 2 * i, 3, "%02x", v[i]);
        hex[len > 0 ? 2 * len : 0] = '\0';
        dev_log("bt rf psmp %s: len %d %s", arg + 5, len, hex);
    } else {
        return false;
    }
    return true;
}
#endif
