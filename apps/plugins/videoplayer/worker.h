/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Video player: one job, run on the second core where there is one (the
 * SL6801's CPU1), else where it is started. On CPU1 the job
 * runs from the plugin's memory with interrupts off: it may not call
 * Rockbox or other code in flash, and shares memory with this core only
 * through volatile data ordered by worker_barrier().
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef WORKER_H
#define WORKER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if CONFIG_CPU == SL6801 && !defined(SIMULATOR)
#define WORKER_CPU1
#endif

/* Between writing shared data and publishing it, and between seeing it
 * published and reading it */
static inline void worker_barrier(void)
{
#ifdef WORKER_CPU1
    __asm__ volatile ("dmb" ::: "memory");
#else
    __asm__ volatile ("" ::: "memory");
#endif
}

/* Start the second core for `job`, on a stack of `size` bytes; false if
 * there is none or no stack (worker_run() then runs the job itself) */
bool worker_start(void (*job)(void), void *stack, size_t size);
void worker_stop(void);
/* Start the job, which must not be running */
void worker_run(void);
/* The job runs (false also once the second core has stopped answering) */
bool worker_busy(void);
/* Microseconds the second core has spent in the job */
uint32_t worker_busy_us(void);

#endif /* WORKER_H */
