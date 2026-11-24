/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Buttons: Up/Down on the ADC ladder, Left/Right on GPIO3.12 (probed with
 * two pin configurations), centre key via a PMU event signalled on GPIO2.2
 * (IRQ3). The scan runs from the kernel tick, one bounded phase per tick. */
#include <stdbool.h>
#include <stdint.h>

#include "system.h"
#include "button.h"
#include "kernel.h"
#include "button-target.h"
#include "adc.h"
#include "adc-yp3.h"
#include "gpio-yp3.h"
#include "irq-yp3.h"
#include "pmu-yp3.h"
#include "dev-yp3.h"
#include "mmio-sl6801.h"

#define SETTLE_TICKS      1000u   /* 1 ms of the 1 MHz counter */

/* Key ladder levels on ADC channel 1 (12-bit sample; idle reads high). */
#define LADDER_DOWN_BELOW 0x5c0u       /* < : Down */
#define LADDER_UP_BELOW   0xc90u       /* < : Up */
#define LADDER_NONE_BELOW 0xd00u       /* < : no key */
#define LADDER_SD_BELOW   0xf50u       /* < : SD-detect level (unverified) */
/* Key ids the ladder levels map to. */
#define KEY_ID_NONE       0x00u
#define KEY_ID_UP         0x25u
#define KEY_ID_SD_DETECT  0x38u        /* not a button */
#define KEY_ID_LEFT       0x42u
#define KEY_ID_RIGHT      0x43u
#define KEY_ID_DOWN       0x45u

/* Debounce: a new state is accepted on the DEBOUNCE_SCANS-th consecutive
 * scan that sees it (the first sighting counts 1). */
#define DEBOUNCE_SCANS    2u

/* Centre-key interrupt pad GPIO2.2 (registers in gpio-yp3.h). */
#define GPIO2_IRQ_PAD     GPIO_IRQ_PAD(GPIO_CENTER_KEY_BANK)
#define GPIO2_IRQ_MASK    GPIO_IRQ_MASK(GPIO_CENTER_KEY_BANK)
#define GPIO2_IRQ_PENDING GPIO_IRQ_PENDING(GPIO_CENTER_KEY_BANK)
#define GPIO2_BIT         (1u << GPIO_CENTER_KEY_PIN)
/* GPIO2.2's field in GPIO2_IRQ_PAD: set to 3 before every rearm
 * (probably the trigger type; unconfirmed). */
#define IRQ_PAD_FIELD     (0xfu << (4u * GPIO_CENTER_KEY_PIN))
#define IRQ_PAD_CENTER    (0x3u << (4u * GPIO_CENTER_KEY_PIN))
#define MUX_FIELD         0xfu         /* one GPIO_MUX field */

/* Unpress if the IRQ path has not seen a release: poll every 100 ms. */
#define CENTER_POLL_TICKS (HZ / 10)

/* The scan runs every tick: from XIP it took ~190 us a tick (2% of the
 * CPU, always) */
#define KEYS_ICODE __attribute__((section(".icode")))

static bool buttons_ready;
static uint32_t stable_buttons, pending_buttons;
static uint8_t pending_count, scan_phase, adc_id, gpio_id;
static uint64_t settle_started;
static volatile uint32_t published_buttons;
static volatile uint32_t center_buttons;

static volatile bool center_ready;
static volatile bool center_pending;
#define CENTER_TAP_TICKS  2u
static uint8_t center_tap;        /* ticks left of a latched tap */


KEYS_ICODE static uint8_t adc_key_id(uint32_t raw)
{
    if (raw < LADDER_DOWN_BELOW) return KEY_ID_DOWN;
    if (raw < LADDER_UP_BELOW)   return KEY_ID_UP;
    if (raw < LADDER_NONE_BELOW) return KEY_ID_NONE;
    if (raw < LADDER_SD_BELOW)   return KEY_ID_SD_DETECT;
    return KEY_ID_NONE;
}

/* Centre key (GPIO2.2): pin mux, IRQ pad and mask, clearing a stale
 * pending. */
static bool center_rearm(void)
{
    int oldlevel = disable_irq_save();
    bool armed = false;
    const uint32_t shift = GPIO_MUX_SHIFT(GPIO_CENTER_KEY_PIN);
    uint32_t value = REG(GPIO_MUX(GPIO_CENTER_KEY_BANK, GPIO_CENTER_KEY_PIN));

    /* Each write is read back; a field that did not take fails the rearm. */
    REG(GPIO_MUX(GPIO_CENTER_KEY_BANK, GPIO_CENTER_KEY_PIN)) =
        (value & ~(MUX_FIELD << shift)) | MUX_CENTER_KEY_IRQ << shift;
    if ((REG(GPIO_MUX(GPIO_CENTER_KEY_BANK, GPIO_CENTER_KEY_PIN)) &
         (MUX_FIELD << shift)) == MUX_CENTER_KEY_IRQ << shift) {
        value = REG(GPIO2_IRQ_PAD);
        REG(GPIO2_IRQ_PAD) = (value & ~IRQ_PAD_FIELD) | IRQ_PAD_CENTER;
        if ((REG(GPIO2_IRQ_PAD) & IRQ_PAD_FIELD) == IRQ_PAD_CENTER) {
            REG(GPIO2_IRQ_PENDING) = GPIO2_BIT;
            value = REG(GPIO2_IRQ_MASK);
            REG(GPIO2_IRQ_MASK) = value | GPIO2_BIT;
            armed = (REG(GPIO2_IRQ_MASK) & GPIO2_BIT) != 0u;
        }
    }
    restore_irq(oldlevel);
    return armed;
}

/* PMU error: stop the centre key and release it. */
static void center_disable(void)
{
    int oldlevel = disable_irq_save();
    uint32_t value = REG(GPIO2_IRQ_MASK);
    REG(GPIO2_IRQ_MASK) = value & ~GPIO2_BIT;
    yp3_irq_disable(CENTER_IRQ);
    center_ready = false;
    center_pending = false;
    restore_irq(oldlevel);
    center_buttons = 0;
}

#ifdef YP3_DEV
/* Dead keys: the last raw PMU[0x31] reads, and whether the PMU's long-press
 * flag came since the last press (a real hold gets it after ~1 s, a key
 * left down by a lost release never does). TODO(cleanup): drop with the
 * other "keys:" logging once found. */
static uint8_t pmu31_raw[8];
static long pmu31_tick[8];
static unsigned pmu31_n;
static bool center_long_seen;
#endif

/* PMU[0x30] events (charger in/out, charge full) for power-yp3.c. */
volatile uint8_t yp3_pmu_events30;

/* PMU status bytes, the key events first, each read and then at once
 * acknowledged (written back), one bridge command per tick: waiting for
 * the bridge inside the tick held the audio DMA off for up to ~19 ms (an
 * audible gap on every Select press). The write-back clears the bits it
 * carries, also one that latched again after the read (write-1-to-clear).
 * With all reads before all
 * write-backs that window was three ticks: a release, then a quick
 * press+release inside it, lost the second release, the next pass read
 * the press alone and Select stayed down (every other key a chord that
 * maps to nothing: dead keys until Select is pressed again). */
static const uint8_t event_regs[3] = {
    PMU_REG_EVENT31, PMU_REG_EVENT30, PMU_REG_EVENT4A
};
#define EVENT_KEYS      0u      /* event_values[] of PMU_REG_EVENT31 */
#define EVENT_CHARGER   1u      /* event_values[] of PMU_REG_EVENT30 */
/* Then the power status, for the centre key's level (PMU2A_KEY_DOWN):
 * after its long-press flag (~1 s) the PMU latches no release, and a
 * held Select stayed down for good (dead keys after Select+Up). */
#define EVENT_STATUS    (2u * ARRAYLEN(event_regs))
#define EVENT_STEPS     (EVENT_STATUS + 1u)
#define EVENT_TICKS_MAX 4u      /* per command (the waiting version allowed
                                   32 ms) */
static uint8_t event_values[ARRAYLEN(event_regs)];
static uint8_t event_status;
static uint8_t event_step;      /* commands started; 0 = none in flight */
static uint8_t event_ticks;

static bool event_start(void)
{
    unsigned i = event_step / 2u;
    bool write = event_step & 1u;       /* even steps read, odd ones ack */
    event_ticks = 0;
    if (event_step == EVENT_STATUS) {
        if (!yp3_pmu_async_start(PMU_REG_POWER_STATUS, false, 0))
            return false;
    } else if (!yp3_pmu_async_start(event_regs[i], write, event_values[i])) {
        return false;
    }
    event_step++;
    return true;
}

/* Collect the command in flight and start the next: 1 = all done,
 * 0 = still running, -1 = failed. */
static int event_advance(void)
{
    uint8_t value;
    int done = yp3_pmu_async_done(&value);
    if (done == 0)
        return ++event_ticks > EVENT_TICKS_MAX ? -1 : 0;
    if (done < 0)
        return -1;
    if (event_step - 1u == EVENT_STATUS)
        event_status = value;
    else if (!((event_step - 1u) & 1u)) /* an event read finished */
        event_values[(event_step - 1u) / 2u] = value;
    if (event_step == EVENT_STEPS)
        return 1;
    return event_start() ? 0 : -1;
}

/* Decode the key events from 0x31 (all three bytes read and acknowledged)
 * and the key level from the power status. */
static void center_decode(void)
{
    uint8_t keys = event_values[EVENT_KEYS];
    yp3_pmu_events30 |= event_values[EVENT_CHARGER];
#ifdef YP3_DEV
    if (keys) {
        pmu31_raw[pmu31_n % ARRAYLEN(pmu31_raw)] = keys;
        pmu31_tick[pmu31_n % ARRAYLEN(pmu31_raw)] = current_tick;
        pmu31_n++;
    }
    if (keys & PMU31_KEY_PRESS)
        center_long_seen = false;
    if (keys & PMU31_KEY_LONG)
        center_long_seen = true;
#endif
    if ((keys & (PMU31_KEY_PRESS | PMU31_KEY_RELEASE))
        == (PMU31_KEY_PRESS | PMU31_KEY_RELEASE) && !center_buttons) {
        /* A tap shorter than one read: report it down for two ticks
         * (button_read() takes a state only when two ticks in a row return
         * it), or the button tick never sees it (a "dead" Select). */
        center_buttons = BUTTON_SELECT;
        center_tap = CENTER_TAP_TICKS;
    } else if (keys & PMU31_KEY_RELEASE) {
        center_buttons = 0;
    } else if (keys & PMU31_KEY_PRESS) {
        center_buttons = BUTTON_SELECT;
    } else if (center_buttons && !center_tap &&
               !(event_status & PMU2A_KEY_DOWN)) {
        center_buttons = 0;     /* up, with no release latched */
    }
    /* The PMU's own long-press flag (PMU31_KEY_LONG) came after about a
     * second, so games that hold Select powered off: center_tick() times
     * the hold itself instead. */
}

/* Centre (power) key held this long: clean Rockbox shutdown, handled by
 * default_event_handler() in the UI thread. Off while a plugin runs. */
#define POWER_HOLD_TICKS  (3 * HZ)
static bool power_hold_enabled = true;

void button_set_power_hold(bool enable)
{
    power_hold_enabled = enable;
}

/* Called from button_read_device() on the tick: service a PMU key event
 * flagged by the IRQ over the next ticks. A PMU lock held elsewhere
 * retries on the next tick. */
static void center_tick(void)
{
    static unsigned held_ticks, power_ticks;
    if (!center_ready)
        return;
    if (center_tap && !--center_tap)
        center_buttons = 0;
    if (center_buttons && power_hold_enabled) {
        if (++power_ticks == POWER_HOLD_TICKS)
            button_queue_try_post(SYS_POWEROFF, 0);
    } else {
        power_ticks = 0;
    }
    /* A lost key interrupt would leave a release unread and Select stuck
     * down, which blocks every other key. While Select is down, poll the
     * PMU every 100 ms as well. */
    if (center_buttons && !center_pending &&
        ++held_ticks >= CENTER_POLL_TICKS) {
        held_ticks = 0;
        center_pending = true;
    }
    if (!center_buttons)
        held_ticks = 0;
    if (event_step) {
        int done = event_advance();
        if (done == 0)
            return;
        event_step = 0;
        yp3_pmu_unlock();
        if (done > 0)
            center_decode();
        else
            center_disable();
        return;
    }
    if (!center_pending)
        return;
    center_pending = false;
    if (!yp3_pmu_try_lock()) {
        center_pending = true;
        return;
    }
    /* Rearm before the reads, not after the write-backs: an event during
     * the ~60 ms of commands then raises the IRQ again (another pass)
     * instead of being cleared with the pending flag. */
    if (!center_rearm() || !event_start()) {
        yp3_pmu_unlock();
        center_disable();
    }
}

static void center_irq(void)
{
    uint32_t value;
    if (!center_ready)
        return;
    if (!(REG(GPIO2_IRQ_PENDING) & GPIO2_BIT))
        return;
    /* Mask and acknowledge; the tick reads the PMU and rearms. The pending
     * bit is written twice (reason not known). */
    value = REG(GPIO2_IRQ_MASK);
    REG(GPIO2_IRQ_MASK) = value & ~GPIO2_BIT;
    REG(GPIO2_IRQ_PENDING) = GPIO2_BIT;
    REG(GPIO2_IRQ_PENDING) = GPIO2_BIT;
    center_pending = true;
}

static void center_init(void)
{
    int oldlevel = disable_irq_save();
    bool armed = center_rearm();
    if (armed) {
        yp3_irq_install(CENTER_IRQ, center_irq);
        yp3_irq_priority(CENTER_IRQ, YP3_PRIO_LOWEST);
        center_ready = true;
        center_pending = false;
    }
    restore_irq(oldlevel);
    if (armed)
        yp3_irq_enable(CENTER_IRQ);
}

KEYS_ICODE static uint32_t map_scan(void)
{
    uint32_t v = 0;
    if (adc_id == KEY_ID_DOWN) v |= BUTTON_DOWN;
    if (adc_id == KEY_ID_UP) v |= BUTTON_UP;
    if (gpio_id == KEY_ID_LEFT) v |= BUTTON_LEFT;
    if (gpio_id == KEY_ID_RIGHT) v |= BUTTON_RIGHT;
    return v;
}

#ifdef YP3_DEV
static uint32_t scans_finished;
#endif

/* A new state is accepted once seen on DEBOUNCE_SCANS consecutive scans. */
KEYS_ICODE static void finish_scan(void)
{
    uint32_t candidate = map_scan();
    if (candidate == stable_buttons) {
        pending_buttons = candidate;
        pending_count = 0;
    } else if (candidate == pending_buttons) {
        if (pending_count < DEBOUNCE_SCANS)
            ++pending_count;
        if (pending_count >= DEBOUNCE_SCANS) {
            stable_buttons = candidate;
            pending_count = 0;
        }
    } else {
        pending_buttons = candidate;
        pending_count = 1u;
    }
    published_buttons = stable_buttons;
    scan_phase = 0;
#ifdef YP3_DEV
    scans_finished++;
#endif
}

/* ADC failure: release any directional key (the centre key is independent). */
KEYS_ICODE static void discard_scan(void)
{
    stable_buttons = 0;
    pending_buttons = 0;
    pending_count = 0;
    adc_id = gpio_id = 0;
    scan_phase = 0;
    published_buttons = 0;
}

KEYS_ICODE static void scan_tick(void)
{
    uint64_t now;
    if (!buttons_ready)
        return;
    if (!yp3_adc_ready()) {
        discard_scan();
        return;
    }
    switch (scan_phase) {
    case 0:
        adc_id = adc_key_id(REG(ADC_KEY_SAMPLE));
        yp3_pin_config(PIN_OWNER_KEYS, PIN_KEY_LR_PULLUP);
        if (!sl6801_counter64(&settle_started))
            return;
        scan_phase = 1;
        break;
    case 1:
        if (!sl6801_counter64(&now) || now - settle_started < SETTLE_TICKS)
            return;
        if (!(REG(PIN_KEY_LR_INPUT) & PIN_KEY_LR_BIT)) {
            gpio_id = KEY_ID_RIGHT;     /* low with the pull-up */
            finish_scan();
            break;
        }
        yp3_pin_config(PIN_OWNER_KEYS, PIN_KEY_LR_PULLDOWN);
        if (!sl6801_counter64(&settle_started))
            return;
        scan_phase = 2;
        break;
    case 2:
        if (!sl6801_counter64(&now) || now - settle_started < SETTLE_TICKS)
            return;
        /* high with the pull-down */
        gpio_id = (REG(PIN_KEY_LR_INPUT) & PIN_KEY_LR_BIT) ? KEY_ID_LEFT
                                                         : KEY_ID_NONE;
        finish_scan();
        break;
    }
}

void button_init_device(void)
{
    adc_init();
    if (!yp3_adc_ready())
        return;
    buttons_ready = true;
    center_init();
}

KEYS_ICODE int button_read_device(void)
{
    /* Called from the tick: advance one bounded scan phase; settle time
     * elapses between ticks. */
    scan_tick();
    center_tick();
#ifdef YP3_DEV
    /* "Dead keys" hunt (user, after plugins and after USB storage): log
     * a key state held 5 s and a scan that stopped finishing.
     * TODO(cleanup): drop once found. */
    {
        static uint32_t last_state, same_ticks, last_scans, idle_ticks;
        uint32_t state = published_buttons | center_buttons;
        same_ticks = (state && state == last_state) ? same_ticks + 1 : 0;
        last_state = state;
        if (same_ticks == 5 * HZ)
            dev_log("keys: 0x%lx held 5 s (adc %02x gpio %02x raw %03lx "
                        "centre rdy %d pend %d)", (unsigned long)state,
                        adc_id, gpio_id,
                        (unsigned long)(REG(ADC_KEY_SAMPLE) & 0xfff),
                        center_ready, center_pending);
        /* Select down 1.5 s without the PMU's long-press flag (a real
         * hold has it after ~1 s): its release was lost. Logged before the
         * 3 s power hold, and while other keys are pressed. */
        static unsigned centre_ticks;
        centre_ticks = center_buttons ? centre_ticks + 1 : 0;
        if (centre_ticks == 3 * HZ / 2 && !center_long_seen) {
            unsigned n = pmu31_n;
#define P31(k) pmu31_raw[(n - (k)) % ARRAYLEN(pmu31_raw)], \
               pmu31_tick[(n - (k)) % ARRAYLEN(pmu31_raw)]
            dev_log("keys: centre down 1.5 s, no PMU long flag; pmu31 "
                        "%02x@%ld %02x@%ld %02x@%ld %02x@%ld now %ld",
                        P31(4), P31(3), P31(2), P31(1), current_tick);
#undef P31
        }
        idle_ticks = (scans_finished == last_scans) ? idle_ticks + 1 : 0;
        last_scans = scans_finished;
        if (idle_ticks == 2 * HZ)
            dev_log("keys: no scan for 2 s (ready %d adc %d phase %d)",
                        buttons_ready, yp3_adc_ready(), scan_phase);
    }
    return (int)(published_buttons | center_buttons) | dev_held_buttons();
#else
    return (int)(published_buttons | center_buttons);
#endif
}

