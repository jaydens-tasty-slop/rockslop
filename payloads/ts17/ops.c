/* Copied unchanged from rockbox-yp3 ts17dev/gadgets/ for rockslop-flasher. */
/* Register-script gadget: runs a list of ops in one hook call (see hook.S) so
 * that a multi-register sequence happens without USB round trips in between.
 * Throwaway probe tooling.
 *
 * While it runs, IRQs are masked and VTOR points at a RAM table whose fault
 * entries skip a precisely faulting instruction and log it, so probing an
 * unknown address reports a fault instead of hanging in the ROM's handler
 * until the watchdog boots stock firmware. Bus stalls are not caught.
 *
 * Layout: n_ops at 0x01018800, current op index at 0x01018804, ops from
 * 0x01018810 (16 bytes each, up to 0x01019810). Output at 0x01040000, see
 * enum out below. */
#include <stdint.h>

struct op { uint32_t kind, addr, val, mask; };
#define N_OPS   (*(volatile uint32_t *)0x01018800)
#define CUR_OP  (*(volatile uint32_t *)0x01018804)
#define OPS     ((volatile struct op *)0x01018810)
#define OUT     ((volatile uint32_t *)0x01040000)
#define TABLE   ((volatile uint32_t *)0x0101a000)  /* RAM vector table */
#define REG(a)  (*(volatile uint32_t *)(a))
#define VTOR    REG(0xE000ED08)
#define CFSR    REG(0xE000ED28)
#define HFSR    REG(0xE000ED2C)
#define MMFAR   REG(0xE000ED34)
#define BFAR    REG(0xE000ED38)
#define MAX_OPS 256
#define MAX_RES 0x3f00
#define POLL_LIMIT 0x100000u
#define TABLE_WORDS 16
#define IMPRECISERR (1u << 10)

enum out {
    O_COUNT, O_NFAULT, O_VTOR_ROM, O_VTOR_SET, O_CFSR, O_HFSR, O_BFAR, O_MMFAR,
    O_FAULT_OPS,                   /* 8 words: indices of the first faults */
    O_RES = O_FAULT_OPS + 8,       /* results follow */
};

enum {
    OP_END,   /* stop */
    OP_W32,   /* *addr = val */
    OP_R32,   /* res <- *addr */
    OP_W8,    /* *(u8 *)addr = val */
    OP_R8,    /* res <- *(u8 *)addr */
    OP_POLL,  /* spin until (*addr & mask) == val; res <- spins, ~0 on timeout */
    OP_DELAY, /* spin val iterations */
    OP_RMW,   /* *addr = (*addr & ~mask) | val */
    OP_RN32,  /* res <- *addr, val times (FIFO drain) */
    OP_RN8,   /* res <- *(u8 *)addr, val times */
    OP_COPY,  /* copy mask words from addr to val (word loads) */
    OP_CALL,  /* res <- addr(r0..r3 = four words at val); Thumb bit added */
};

void fault_c(uint32_t *frame)
{
    uint32_t cfsr = CFSR, hfsr = HFSR, n = OUT[O_NFAULT];
    if (n == 0) {
        OUT[O_CFSR] = cfsr;
        OUT[O_HFSR] = hfsr;
        OUT[O_BFAR] = BFAR;
        OUT[O_MMFAR] = MMFAR;
    }
    if (n < 8)
        OUT[O_FAULT_OPS + n] = CUR_OP;
    OUT[O_NFAULT] = n + 1;
    if (!(cfsr & IMPRECISERR)) {
        /* precise: the stacked PC is the faulting instruction; skip it */
        uint32_t pc = frame[6];
        uint16_t hw = *(volatile uint16_t *)pc;
        frame[6] = pc + (hw >= 0xe800 ? 4 : 2);
    }
    CFSR = cfsr;    /* write-one-to-clear */
    HFSR = hfsr;
}

__attribute__((naked)) void fault_entry(void)
{
    __asm__ volatile(
        "tst lr, #4\n"
        "ite eq\n"
        "mrseq r0, msp\n"
        "mrsne r0, psp\n"
        "b fault_c\n");
}

__attribute__((naked)) void nmi_entry(void)
{
    __asm__ volatile("bx lr\n");
}

static void run_ops(void)
{
    uint32_t n = N_OPS, k = 0;
    if (n > MAX_OPS)
        n = MAX_OPS;
    for (uint32_t i = 0; i < n; i++) {
        uint32_t kind = OPS[i].kind, a = OPS[i].addr;
        uint32_t v = OPS[i].val, m = OPS[i].mask;
        volatile uint32_t *p = (volatile uint32_t *)a;
        volatile uint8_t *b = (volatile uint8_t *)a;
        CUR_OP = i;
        __asm__ volatile ("dsb");
        if (kind == OP_END)
            break;
        switch (kind) {
        case OP_W32: *p = v; break;
        case OP_W8: *b = (uint8_t)v; break;
        case OP_R32: if (k < MAX_RES) OUT[O_RES + k++] = *p; break;
        case OP_R8: if (k < MAX_RES) OUT[O_RES + k++] = *b; break;
        case OP_RMW: *p = (*p & ~m) | v; break;
        case OP_DELAY: for (volatile uint32_t d = 0; d < v; d++) ; break;
        case OP_POLL: {
            uint32_t s = 0;
            while ((*p & m) != v && s < POLL_LIMIT)
                s++;
            if (k < MAX_RES) OUT[O_RES + k++] = s < POLL_LIMIT ? s : ~0u;
            break;
        }
        case OP_RN32:
        case OP_RN8:
            for (uint32_t j = 0; j < v && k < MAX_RES; j++)
                OUT[O_RES + k++] = kind == OP_RN32 ? *p : *b;
            break;
        case OP_CALL: {
            volatile uint32_t *r = (volatile uint32_t *)v;
            uint32_t (*fn)(uint32_t, uint32_t, uint32_t, uint32_t) =
                (uint32_t (*)(uint32_t, uint32_t, uint32_t, uint32_t))(a | 1);
            uint32_t ret = fn(r[0], r[1], r[2], r[3]);
            if (k < MAX_RES)
                OUT[O_RES + k++] = ret;
            break;
        }
        case OP_COPY: {
            volatile uint32_t *d = (volatile uint32_t *)v;
            for (uint32_t j = 0; j < m; j++)
                d[j] = p[j];
            break;
        }
        }
        __asm__ volatile ("dsb");
    }
    OUT[O_COUNT] = k;
}

__attribute__((section(".text.start"), used))
int entry(void)
{
    uint32_t primask, vtor = VTOR;
    __asm__ volatile ("mrs %0, primask\n cpsid i" : "=r"(primask) :: "memory");
    for (int i = 1; i < O_RES; i++)
        OUT[i] = 0;
    for (int i = 0; i < TABLE_WORDS; i++)
        TABLE[i] = (uint32_t)nmi_entry;
    for (int i = 3; i <= 6; i++)    /* HardFault, MemManage, BusFault, UsageFault */
        TABLE[i] = (uint32_t)fault_entry;
    __asm__ volatile ("dsb");
    VTOR = (uint32_t)TABLE;
    __asm__ volatile ("dsb\n isb");
    OUT[O_VTOR_ROM] = vtor;
    OUT[O_VTOR_SET] = VTOR;
    run_ops();
    VTOR = vtor;
    __asm__ volatile ("dsb\n isb\n msr primask, %0" :: "r"(primask) : "memory");
    return 0;
}
