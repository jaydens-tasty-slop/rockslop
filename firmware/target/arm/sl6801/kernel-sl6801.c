/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "config.h"
#include "system.h"
#include "mmio-sl6801.h"
#include "irq-yp3.h"
#include "pcm-yp3.h"
#include "kernel.h"
#include <string.h>
#include "regs/cortex-m/cm_scb.h"
#include "regs/cortex-m/cm_systick.h"
#ifdef YP3_DEV
#include "dev-yp3.h"
#include "../kernel/thread-internal.h"
#endif

/* SysTick counts the 1 MHz external reference (CSR.CLKSOURCE = 0). */
#define SYSTICK_REF_HZ 1000000

void tick_start(unsigned int interval_ms)
{
    reg_var(CM_SYSTICK_CSR) = 0;
    reg_assignf(CM_SCB_ICSR, PENDSTCLR(1));
    reg_var(CM_SYSTICK_RVR) = SYSTICK_REF_HZ / 1000 * interval_ms - 1;
    reg_var(CM_SYSTICK_CVR) = 0;
    reg_writef(CM_SCB_SHPR3, PRI_SYSTICK(YP3_PRIO_LOWEST));
    reg_assignf(CM_SYSTICK_CSR, ENABLE(1), TICKINT(1));
}

#ifdef YP3_DEV
/* Words the stack scans below take as return addresses (dev-yp3.h) */
static inline bool ICODE_ATTR is_xip_return(uint32_t v)
{
    return (v & 1) && v >= YP3_DEV_CODE_BEGIN && v < YP3_DEV_CODE_END;
}

#define PROF_CALLERS     4      /* return addresses recorded per sample */
#define PROF_SCAN_WORDS  96     /* stack words searched above the frame */

/* Dev sampling profiler: the interrupted PC of every tick (apps/dev_shell.c
 * "prof", yp3dev/profile.py). With yp3_prof_callers, each sample also adds
 * up to 4 Thumb return addresses into XIP found on the interrupted stack, so
 * the histogram shows callers too (approximate: stale words also match). */
uint32_t *yp3_prof_pc;          /* heap buffer while profiling */
unsigned yp3_prof_size, yp3_prof_count;
bool yp3_prof_on, yp3_prof_callers;
unsigned yp3_prof_late;         /* irq samples only when the audio period is
                                   this late (us; 0: all) */

static void prof_sample(const uint32_t *frame)
{
    if (yp3_prof_on && yp3_prof_count + 1 + PROF_CALLERS <= yp3_prof_size) {
        yp3_prof_pc[yp3_prof_count++] = frame[EXC_FRAME_PC];
        for (int i = EXC_FRAME_WORDS, n = 0; yp3_prof_callers &&
             i < EXC_FRAME_WORDS + PROF_SCAN_WORDS && n < PROF_CALLERS; i++)
            if (is_xip_return(frame[i])) {
                yp3_prof_pc[yp3_prof_count++] = frame[i] & ~1u;
                n++;
            }
    }
}

/* Threads that wake on the tick do their work and wait again before the
 * next one, so tick samples see the idle loop only. Sampling another
 * interrupt instead (the audio DMA's, paced by the audio clock) sees them:
 * yp3_prof_irq(IRQ) puts a shim in front of that IRQ's handler, -1 takes
 * it out again. */
static unsigned prof_irq = ~0u;
uint32_t prof_irq_next;         /* the handler the shim chains to */

void __attribute__((used)) prof_irq_body(const uint32_t *frame);
void prof_irq_body(const uint32_t *frame)
{
    if (yp3_prof_late && yp3_pcm_late_now() < (long)yp3_prof_late)
        return;
    prof_sample(frame);
}

__attribute__((naked)) static void prof_irq_shim(void)
{
    __asm__ volatile("tst lr, #4\n"
                     "ite eq\n"
                     "mrseq r0, msp\n"
                     "mrsne r0, psp\n"
                     "push {r4, lr}\n"
                     "bl prof_irq_body\n"
                     "pop {r4, lr}\n"
                     "ldr r1, =prof_irq_next\n"
                     "ldr r1, [r1]\n"
                     "bx r1\n");
}

void yp3_prof_irq(int irq)
{
    extern uint32_t __vectors_arm[];
    int old = disable_irq_save();
    if (prof_irq != ~0u)
        __vectors_arm[16u + prof_irq] = prof_irq_next;
    prof_irq = irq < 0 ? ~0u : (unsigned)irq;
    if (irq >= 0) {
        prof_irq_next = __vectors_arm[16u + irq];
        __vectors_arm[16u + irq] = (uint32_t)(uintptr_t)prof_irq_shim | 1u;
    }
    __asm__ volatile("dsb\n\tisb" ::: "memory");
    restore_irq(old);
}

/* Scheduler snapshot for yp3.py threads, in the retained dev log block
 * (the ROM reuses the scheduler's own SRAM after a watchdog return): one
 * slot copied per tick, all of them by the first tick that finds a slot
 * whose id names another slot, a saved sp off its stack or a running
 * pointer that is no slot, which freezes it (the words around go to the
 * log). */
#define SNAP_RETURNS    3       /* return addresses kept per thread */
#define SNAP_SCAN_WORDS 64      /* stack words searched for them */
#define SNAP_RUNNING    0xff    /* thread_snap.bad: the running pointer */
#define SNAP_DUMP_WORDS 42      /* logged from 4 words before the bad entry */
#define SNAP_DUMP_LINE  7

struct thread_snap {
    uint32_t magic, size;       /* dev-yp3.c's */
    uint8_t slots, tasks, returns;
    uint8_t bad;                /* 0, or the failed check: slot + 1 or
                                   SNAP_RUNNING */
    uint32_t tick;              /* current_tick of the copy */
    uint32_t bad_tick;
    uint32_t running;
    uint32_t tick_funcs[MAX_NUM_TICK_TASKS];
    struct {
        uint32_t id, name, sp, wqp;
        uint32_t ret[SNAP_RETURNS];
        uint8_t state, priority, base_priority, pad;
    } slot[MAXTHREADS];
};
_Static_assert(sizeof(struct thread_snap) <= YP3_DEV_SNAP_BYTES,
               "thread snapshot");

static bool ICODE_ATTR in_sram(const void *p, size_t size)
{
    uintptr_t a = (uintptr_t)p;
    return a >= SL6801_RAM_BASE &&
           a + size <= SL6801_RAM_BASE + SL6801_RAM_SIZE;
}

static void ICODE_ATTR thread_snap_slot(struct thread_snap *s, unsigned int i)
{
    const struct thread_entry *t = __thread_slot_entry(i);
    memset(&s->slot[i], 0, sizeof s->slot[i]);
    if (!in_sram(t, sizeof *t))
        return;
    s->slot[i].id = t->id;
    s->slot[i].name = (uintptr_t)t->name;
    s->slot[i].sp = t->context.sp;
    s->slot[i].wqp = (uintptr_t)t->wqp;
    s->slot[i].state = t->state;
    s->slot[i].priority = t->priority;
    s->slot[i].base_priority = t->base_priority;
    if (t->state == STATE_KILLED)
        return;
    const uint32_t *sp = (const uint32_t *)t->context.sp;
    const uint32_t *end = (const uint32_t *)((char *)t->stack + t->stack_size);
    for (int k = 0, n = 0; k < SNAP_SCAN_WORDS && n < SNAP_RETURNS &&
         &sp[k] < end && in_sram(&sp[k], 4); k++)
        if (is_xip_return(sp[k]))
            s->slot[i].ret[n++] = sp[k] & ~1u;
}

/* The header, and slot SLOT or (-1) every slot */
static void ICODE_ATTR thread_snap_copy(struct thread_snap *s,
                                        const struct thread_entry *run,
                                        int slot)
{
    extern void (*tick_funcs[MAX_NUM_TICK_TASKS+1])(void);
    s->slots = MAXTHREADS;
    s->tasks = MAX_NUM_TICK_TASKS;
    s->returns = SNAP_RETURNS;
    s->tick = current_tick;
    s->running = (uintptr_t)run;
    for (int i = 0; i < MAX_NUM_TICK_TASKS; i++)
        s->tick_funcs[i] = (uintptr_t)tick_funcs[i];
    for (unsigned int i = 0; i < MAXTHREADS; i++)
        if (slot < 0 || (unsigned int)slot == i)
            thread_snap_slot(s, i);
}

/* In SRAM: from XIP the per-tick work (~410 us) still left 1 ms gaps in
 * the audio DMA */
static void ICODE_ATTR NO_INLINE thread_snap_tick(uint32_t pc)
{
    struct thread_snap *s = yp3_dev_snap_area();
    if (s->bad)
        return;                 /* frozen at the first failed check */
    const struct thread_entry *run = __running_self_entry();
    /* One slot checked and copied per tick: all of them took ~5 ms from
     * XIP, which delays the audio DMA (not its IRQ) longer than a DAC
     * period (dev playback ran ~4% slow). The running pointer is checked
     * every tick. */
    unsigned int i = current_tick % MAXTHREADS;
    const struct thread_entry *t = __thread_slot_entry(i);
    const void *at = t;
    const char *why = NULL;
    unsigned int bad = i + 1;
    if (!in_sram(t, sizeof *t) || THREAD_ID_SLOT(t->id) != i)
        why = "a slot's id names another";
    /* a switched-out thread resumes on its saved sp */
    else if (t != run && t->state != STATE_KILLED &&
             (t->context.sp < (uintptr_t)t->stack ||
              t->context.sp > (uintptr_t)t->stack + t->stack_size))
        why = "a slot's saved sp is off its stack";
    /* NULL from thread_exit() to the switch away */
    else if (run) {
        unsigned int k = 0;
        while (k < MAXTHREADS && __thread_slot_entry(k) != run)
            k++;
        if (k == MAXTHREADS) {
            bad = SNAP_RUNNING;
            at = __core_id_entry(CURRENT_CORE);
            why = "running is no slot";
        }
    }
    if (!why) {
        thread_snap_copy(s, run, i);
        return;
    }
    thread_snap_copy(s, run, -1);
    s->bad = bad;
    s->bad_tick = current_tick;
    dev_log("thread check: %s (running %08lx) at tick %ld, pc %08lx; "
            "words from %08lx:",
            why, (unsigned long)(uintptr_t)run, current_tick,
            (unsigned long)pc, (unsigned long)((uintptr_t)at - 16));
    const uint32_t *w = (const uint32_t *)at - 4;
    if (!in_sram(w, 4 * SNAP_DUMP_WORDS))
        return;
    for (int k = 0; k < SNAP_DUMP_WORDS; k += SNAP_DUMP_LINE)
        dev_log("  %08lx %08lx %08lx %08lx %08lx %08lx %08lx",
                (unsigned long)w[k], (unsigned long)w[k + 1],
                (unsigned long)w[k + 2], (unsigned long)w[k + 3],
                (unsigned long)w[k + 4], (unsigned long)w[k + 5],
                (unsigned long)w[k + 6]);
}

void yp3_dev_watch_thread_sp(unsigned int slot)
{
    struct thread_entry *t = __thread_slot_entry(slot);
    yp3_dev_watch_range((uintptr_t)&t->context.sp, (uintptr_t)t->stack,
                        (uintptr_t)t->stack + t->stack_size);
}

unsigned long yp3_tick_task_us[MAX_NUM_TICK_TASKS + 1];
unsigned long yp3_tick_us, yp3_dma_us, yp3_dma_irqs, yp3_switches;

void systick_body(const uint32_t *frame)
{
    unsigned long t0 = sl6801_us();
    yp3_dev_trace_tick(frame[EXC_FRAME_PC]);
    thread_snap_tick(frame[EXC_FRAME_PC]);
    if (prof_irq == ~0u)
        prof_sample(frame);
    /* call_tick_tasks(), timing each task */
    extern void (*tick_funcs[MAX_NUM_TICK_TASKS+1])(void);
    current_tick++;
    unsigned long t1 = sl6801_us();
    yp3_tick_task_us[MAX_NUM_TICK_TASKS] += t1 - t0;
    for (int i = 0; tick_funcs[i]; i++) {
        tick_funcs[i]();
        unsigned long t2 = sl6801_us();
        yp3_tick_task_us[i] += t2 - t1;
        t1 = t2;
    }
    yp3_tick_us += sl6801_us() - t0;
}

/* Pass the exception frame to the body: EXC_RETURN bit 2 (lr) says whether
 * it was stacked on the MSP (clear) or PSP (set). */
__attribute__((naked)) void systick_handler(void)
{
    __asm__ volatile("tst lr, #4\n"
                     "ite eq\n"
                     "mrseq r0, msp\n"
                     "mrsne r0, psp\n"
                     "b systick_body\n");
}
#else
void systick_handler(void)
{
    call_tick_tasks();
}
#endif

#ifdef YP3_DEV
/* CPU time per thread for the dev shell's "load": switch_thread() charges
 * the time since its last call to the thread it switches away from (slot
 * MAXTHREADS: the scheduler, including the idle loop). Interrupts count
 * for the thread they interrupted. */
static unsigned long thread_us[MAXTHREADS + 1], account_t0;

void thread_cpu_account(int slot)
{
    unsigned long now = sl6801_us();
    yp3_switches += slot >= 0;
    thread_us[slot < 0 ? MAXTHREADS : slot] += now - account_t0;
    account_t0 = now;
}

/* Log each thread's share of the next SECS seconds (0.5% or more) */
void yp3_dev_thread_load(int secs)
{
    static unsigned long before[MAXTHREADS + 1];
    unsigned long i0 = yp3_idle_us, tick0 = yp3_tick_us, dma0 = yp3_dma_us;
    unsigned long irqs0 = yp3_dma_irqs, sw0 = yp3_switches;
    static unsigned long tt0[MAX_NUM_TICK_TASKS + 1];
    memcpy(tt0, yp3_tick_task_us, sizeof tt0);
    unsigned long t0 = sl6801_us();
    memcpy(before, thread_us, sizeof before);
    sleep(secs * HZ);
    unsigned long all = sl6801_us() - t0;
    unsigned long idle = yp3_idle_us - i0;
    dev_log("load %lu ms: idle %lu.%lu%% at %ld MHz", all / 1000,
                idle * 100 / all, idle * 1000 / all % 10,
                cpu_frequency / 1000000);
    dev_log("  tick isr %lu us/s, dma isr %lu us/s (%lu/s), %lu switches/s",
                (yp3_tick_us - tick0) / secs, (yp3_dma_us - dma0) / secs,
                (yp3_dma_irqs - irqs0) / secs, (yp3_switches - sw0) / secs);
    dev_log("  pcm late: irq max %lu us >100 %lu >300 %lu; stall max %lu us "
                ">100 %lu >300 %lu (lock %08lx held %lu us)",
                yp3_pcm_late.max_us[0], yp3_pcm_late.over100[0],
                yp3_pcm_late.over300[0], yp3_pcm_late.max_us[1],
                yp3_pcm_late.over100[1], yp3_pcm_late.over300[1],
                (unsigned long)yp3_pcm_late.lock_caller, yp3_pcm_late.lock_us);
    memset(&yp3_pcm_late, 0, sizeof yp3_pcm_late);
    extern void (*tick_funcs[MAX_NUM_TICK_TASKS+1])(void);
    for (int i = 0; i <= MAX_NUM_TICK_TASKS; i++) {
        unsigned long us = (yp3_tick_task_us[i] - tt0[i]) / secs;
        if (us >= 200)
            dev_log("  tick %08lx %lu us/s", i == MAX_NUM_TICK_TASKS ? 0ul :
                        (unsigned long)(uintptr_t)tick_funcs[i], us);
    }
    for (int i = 0; i <= MAXTHREADS; i++) {
        unsigned long us = thread_us[i] - before[i];
        if (i == MAXTHREADS)
            us -= idle;
        if (us * 200 < all)
            continue;
        const char *name = i == MAXTHREADS ? "(scheduler)" :
                           __thread_slot_entry(i)->name;
        dev_log("  %-12s %2lu.%lu%%", name ? name : "?",
                    us * 100 / all, us * 1000 / all % 10);
    }
}
#endif
