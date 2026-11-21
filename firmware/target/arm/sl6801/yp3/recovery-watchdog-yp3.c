/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "config.h"
#include "system.h"
#include "kernel.h"
#include "recovery-yp3.h"
#include "dev-yp3.h"

/* The copied early entry armed the ~16 s hardware watchdog (and in dev
 * builds PMU reason 7) before anything else ran. Feed it once a second from
 * the kernel tick, but only while the heartbeat thread keeps running: a
 * fault/panic masks IRQs (the tick stops), and a thread that never yields
 * starves the heartbeat (it runs at the UI/playback priority), so either
 * resets the player: into ROM USB, or with YP3_WATCHDOG_POWEROFF into a
 * power-off. A deadlock where everything sleeps is not caught here. */
/* slow but progressing work (FS, metadata scans) can take >8 s */
#define HEARTBEAT_LIMIT (20 * HZ)
static volatile long heartbeat_tick;
static long heartbeat_stack[DEFAULT_STACK_SIZE / sizeof(long)];

static void heartbeat_thread(void)
{
    for (;;) {
        heartbeat_tick = current_tick;
        sleep(HZ);
    }
}

static void watchdog_tick(void)
{
    static unsigned ticks;
    if (++ticks < HZ)
        return;
    ticks = 0;
#ifdef YP3_DEV
    /* Development loop: stop feeding after the configured time. */
    if (yp3_dev_return_seconds &&
        current_tick > (long)yp3_dev_return_seconds * HZ) {
        static bool logged;
        if (!logged) {
            logged = true;
            dev_log("dev return: watchdog feeds stopped at tick %ld",
                    current_tick);
            yp3_usb_drop_softconn();
        }
        return;
    }
#endif
    if (heartbeat_tick && current_tick - heartbeat_tick > HEARTBEAT_LIMIT) {
#ifdef YP3_DEV
        static bool logged;
        if (!logged) {
            logged = true;
            dev_log("HANG: heartbeat %ld s old, watchdog feeds stopped",
                        (current_tick - heartbeat_tick) / HZ);
        }
#endif
        return;
    }
    yp3_recovery_watchdog_refresh();
}

void yp3_recovery_watchdog_start(void)
{
    yp3_recovery_watchdog_refresh();
    tick_add_task(watchdog_tick);
}

/* Once threads can be created (kernel up). */
void yp3_recovery_heartbeat_start(void)
{
    create_thread(heartbeat_thread, heartbeat_stack, sizeof heartbeat_stack,
                  0, "heartbeat" IF_PRIO(, PRIORITY_USER_INTERFACE)
                  IF_COP(, CPU));
}

#ifdef YP3_WATCHDOG_POWEROFF
/* Run marker in the watchdog-retained SRAM (app.lds .yp3_retained): set
 * while the firmware runs, cleared by system_reboot() and power_off().
 * SRAM survives a watchdog reset but not a power-on or a PMU software
 * reset; PMU[0x27] reads 0x41 after either reset, so
 * it does not tell them apart. */
#define RUN_MAGIC 0x4e555259u   /* bytes "YRUN" */
static volatile uint32_t run_marker[2]
    __attribute__((section(".noinit.yp3_run_marker"), used, aligned(8)));

void yp3_recovery_clear_run_marker(void)
{
    run_marker[0] = run_marker[1] = 0;
}

void yp3_recovery_check_watchdog_reset(void)
{
    if (run_marker[0] == RUN_MAGIC && run_marker[1] == ~RUN_MAGIC)
        yp3_power_off_after_hang();
    run_marker[0] = RUN_MAGIC;
    run_marker[1] = ~RUN_MAGIC;
}
#endif
