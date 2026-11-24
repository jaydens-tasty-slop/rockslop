/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SL6801 calendar RTC in the PMU block (aligned 32-bit loads/stores). It
 * survives watchdog resets and a power-off followed by a key power-on. It
 * counts an internal ~53.7 kHz oscillator, which the PMU
 * measures against the crystal (P+0x84, Hz, read-only); once the board
 * set-up has run that reads ~0xd1dd and the calendar counts real seconds
 * (2026-11-01). Before it, it read 0x8000 and the calendar ran ~1.63x fast.
 *   0x40085004: day[4:0] month[11:8] year-2000[22:16] leap year[24]
 *   0x40085008: sec[5:0] min[13:8] hour[20:16] weekday[26:24] (Sunday = 0)
 */
#include "config.h"
#include "system.h"
#include "rtc.h"
#include "timefuncs.h"
#include <time.h>
#include "mmio-sl6801.h"

#define RTC_DATE    REG(PMU_RTC_DATE)   /* P+0x04 */
#define RTC_TIME    REG(PMU_RTC_TIME)   /* P+0x08 */

/* Calendar fields (layout above). The year field holds year - 2000;
 * struct tm years count from 1900 and months from 0. */
#define TIME_SEC_MASK    0x3f
#define TIME_MIN_SHIFT   8
#define TIME_MIN_MASK    0x3f
#define TIME_HOUR_SHIFT  16
#define TIME_HOUR_MASK   0x1f
#define TIME_WDAY_SHIFT  24
#define DATE_DAY_MASK    0x1f
#define DATE_MONTH_SHIFT 8
#define DATE_MONTH_MASK  0x0f
#define DATE_YEAR_SHIFT  16
#define DATE_YEAR_MASK   0x7f
#define DATE_LEAP        (1u << 24)
#define RTC_YEAR_BASE    2000
#define TM_YEAR_RTC_BASE 100            /* 2000 as a tm_year */

static bool leap_year(int year)
{
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

/* Calendar as seconds (time_t). */
static time_t calendar_read(void)
{
    uint32_t date, time;
    struct tm tm;
    /* Re-read if the date rolled over between the two words. */
    do {
        date = RTC_DATE;
        time = RTC_TIME;
    } while (date != RTC_DATE);
    tm.tm_sec = time & TIME_SEC_MASK;
    tm.tm_min = (time >> TIME_MIN_SHIFT) & TIME_MIN_MASK;
    tm.tm_hour = (time >> TIME_HOUR_SHIFT) & TIME_HOUR_MASK;
    tm.tm_mday = date & DATE_DAY_MASK;
    tm.tm_mon = ((date >> DATE_MONTH_SHIFT) & DATE_MONTH_MASK) - 1;
    tm.tm_year = ((date >> DATE_YEAR_SHIFT) & DATE_YEAR_MASK) +
                 TM_YEAR_RTC_BASE;
    return mktime(&tm);
}

static void calendar_write(const struct tm *tm)
{
    int year = tm->tm_year + 1900;
    /* Time word first, then the date (hardware need not established). */
    RTC_TIME = (uint32_t)tm->tm_wday << TIME_WDAY_SHIFT |
               (uint32_t)tm->tm_hour << TIME_HOUR_SHIFT |
               (uint32_t)tm->tm_min << TIME_MIN_SHIFT | tm->tm_sec;
    RTC_DATE = (leap_year(year) ? DATE_LEAP : 0) |
               (uint32_t)(year - RTC_YEAR_BASE) << DATE_YEAR_SHIFT |
               (uint32_t)(tm->tm_mon + 1) << DATE_MONTH_SHIFT | tm->tm_mday;
}

void rtc_init(void)
{
}

int rtc_read_datetime(struct tm *tm)
{
    time_t t = calendar_read();
    gmtime_r(&t, tm);
    return 1;
}

int rtc_write_datetime(const struct tm *tm)
{
    calendar_write(tm);
    return 1;
}
