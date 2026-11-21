/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * FPU for one plugin thread on YP3 (as rockgpt does). The core is
 * soft-float and runs with FP access off and no FP state stacking, so
 * nothing else ever touches the FP registers. A plugin may compile its game
 * code with -mfloat-abi=softfp -mfpu=fpv4-sp-d16 (PLUGIN_FPU set; ABI
 * compatible with the soft-float core) and enable access while it runs,
 * provided only its main thread runs that code: interrupt-context
 * callbacks (mixer get_more, timers) and other threads must stay soft-float
 * (yp3dev/fpucheck.py checks a built plugin).
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
#ifndef PLUGIN_FPU_H
#define PLUGIN_FPU_H

#if defined(PLUGIN_FPU) && defined(YP3_SL6801) && \
    (CONFIG_PLATFORM & PLATFORM_NATIVE)
#define PLUGIN_CPACR       (*(volatile unsigned long *)0xe000ed88)
#define PLUGIN_CPACR_FPU   0x00f00000ul  /* CP10/CP11 full access */

/* Call with true before any FPU code runs and with false on every exit
 * path (an exit handler covers exit() and USB). The calling function must
 * contain no FP code itself (call the game through a noinline function):
 * its prologue would save FP registers before access is on. */
static inline void plugin_fpu_enable(bool on)
{
    if (on)
        PLUGIN_CPACR |= PLUGIN_CPACR_FPU;
    else
        PLUGIN_CPACR &= ~PLUGIN_CPACR_FPU;
    __asm__ volatile ("dsb\n\tisb" ::: "memory");
}
#else
static inline void plugin_fpu_enable(bool on) { (void)on; }
#endif

#endif /* PLUGIN_FPU_H */
