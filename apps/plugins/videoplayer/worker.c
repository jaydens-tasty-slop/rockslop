/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Video player: the second core's job (the SL6801's
 * CPU1). The ROM sends CPU1 to the launch words when its clock gate is
 * pulsed; the trampoline sets its stack, the entry its vectors, and it
 * waits in WFI with interrupts off for a run request: a wake message on
 * the inter-core FIFO ends the WFI. Stopping parks it and turns its clock
 * gate off, so it never runs the plugin's memory once that is freed.
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
#include "plugin.h"
#include "worker.h"

static void (*job)(void);

#ifndef WORKER_CPU1

bool worker_start(void (*fn)(void), void *stack, size_t size)
{
    (void)stack;
    (void)size;
    job = fn;
    return false;
}

void worker_stop(void)
{
}

void worker_run(void)
{
    job();
}

bool worker_busy(void)
{
    return false;
}

uint32_t worker_busy_us(void)
{
    return 0;
}

#else /* WORKER_CPU1 */

#include "mmio-sl6801.h"

#define BOOT_TRAMPOLINE 0x30000600u     /* in the SPI controller's block */
#define BOOT_SP         0x30000604u
#define BOOT_ENTRY      0x30000608u
#define CPU1_GATE       0
#define SPI_GATE        1               /* the launch words' block */
#define IPC_GATE        0x28            /* the inter-core FIFO */

/* The FIFO's channel 1, which CPU1 receives: 2-word messages raise its
 * IRQ 55 once CPU1 has enabled that (a write from CPU0 does not take) */
#define IPC_CTRL        0x40070000u
#define IPC_CTRL_RX1    (0x11u << 8)    /* channel 1's receiver: */
#define IPC_CTRL_CPU1   (0x01u << 8)    /* CPU1 (0x10 << 8: CPU0) */
#define IPC_IRQ_EN1     0x40070028u
#define IPC_STATUS1     0x4007002cu
#define IPC_COUNT1      0x40070064u     /* words queued [2:0] */
#define IPC_DATA1       0x40070068u
#define IPC_BIT1        (1u << 2)
#define IPC_IRQ         55

#define STOP_WAIT       (HZ / 10)
/* A run is a few parts' decoding; one that has not ended after this means
 * CPU1 is not coming back */
#define RUN_TIMEOUT     HZ

static struct {
    volatile uint32_t seq, done;        /* runs asked for and finished */
    volatile bool stop, parked;
    volatile uint32_t fault_pc;         /* where CPU1 faulted, if it did */
    volatile uint32_t busy_us;
    bool on;                            /* this core's view */
    long started;                       /* the latest run's tick */
    bool ipc_set;                       /* the FIFO is set up for CPU1 */
    uint32_t ctrl;                      /* its setting before */
    bool ipc_was_on;
} w;

static bool gate_on(unsigned id)
{
    uint32_t bit = CLK_GATE_BIT(id);

    if ((REG(CLK_GATE_REQ(id)) & bit) && (REG(CLK_GATE_ACK(id)) & bit))
        return true;
    int old = disable_irq_save();
    REG(CLK_GATE_REQ(id)) |= bit;
    REG(CLK_GATE_ACK(id)) |= bit;
    __asm__ volatile ("dsb" ::: "memory");
    restore_irq(old);
    for (int i = 0; i < 10000; i++)
        if (REG(CLK_GATE_ACK(id)) & bit)
            return true;
    return false;
}

static void gate_off(unsigned id)
{
    uint32_t bit = CLK_GATE_BIT(id);
    int old = disable_irq_save();
    REG(CLK_GATE_ACK(id)) &= ~bit;
    REG(CLK_GATE_REQ(id)) &= ~bit;
    restore_irq(old);
}

static bool gate_is_on(unsigned id)
{
    return REG(CLK_GATE_ACK(id)) & CLK_GATE_BIT(id);
}

/* ---- CPU1 ---- */

static void __attribute__((used, noinline)) fault_body(const uint32_t *frame)
{
    w.fault_pc = frame[EXC_FRAME_PC] | 1;
    for (;;)
        __asm__ volatile ("wfi");
}

static void __attribute__((naked)) fault(void)
{
    __asm__ volatile ("mov r0, sp\n\tb fault_body");
}

/* The exceptions only: the FIFO's IRQ stays masked (it ends WFI anyway) */
static void (*const vectors[16])(void) __attribute__((aligned(128))) = {
    [0 ... 15] = fault,
};

/* Drop the wake messages; one sent after this ends the next WFI */
static void wake_ack(void)
{
    REG(IPC_STATUS1) = REG(IPC_STATUS1);
    while (REG(IPC_COUNT1) & 7)
        (void)REG(IPC_DATA1);
    REG(NVIC_ICPR(IPC_IRQ)) = NVIC_BIT(IPC_IRQ);
    __asm__ volatile ("dsb" ::: "memory");
}

static void __attribute__((noreturn)) serve(void)
{
    uint32_t seen = 0;                  /* (a run may be asked for already) */

    for (;;)
    {
        wake_ack();
        if (w.stop)
        {
            REG(IPC_IRQ_EN1) &= ~IPC_BIT1;
            w.parked = true;
            for (;;)
                __asm__ volatile ("wfi");
        }
        if (w.seq != seen)
        {
            seen = w.seq;
            worker_barrier();
            uint32_t t0 = sl6801_us();
            job();
            w.busy_us += sl6801_us() - t0;
            worker_barrier();
            w.done = seen;
            continue;
        }
        __asm__ volatile ("wfi");
    }
}

static void entry(void)
{
    REG(SCB_VTOR) = (uint32_t)vectors;
    REG(SCB_CPACR) |= 0x00f00000;       /* FPU access, as stock grants it */
    REG(0x40000060) = 8;                /* as stock does (meaning unknown) */
    REG(IPC_IRQ_EN1) |= IPC_BIT1;
    REG(NVIC_ISER(IPC_IRQ)) = NVIC_BIT(IPC_IRQ);
    __asm__ volatile ("dsb\n\tisb" ::: "memory");
    serve();
}

/* From the ROM: interrupts off, then the stack and entry from the launch
 * words */
static void __attribute__((naked, used)) trampoline(void)
{
    __asm__ volatile (
        "cpsid  i\n\t"
        "ldr    r0, =0x30000604\n\t"    /* BOOT_SP */
        "ldr    sp, [r0]\n\t"
        "ldr    r0, =0x30000608\n\t"    /* BOOT_ENTRY */
        "ldr    r0, [r0]\n\t"
        "bx     r0\n\t"
        ".ltorg");
}

/* ---- CPU0 ---- */

static void wake(void)
{
    __asm__ volatile ("dsb" ::: "memory");
    if ((REG(IPC_COUNT1) & 7) <= 2)     /* else a wake is on its way */
    {
        REG(IPC_DATA1) = 0;
        REG(IPC_DATA1) = 0;
    }
}

bool worker_start(void (*fn)(void), void *stack, size_t size)
{
    job = fn;
    rb->memset((void *)&w, 0, sizeof(w));
    if (!stack || !gate_on(SPI_GATE))
        return false;
    w.ipc_was_on = gate_is_on(IPC_GATE);
    if (!gate_on(IPC_GATE))
        return false;
    w.ctrl = REG(IPC_CTRL);
    w.ipc_set = true;
    REG(IPC_CTRL) = (w.ctrl & ~IPC_CTRL_RX1) | IPC_CTRL_CPU1;
    while (REG(IPC_COUNT1) & 7)
        (void)REG(IPC_DATA1);

    uint32_t t = (uint32_t)trampoline | 1, e = (uint32_t)entry | 1;
    uint32_t sp = ((uintptr_t)stack + size) & ~7u;
    REG(BOOT_TRAMPOLINE) = t;
    REG(BOOT_SP) = sp;
    REG(BOOT_ENTRY) = e;
    __asm__ volatile ("dsb" ::: "memory");
    if (REG(BOOT_TRAMPOLINE) != t || REG(BOOT_SP) != sp ||
        REG(BOOT_ENTRY) != e)
    {
        worker_stop();
        return false;
    }
    gate_off(CPU1_GATE);
    __asm__ volatile ("dsb" ::: "memory");
    w.on = gate_on(CPU1_GATE);
    if (!w.on)
        worker_stop();
    return w.on;
}

void worker_stop(void)
{
    if (w.on)
    {
        w.stop = true;
        wake();
        long until = *rb->current_tick + STOP_WAIT;
        while (!w.parked && TIME_BEFORE(*rb->current_tick, until))
            ;
        w.on = false;
    }
    if (!w.ipc_set)
        return;
    gate_off(CPU1_GATE);
    REG(IPC_CTRL) = w.ctrl;
    if (!w.ipc_was_on)
        gate_off(IPC_GATE);
    w.ipc_set = false;
}

void worker_run(void)
{
    if (!w.on)
    {
        job();
        return;
    }
    w.started = *rb->current_tick;
    worker_barrier();
    w.seq++;
    wake();
}

bool worker_busy(void)
{
    if (!w.on)
        return false;
    if (w.done != w.seq)
    {
        if (!w.fault_pc && TIME_BEFORE(*rb->current_tick, w.started + RUN_TIMEOUT))
            return true;
#ifdef HAVE_DEVLOG
        rb->dev_msg("video", "cpu1 stopped (fault pc %08lx)",
                        (unsigned long)w.fault_pc);
#endif
        /* this core does the work from now on */
        gate_off(CPU1_GATE);
        w.on = false;
        return false;
    }
    worker_barrier();
    return false;
}

uint32_t worker_busy_us(void)
{
    return w.busy_us;
}

#endif /* WORKER_CPU1 */
