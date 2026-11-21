/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Battery benchmark log, HAVE_BATTLOG builds only:
 * an existing /battlog.csv (empty = armed) gets a header line per boot and
 * then "uptime_s,mv,level,playing,charging,cpu_mhz" every minute, each line
 * written with open/append/close so a sudden power cut loses at most one;
 * plus a line when the system powers off or reboots. */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "battlog.h"
#include "system.h"
#include "kernel.h"
#include "thread.h"
#include "file.h"
#include "power.h"
#include "powermgmt.h"
#include "audio.h"
#include "usb.h"
#include "settings.h"
#include "version.h"

#define BATTLOG_FILE   "/battlog.csv"
#define BATTLOG_PERIOD (60 * HZ)

static struct event_queue battlog_queue SHAREDBSS_ATTR;
static long battlog_stack[2048 / sizeof(long)];

static void battlog_line(const char *fmt, ...)
{
    char buf[160];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    int fd = open(BATTLOG_FILE, O_WRONLY | O_APPEND);
    if (fd < 0)
        return;
    write(fd, buf, strlen(buf));
    close(fd);
}

static void battlog_sample(const char *what)
{
    int status = audio_status();
    battlog_line("%s%ld,%d,%d,%d,%d,%ld\n", what, current_tick / HZ,
                 battery_voltage(), battery_level(),
                 (status & AUDIO_STATUS_PLAY) && !(status & AUDIO_STATUS_PAUSE),
                 charging_state(), cpu_frequency / 1000000);
}

static void battlog_thread(void)
{
    struct queue_event ev;

    battlog_line("# boot %s volume %d backlight %d bt %d\n"
                 "uptime_s,mv,level,playing,charging,cpu_mhz\n", rbversion,
                 global_status.volume, global_settings.backlight_timeout,
#ifdef HAVE_BLUETOOTH
                 global_settings.bt_enabled
#else
                 0
#endif
                 );
    for (;;) {
        queue_wait_w_tmo(&battlog_queue, &ev, BATTLOG_PERIOD);
        switch (ev.id) {
        case SYS_TIMEOUT:
            battlog_sample("");
            break;
        case SYS_POWEROFF:
            battlog_sample("# poweroff ");
            break;
        case SYS_REBOOT:
            battlog_sample("# reboot ");
            break;
        case SYS_USB_CONNECTED:
            usb_acknowledge(SYS_USB_CONNECTED_ACK, ev.data);
            usb_wait_for_disconnect(&battlog_queue);
            break;
        }
    }
}

void battlog_start(void)
{
    if (!file_exists(BATTLOG_FILE))
        return;
    queue_init(&battlog_queue, true);
    create_thread(battlog_thread, battlog_stack, sizeof battlog_stack, 0,
                  "battlog" IF_PRIO(, PRIORITY_BACKGROUND) IF_COP(, CPU));
}
