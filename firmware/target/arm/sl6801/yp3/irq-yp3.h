/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef IRQ_YP3_H
#define IRQ_YP3_H

#include <stdint.h>
#include "mmio-sl6801.h"

/* Platform interrupt n is vector 16 + n in the RAM vector table. */
extern uint32_t __vectors_arm[];

static inline void yp3_irq_install(unsigned irq, void (*handler)(void))
{
    __vectors_arm[16u + irq] = (uint32_t)(uintptr_t)handler | 1u;
    __asm__ volatile("dsb\n\tisb" ::: "memory");
}

/* ---- Interrupt lines (platform interrupt numbers) ---------------------- */
#define CENTER_IRQ         3u       /* GPIO2 pad interrupt: the centre key */
#define ADC_IRQ            20u      /* general-purpose ADC */
#define BT_IRQ             26u      /* Bluetooth baseband */
#define USB_IRQ            41       /* USB controller */
#define USB_DMA_IRQ        42       /* USB controller's DMA engine */
/* Spare software-pended line for the Bluetooth scheduler (the SoC routes
 * no source to line 56). */
#define BT_SCHED_IRQ       56u
#define DMA_CH_IRQ(ch)     (60 + (ch))   /* DMA channel ch */

/* ---- Priorities ---------------------------------------------------------
 * One byte per line, upper bits significant, lower value = more urgent.
 * Every NVIC line (and SysTick) reads priority 0xf0 at run time, 4 bits
 * implemented: a line preempts nothing unless its driver raises it. From
 * most to least urgent: */
/* Bluetooth baseband (IRQ 26): preempts the scheduler below. */
#define BT_IRQ_PRIO        0x70u
/* The audio DMA channels restart one period per interrupt and the DAC/ADC
 * FIFOs last only ~145 us after it: above the tick (its tasks ran ~150 us
 * from XIP at 64 MHz: one-sample slips), USB and the Bluetooth scheduler;
 * below the Bluetooth baseband. */
#define YP3_AUDIO_DMA_PRIO 0x80u
/* Bluetooth scheduler line: below the baseband, but above USB and SysTick
 * (which the stack's media path runs from), so neither preempts it. */
#define BT_SCHED_PRIO      0xa0u
/* Both USB lines (the DMA and controller handlers never preempt each
 * other): above the lines left at the default, SysTick and the centre
 * key. */
#define USB_IRQ_PRIO       0xc0u
/* SysTick and the centre key (reads back as 0xf0, the default). */
#define YP3_PRIO_LOWEST    0xffu

/* NVIC enable, disable, clear pending, priority (0 most urgent) of one
 * interrupt; inlined for callers in SRAM */
#define IRQ_INLINE static inline __attribute__((always_inline))
IRQ_INLINE void yp3_irq_enable(unsigned irq)
{
    REG(NVIC_ISER(irq)) = NVIC_BIT(irq);
}
IRQ_INLINE void yp3_irq_disable(unsigned irq)
{
    REG(NVIC_ICER(irq)) = NVIC_BIT(irq);
}
IRQ_INLINE void yp3_irq_clear(unsigned irq)
{
    REG(NVIC_ICPR(irq)) = NVIC_BIT(irq);
}
IRQ_INLINE void yp3_irq_pend(unsigned irq)
{
    REG(NVIC_ISPR(irq)) = NVIC_BIT(irq);
}
IRQ_INLINE void yp3_irq_priority(unsigned irq, uint8_t prio)
{
    REG8(NVIC_IPR(irq)) = prio;
}

#endif
