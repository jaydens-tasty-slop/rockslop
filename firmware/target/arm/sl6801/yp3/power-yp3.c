/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "system.h"
#include "kernel.h"
#include "power.h"
#include "powermgmt.h"
#include "backlight-target.h"
#include "regs/cortex-m/cm_systick.h"
#include "pmu-yp3.h"
#include "recovery-yp3.h"
#include "dev-yp3.h"
#include "mmio-sl6801.h"

#define PMU_POLLS 1000     /* bridge poll budget (capped at 64 polls) */

/* Thread-context lock wait: up to 50 sleeps of one tick each. */
#define PMU_LOCK_TRIES 50

/* Battery voltage: PMU_REG_BATTERY raw code -> mV = 2800 + 14.173 * raw
 * (not checked against a meter). */
#define BATTERY_MV_BASE          2800
#define BATTERY_UV_PER_STEP      14173
#define BATTERY_MV_INITIAL       3800    /* returned until the first reading */

/* Cached by the power thread (_battery_voltage): PMU[0x2a] bit 2 = external
 * power present, PMU[0x17] bit 2 = charging enabled. */
static bool power_present, charge_enabled;

unsigned short battery_level_disksafe = 3400;
/* The 0% point: the 3350 mV cut at the end of our discharge runs
 * (percent_to_volt_discharge[0]). */
unsigned short battery_level_shutoff = 3350;
/* Readings during MP3 playback at each tenth of the run time, from a full
 * charge to the 3350 cut (~19.3 h). */
unsigned short percent_to_volt_discharge[11] =
{
    3350, 3551, 3690, 3791, 3834, 3860, 3890, 3976, 4006, 4021, 4075
};
/* Readings while charging at each tenth of the charge time, from the cut to
 * the PMU full event (~1 h, two charges). */
unsigned short percent_to_volt_charge[11] =
{
    3650, 3742, 3877, 3940, 3983, 4011, 4068, 4125, 4153, 4181, 4224
};
/* On USB with charging off the system runs from USB and the battery rests:
 * readings do not move with playback (2026-10-30). At rest they sit 72 mV
 * above the playback readings above (R-A 1 start); the charge logs' 60 s
 * pauses agree within a step. 100% is the rest after the full event. */
unsigned short percent_to_volt_rest[11] =
{
    3422, 3623, 3762, 3863, 3906, 3932, 3962, 4048, 4078, 4093, 4118
};

unsigned int power_input_status(void)
{
    return power_present ? POWER_INPUT_USB_CHARGER : POWER_INPUT_NONE;
}

bool charging_state(void)
{
    return power_present && charge_enabled;
}

/* PMU sequences from thread context: the button tick may hold the bridge
 * for a few ticks (a key event), so wait for the lock with sleeps. */
static bool pmu_lock_wait(void)
{
    for (int i = 0; i < PMU_LOCK_TRIES; i++) {
        if (yp3_pmu_try_lock())
            return true;
        sleep(1);
    }
    return false;
}

/* Lock for a final PMU sequence (reboot, power-off). The holder may be a
 * thread sleeping in _battery_voltage, so wait for it with the bounded
 * pmu_lock_wait(); that sleeps, so outside a thread with interrupts on
 * try once. Either way the caller goes ahead, with or without the lock:
 * once it disables interrupts no other PMU user runs again. */
static void pmu_lock_final(void)
{
    if (irq_enabled() && is_thread_context())
        pmu_lock_wait();
    else
        yp3_pmu_try_lock();
}

uint8_t yp3_pmu_boot_regs[5];   /* PMU 0x24, 0x26..0x29 at boot */

/* ---- Core supply ----------------------------------------------------------
 * The SoC's core LDO is off at boot: the core sits on the
 * DCDC/rail-4 node at ~1200 mV. The board set-up turns it on
 * (core_ldo_enable) and from then on moves only its selector: one voltage
 * per CPU clock, raised and settled before the clock goes up, lowered after
 * it comes down. */
#define CORE_LDO_BUSY            0x80000000u  /* wait clear, then write */
#define CORE_LDO_BIT29           0x20000000u  /* wait clear after bypass off */
#define CORE_LDO_READY           0x02000000u  /* wait set after enable */
#define CORE_LDO_BYPASS          0x00080000u
#define CORE_LDO_ON              0x00010000u
#define CORE_LDO_SEL_MASK        0x1fu
#define CORE_LDO_SEL2_SHIFT      8
/* Selector indexes into core_ldo_mv (1210 mV - idx * ~16). */
#define CORE_LDO_SEL_1180MV      1u         
#define CORE_LDO_SEL2_700MV      30u        
#define CORE_LDO_SEL_SETTLE_US   3000u 
#define CORE_LDO_PRE_US          1000u
#define CORE_LDO_POLLS           10000u       /* 1 us apart */
#define CORE_LDO_RAISE_US        100u
#define A0_LDO_TRIM_SHIFT        16           /* [19:16] */
#define A4_LDO                   0x10u

/* Trim bank (TRIM_*, sl6801.h; gate 0x58, selector 0x1b).
 * audiohw-yp3.c and bt-rf-yp3.c open the bank with helpers of their own,
 * which differ in detail. */
#define TRIM1_LDO_SHIFT          18       /* [21:18] */
#define TRIM_LDO_MASK            0xfu

/* Selector index -> mV (from stock firmware). */
static const unsigned short core_ldo_mv[32] =
{
    1210, 1180, 1150, 1130, 1100, 1080, 1050, 1030, 1010, 990, 970, 950,
    930, 920, 900, 880, 870, 850, 840, 820, 810, 800, 780, 770, 760, 750,
    740, 730, 720, 710, 700, 690
};

/* Memory timing: below 1020 mV the slow values are written before lowering
 * the core, and the fast ones after raising it to 1020 or more (from stock
 * firmware; hardware need not established). The values found at boot stand
 * in for the fast ones and are put back exactly, since only the 192 MHz /
 * 1050 mV state is known to work. */
#define MEM_TIMING_A_FIELDS      0x07bf07bfu  /* bits outside the field are kept */
#define MEM_TIMING_A_SLOW        0x05820582u
#define MEM_TIMING_B_FIELDS      0x1fu
#define MEM_TIMING_B_SLOW        0x12u
#define CORE_MV_SLOW_TIMING      1020               /* below: slow timing */

/* Core voltage per CPU clock: [0] CPUFREQ_NORMAL (64 MHz), [1] CPUFREQ_MAX
 * (192 MHz). 192 MHz is known good at 1050 mV. 64 MHz stays at 1050 until lower steps pass under test, and
 * until a watchdog reset is known not to leave the boot ROM's 192 MHz on a
 * 64 MHz voltage. */
static unsigned short core_mv_for[2] = { 1050, 1050 };
static unsigned core_mv;            /* selected now; 0 = LDO off, no scaling */
static uint32_t mem_timing_boot[2]; /* MEM_TIMING_A/B while not slow */
static bool mem_timing_slow;

static void wait_us(uint32_t us)
{
    for (uint32_t t = REG(SL6801_COUNTER_LO);
         REG(SL6801_COUNTER_LO) - t < us; )
        ;
}

static bool core_ldo_wait(uint32_t mask, uint32_t want)
{
    for (uint32_t i = 0; i < CORE_LDO_POLLS; i++) {
        if ((REG(PMU_PORT_CORE_LDO) & mask) == want)
            return true;
        wait_us(1);
    }
    return false;
}

/* Selector write: wait not busy, set the 5-bit field, settle. */
static bool core_ldo_select(uint32_t shift, uint32_t index)
{
    if (!core_ldo_wait(CORE_LDO_BUSY, 0))
        return false;
    REG(PMU_PORT_CORE_LDO) = (REG(PMU_PORT_CORE_LDO) &
                              ~(CORE_LDO_SEL_MASK << shift)) | index << shift;
    wait_us(CORE_LDO_SEL_SETTLE_US);
    return true;
}

/* Core LDO on: the core leaves the DCDC/rail-4 node for the LDO at 1180 mV. False if a
 * step did not complete; the caller then keeps rail 4 on. */
static bool core_ldo_enable(void)
{
    if (!core_ldo_select(0, CORE_LDO_SEL_1180MV) ||
        !core_ldo_select(CORE_LDO_SEL2_SHIFT, CORE_LDO_SEL2_700MV))
        return false;
    wait_us(CORE_LDO_PRE_US);
    uint32_t trim = (REG(TRIM_WORD_24) >> TRIM1_LDO_SHIFT) & TRIM_LDO_MASK;
    REG(PMU_PORT_A0) = (REG(PMU_PORT_A0) &
                        ~(TRIM_LDO_MASK << A0_LDO_TRIM_SHIFT)) |
                       trim << A0_LDO_TRIM_SHIFT;
    if (!core_ldo_wait(CORE_LDO_BUSY, 0))
        return false;
    REG(PMU_PORT_CORE_LDO) |= CORE_LDO_ON;
    if (!core_ldo_wait(CORE_LDO_READY, CORE_LDO_READY) ||
        !core_ldo_wait(CORE_LDO_BUSY, 0))
        return false;
    REG(PMU_PORT_CORE_LDO) &= ~CORE_LDO_BYPASS;
    if (!core_ldo_wait(CORE_LDO_BIT29, 0))
        return false;
    REG(PMU_PORT_A4) |= A4_LDO;
    core_mv = core_ldo_mv[CORE_LDO_SEL_1180MV];
    return true;
}

/* From SRAM with IRQs masked (whether the timing is SRAM's or XIP's is
 * unknown). */
static void ICODE_ATTR NO_INLINE mem_timing_write(uint32_t a, uint32_t b)
{
    REG(MEM_TIMING_A) = a;
    REG(MEM_TIMING_B) = b;
}

/* Selector index of the lowest table voltage at or above mv, capped at
 * 1180. */
static unsigned core_ldo_index(unsigned mv)
{
    unsigned idx = ARRAYLEN(core_ldo_mv) - 1;
    while (idx > CORE_LDO_SEL_1180MV && core_ldo_mv[idx] < mv)
        idx--;
    return idx;
}

/* Move the LDO selector. A raise settles for 3 ms before returning; a lower does not wait (the next raise settles).
 * Busy waits only: no yield, so no other cpu_boost() runs in between. */
static void core_set_mv(unsigned mv)
{
    unsigned idx = core_ldo_index(mv);
    mv = core_ldo_mv[idx];
    if (mv == core_mv || !core_ldo_wait(CORE_LDO_BUSY, 0))
        return;
    bool raise = mv > core_mv, slow = mv < CORE_MV_SLOW_TIMING;
    int irq = disable_irq_save();
    if (slow && !mem_timing_slow) {
        mem_timing_boot[0] = REG(MEM_TIMING_A);
        mem_timing_boot[1] = REG(MEM_TIMING_B);
        mem_timing_write(
            (mem_timing_boot[0] & ~MEM_TIMING_A_FIELDS) | MEM_TIMING_A_SLOW,
            (mem_timing_boot[1] & ~MEM_TIMING_B_FIELDS) | MEM_TIMING_B_SLOW);
        mem_timing_slow = true;
    }
    REG(PMU_PORT_CORE_LDO) = (REG(PMU_PORT_CORE_LDO) & ~CORE_LDO_SEL_MASK) |
                             idx;
    restore_irq(irq);
    core_mv = mv;
    if (raise)
        wait_us(CORE_LDO_SEL_SETTLE_US);
    if (!slow && mem_timing_slow) {
        irq = disable_irq_save();
        mem_timing_write(mem_timing_boot[0], mem_timing_boot[1]);
        restore_irq(irq);
        mem_timing_slow = false;
    }
}

bool yp3_core_voltage(bool max_clock, bool after)
{
    unsigned mv = core_ldo_mv[core_ldo_index(core_mv_for[max_clock])];
    if (core_mv && (after ? mv < core_mv : mv > core_mv)) {
        core_set_mv(mv);
        if (!after)
            wait_us(CORE_LDO_RAISE_US);
    }
    return !core_mv || core_mv >= mv;
}

#ifdef YP3_DEV
static void core_voltage_log(void)
{
    dev_log("vcore %u/%u mV, now %u: P98=%08lx timing %08lx %08lx",
                core_mv_for[0], core_mv_for[1], core_mv,
                (unsigned long)REG(PMU_PORT_CORE_LDO),
                (unsigned long)REG(MEM_TIMING_A),
                (unsigned long)REG(MEM_TIMING_B));
}

void yp3_core_voltage_dev(unsigned mv_normal, unsigned mv_max)
{
    if (mv_normal)
        core_mv_for[0] = mv_normal;
    if (mv_max)
        core_mv_for[1] = mv_max;
    if (core_mv)
        core_set_mv(core_mv_for[cpu_frequency >= CPUFREQ_MAX]);
    core_voltage_log();
}
#endif

/* To be called after core_ldo_enable(): the voltage for the clock running now. Dev
 * config vcore=MV64[,MV192] replaces the defaults. */
static void core_voltage_init(void)
{
#ifdef YP3_DEV
    char v[16], *comma;
    if (dev_config("vcore", v, sizeof v)) {
        core_mv_for[0] = atoi(v);
        if ((comma = strchr(v, ',')))
            core_mv_for[1] = atoi(comma + 1);
    }
#endif
    core_set_mv(core_mv_for[cpu_frequency >= CPUFREQ_MAX]);
#ifdef YP3_DEV
    core_voltage_log();
#endif
}

/* ---- Board set-up at boot (F-0037, F-0039, F-0041..F-0043) ----------------
 * Without it a power-off restarted the chip warm, or woke with dead keys
 * (F-0037). Which write matters is not known, so all are kept, except
 * acknowledging the power-off and event registers, measured unneeded
 * (F-0050). PMU[0x26] is never written (F-0038). */
#define P_AC                     (PMU_BASE + 0xac)
#define P_AC_BOOT                0x30u
#define A4_BIT0                  0x01u
#define A4_BIT1                  0x02u
#define A4_FIELD_MASK            0x60u      /* 6:5 */
#define A4_FIELD_2               0x40u
#define EMERGENCY_BOOT           0x8020u
#define P_DC                     (PMU_BASE + 0xdc)
#define P_E0                     (PMU_BASE + 0xe0)
#define P_E4                     (PMU_BASE + 0xe4)
#define SLOW_CLOCK_POLLS         20000u     /* 1 us apart */
/* Sequencer step times (us), stored in slow-clock periods next to fixed
 * bits whose roles are unknown. */
#define P_DC_STEP_US             3000u
#define P_DC_FIXED               0x0a358000u
#define P_E0_STEP_US             500u
#define P_E0_STEP_SHIFT          24
#define P_E0_FIXED               0x00800060u
#define P_E4_STEP0_US            400u
#define P_E4_STEP1_US            200u
#define P_E4_STEP1_SHIFT         8
#define P_E4_FIXED               0x03700000u

static bool pmu_update(uint8_t reg, uint8_t clear, uint8_t set)
{
    uint8_t v;
    return yp3_pmu_read(reg, PMU_POLLS, &v) &&
           yp3_pmu_write(reg, (v & ~clear) | set, PMU_POLLS);
}

/* The trim bank holds the core LDO trim (F-0041 step 4, F-0047). */
static void trim_bank_on(void)
{
    sl6801_clock_gate_enable(CLK_GATE_ID_TRIM);
    if ((REG(CLK_SEL_1B) & ~CLK_SEL_ENABLE) == 0u)
        REG(CLK_SEL_1B) |= CLK_SEL_ENABLE;
}

/* F-0043: sequencer step times from the measured slow-clock rate. */
static void sequencer_timing(void)
{
    uint32_t hz = 0;
    for (uint32_t i = 0; i < SLOW_CLOCK_POLLS && !(hz = REG(PMU_PORT_SLOW_CLOCK)); i++)
        wait_us(1);
    if (!hz)
        return;
    uint32_t d = 1000000u / hz;             /* slow-clock period, us */
    if (!d)
        return;
    REG(P_DC) = (P_DC_STEP_US / d) | P_DC_FIXED;
    REG(P_E0) = ((P_E0_STEP_US / d) << P_E0_STEP_SHIFT) | P_E0_FIXED;
    REG(P_E4) = (P_E4_STEP0_US / d) | ((P_E4_STEP1_US / d) << P_E4_STEP1_SHIFT) |
                P_E4_FIXED;
}

static void yp3_board_setup(void)
{
    if (!yp3_pmu_try_lock())
        return;
    int irq = disable_irq_save();
    yp3_pmu_write(PMU_REG_2F, 0, PMU_POLLS);
    yp3_pmu_write(PMU_REG_MASK19, PMU_MASK19_BOOT, PMU_POLLS);
    yp3_pmu_write(PMU_REG_MASK1A, PMU_MASK1A_BOOT, PMU_POLLS);
    yp3_pmu_write(PMU_REG_MASK4B, PMU_MASK4B_BOOT, PMU_POLLS);
    yp3_pmu_write(PMU_REG_RESET, PMU20_NORMAL, PMU_POLLS);
    yp3_pmu_write(PMU_REG_22, 0, PMU_POLLS);
    pmu_update(PMU_REG_00, PMU00_BITS21, 0);
    trim_bank_on();
    /* The core runs from the rail-4 node until the LDO is up (F-0039). */
    if (core_ldo_enable())
        pmu_update(PMU_REG_RAILS, PMU2C_RAIL4, 0);
    pmu_update(PMU_REG_03, (uint8_t)~PMU03_KEEP, PMU03_3300MV);
    pmu_update(PMU_REG_RAILS, 0, PMU2C_RAIL3);
    pmu_update(PMU_REG_24, PMU24_FIELDS, PMU24_LONG_RESET_8S);
    REG(P_AC) = P_AC_BOOT;
    REG(PMU_PORT_A4) |= A4_BIT1;
    REG(PMU_PORT_A4) |= A4_BIT0;
    REG(PMU_PORT_A4) |= A4_LDO;
    REG(PMU_PORT_A4) = (REG(PMU_PORT_A4) & ~A4_FIELD_MASK) | A4_FIELD_2;
    REG(PMU_EMERGENCY) = EMERGENCY_BOOT;
    pmu_update(PMU_REG_00, PMU00_STATE_MASK, PMU00_STATE_RUN);
    pmu_update(PMU_REG_1B, 0, PMU1B_PULSE);
    pmu_update(PMU_REG_1B, PMU1B_PULSE, 0);
    sequencer_timing();
    restore_irq(irq);
    yp3_pmu_unlock();
    if (core_mv)
        core_voltage_init();
#ifdef YP3_DEV
    uint8_t rails = 0;
    yp3_pmu_read(PMU_REG_RAILS, PMU_POLLS, &rails);
    dev_log("board: P98=%08lx 2c=%02x dc=%08lx e0=%08lx e4=%08lx",
            (unsigned long)REG(PMU_PORT_CORE_LDO), rails,
            (unsigned long)REG(P_DC), (unsigned long)REG(P_E0),
            (unsigned long)REG(P_E4));
#endif
}

void power_init(void)
{
#ifdef YP3_RECOVERY_WATCHDOG
    /* first target hook after kernel_init(): threads can be created */
    yp3_recovery_heartbeat_start();
#endif
    /* Power-on/off reasons and key timers: which wake was a hard reset after
     * a long power-key hold. Shown
     * in System > Debug > View HW info. */
    static const uint8_t regs[] = {
        PMU_REG_24, PMU_REG_26, PMU_REG_PON27, PMU_REG_PON28, PMU_REG_POFF29
    };
    if (yp3_pmu_try_lock()) {
        for (unsigned i = 0; i < ARRAYLEN(regs); i++)
            yp3_pmu_read(regs[i], PMU_POLLS, &yp3_pmu_boot_regs[i]);
        yp3_pmu_unlock();
    }
    dev_log("pmu boot: 24=%02x 26=%02x 27=%02x 28=%02x 29=%02x",
                yp3_pmu_boot_regs[0], yp3_pmu_boot_regs[1],
                yp3_pmu_boot_regs[2], yp3_pmu_boot_regs[3],
                yp3_pmu_boot_regs[4]);
    yp3_board_setup();
}

/* Charging: the PMU charger is configured by the stock loader
 * (PMU[0x13..0x16], [0x1c]); charging runs while PMU[0x17] bit 2 is set. It
 * is set on charger-in and cleared on charger-out and on the charge-full
 * event (PMU[0x30] bit 1). The voltage limit is a safety net: readings while
 * charging sit ~80 mV high. */
#define CHARGE_STOP_MV    4250      /* = the charger's CV tier (PMU[0x14]) */
#define CHARGE_RESUME_MV  4100      /* hysteresis after a stop */
#define CHARGE_LIMIT_BAND 10        /* % below the limit before it resumes */

static int charge_limit = 100;      /* Settings > Battery > Charge Limit */
static bool limit_reached;

void set_charge_limit(int percent)
{
    if (percent != charge_limit) {
        charge_limit = percent;
        limit_reached = false;      /* decide afresh against the new limit */
    }
}

/* The limit applies to battery_level(), the level the user sees: from the
 * charge curve while charging, the rest curve when stopped (they agree
 * within a few percent). Charging stops at the
 * limit and resumes CHARGE_LIMIT_BAND below it; plugged in at or above
 * the limit, it does not start. */
static bool charge_limited(void)
{
    int level = battery_level();
    if (!power_present || charge_limit >= 100 || level < 0)
        limit_reached = false;
    else if (charge_enabled)
        limit_reached = level >= charge_limit;
    else
        limit_reached = level >= charge_limit -
                                 (limit_reached ? CHARGE_LIMIT_BAND : 0);
    return limit_reached;
}

static void charge_guard(int mv)
{
    static bool stopped;            /* full: wait for RESUME_MV */
    uint8_t v, input;
    if (yp3_pmu_read(PMU_REG_POWER_STATUS, PMU_POLLS, &input))
        power_present = input & PMU2A_EXT_POWER;
    if (!yp3_pmu_read(PMU_REG_CHARGE, PMU_POLLS, &v))
        return;
    int irq = disable_irq_save();
    uint8_t events = yp3_pmu_events30;
    yp3_pmu_events30 = 0;
    restore_irq(irq);
    if ((events & PMU30_CHARGE_FULL) || mv >= CHARGE_STOP_MV)
        stopped = true;
    else if (!power_present || mv < CHARGE_RESUME_MV)
        stopped = false;
    bool limited = charge_limited();    /* every time: it tracks unplugs */
    bool want = power_present && !stopped && !limited;
    if (want != (bool)(v & PMU17_CHARGE_ENABLE)) {
        v = want ? (v | PMU17_CHARGE_ENABLE) : (v & ~PMU17_CHARGE_ENABLE);
        if (!yp3_pmu_write(PMU_REG_CHARGE, v, PMU_POLLS))
            return;
        dev_log("charging %s at %d mV, level %d%%, limit %d%%",
                    want ? "on" : "off", mv, battery_level(), charge_limit);
    }
    charge_enabled = want;
}

/* Request a conversion with PMU[0x47] bit 4, read the 7-bit result from
 * PMU[0x2e] (constants above). */
int _battery_voltage(void)
{
    static int last_mv = BATTERY_MV_INITIAL;
    uint8_t ctl, raw;

    if (!pmu_lock_wait())
        return last_mv;
    if (yp3_pmu_read(PMU_REG_47, PMU_POLLS, &ctl) &&
        yp3_pmu_write(PMU_REG_47, ctl | PMU47_BATTERY_MEASURE, PMU_POLLS)) {
        /* conversion time: 2 ms read stale data, 20 ms works;
         * +1 tick so at least 20 ms pass */
        sleep(HZ / 50 + 1);
        if (yp3_pmu_read(PMU_REG_BATTERY, PMU_POLLS, &raw))
            last_mv = BATTERY_MV_BASE +
                      (raw & PMU2E_BATTERY_MASK) * BATTERY_UV_PER_STEP / 1000;
        yp3_pmu_write(PMU_REG_47, ctl & ~PMU47_BATTERY_MEASURE, PMU_POLLS);
        charge_guard(last_mv);
    }
    yp3_pmu_unlock();
    return last_mv;
}

/* Clear the ROM recovery reason (7, armed at boot by the recovery
 * watchdog) so the next start, after a reset or a power-on, goes to the
 * firmware (same operation as yp3dev `boot`). High bits first, so stopping
 * after the first step cannot leave reason 6 (SD update). If clearing
 * PMU[0x23] fails, PMU[0] bit 0 is left alone and the reason stays 7: the
 * player lands in ROM USB, which is recoverable; clearing bit 0 alone
 * could turn it into 6. */
static void pmu_clear_recovery_reason(void)
{
    uint8_t v;
    if (yp3_pmu_read(PMU_REG_REASON, PMU_POLLS, &v) &&
        yp3_pmu_write(PMU_REG_REASON, v & ~PMU23_REASON_MASK, PMU_POLLS) &&
        yp3_pmu_read(PMU_REG_00, PMU_POLLS, &v))
        yp3_pmu_write(PMU_REG_00, v & ~PMU00_REASON_BIT0, PMU_POLLS);
}

/* PMU reset into the firmware again. */
void system_reboot(void)
{
    uint8_t v;
#ifdef YP3_WATCHDOG_POWEROFF
    yp3_recovery_clear_run_marker();
#endif
    pmu_lock_final();
    disable_irq();
    pmu_clear_recovery_reason();
    if (yp3_pmu_read(PMU_REG_RESET, PMU_POLLS, &v))
        yp3_pmu_write(PMU_REG_RESET, v | PMU20_RESET_REQUEST, PMU_POLLS);
    yp3_recovery_park();          /* reset takes effect; else watchdog */
}

void system_exception_wait(void)
{
    /* Park without feeding: with the recovery watchdog armed the player
     * returns to ROM USB, otherwise the user resets it. */
    yp3_recovery_park();
}

/* ---- Power-off (F-0044) ---------------------------------------------------
 * The clocks must be down first (measured, F-0051) and the queued writes
 * take effect only when the core sleeps. Steps 3-7 follow F-0044's order
 * (GUESS). */
#define CLK_SEL_DIV_POLLS        10000u
#define CLK_REG_FC               0x400800fcu
#define P_60                     (PMU_BASE + 0x60)
#define P_80                     (PMU_BASE + 0x80)
#define P_80_OFF                 0xbff00001u
#define P_B0                     (PMU_BASE + 0xb0)
#define P_B8                     (PMU_BASE + 0xb8)
#define P_D0                     (PMU_BASE + 0xd0)

/* Clock selectors switched off; the ones given by address have no name
 * in sl6801.h (role unknown). */
static const uint32_t off_selectors[] = {
    CLK_SEL_11, CLK_SEL_12, CLK_SEL_13, 0x40080090, 0x400800b4, 0x400800c4,
    0x400800d0, 0x400800d4, CLK_SEL_1B, CLK_SEL_1C, CLK_SEL_1D, 0x400800ec,
    0x400800f0,
};

static inline __attribute__((always_inline)) void off_wait_us(uint32_t us)
{
    for (uint32_t t = REG(SL6801_COUNTER_LO); REG(SL6801_COUNTER_LO) - t < us; )
        ;
}

static inline __attribute__((always_inline))
void off_divider(uint32_t reg, uint32_t clear, uint32_t busy)
{
    REG(reg) &= ~clear;
    off_wait_us(15);
    for (uint32_t i = 0; i < CLK_SEL_DIV_POLLS && (REG(reg) & busy); i++)
        ;
}

/* F-0044 step 2, from SRAM: the CPU and NOR clocks change under it. */
static void ICODE_ATTR NO_INLINE clocks_down(void)
{
    for (unsigned i = 0; i < ARRAYLEN(off_selectors); i++)
        REG(off_selectors[i]) &= ~CLK_SEL_ENABLE;
    REG(CLK_CPU) &= ~CLK_CPU_SRC_MASK;
    REG(CLK_AHB) &= ~CLK_AHB_SRC_MASK;
    REG(CLK_SEL_2B) &= ~CLK_SEL_2B_SOURCE_MASK;
    off_wait_us(5);
    off_divider(CLK_CPU, CLK_CPU_DIV_MASK, CLK_CPU_BUSY);
    off_divider(CLK_CPU, 0x7000u, 1u << 30);        /* 14:12 */
    off_divider(CLK_CPU, 0x700000u, 1u << 30);      /* 22:20 */
    off_divider(CLK_AHB, CLK_AHB_DIV_MASK, 1u << 31);
    off_wait_us(5);
    REG(CLK_ROOT) &= ~(CLK_ROOT_CLK29 | CLK_ROOT_SOURCE8_ENABLE |
                       CLK_ROOT_CLK9);
    REG(CLK_REG_FC) &= ~0x10u;
    off_wait_us(5);
    REG(CLK_PLL2) &= ~CLK_PLL2_ENABLE;
    REG(CLK_AUDIO_PLL0) &= ~AUDIO_PLL0_ENABLE;
}

static void yp3_pmu_off_arm(void)
{
    uint8_t a = 0;

    pmu_update(PMU_REG_24, PMU24_KEY_TIMERS, 0);
    clocks_down();
    REG(P_60) = 1;
    REG(PMU_D8) &= ~0x6u;
    pmu_update(PMU_REG_00, PMU00_STATE_MASK, 0);
    for (unsigned i = 0; i < NVIC_WORDS; i++) {
        REG(NVIC_ICER_BASE + 4 * i) = 0xffffffffu;
        REG(NVIC_ICPR_BASE + 4 * i) = 0xffffffffu;
    }
    REG(P_80) &= ~(1u << 31);
    REG(P_80) = P_80_OFF;
    yp3_pmu_read(PMU_REG_21, PMU_POLLS, &a);
    uint32_t b = (a & 0xe7u) | 4u;
    REG(P_B0) = ((uint32_t)(a | 3u) << 8) | 0x21u;
    /* P+0xd0 bits 0 and 3 are both needed for the off (F-0212). Never set
     * bit 3 without bit 0: that left the player unable to boot (F-0213). */
    REG(P_D0) |= 1u;
    REG(P_B8) = (b << 16) | 0x21u | ((b & 0xfbu) << 24);
    pmu_update(PMU_REG_00, 0, PMU00_BIT3);
    REG(P_D0) |= 8u;
}

/* Power-off. On battery power drops and a ~3 s centre press powers on (the
 * RTC keeps running); with USB attached the PMU restarts the chip after ~4 s.
 * At full clock the off needed the PMU's ~10 s long-press reset to power on
 * again (2026-10-22). */
void power_off(void)
{
#ifdef YP3_DEV
    dev_poweroff_check();
    dev_log("power_off()");
#endif
#ifdef YP3_WATCHDOG_POWEROFF
    /* Deliberate: the next boot (a warm restart on USB) runs normally. */
    yp3_recovery_clear_run_marker();
#endif
    /* Dark at once: "Shutting down" stayed lit while the rails dropped
     * (user, 2026-10-19). */
    backlight_hw_off();
    /* Lock before interrupts go off: a holder sleeping in _battery_voltage
     * needs the scheduler to finish. Nothing below needs the lock taken
     * with interrupts already off. */
    pmu_lock_final();
    disable_irq();
    reg_var(CM_SYSTICK_CSR) = 0;                /* SysTick off */
#ifdef YP3_RECOVERY_WATCHDOG
    /* The off takes longer than the watchdog's ~16 s: its reset restarted
     * the player every time (2026-10-22). With it stopped, power drops; a
     * hang from here on needs the PMU's long-press reset. */
    yp3_recovery_watchdog_stop();
#endif
    /* With reason 7 left armed, the next power-on started the boot ROM's
     * USB recovery (dark screen), so a short key press seemed to do
     * nothing; only the PMU's 8 s long-press reset, which clears the
     * one-shot reason, gave a normal boot (2026-10-22). Cleared before
     * the final off steps: after them the clear seemed not to take. If power fails to
     * drop, the watchdog reset boots the firmware, which arms the reason
     * again. */
    pmu_clear_recovery_reason();
#ifdef YP3_DEV
    uint8_t v, a;
    if (yp3_pmu_read(PMU_REG_REASON, PMU_POLLS, &v) &&
        yp3_pmu_read(PMU_REG_00, PMU_POLLS, &a))
        dev_log("power_off: pmu 23=%02x 00=%02x", v, a);
#endif
    yp3_pmu_off_arm();
    /* The sequencer applies the queued off writes when the core sleeps; a
     * busy park kept the chip up until it restarted ~15 s later
     * (2026-10-22). PRIMASK is set, so WFI does not return; if power stays
     * up, the player stays dark until the long-press reset. */
    for (;;)
        __asm__ volatile("dsb\n\twfi\n\tisb" ::: "memory");
}

#ifdef YP3_WATCHDOG_POWEROFF
#define HANG_POLL_US 500000u        /* park: USB and key check interval */

/* Read a PMU event register and acknowledge it (write the value back). */
static uint8_t pmu_take_events(uint8_t reg)
{
    uint8_t v;
    if (!yp3_pmu_read(reg, PMU_POLLS, &v))
        return 0;
    if (v)
        yp3_pmu_write(reg, v, PMU_POLLS);
    return v;
}

/* Boot after a watchdog reset (system_init, IRQs still masked): power off.
 * With USB attached power does not drop and the PMU restarts the player
 * (power_off notes), which would repeat a hang at boot forever. There it
 * parks dark with charging off (no charge guard runs), until the cable goes
 * (then powers off) or the centre key is pressed (then boots). */
void yp3_power_off_after_hang(void)
{
    uint8_t v;
    dev_log("watchdog reset: powering off");
    if (!yp3_pmu_read(PMU_REG_POWER_STATUS, PMU_POLLS, &v) ||
        !(v & PMU2A_EXT_POWER))
        power_off();
    yp3_recovery_clear_run_marker();
    yp3_recovery_watchdog_stop();
    if (yp3_pmu_read(PMU_REG_CHARGE, PMU_POLLS, &v) &&
        (v & PMU17_CHARGE_ENABLE))
        yp3_pmu_write(PMU_REG_CHARGE, v & ~PMU17_CHARGE_ENABLE, PMU_POLLS);
    pmu_take_events(PMU_REG_EVENT31);       /* presses before the reset */
    for (;;) {
        sl6801_udelay(HANG_POLL_US);
        if (yp3_pmu_read(PMU_REG_POWER_STATUS, PMU_POLLS, &v) &&
            !(v & PMU2A_EXT_POWER))
            power_off();
        if (pmu_take_events(PMU_REG_EVENT31) & PMU31_KEY_PRESS)
            system_reboot();
    }
}
#endif
