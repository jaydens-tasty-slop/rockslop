/* SPDX-License-Identifier: GPL-2.0-or-later */
/* ROM Bluetooth controller glue for the YP3.
 *
 * The SL6801 boot ROM carries a RivieraWaves dual-mode controller (kernel at
 * ROM 0x2d000..0x46000). It reaches the outside world through ~35 RAM "hook"
 * slots in ROM BSS that the firmware must fill, plus an interrupt (IRQ 26), a
 * cooperative kernel scheduler and a critical section; this file provides
 * them.
 *
 * Memory rule: the ROM controller's state lives in ROM
 * BSS below 0x804c00, which Rockbox never uses and the ROM itself reads, so
 * everything there (link envs, hook slots, kernel env, the glue command env
 * at 0x803c28, the link-controller env at 0x80296c..0x8031ff) stays at its
 * absolute address. This file's own state is in .bss. ROM kernel entry
 * points are Thumb: called through pointers with bit0 set (ROM_* below).
 *
 * The controller runs entirely in interrupt context (Rockbox threads are
 * cooperative and UI work can hold the CPU 100+ ms). IRQ 26 services the
 * baseband; ke_event_set() pends a spare, lower-priority NVIC line whose
 * handler runs the kernel scheduler until no event bits remain. Hot code
 * (critical section, event set, scheduler, ISR) runs from SRAM (ICODE_ATTR);
 * the rest stays in XIP.
 *
 * CONTROLLER SLEEP is not implemented (status bits 2/12, register
 * 0x40020030). The scheduler always runs from the spare NVIC line while the
 * controller is up; this is simpler and fine for an always-on A2DP source. */

#include "config.h"
#include "system.h"
#include "sl6801.h"
#include "irq-yp3.h"
#include "mmio-sl6801.h"
#include "bt-yp3.h"
#include "bt_drv.h"
#include "dev-yp3.h"
#include "core_alloc.h"
#include "core_alloc_top.h"
#include "panic.h"
#include <string.h>
#ifdef YP3_DEV
#include <stdio.h>
#endif

#define SLOT(a)  REG(a)

/* ======================================================================
 * ROM kernel ABI (Thumb; call via pointer with bit0 set). Typedef names give
 * the return type then the arguments (v void, p pointer, u/i u32).
 * ====================================================================== */
typedef void  (*rom_v_v)(void);
typedef void *(*rom_p_alloc)(uint32_t, uint32_t, uint32_t, uint32_t);
typedef void  (*rom_v_p)(void *);
typedef void  (*rom_v_basic)(uint32_t, uint32_t, uint32_t);
typedef void  (*rom_v_td)(uint32_t, const void *);
typedef void  (*rom_v_state)(uint32_t, uint32_t);
typedef void *(*rom_p_v)(void);
typedef void *(*rom_p_list)(void *list);
typedef void  (*rom_v_list2)(void *list, void *elem);
typedef uint32_t (*rom_u_u)(uint32_t);
typedef uint16_t (*rom_h_u)(uint32_t);

#define ROM_KE_MSG_ALLOC   ((rom_p_alloc)BT_THUMB(0x309ec))
#define ROM_KE_MSG_SEND    ((rom_v_p)    BT_THUMB(0x30a30))
#define ROM_KE_MSG_SEND_B  ((rom_v_basic)BT_THUMB(0x30a60))
                                                /* alloc(len 0)+send */
#define ROM_KE_MSG_FREE    ((rom_v_p)    BT_THUMB(0x30a9e))  /* (msg header) */
#define ROM_KE_TASK_CREATE ((rom_v_td)   BT_THUMB(0x3089c))
#define ROM_KE_STATE_SET_ADDR 0x3092c
#define ROM_KE_STATE_SET   ((rom_v_state)BT_THUMB(ROM_KE_STATE_SET_ADDR))
#define ROM_KE_ACL_ALLOC   ((rom_p_v)    BT_THUMB(0x305b8))
#define ROM_KE_SCHED       ((rom_v_v)    BT_THUMB(0x30db4))
                                                /* -> [HOOK_SCHED] */
#define ROM_CO_LIST_POP    ((rom_p_list) BT_THUMB(0x303cc))
                                                /* pop_front(list) */
#define ROM_CO_LIST_PUSH   ((rom_v_list2)BT_THUMB(0x303ac))
                                                /* push_back(list,e) */
#define ROM_LC_CLK_ISR     ((rom_v_v)    BT_THUMB(0x2f3a4))
#define ROM_LC_RX_ISR      ((rom_v_v)    BT_THUMB(0x2f388))
#define ROM_SET_BDADDR     ((rom_v_p)    BT_THUMB(0x3ba30))
#define ROM_KE_FLUSH       ((rom_v_v)    BT_THUMB(0x30ab0))
                                                /* free pending msgs/timers */
/* Link-layer data-path lookups for a connection handle (bt_ll_data_*). */
#define ROM_LL_DATA_STATE  ((rom_u_u)    BT_THUMB(0x3075c))
#define ROM_LL_DATA_TASK   ((rom_h_u)    BT_THUMB(0x3076c))

/* ======================================================================
 * ROM BSS absolute addresses (< 0x804c00, shared with the ROM).
 * ====================================================================== */
/* Kernel env. The task table holds a u8 count, then one 8-byte entry per
 * task: u8 type at +0, descriptor pointer at +4. Each co_list is {head,
 * tail}. */
#define KE_ENV            0x0080415c   /* task table */
#define KE_ENV_TASK_TYPE(i) (KE_ENV + 4 + 8 * (i))
#define KE_ENV_TASK_DESC(i) (KE_ENV + 8 + 8 * (i))
#define KE_MSG_QUEUE      0x008041b4   /* co_list: ke_msg_send target */
#define KE_MSG_SAVED_LIST 0x008041bc   /* co_list for KE_MSG_SAVED */
#define KE_EVT_FIELD      0x008041e4   /* u32 pending event bitfield */
#define KE_EVT_CB(bit)    (0x008041e8 + 4 * (bit))  /* per-bit callback */
/* Hook slots. The ROM's kernel API functions are trampolines that jump
 * through these (e.g. ke_msg_alloc 0x309ec reads HOOK_KE_MSG_ALLOC). */
#define HOOK_CRIT_ENTER   0x00803c1c
#define HOOK_CRIT_EXIT    0x00803c20
#define HOOK_RANDOM       0x00803c24
#define HOOK_POOL_ALLOC   0x008040d0
#define HOOK_POOL_FREE    0x008040d4
#define HOOK_POOL1_ALLOC  0x00804124   /* reader not traced */
#define HOOK_KE_MSG_ALLOC 0x008041a8   /* trampoline 0x309ec */
#define HOOK_KE_MSG_SEND  0x008041ac   /* trampoline 0x30a30 */
#define HOOK_KE_MSG_FWD   0x008041b0   /* trampoline 0x30a70 */
#define HOOK_KE_MALLOC    0x008041cc
#define HOOK_KE_FREE      0x008041d0
#define HOOK_KE_TIMER_HW  0x008041d4   /* reader not traced */
#define HOOK_KE_TIMER_SET 0x008041d8   /* trampoline 0x30bec */
#define HOOK_KE_TIMER_CLR 0x008041dc   /* trampoline 0x30c9c */
#define HOOK_KE_TIMER_ACT 0x008041e0   /* trampoline 0x30d0c */
#define HOOK_SCHED        0x00804200
#define HOOK_EVT_SET      0x00804204
/* ROM bodies for those kernel slots. */
#define ROM_KE_MSG_ALLOC_IMPL 0x30a01  /* header + param_len from ke_malloc */
#define ROM_KE_MSG_SEND_IMPL  0x30a3d
#define ROM_KE_MSG_FWD_IMPL   0x30a7d  /* (param, dest, src): retarget, send */
#define ROM_KE_TIMER_HW_IMPL  0x30b59  /* program hw timer for head timer */
#define ROM_KE_TIMER_SET_IMPL 0x30bf9
#define ROM_KE_TIMER_CLR_IMPL 0x30ca9  /* (id, task): unqueue and free */
#define ROM_KE_TIMER_ACT_IMPL 0x30d19  /* (id, task): 1 if queued */
#define ROM_KE_TIMER_EXPIRE   0x30b8d  /* event bit 1 callback */
#define ROM_POOL1_ALLOC_IMPL  0x305d9  /* block from pool 1, + 4 */

/* Packet pool free-list heads (see struct pool_blk). */
#define POOL_HEAD_0       0x0080296c
#define POOL_HEAD(n)      (POOL_HEAD_0 + 4 * (n))
/* Link envs, one per link-manager task instance. */
#define LINK_COUNT        7
/* LT_ADDRs of the links we are master of, bits 1..7: ROM 0x3b9ec takes the
 * lowest free one (0 if none), 0x3ba14 frees one. */
#define LT_ADDR_MAP       0x00802c21

/* BT core registers (0x40020000). The layout matches the
 * RivieraWaves BT core: control, interrupt control, status, ack. */
#define BT_CORE_CTRL_SOFTRST (1u << 29)  /* register reset (cold start) */
#define BT_CORE_CTRL_MASTER_RST (1u << 31) /* state reset, keeps registers */
#define BT_CORE_CTRL_EN   (1u << 8)
#define BT_CORE_ACK_ALL   0xffffffff
/* BT_CORE_2C is unidentified; its start value is 0x1ff (from stock
 * firmware). */
#define BT_CORE_2C_INIT   0x1ff
/* RADIOPWRUPDN: radio power-up/down timing, programmed
 * field by field (0x1b460846 overall; values from stock firmware). Field
 * names are the RivieraWaves ones, whose layout these masks match. */
#define RTRIP_DELAY_MASK  0x7f000000u
#define RTRIP_DELAY_INIT  0x1b000000u
#define RXPWRUP_MASK      0x00ff0000u
#define RXPWRUP_INIT      0x00460000u
#define TXPWRDN_MASK      0x00000f00u
#define TXPWRDN_INIT      0x00000800u
#define TXPWRUP_MASK      0x000000ffu
#define TXPWRUP_INIT      0x00000046u
/* Polls of the self-clearing reset bits before giving up. */
#define BT_CORE_RESET_SPINS 100000

/* Exchange memory. It starts with 16 halfwords set to 0x6000 (from stock
 * firmware; the hop tables follow at 0x40030020). */
#define EM_HDR_BYTES      0x20
#define EM_HDR_FILL       0x6000

/* The baseband interrupt and the scheduler's spare line, and their
 * priorities: BT_IRQ, BT_SCHED_IRQ (irq-yp3.h). */
/* Cap scheduler passes per interrupt so a stuck event bit cannot spin in
 * interrupt context; the handler re-pends itself if work remains. */
#define BT_SCHED_MAX_PASSES 64u

/* ke event bits. Their callbacks KE_EVT_CB(0..KE_EVT_MAX) end where
 * HOOK_SCHED starts. */
#define KE_EVT_MSG        0
#define KE_EVT_TIMER      1
#define KE_EVT_HCI_TX     5
#define KE_EVT_MAX        5

/* ======================================================================
 * Our .bss state (kept small).
 * ====================================================================== */
static volatile uint32_t bt_irq_count;
static volatile uint32_t bt_sched_count;
#ifdef YP3_DEV
#define BT_ST_COUNTED 16             /* "bt ctrl state": status bit counts */
static uint32_t bt_irq_bits[BT_ST_COUNTED];
static bool ke_trace;                /* "bt ctrl trace on|off" */
#endif
static bool bt_running;

/* ke_malloc arena. The ROM's own heap overlaps the Rockbox image (a 144 KiB
 * alloc landed at 0x83a090, measured), so the kernel gets its own first-fit
 * arena: a pinned core_alloc made by bt_ctrl_yp3_start() and freed by stop,
 * so Bluetooth off costs nothing. Every kernel message (ROM ke_msg_alloc
 * 0x30a00: param_len + 16) and timer (ROM 0x30c38: 12 bytes) comes from
 * here, and the ROM does not check for NULL, so exhaustion panics.
 * The 3 KiB is a fixed size, not derived from a measured peak; dev builds
 * show the high-water mark in "bt ctrl state". */
#define KE_ARENA_BYTES 3072
/* Header in front of every block, free or used; words counts the payload. */
struct ke_block { struct ke_block *next; uint32_t words; uint32_t used; };
#define KE_BLOCK_WORDS (sizeof(struct ke_block) / 4)
static int ke_arena_handle;          /* core_alloc handle, 0 = none */
static struct ke_block *ke_arena;    /* first block */
static uint32_t ke_arena_used, ke_arena_peak;  /* words incl. headers */

/* ======================================================================
 * Critical section. Masks all maskable interrupts (so IRQ 26 and the
 * scheduler line) with a nesting count, as the ROM enters and leaves it
 * around every list/pool operation.
 * ====================================================================== */
static volatile int bt_crit_nest;   /* .bss (SRAM); accessed from ICODE */
static int bt_crit_saved;

void ICODE_ATTR bt_ctrl_enter_critical(void)
{
    int primask = disable_irq_save();
    if (bt_crit_nest++ == 0)
        bt_crit_saved = primask;
}

void ICODE_ATTR bt_ctrl_exit_critical(void)
{
    if (--bt_crit_nest == 0)
        restore_irq(bt_crit_saved);
}

/* ======================================================================
 * Kernel event set and scheduler (NVIC line).
 * ====================================================================== */
static void ICODE_ATTR ke_sched_pend(void)
{
    yp3_irq_clear(BT_SCHED_IRQ);
    yp3_irq_pend(BT_SCHED_IRQ);
}

/* Installed at HOOK_EVT_SET; the ROM ke_event_set trampoline (0x30dc0) calls
 * this. Set the bit, then pend the scheduler line. */
static void ICODE_ATTR ke_event_set_hook(uint32_t bit)
{
    bt_ctrl_enter_critical();
    if (bit <= KE_EVT_MAX)
        REG(KE_EVT_FIELD) |= (1u << bit);
    bt_ctrl_exit_critical();
    ke_sched_pend();
}

/* Installed at HOOK_SCHED; the ROM ke_schedule trampoline (0x30db4) calls
 * this once per pass: dispatch the highest pending event bit to its
 * callback. A bit with no callback would otherwise never clear and spin the
 * scheduler, so clear and log it here. */
static void ICODE_ATTR ke_sched_dispatch(void)
{
    int primask = disable_irq_save();
    uint32_t field = REG(KE_EVT_FIELD);
    restore_irq(primask);
    for (int bit = KE_EVT_MAX; bit >= 0; bit--)
        if (field & (1u << bit)) {
            void (*cb)(void) = *(void (**)(void))(uintptr_t)KE_EVT_CB(bit);
            if (cb) {
                cb();
            } else {
                bt_ctrl_enter_critical();
                REG(KE_EVT_FIELD) &= ~(1u << bit);
                bt_ctrl_exit_critical();
                dev_log("bt: ke event %d has no callback", bit);
            }
            return;
        }
}

/* Scheduler NVIC handler: run the ROM scheduler until no event bits remain,
 * capped per interrupt so a bit a callback fails to clear cannot spin here
 * forever; re-pend if work is left. */
static void ICODE_ATTR bt_sched_irq(void)
{
    bt_sched_count++;
    unsigned passes = 0;
    while (REG(KE_EVT_FIELD) != 0) {
        if (++passes > BT_SCHED_MAX_PASSES) {
            ke_sched_pend();          /* finish the rest after this returns */
            break;
        }
        ROM_KE_SCHED();
    }
}

/* ======================================================================
 * Random: 1 MHz timestamp * 123 % 80 (from stock firmware).
 * ====================================================================== */
#define RANDOM_MUL   123u
#define RANDOM_RANGE 80u            /* result 0..79 */
static uint32_t bt_random_hook(void)
{
    return (REG(SL6801_COUNTER_LO) * RANDOM_MUL) % RANDOM_RANGE;
}

/* ======================================================================
 * ke_malloc / ke_free (see KE_ARENA_BYTES). Installed at HOOK_KE_MALLOC /
 * HOOK_KE_FREE.
 * ====================================================================== */
/* The ROM passes (size, type); type is unused here. */
static void *ke_malloc_hook(uint32_t size)
{
    uint32_t need = (size + 3) / 4;
    void *ret = NULL;
    bt_ctrl_enter_critical();
    for (struct ke_block *b = ke_arena; b; b = b->next)
        if (!b->used && b->words >= need) {
            uint32_t rest = b->words - need;
            b->used = 1;
            if (rest > KE_BLOCK_WORDS) {        /* split off the remainder */
                struct ke_block *n =
                    (struct ke_block *)((uint32_t *)(b + 1) + need);
                n->next = b->next;
                n->words = rest - KE_BLOCK_WORDS;
                n->used = 0;
                b->next = n;
                b->words = need;
            }
            ke_arena_used += KE_BLOCK_WORDS + b->words;
            if (ke_arena_used > ke_arena_peak)
                ke_arena_peak = ke_arena_used;
            ret = b + 1;
            break;
        }
    bt_ctrl_exit_critical();
    if (!ret)
        panicf("bt: ke_malloc(%lu) out of arena", (unsigned long)size);
    return ret;
}

static void ke_free_hook(void *ptr)
{
    if (!ptr)
        return;
    bt_ctrl_enter_critical();
    struct ke_block *b = (struct ke_block *)ptr - 1;
    b->used = 0;
    ke_arena_used -= KE_BLOCK_WORDS + b->words;
    for (struct ke_block *c = ke_arena; c; c = c->next)
        while (!c->used && c->next && !c->next->used) {   /* coalesce */
            c->words += KE_BLOCK_WORDS + c->next->words;
            c->next = c->next->next;
        }
    bt_ctrl_exit_critical();
}

/* Thread context only (core_alloc). */
static bool ke_arena_open(void)
{
    ke_arena_handle = core_alloc_top(KE_ARENA_BYTES);   /* pinned while up */
    if (ke_arena_handle <= 0) {
        ke_arena_handle = 0;
        return false;
    }
    core_pin(ke_arena_handle);
    ke_arena = core_get_data(ke_arena_handle);
    ke_arena->next = NULL;
    ke_arena->words = KE_ARENA_BYTES / 4 - KE_BLOCK_WORDS;
    ke_arena->used = 0;
    ke_arena_used = ke_arena_peak = 0;
    return true;
}

static void ke_arena_close(void)
{
    if (ke_arena_handle)
        core_free(ke_arena_handle);
    ke_arena_handle = 0;
    ke_arena = NULL;
}

/* ======================================================================
 * Packet pools. The ROM builds free lists of exchange-memory blocks during
 * its pool init (heads POOL_HEAD(n)) and calls HOOK_POOL_ALLOC with the head
 * to take a block from (it gets the payload) and HOOK_POOL_FREE with a
 * payload to put back.
 * ====================================================================== */
struct pool_blk {
    uint32_t link;      /* free: next free block; allocated: its list head */
    uint32_t hdr;       /* second header word (not used here) */
    uint8_t payload[];  /* what the ROM sees */
};

/* F-0057: alloc gets the address of a head word: take the first block,
 * store the head's address in its link word (free finds the list by it),
 * return the payload; NULL when empty. Both run in the critical section,
 * as IRQ 26 allocates and frees too. */
static void *pool_take(uint32_t *head)
{
    bt_ctrl_enter_critical();
    struct pool_blk *b = (struct pool_blk *)(uintptr_t)*head;
    if (b) {
        *head = b->link;
        b->link = (uint32_t)(uintptr_t)head;
    }
    bt_ctrl_exit_critical();
    return b ? b->payload : NULL;
}

static void pool_give(void *payload)
{
    if (!payload)
        return;
    struct pool_blk *b = (struct pool_blk *)((uint8_t *)payload -
                                             offsetof(struct pool_blk, payload));
    uint32_t *head = (uint32_t *)(uintptr_t)b->link;
    if ((uintptr_t)head < POOL_HEAD(0) || (uintptr_t)head > POOL_HEAD(3) ||
        ((uintptr_t)head & 3u)) {
        dev_log("bt ctrl: pool free of %p, bad list %p", payload, head);
        return;
    }
    bt_ctrl_enter_critical();
    b->link = *head;
    *head = (uint32_t)(uintptr_t)b;
    bt_ctrl_exit_critical();
}

/* Pool 0's blocks (ACL TX), recorded once the ROM has built the pools,
 * while all are free (from controller_init()). */
#define ACL_TX_HEAD       ((uint32_t *)POOL_HEAD(0))
#define ACL_TX_BLKS_MAX   16
static struct pool_blk *acl_tx_blks[ACL_TX_BLKS_MAX];
static unsigned acl_tx_nblks;

static void acl_tx_blks_record(void)
{
    struct pool_blk *b = (struct pool_blk *)(uintptr_t)*ACL_TX_HEAD;
    for (acl_tx_nblks = 0; b && acl_tx_nblks < ACL_TX_BLKS_MAX;
         b = (struct pool_blk *)(uintptr_t)b->link)
        acl_tx_blks[acl_tx_nblks++] = b;
}

/* A link lost by supervision timeout keeps the ACL packets it still had to
 * send: the ROM then stops the link's LD (0x3d3d8) without the flush a
 * disconnect runs first (0x2a8b4 -> 0x302b8), and those buffers never come
 * back to pool 0 (2 per link lost while streaming; with all 9 gone the next
 * links send nothing). Pool 0 holds host ACL data on its way out (the LM
 * copies each packet into one, 0x2dd78), the host stack keeps one link, and
 * it sends nothing on a new link before Connection Complete: a block still
 * taken then, with no other link up, is a lost link's. Put it back.
 * F-0077; only with no link left (F-0203). */
#define ACL_HANDLE(idx)   (0x80u + (idx))
void bt_ctrl_acl_tx_reclaim(uint16_t handle)
{
    unsigned n = 0;
    for (uint32_t i = 0; i < LINK_COUNT; i++)
        if (ACL_HANDLE(i) != handle &&
            bt_ll_data_task(ACL_HANDLE(i)) != BT_LL_NO_TASK)
            return;
    bt_ctrl_enter_critical();
    for (unsigned i = 0; i < acl_tx_nblks; i++) {
        struct pool_blk *b = acl_tx_blks[i];
        if (b->link == (uint32_t)(uintptr_t)ACL_TX_HEAD) {
            b->link = *ACL_TX_HEAD;
            *ACL_TX_HEAD = (uint32_t)(uintptr_t)b;
            n++;
        }
    }
    bt_ctrl_exit_critical();
    if (n)
        dev_log("bt ctrl: %u ACL TX buffers back from a lost link", n);
}

/* A master link lost by supervision timeout keeps its LT_ADDR as well: that
 * disconnect (0x2a8b4) goes straight to the teardown, past 0x3d1c4, the only
 * step that saves the link's LT_ADDR (link env +0x42) for the teardown to
 * free (0x2a3b6). With all 7 gone, starting a page fails 0x0c (0x3618c)
 * after the LM took the page's link table entry and instance and keeps
 * them: pages to that device then fail 0x0b and its pages to us 0x22 until
 * Bluetooth goes off and on. At a Disconnection Complete with no other link
 * left none is in use: free them all (a later free of one is harmless).
 * F-0204; safe only with no link left, so two links stay correct (F-0203). */
void bt_ctrl_lt_addr_reclaim(uint16_t handle)
{
    volatile uint8_t *map = (volatile uint8_t *)LT_ADDR_MAP;
    for (uint32_t i = 0; i < LINK_COUNT; i++)
        if (ACL_HANDLE(i) != handle &&
            bt_ll_data_task(ACL_HANDLE(i)) != BT_LL_NO_TASK)
            return;
    bt_ctrl_enter_critical();
    uint8_t taken = *map & 0xfe;
    *map &= 0x01;
    bt_ctrl_exit_critical();
    if (taken)
        dev_log("bt ctrl: LT_ADDRs %02x back at a disconnect", taken);
}

/* ======================================================================
 * Kernel message dispatch: ke event bit 0 callback. Pops the sent-message
 * list, resolves each message's destination task handler from the ROM
 * descriptor (RivieraWaves ke_state_handler), and calls it. The HCI task
 * (BT_KE_TASK_HCI) handler is ours, registered via a C descriptor. Return
 * values: KE_MSG_CONSUMED (freed here), KE_MSG_NO_FREE (the handler kept or
 * forwarded it), KE_MSG_SAVED (requeued on the saved list). Messages for
 * unknown tasks or without a handler are freed.
 * ====================================================================== */
/* Descriptor structs and ke_msg_func live in bt-yp3.h (shared with bt-hci). */
static ke_msg_func link_patch(ke_msg_func h);
static ke_msg_func ke_handler_search(uint16_t id,
                                     const struct ke_state_handler *sh)
{
    if (!sh || !sh->tab || sh->nb == 0)
        return NULL;
    ke_msg_func dflt = NULL;
    for (unsigned i = 0; i < sh->nb; i++) {
        if (sh->tab[i].id == id)
            return (ke_msg_func)sh->tab[i].func;
        if (sh->tab[i].id == KE_MSG_DEFAULT)
            dflt = (ke_msg_func)sh->tab[i].func;
    }
    return dflt;
}

/* Find a task's descriptor in the ROM task table. */
static const struct ke_task_desc *ke_task_desc(uint16_t dest)
{
    uint8_t count = REG8(KE_ENV);
    uint8_t type = BT_KE_TASK_TYPE(dest);
    for (unsigned i = 0; i < count; i++)
        if (REG8(KE_ENV_TASK_TYPE(i)) == type)
            return *(const struct ke_task_desc **)
                       (uintptr_t)KE_ENV_TASK_DESC(i);
    return NULL;
}

#ifdef YP3_DEV
#define KE_TRACE_BYTES 20u          /* parameter bytes logged per message */
#endif
static void ke_msg_dispatch(void)
{
    for (;;) {
        /* Clear the event with the queue found empty, in one critical
         * section: a message IRQ 26 sends in between sets it again. */
        bt_ctrl_enter_critical();
        struct bt_ke_msg *msg = (struct bt_ke_msg *)
            ROM_CO_LIST_POP((void *)KE_MSG_QUEUE);
        if (!msg)
            REG(KE_EVT_FIELD) &= ~(1u << KE_EVT_MSG);
        bt_ctrl_exit_critical();
        if (!msg)
            break;

        int rc = KE_MSG_CONSUMED;
#ifdef YP3_DEV
        if (ke_trace) {
            char hex[2 * KE_TRACE_BYTES + 1];
            unsigned n = MIN(msg->param_len, KE_TRACE_BYTES);
            for (unsigned i = 0; i < n; i++)
                snprintf(hex + 2 * i, 3, "%02x", msg->param[i]);
            hex[2 * n] = '\0';
            dev_log("ke %04x %04x->%04x len %u %s", msg->id, msg->src_id,
                        msg->dest_id, msg->param_len, hex);
        }
#endif
        const struct ke_task_desc *d = ke_task_desc(msg->dest_id);
        if (d && BT_KE_TASK_IDX(msg->dest_id) < d->idx_max) {
            uint8_t state = d->current_state ?
                            d->current_state[BT_KE_TASK_IDX(msg->dest_id)] : 0;
            const struct ke_state_handler *sh =
                d->state_handler ? &d->state_handler[state] : NULL;
            ke_msg_func h = ke_handler_search(msg->id, sh);
            if (!h)
                h = ke_handler_search(msg->id, d->default_handler);
            if (h)
                rc = link_patch(h)(msg->id, msg->param, msg->dest_id, msg->src_id);
        }
        if (rc == KE_MSG_SAVED) {
            bt_ctrl_enter_critical();
            ROM_CO_LIST_PUSH((void *)KE_MSG_SAVED_LIST, msg);
            bt_ctrl_exit_critical();
        } else if (rc == KE_MSG_CONSUMED) {
            bt_ke_msg_free(msg->param);
        }   /* KE_MSG_NO_FREE: the handler kept or forwarded it */
    }
}

/* Ke event bit 5: no ROM caller of ke_event_set(5) is known, and bt-hci
 * delivers to the host synchronously, so this only clears the bit if it is
 * raised. */
static void ke_hci_tx_done(void)
{
    bt_ctrl_enter_critical();
    REG(KE_EVT_FIELD) &= ~(1u << KE_EVT_HCI_TX);
    bt_ctrl_exit_critical();
}

/* ======================================================================
 * ROM kernel wrappers (bt-yp3.h interface) + host sink.
 * ====================================================================== */
void *bt_ke_msg_alloc(uint16_t id, uint16_t dest, uint16_t src, uint16_t len)
{ return ROM_KE_MSG_ALLOC(id, dest, src, len); }
void bt_ke_msg_send(void *param)       { ROM_KE_MSG_SEND(param); }
void bt_ke_msg_send_basic(uint16_t id, uint16_t dest, uint16_t src)
{ ROM_KE_MSG_SEND_B(id, dest, src); }
/* ROM ke_msg_free (0x30a9e, a tail call to ke_free) takes the header, not the
 * parameters like the other wrappers. */
void bt_ke_msg_free(void *param)
{ ROM_KE_MSG_FREE((uint8_t *)param - sizeof(struct bt_ke_msg)); }
void *bt_ke_acl_payload_alloc(void)    { return ROM_KE_ACL_ALLOC(); }
void bt_ke_task_create(uint32_t type, const struct ke_task_desc *desc)
{ ROM_KE_TASK_CREATE(type, desc); }
void bt_ke_state_set(uint16_t task, uint8_t state)
{ ROM_KE_STATE_SET(task, state); }
/* Link-layer data-path lookups for a connection handle (ROM 0x3075c state,
 * 0x3076c destination link task) used by the ACL TX path. */
uint32_t bt_ll_data_state(uint16_t handle)
{ return ROM_LL_DATA_STATE(handle); }
uint16_t bt_ll_data_task(uint16_t handle)
{ return ROM_LL_DATA_TASK(handle); }
bool bt_ctrl_yp3_running(void)         { return bt_running; }

/* ======================================================================
 * IRQ 26 handler. Read and acknowledge the status, demux the baseband bits
 * to the ROM link controller, and let ke_event_set pend the scheduler. Bits:
 * 0 clock, 1 rx, 2 sleep, 10 timer, 12 wake.
 * ====================================================================== */
#define BT_ST_CLK   (1u << 0)
#define BT_ST_RX    (1u << 1)
#define BT_ST_TIMER (1u << 10)

static void ICODE_ATTR bt_irq_handler(void)
{
    bt_irq_count++;
    uint32_t status = REG(BT_CORE_STATUS);
    REG(BT_CORE_ACK) = status;      /* W1C */
#ifdef YP3_DEV
    for (unsigned b = 0; b < BT_ST_COUNTED; b++)
        if (status & (1u << b))
            bt_irq_bits[b]++;
#endif

    if (status & BT_ST_CLK)
        ROM_LC_CLK_ISR();
    if (status & BT_ST_RX)
        ROM_LC_RX_ISR();
    if (status & BT_ST_TIMER)
        ke_event_set_hook(KE_EVT_TIMER);
    /* Sleep/wake bits are acknowledged above; sleep is not implemented. */

    if (REG(KE_EVT_FIELD))
        ke_sched_pend();
}

/* ======================================================================
 * Hook slots: {slot, value}. A value is one of our functions or a ROM
 * routine (ROM_HOOK, Thumb). The HCI command-interface callbacks are owned
 * by bt-hci (bt_hci_install_cmd_hooks).
 * ====================================================================== */
struct hook { uint32_t slot; uintptr_t fn; };
#define OUR_HOOK(f) ((uintptr_t)(f))
#define ROM_HOOK(a) ((uintptr_t)(a) | 1u)

static void install_hooks(const struct hook *h, size_t n)
{
    for (; n--; h++)
        SLOT(h->slot) = (uint32_t)h->fn;
}
#define INSTALL_HOOKS(tbl) install_hooks(tbl, ARRAYLEN(tbl))

/* Kernel slots and event callbacks. */
static const struct hook kernel_hooks[] = {
    { HOOK_CRIT_ENTER,   OUR_HOOK(bt_ctrl_enter_critical) },
    { HOOK_CRIT_EXIT,    OUR_HOOK(bt_ctrl_exit_critical) },
    { HOOK_RANDOM,       OUR_HOOK(bt_random_hook) },
    { HOOK_POOL_ALLOC,   OUR_HOOK(pool_take) },
    { HOOK_POOL_FREE,    OUR_HOOK(pool_give) },
    { HOOK_POOL1_ALLOC,  ROM_HOOK(ROM_POOL1_ALLOC_IMPL) },
    { HOOK_KE_MSG_ALLOC, ROM_HOOK(ROM_KE_MSG_ALLOC_IMPL) },
    { HOOK_KE_MSG_SEND,  ROM_HOOK(ROM_KE_MSG_SEND_IMPL) },
    { HOOK_KE_MSG_FWD,   ROM_HOOK(ROM_KE_MSG_FWD_IMPL) },
    { HOOK_KE_MALLOC,    OUR_HOOK(ke_malloc_hook) },
    { HOOK_KE_FREE,      OUR_HOOK(ke_free_hook) },
    { HOOK_KE_TIMER_HW,  ROM_HOOK(ROM_KE_TIMER_HW_IMPL) },
    { HOOK_KE_TIMER_SET, ROM_HOOK(ROM_KE_TIMER_SET_IMPL) },
    { HOOK_KE_TIMER_CLR, ROM_HOOK(ROM_KE_TIMER_CLR_IMPL) },
    { HOOK_KE_TIMER_ACT, ROM_HOOK(ROM_KE_TIMER_ACT_IMPL) },
    { KE_EVT_CB(KE_EVT_MSG),    OUR_HOOK(ke_msg_dispatch) },
    { KE_EVT_CB(KE_EVT_TIMER),  ROM_HOOK(ROM_KE_TIMER_EXPIRE) },
    { KE_EVT_CB(KE_EVT_HCI_TX), OUR_HOOK(ke_hci_tx_done) },
    { HOOK_SCHED,        OUR_HOOK(ke_sched_dispatch) },
    { HOOK_EVT_SET,      OUR_HOOK(ke_event_set_hook) },
};

/* Link-layer slots that take ROM routines, by slot address. */
static const struct hook rom_link_hooks[] = {
    { 0x008021c0, ROM_HOOK(0x290c3) }, { 0x008021c4, ROM_HOOK(0x2154d) },
    { 0x008021c8, ROM_HOOK(0x2d7c1) }, { 0x008021cc, ROM_HOOK(0x2aa6d) },
    { 0x008021d0, ROM_HOOK(0x2b701) }, { 0x008021d4, ROM_HOOK(0x29891) },
    { 0x008021d8, ROM_HOOK(0x336e9) }, { 0x008021ec, ROM_HOOK(0x3381d) },
    { 0x008021f0, ROM_HOOK(0x338d9) },
    { 0x00802914, ROM_HOOK(0x2e649) }, { 0x0080291c, ROM_HOOK(0x3a7ed) },
    { 0x00802920, ROM_HOOK(0x2de05) }, { 0x00802924, ROM_HOOK(0x2e041) },
    { 0x00802928, ROM_HOOK(0x2e2a5) }, { 0x0080292c, ROM_HOOK(0x2e4c5) },
    { 0x00802930, ROM_HOOK(0x3a8b9) }, { 0x00802938, ROM_HOOK(0x3ae9d) },
    { 0x0080293c, ROM_HOOK(0x3c119) }, { 0x00802940, ROM_HOOK(0x3a439) },
    { 0x00802944, ROM_HOOK(0x3a781) }, { 0x0080294c, ROM_HOOK(0x3b379) },
    { 0x00802958, ROM_HOOK(0x3daa9) }, { 0x0080295c, ROM_HOOK(0x366e9) },
    { 0x00802960, ROM_HOOK(0x3ab65) },
    { 0x00802bb0, ROM_HOOK(0x36485) }, { 0x00802bb4, ROM_HOOK(0x364ed) },
    { 0x00802bcc, ROM_HOOK(0x34b5d) },
    { 0x008030d0, ROM_HOOK(0x3e791) }, { 0x008030d4, ROM_HOOK(0x3e181) },
    { 0x00803344, ROM_HOOK(0x31979) },
    { 0x00803ba4, ROM_HOOK(0x375b9) }, { 0x00803bac, ROM_HOOK(0x37c05) },
    { 0x00803bb8, ROM_HOOK(0x38bcd) },
};

typedef uint32_t (*rom_fn3)(uint32_t, uint32_t, uint32_t);
#define ROM_CALL(addr, a, b, c) (((rom_fn3)BT_THUMB(addr))((a), (b), (c)))
#define ROM_CALL0(addr)         ROM_CALL(addr, 0, 0, 0)

/* ======================================================================
 * Link-controller hooks (F-0094..F-0110, F-0129). CS(ld) is a link's
 * control structure in EM, LE(i) a link-manager link env, LD the
 * link-driver env. The TX and RX hooks run in IRQ 26.
 * ====================================================================== */
typedef uint32_t (*rom_fn7)(uint32_t, uint32_t, uint32_t, uint32_t,
                            uint32_t, uint32_t, uint32_t);
#define ROM_CALL7(addr, a, b, c, d, e, f, g) \
    (((rom_fn7)BT_THUMB(addr))((a), (b), (c), (d), (e), (f), (g)))

#define CS(ld)              (BT_EM_CS_BASE + (ld) * 0x5cu)
#define LE(i)               (0x00801384u + (i) * LE_BYTES)
#define LE_BYTES            0x208u
#define LD                  0x0080296cu
#define LD_COUNT            10          /* link drivers (ROM link-init loop) */
#define LD_STATE(ld)        (LD + 0x14 + 4 * (ld))  /* u32 */
#define LD_ROLE(ld)         (LD + 0x110 + (ld))     /* 0 master, 1 slave */
#define LD_MARK(ld)         (LD + 0x14f + (ld))
#define LD_STATE_MASK       0x3f00000u
#define LD_STATE_PRE_FHS    0x0400000u
#define LD_STATE_FHS        0x1000000u
#define TX_POWER_DEFAULT    0x00802cf0u /* u8, set-up writes 0x12 */
#define TX_POWER_MAX        0x13
#define TX_POWER_FLOOR      2
#define CS_FORMAT           0x0a
#define CS_AFH              0x0d        /* bit 6: the link hops with AFH */
#define CS_AFH_ON           0x40
#define CS_TX_POWER         0x14
#define CS_TIMING           0x30
/* +0x4d reads 0xff in nearly every packet slot near or far: not a valid
 * RSSI at hook time (F-0215); no valid RSSI location is known yet */
#define CS_RSSI             0x4d
#define PEER_FLOW(ld)       (0x008031f9u + (ld))
#define ACL_TX_ON(ld)       (0x008031f0u + (ld))
#define AFH_COUNT_A         0x008026a8u /* u32[79] good samples */
#define AFH_COUNT_B         0x00802568u
#define AFH_CHANNELS        79
#define AFH_STATE(ld)       (0x00802224u + (ld))
#define FLOW_LIMIT_US       200000u     /* our choice (F-0108) */
#define US_COUNTER          0x40082104u

/* CS TX set-up (F-0097): format bits 7:2, TX power index, timing bits 30:27 */
static inline void cs_tx_setup(uint32_t cs, uint8_t format)
{
    REG8(cs + CS_FORMAT) = (REG8(cs + CS_FORMAT) & 3) | format;
    REG8(cs + CS_TX_POWER) = REG8(TX_POWER_DEFAULT);
    REG(cs + CS_TIMING) = (REG(cs + CS_TIMING) & ~0x78000000u) |
                            0x10000000u;
}
static void ICODE_ATTR cs_tx_40(uint32_t cs) { cs_tx_setup(cs, 0x40); }
static void ICODE_ATTR cs_tx_58(uint32_t cs) { cs_tx_setup(cs, 0x58); }
static void ICODE_ATTR cs_tx_power(uint32_t cs)
{
    REG8(cs + CS_TX_POWER) = REG8(TX_POWER_DEFAULT);
}

/* Role switch in progress (F-0107): set at the switch FHS, used once by
 * the AFH switch hook, dropped at Role Change or Disconnection Complete */
static volatile bool role_switching;
static uint32_t flow_zero_since[LD_COUNT];  /* 0: FLOW byte not 0 */
#ifdef YP3_DEV
static uint32_t flow_stops[LD_COUNT], flow_forced[LD_COUNT];
static uint32_t lmp_tx_count, lmp_tx_full;
static bool lmp_trace;
/* RX status shapes per slot: CS+0x28 bits 14:0. In both roles (F-0207):
 * bits 13:12 clear with bits 11:8 all set is no packet; bits 13:12 set is
 * a packet. Bit 0 follows the role, so it is not the test (F-0175 holds
 * for slave only). Anything else is suspect. HEC/CRC/FEC bits unknown. */
#define CS_RX_STATUS        0x28
#define RX_KIND             (3u << 12 | 0xfu << 8)
#define RX_NONE             (0xfu << 8)
#define RX_PACKET           (3u << 12)
/* a packet with bit 8 set: 3-10x more on a weak link (F-0209); GUESS a
 * failed check, which one is not known */
#define RX_PACKET_FLAG      (1u << 8)
static uint32_t rx_flagged[LD_COUNT];
static uint32_t rx_packets[LD_COUNT], rx_none[LD_COUNT], rx_suspect[LD_COUNT];
static uint16_t rx_suspect_last[LD_COUNT];
/* dev: the first few distinct suspect shapes, all links */
#define RX_SHAPES 8
static uint16_t rx_shape[RX_SHAPES];
static uint32_t rx_shape_n[RX_SHAPES];
#endif

/* RX slot of a link (F-0098, F-0108) */
static void ICODE_ATTR rx_slot_hook(uint32_t cs, uint32_t ld)
{
    (void)cs;
    if (ld >= LD_COUNT)
        return;
#ifdef YP3_DEV
    uint16_t st = REG16(cs + CS_RX_STATUS) & 0x7fff;
    if ((st & RX_KIND) == RX_NONE)
        rx_none[ld]++;
    else if ((st & RX_PACKET) == RX_PACKET) {
        rx_packets[ld]++;
        if (st & RX_PACKET_FLAG)
            rx_flagged[ld]++;
    }
    else {
        rx_suspect[ld]++;
        rx_suspect_last[ld] = st;
        for (int i = 0; i < RX_SHAPES; i++) {
            if (rx_shape_n[i] == 0)
                rx_shape[i] = st;
            if (rx_shape[i] == st) {
                rx_shape_n[i]++;
                break;
            }
        }
    }
#endif
    if (REG8(LD_ROLE(ld)) == 0) {
        REG8(LD_MARK(ld)) = 2;
        ROM_CALL(0x3bd28, ld, 2, 1);
    }
    /* A STOP from a peer that sends few payloads would hold the link
     * silent: release it after FLOW_LIMIT_US; baseband ARQ still holds
     * back what the peer does not take. */
    uint32_t now = REG(US_COUNTER) | 1;
    if (REG8(PEER_FLOW(ld))) {
        flow_zero_since[ld] = 0;
    } else if (!flow_zero_since[ld]) {
        flow_zero_since[ld] = now;
#ifdef YP3_DEV
        flow_stops[ld]++;
#endif
    } else if (now - flow_zero_since[ld] > FLOW_LIMIT_US) {
        REG8(PEER_FLOW(ld)) = 1;
        flow_zero_since[ld] = 0;
#ifdef YP3_DEV
        flow_forced[ld]++;
#endif
    }
}

static void ICODE_ATTR hook_nop(void) { }
static uint32_t hook_zero(void) { return 0; }

/* AFH assessment, per received packet (F-0101). No rule for a good
 * sample is known and RSSI is stale here (F-0099): count every sample as
 * good, so the map keeps all 79 channels, as measured on the earlier
 * driver and stock. */
static void ICODE_ATTR afh_assess(uint32_t unused, uint32_t channel)
{
    (void)unused;
    if (channel < AFH_CHANNELS) {
        REG(AFH_COUNT_A + 4 * channel)++;
        REG(AFH_COUNT_B + 4 * channel)++;
    }
}

/* AFH switch at the instant (F-0101, F-0107) */
static void afh_switch(uint32_t ld, uint32_t b, uint32_t c, uint32_t d)
{
    ((void (*)(uint32_t, uint32_t, uint32_t, uint32_t))BT_THUMB(0x39e00))
        (ld, b, c, d);
    if (role_switching && ld < LD_COUNT) {
        REG8(CS(ld) + CS_AFH) &= ~CS_AFH_ON;
        role_switching = false;
    }
}

/* Per-link scheduling (F-0102): ROM 0x3c778, watching for the switch FHS */
static void ICODE_ATTR link_sched(uint32_t a, uint32_t ld)
{
    uint32_t before = ld < LD_COUNT ? REG(LD_STATE(ld)) & LD_STATE_MASK : 0;
    ROM_CALL(0x3c778, a, ld, 0);
    if (before == LD_STATE_PRE_FHS &&
        (REG(LD_STATE(ld)) & LD_STATE_MASK) == LD_STATE_FHS)
        role_switching = true;
}

void bt_ctrl_role_switch_end(void)
{
    role_switching = false;
}

/* 0x803ba0 (F-0095): ROM 0x374dc with the stack arguments as bytes */
static uint32_t fwd_374dc(uint32_t a, uint32_t b, uint32_t c, uint32_t d,
                          uint32_t e, uint32_t f, uint32_t g)
{
    ROM_CALL7(0x374dc, a, b, c, d, (uint8_t)e, (uint8_t)f, (uint8_t)g);
    return 0;
}

/* LMP connection set-up complete (F-0103). Local feature bytes from the
 * table the controller reports (F-0063, F-0135). */
static const uint8_t local_features[8];
#define LMP_FEAT_POWER_CTRL 18
static void lmp_setup_done(uint32_t task)
{
    uint32_t le = LE(BT_KE_TASK_IDX(task));
    uint32_t ld = REG8(le + 0x41);
    const uint32_t remote = le + 0x6c;

    if (!REG8(le + 0x55)) {
        ROM_CALL(0x29f82, task, 0, 0);      /* HCI Connection Complete */
        REG8(le + 0x55) = 1;
        ROM_CALL(0x36550, ld, 0, 0);
    }
    REG8(le + 0x52) = 1;
    uint16_t types = REG16(le + 0x22);
    /* F-0135: local and remote bytes interleaved; the OR of both results */
    REG16(le + 0x22) =
        ROM_CALL(0x311cc, types, local_features[0], REG8(remote)) |
        ROM_CALL7(0x31214, types, local_features[3], REG8(remote + 3),
                  local_features[4], REG8(remote + 4),
                  local_features[5], REG8(remote + 5));
    ROM_CALL(0x3a774, ld, 0, 0);
    bt_ke_msg_send_basic(0xe485, task, task);
    bool power = REG8(remote + LMP_FEAT_POWER_CTRL / 8) &
                 (1u << (LMP_FEAT_POWER_CTRL % 8));
    ROM_CALL(0x36804, ld, !power, !power);
    REG8(le + 0x204) = 0;
    bt_ke_msg_send_basic(0xe4fa, task, task);
    ROM_CALL(ROM_KE_STATE_SET_ADDR, task, 5, 0);
    bt_ke_msg_send_basic(0xe4ee, task, task);
}

/* RF API (F-0099, F-0100) */
static uint32_t tx_power_up(uint32_t ld)
{
    uint32_t a = CS(ld) + CS_TX_POWER, v = REG8(a);
    if (v >= TX_POWER_MAX)
        return 1;
    v = ((v + 1) & 1 || v + 1 == TX_POWER_MAX) ? v + 1 : v + 2;
    REG8(a) = MIN(v, TX_POWER_MAX);
    return 0;
}
static uint32_t tx_power_down(uint32_t ld)
{
    uint32_t a = CS(ld) + CS_TX_POWER, v = REG8(a);
    if (v <= TX_POWER_FLOOR)
        return 1;
    REG8(a) = (v & 1) ? v - 2 : v - 1;
    return 0;
}
#define POWER_RES_STEP      0x15
#define POWER_RES_MAX       0x2a
#define POWER_RES_MIN       0x3f
static uint32_t tx_power_request(uint32_t ld, uint32_t mode)
{
    switch (mode) {
    case 0: return tx_power_down(ld) ? POWER_RES_MIN : POWER_RES_STEP;
    case 1: return tx_power_up(ld) ? POWER_RES_MAX : POWER_RES_STEP;
    case 2:
        REG8(CS(ld) + CS_TX_POWER) = TX_POWER_MAX;
        return POWER_RES_MAX;
    }
    return 0;
}
static int8_t rssi_dbm(uint8_t raw)
{
    return (int8_t)((((raw >> 3) * 2) - 64) & 0xfe);
}
static uint32_t rssi_convert(uint32_t raw) { return (uint8_t)rssi_dbm(raw); }

#define RF_API_OFFSET       0x00803c12u /* u16 slot offset */
#define RF_API_OFFSET_VALUE 0x14

/* LMP TX (slot 0x80295c): ROM 0x366e8, counted and traced in dev builds */
static uint32_t lmp_tx(uint32_t ld, uint32_t pdu, uint32_t len, uint32_t ext)
{
    uint32_t r = ((uint32_t (*)(uint32_t, uint32_t, uint32_t, uint32_t))
                      BT_THUMB(0x366e8))(ld, pdu, len, ext);
#ifdef YP3_DEV
    lmp_tx_count++;
    if (r == 7)
        lmp_tx_full++;
    if (lmp_trace) {
        const uint8_t *p = (const uint8_t *)pdu;
        dev_log("lmp tx ld %lu op %u%s%02x len %lu%s", (unsigned long)ld,
                p[0] >> 1, p[0] >> 1 == 127 ? " ext " : " ",
                p[0] >> 1 == 127 ? p[1] : 0, (unsigned long)len,
                r == 7 ? " DROPPED" : "");
    }
#endif
    return r;
}

static const struct hook board_link_hooks[] = {
    { 0x00802bb8, OUR_HOOK(cs_tx_40) },   { 0x00802bbc, OUR_HOOK(cs_tx_40) },
    { 0x00802bc0, OUR_HOOK(cs_tx_58) },   { 0x00802bc8, OUR_HOOK(cs_tx_58) },
    { 0x00802bc4, OUR_HOOK(cs_tx_power) },
    { 0x00802904, OUR_HOOK(rx_slot_hook) },
    { 0x00802908, OUR_HOOK(hook_nop) },   { 0x0080290c, OUR_HOOK(hook_nop) },
    { 0x00802910, OUR_HOOK(afh_assess) },
    { 0x00802918, OUR_HOOK(afh_switch) },
    { 0x00802934, OUR_HOOK(link_sched) },
    { 0x00802948, OUR_HOOK(hook_nop) },   { 0x008030cc, OUR_HOOK(hook_nop) },
    { 0x00802950, OUR_HOOK(hook_nop) },   { 0x00802954, OUR_HOOK(hook_nop) },
    { 0x008021bc, OUR_HOOK(lmp_setup_done) },
    { 0x00803ba0, OUR_HOOK(fwd_374dc) },
    { 0x0080295c, OUR_HOOK(lmp_tx) },
    /* RF API block 0x803bdc + 4n */
    { 0x00803bdc, OUR_HOOK(hook_nop) },   { 0x00803be0, OUR_HOOK(hook_nop) },
    { 0x00803be4, OUR_HOOK(tx_power_up) },
    { 0x00803be8, OUR_HOOK(tx_power_down) },
    { 0x00803bec, OUR_HOOK(tx_power_request) },
    { 0x00803bf0, OUR_HOOK(hook_zero) },  { 0x00803bf4, OUR_HOOK(hook_zero) },
    { 0x00803bf8, OUR_HOOK(rssi_convert) },
    { 0x00803bfc, OUR_HOOK(hook_nop) },   { 0x00803c00, OUR_HOOK(hook_zero) },
    { 0x00803c04, OUR_HOOK(hook_nop) },   { 0x00803c08, OUR_HOOK(hook_nop) },
};

/* ---- Message-handler patches (F-0104..F-0106): the dispatcher runs ours
 * in place of a ROM handler it resolved ---- */
#define LM_STATE(task)      ((uint8_t)ROM_CALL(0x309b0, (task), 0, 0))
#define ROM_AUTH_REQ        0x201c5
#define ROM_PREFERRED_RATE  0x25439
#define ROM_ACL_DRAINED     0x2290d
#define ROM_HANDLER(a)      ((ke_msg_func)(uintptr_t)(a))

/* Authentication started in state 0x59 drops the EDR-table answers: wait */
static int patch_auth_req(uint16_t id, void *p, uint16_t dest, uint16_t src)
{
    if (LM_STATE(dest) == 0x59)
        return KE_MSG_SAVED;
    return ROM_HANDLER(ROM_AUTH_REQ)(id, p, dest, src);
}

/* Refuse FEC (DM packets): a board choice, measured better range */
static int patch_preferred_rate(uint16_t id, void *p, uint16_t dest,
                                uint16_t src)
{
    ((uint8_t *)p)[2] |= 1;
    return ROM_HANDLER(ROM_PREFERRED_RATE)(id, p, dest, src);
}

/* As slave in state 0x8a, pause encryption in the master's transaction */
static int patch_acl_drained(uint16_t id, void *p, uint16_t dest,
                             uint16_t src)
{
    unsigned i = BT_KE_TASK_IDX(dest);
    if (LM_STATE(dest) == 0x8a && REG8(LE(i) + 0x3d) == 1) {
        REG8(LE(i) + 0x41) = ((uint8_t *)p)[0];
        ROM_CALL(0x2ab44, dest, 0, 0);
        ROM_CALL(0x215d0, i, 0, 0);
        ROM_CALL(0x3092c, dest, 0x39, 0);
        return KE_MSG_CONSUMED;
    }
    return ROM_HANDLER(ROM_ACL_DRAINED)(id, p, dest, src);
}

static ke_msg_func link_patch(ke_msg_func h)
{
    if (h == ROM_HANDLER(ROM_AUTH_REQ))
        return patch_auth_req;
    if (h == ROM_HANDLER(ROM_PREFERRED_RATE))
        return patch_preferred_rate;
    if (h == ROM_HANDLER(ROM_ACL_DRAINED))
        return patch_acl_drained;
    return h;
}

static void link_hooks_install(void)
{
    INSTALL_HOOKS(board_link_hooks);
    REG8(0x00803c0d) = REG8(0x00803c0e) = REG8(0x00803c0f) = 0;
    REG16(RF_API_OFFSET) = RF_API_OFFSET_VALUE;
    role_switching = false;
    memset(flow_zero_since, 0, sizeof flow_zero_since);
}

/* ======================================================================
 * BD_ADDR: the "bt_addr" record of the PSMP settings store, read with
 * bt_yp3_psmp_read() (bt-rf-yp3.c; CRC checked) and programmed with ROM
 * 0x3ba30. For controller_init().
 * ====================================================================== */
#define BDADDR_LEN 6

static void program_bdaddr(void)
{
    uint8_t a[BDADDR_LEN];
    if (bt_yp3_psmp_read("bt_addr", a, sizeof a) != BDADDR_LEN) {
        dev_log("bt ctrl: no bt_addr in PSMP");
        return;
    }
    struct { uint32_t lo; uint16_t hi; } bd;
    bd.lo = a[0] | (a[1] << 8) | (a[2] << 16) | ((uint32_t)a[3] << 24);
    bd.hi = a[4] | (a[5] << 8);
    ROM_SET_BDADDR(&bd);
}

/* Set the EM header halfwords to EM_HDR_FILL. */
static void em_header_fill(void)
{
    for (uint32_t a = BT_EM_BASE; a < BT_EM_BASE + EM_HDR_BYTES; a += 2)
        REG16(a) = EM_HDR_FILL;
}

/* ======================================================================
 * Controller set-up (F-0058..F-0065, F-0070, F-0121). ROM BSS values are
 * labelled, roles mostly unknown; the order is the one shown to work
 * (GUESS beyond the dependencies the facts state: zeroing before group A
 * and before pool init).
 * ====================================================================== */

#define ROM_KE_INIT        0x30da4
#define ROM_LT_ADDR_FREE   0x30656  /* all 7 link-address entries free */
#define ROM_POOL_INIT      0x3052a
#define ROM_LINK_INIT      0x3a42c  /* per-link loop: needed (F-0121) */
#define ROM_MEM_ALLOC      0x3062c
#define ROM_FREE           0x3deb0
#define KE_TIMER_QUEUE     0x008041c4
#define KE_ENV_BYTES       0x4c
#define DESC_LM            0x008005f8
#define DESC_LC            0x00800660
#define DESC_LINK          0x00800608
#define LM_TX_PTRS         0x008021dcu
#define LM_TX_PTRS_END     0x008021ecu
#define EM_WIN_BYTES       0xf0u
#define EM_WIN_VALUE       0x014313a8u
#define CORE_CTRL_CLEAR    0x01407000u
#define CORE_CTRL_SET      0x00200107u
#define CORE_INTCNTL_INIT  0xffff0003u
#define CORE_1E4_INIT      0x1590u

#define W8(a, v)  (REG8(a) = (uint8_t)(v))
#define W16(a, v) (REG16(a) = (uint16_t)(v))
#define W32(a, v) (REG(a) = (uint32_t)(v))

static void zero(uint32_t addr, uint32_t len)
{
    memset((void *)(uintptr_t)addr, 0, len);
}

/* F-0059: configuration bytes. */
static const struct { uint32_t a; uint8_t v; } config_bytes[] = {
    {0x802b46, 0x1c}, {0x802b44, 100}, {0x802964, 0x32}, {0x803c17, 6},
    {0x803c16, 0x27}, {0x803c15, 0x0f}, {0x803c14, 0}, {0x802b42, 0x1e},
    {0x802969, 3}, {0x802966, 7}, {0x802b41, 8}, {0x802b40, 9},
    {0x802b3f, 10}, {0x802b3e, 0x0b}, {0x802b3d, 0x0c}, {0x802b3c, 0x0d},
    {0x802b3b, 0x0e}, {0x802967, 0x0f}, {0x802b3a, 0x10}, {0x802b39, 0x11},
    {0x802b38, 0x12}, {0x802b37, 0x13}, {0x80296a, 0x1c}, {0x802b36, 1},
    {0x802968, 2}, {0x80296b, 3}, {0x80422e, 0x1e}, {0x80420b, 0x0e},
    {0x80420a, 2}, {0x80422d, 0x12}, {0x80422c, 2}, {0x80422b, 0x16},
    {0x80422a, 3}, {0x804223, 0x16}, {0x804222, 3}, {0x804209, 10},
    {0x804208, 2},
};

/* F-0070: ranges zeroed before the set-up. */
static const struct { uint32_t a, len; } zero_ranges[] = {
    {0x80296c, 0x1c4}, {0x802c04, 0x28}, {0x802cf4, 0x374}, {0x8030d8, 0x148},
    {0x803220, 0x6c}, {0x802b48, 0x68}, {0x802208, 0x614}, {0x802c2c, 0xc0},
    {0x80281c, 0xe8}, {0x802bd8, 0x12},
};

/* F-0063: local LMP features (EDR set, with 0x802b34 = 1) and the
 * supported-command filter tables. */
static const uint8_t local_features[8] = {
    0x7f, 0xfe, 0xcd, 0xfa, 0xdb, 0xfd, 0x7b, 0x87,
};
#define FILTER_A      0x00802cf4u
#define FILTER_B      0x00802d3au
#define FILTER_BYTES  0x46u
static const uint8_t filter_a[] = {
    0x1e, 0, 0x3c, 0, 0x1d, 0, 0x3c, 0, 0, 1,
};
static const uint8_t filter_b[] = {
    0x1e, 0, 0x3c, 0, 0x05, 0, 0x3c, 0, 0, 1,
    0x1e, 0, 0x3c, 0, 0x05, 0, 0x3c, 0, 0, 2,
    0x14, 0, 0x3c, 0, 0x05, 0, 0x3c, 0, 1, 2,
};

static void filter_fill(uint32_t addr, const uint8_t *v, size_t n)
{
    memset((void *)(uintptr_t)addr, 0xff, FILTER_BYTES);
    memcpy((void *)(uintptr_t)addr, v, n);
}

/* F-0061 group A (RF API and EDR config; after the zeroing). */
static void group_a(void)
{
    W8(0x802c1f, 0xfa);
    W8(0x802c20, 10);
    W16(0x802b04, REG8(0x803c0d) << 7);
    W16(0x802b06, REG8(0x803c0e) << 7);
    W8(0x802b08, REG8(0x803c0f));
    W8(0x802b34, 1);
    W16(RF_API_OFFSET, RF_API_OFFSET_VALUE);
}

static void group_b(void)
{
    W16(0x802b88, 0x2000);
    W8(0x802b96, 0x38);
    W8(0x802b2e, 0); W8(0x802b95, 0); W8(0x802baf, 0); W8(0x802b91, 0);
    W8(0x802bad, 1);
    W8(0x802bae, 0);
    W8(0x802c1d, 1);
    W8(0x802bd1, 1);
}

static void group_c(void)
{
    W8(0x802b03, 0);
    W8(0x803c0c, 0x12);
    W8(0x802cf0, 0x12);
    W8(0x802b09, 0);
    W8(0x802bd8, 0);
    W32(0x802b10, 0x1b);
    W32(0x802b14, 0x070004ff);
    W8(0x802b0a, 0);
    W32(0x802c24, 0);
    W8(0x8031eb, 1);
    W8(0x802c1e, 0);
    W8(LT_ADDR_MAP, 0);
    W32(0x802c0c, 0);
    W32(0x802c10, 0);
}

/* F-0061: link-controller block (cold start and HCI_Reset). The ROM init
 * calls here are unneeded for an inquiry (F-0121) and for a connect plus a
 * 30 s A2DP stream (F-0173); kept, since encryption, sniff, role switch,
 * HID and long runs were not tested without them. */
static void link_controller_block(void)
{
    ROM_CALL0(ROM_LT_ADDR_FREE);
    zero(0x8038f4, 0x220);
    zero(0x8032f0, 0x54);
    W8(0x803b0c, 1);
    W8(0x803b0e, 1);
    W16(0x8032f6, 0x1fa0);
    W16(0x8032f8, 0);
    W8(0x803340, 5);
    W8(0x8032f4, 0);
    zero(0x803911, 0xf8);
    W8(0x803910, 0);
    W8(0x8032f0, 0);
    W8(0x8032f2, 7);
    W8(0x8032f3, 0x10);
    W8(0x8032f1, 0);
    W16(0x8038fe, 0x80);
    W16(0x803906, 0x80);
    W8(0x803b0a, 0);
    W16(0x803900, 0x400);
    W16(0x803908, 0x400);
    W8(0x803b11, 0);
    W8(0x803b0d, 0);
    W32(0x8038f8, ROM_CALL(ROM_MEM_ALLOC, 0x400, 0, 0));
    W32(0x8038f4, ROM_CALL(ROM_MEM_ALLOC, 0x400, 0, 0));
    W16(0x803b02, 0x8b33);
    W8(0x803b04, 0x9e);
    ROM_CALL(0x31464, 0x8032fa, 0, 0);
    ROM_CALL0(0x38d18);
    ROM_CALL0(0x328f8);
    ROM_CALL0(0x31188);
    ROM_CALL0(0x31de4);
}

/* F-0064 steps (3)..(13): run at the cold start and again by HCI_Reset
 * (F-0065), which rebuilds the pools. */
static void controller_setup(void)
{
    /* (3) */
    ROM_KE_TASK_CREATE(BT_KE_TASK_LD, (const void *)DESC_LINK);
    program_bdaddr();
    group_a();
    /* (4) */
    em_header_fill();
    /* (5) */
    ROM_CALL0(ROM_POOL_INIT);
    acl_tx_blks_record();
    /* (6) */
    ROM_CALL0(0x3d23c);     /* needed: without it the peer drops the link (F-0173) */
    for (uint32_t i = 0; i < 10; i++)
        ROM_CALL(ROM_LINK_INIT, i, 0x802c14, 0x802beb);
    /* (7) */
    group_b();
    /* (8) */
    zero(BT_EM_WIN_AREA, EM_WIN_BYTES);
    REG(BT_CORE_EM_WIN) = EM_WIN_VALUE;
    /* (9) */
    group_c();
    /* (10) */
    ROM_CALL0(0x3dfcc);
    ROM_CALL0(0x39554);     /* needed: pages never reach the peer without it (F-0173) */
    /* (11) */
    filter_fill(FILTER_A, filter_a, sizeof filter_a);
    filter_fill(FILTER_B, filter_b, sizeof filter_b);
    /* (12) */
    memset((void *)0x802af3, 0xff, 0x10);
    memcpy((void *)0x802c04, local_features, sizeof local_features);
    /* (13) */
    REG(BT_CORE_1E4) = CORE_1E4_INIT;
    REG(BT_CORE_INTCNTL) = CORE_INTCNTL_INIT;
    REG(BT_CORE_1B0) = REG8(0x802b42) | 0x8000u;
    REG(BT_CORE_118) |= 1u << 3;
    REG(BT_CORE_CTRL) = (REG(BT_CORE_CTRL) & ~CORE_CTRL_CLEAR) |
                          CORE_CTRL_SET;
}

/* Kernel init (F-0058 step 3), then the set-up in the critical section. */
static void controller_init(void)
{
    ROM_CALL0(ROM_KE_INIT);
    /* Kernel init writes some slots: the queues, task table and hooks go
     * in after it. */
    zero(KE_MSG_QUEUE, 8);
    zero(KE_MSG_SAVED_LIST, 8);
    zero(KE_TIMER_QUEUE, 8);
    zero(KE_ENV, KE_ENV_BYTES);
    INSTALL_HOOKS(kernel_hooks);

    bt_ctrl_enter_critical();
    /* (1) */
    ROM_CALL0(ROM_LT_ADDR_FREE);
    W16(RF_API_OFFSET, RF_API_OFFSET_VALUE);
    bt_hci_task_desc_init();
    /* (2) */
    for (unsigned i = 0; i < ARRAYLEN(zero_ranges); i++)
        zero(zero_ranges[i].a, zero_ranges[i].len);
    for (unsigned i = 0; i < ARRAYLEN(config_bytes); i++)
        W8(config_bytes[i].a, config_bytes[i].v);
    controller_setup();
    /* (14) link manager */
    ROM_KE_TASK_CREATE(BT_KE_TASK_LM, (const void *)DESC_LM);
    for (unsigned i = 0; i < LINK_COUNT; i++) {
        ROM_KE_STATE_SET(BT_KE_TASK_ID(BT_KE_TASK_LM, i), 0);
        zero(LE(i), LE_BYTES);
        ROM_CALL(0x2d960, 1, 2, 0);
    }
    zero(LM_TX_PTRS, LM_TX_PTRS_END - LM_TX_PTRS);
    W16(0x8021f4, 0);
    /* (15) link controller */
    link_controller_block();
    ROM_KE_TASK_CREATE(BT_KE_TASK_LC, (const void *)DESC_LC);
    ROM_KE_STATE_SET(BT_KE_TASK_LC, 0);
    bt_ctrl_exit_critical();
}

/* Disable the core, master-reset it and re-fill the EM header, for a reset
 * of a running controller (controller_reset()). This is bit 31, not the
 * cold-start bit 29: bit 29 returns every core register to its reset value,
 * and only the cold start reprograms RADIOPWRUPDN and BT_CORE_2C afterwards. */
static void em_core_reset(void)
{
    REG(BT_CORE_CTRL) &= ~BT_CORE_CTRL_EN;
    REG(BT_CORE_CTRL) |= BT_CORE_CTRL_MASTER_RST;
    for (unsigned i = 0; i < BT_CORE_RESET_SPINS &&
                         (REG(BT_CORE_CTRL) & BT_CORE_CTRL_MASTER_RST); i++)
        ;
    em_header_fill();
    REG(BT_CORE_INTCNTL) = 0;
    REG(BT_CORE_ACK) = BT_CORE_ACK_ALL;
}

/* HCI_Reset (F-0065): flush the kernel, reset the HCI glue, master-reset
 * the core, rerun the set-up from step (3), then the link manager and link
 * controller back to their start state. */
static void controller_reset(void)
{
    bt_ctrl_enter_critical();
    ROM_KE_FLUSH();
    bt_hci_task_desc_init();
    em_core_reset();
    controller_setup();
    for (unsigned i = 0; i < LINK_COUNT; i++) {
        ROM_KE_STATE_SET(BT_KE_TASK_ID(BT_KE_TASK_LM, i), 0);
        zero(LE(i), LE_BYTES);
    }
    W8(0x802200, 0);
    for (uint32_t a = LM_TX_PTRS; a <= 0x8021e8; a += 4)
        if (REG(a)) {
            ROM_CALL(ROM_FREE, REG(a), 0, 0);
            REG(a) = 0;
        }
    W16(0x8021f4, 0);
    link_controller_block();
    bt_ctrl_exit_critical();
}

bool bt_ctrl_yp3_soft_reset(void)
{
    if (!bt_running)
        return false;
    controller_reset();
    return true;
}

/* ======================================================================
 * Start / stop.
 * ====================================================================== */
bool bt_ctrl_yp3_start(void)
{
    if (bt_running)
        return true;

    bt_irq_count = 0;
    bt_sched_count = 0;
#ifdef YP3_DEV
    memset(bt_irq_bits, 0, sizeof bt_irq_bits);
#endif
    bt_crit_nest = 0;

    /* The controller core and exchange memory are on the RF clock domain
     * (gate 34, selector 0x1d); touching them with the clocks off stalls the
     * bus. Turn them on first (RF calibration is separate, in bt_rf_yp3_init
     * / bt_drv_open) and fail cleanly if a gate does not acknowledge. */
    if (!bt_yp3_clocks_on()) {
        dev_log("bt ctrl: BT clocks did not come up");
        return false;
    }
    if (!ke_arena_open()) {
        dev_log("bt ctrl: no RAM for the kernel arena");
        return false;
    }

    /* Zero exchange memory, request the register reset, poll it clear. */
    for (uint32_t a = BT_EM_BASE; a < BT_EM_END; a += 4)
        REG(a) = 0;
    REG(BT_CORE_CTRL) |= BT_CORE_CTRL_SOFTRST;
    unsigned spin = 0;
    while ((REG(BT_CORE_CTRL) & BT_CORE_CTRL_SOFTRST) &&
           ++spin < BT_CORE_RESET_SPINS)
        ;
    if (spin >= BT_CORE_RESET_SPINS) {
        dev_log("bt ctrl: soft reset did not clear (clocks? bus?)");
        ke_arena_close();
        return false;           /* the caller gates the clocks (bt_drv_open) */
    }

    INSTALL_HOOKS(kernel_hooks);
    INSTALL_HOOKS(rom_link_hooks);
    bt_hci_install_cmd_hooks();
    link_hooks_install();
    controller_init();

    /* Radio power-up/down timing and register 0x2c. */
    {
        uint32_t v = REG(BT_CORE_RADIOPWRUPDN);
        v = (v & ~RTRIP_DELAY_MASK) | RTRIP_DELAY_INIT;
        v = (v & ~RXPWRUP_MASK) | RXPWRUP_INIT;
        v = (v & ~TXPWRDN_MASK) | TXPWRDN_INIT;
        v = (v & ~TXPWRUP_MASK) | TXPWRUP_INIT;
        REG(BT_CORE_RADIOPWRUPDN) = v;
        REG(BT_CORE_2C) = BT_CORE_2C_INIT;
    }

    yp3_irq_install(BT_SCHED_IRQ, bt_sched_irq);
    yp3_irq_priority(BT_SCHED_IRQ, BT_SCHED_PRIO);
    yp3_irq_clear(BT_SCHED_IRQ);
    yp3_irq_enable(BT_SCHED_IRQ);

    yp3_irq_install(BT_IRQ, bt_irq_handler);
    yp3_irq_priority(BT_IRQ, BT_IRQ_PRIO);
    yp3_irq_clear(BT_IRQ);
    yp3_irq_enable(BT_IRQ);

    bt_running = true;
    dev_log("bt ctrl: up (sched irq%u, irq26 prio%u)", BT_SCHED_IRQ,
                BT_IRQ_PRIO);
    return true;
}

void bt_ctrl_yp3_stop(void)
{
    if (!bt_running)
        return;
    yp3_irq_disable(BT_IRQ);
    yp3_irq_disable(BT_SCHED_IRQ);
    bt_running = false;
    /* Both NVIC lines are masked, so the kernel is quiescent. Flush any
     * pending messages/timers back to the arena (ROM 0x30ab0) before
     * releasing it. */
    bt_ctrl_enter_critical();
    ROM_KE_FLUSH();
    bt_ctrl_exit_critical();
    ke_arena_close();
    dev_log("bt ctrl: down");
}

#ifdef YP3_DEV
void bt_ctrl_yp3_dev_status(struct bt_ctrl_dev_status *out)
{
    out->core_status = REG(BT_CORE_STATUS);
    out->core_ctrl   = REG(BT_CORE_CTRL);
    out->ke_events   = REG(KE_EVT_FIELD);
    out->irq_count   = bt_irq_count;
    out->sched_count = bt_sched_count;
    out->cmd_credits = bt_hci_yp3_cmd_credits();
    out->running     = bt_running;
}

/* "bt ctrl link": per in-use link driver, RSSI, TX power, AFH and flow
 * (F-0110); "bt ctrl lmp on|off": the LMP TX trace. CRC/HEC/FEC error
 * counters are not located (F-0110). */
static bool link_dev_cmd(char *arg)
{
    if (!strncmp(arg, "lmp ", 4)) {
        lmp_trace = !strcmp(arg + 4, "on");
        dev_log("bt ctrl lmp trace %s", lmp_trace ? "on" : "off");
        return true;
    }
    if (strcmp(arg, "link"))
        return false;
    unsigned n = 0;
    for (unsigned ld = 0; ld < LINK_COUNT; ld++) {
        uint8_t role = REG8(LD_ROLE(ld));
        if (role == 0xff)
            continue;
        uint32_t cs = CS(ld);
        n++;
        dev_log("bt link %u: %s rssi? %d dBm (raw %02x) txpwr %u afh %s "
                "state %u", ld, role ? "slave" : "master",
                rssi_dbm(REG8(cs + CS_RSSI)), REG8(cs + CS_RSSI),
                REG8(cs + CS_TX_POWER),
                REG8(cs + CS_AFH) & CS_AFH_ON ? "on" : "off",
                REG8(AFH_STATE(ld)));
        dev_log("  flow %u acl_tx %u queue %08lx stops %lu forced %lu",
                REG8(PEER_FLOW(ld)), REG8(ACL_TX_ON(ld)),
                (unsigned long)REG(0x00803038u + 4 * ld),
                (unsigned long)flow_stops[ld],
                (unsigned long)flow_forced[ld]);
        dev_log("  rx packets %lu (bit 8: %lu) none %lu suspect %lu (last %04x)",
                (unsigned long)rx_packets[ld], (unsigned long)rx_flagged[ld],
                (unsigned long)rx_none[ld],
                (unsigned long)rx_suspect[ld], rx_suspect_last[ld]);
    }
    for (int i = 0; i < RX_SHAPES && rx_shape_n[i]; i++)
        dev_log("  suspect %04x: %lu", rx_shape[i],
                (unsigned long)rx_shape_n[i]);
    dev_log("bt link: %u up; lmp tx %lu (%lu dropped full); role switch %d",
            n, (unsigned long)lmp_tx_count, (unsigned long)lmp_tx_full,
            role_switching);
    return true;
}

/* "bt ctrl start | stop | state | trace on|off", then link_dev_cmd(). */
bool bt_ctrl_yp3_dev(char *arg)
{
    if (!strncmp(arg, "trace ", 6)) {
        ke_trace = !strcmp(arg + 6, "on");
        dev_log("bt ctrl trace %s", ke_trace ? "on" : "off");
    } else if (!strcmp(arg, "start")) {
        bool ok = bt_ctrl_yp3_start();
        dev_log("bt ctrl start: %s", ok ? "up" : "FAILED");
    } else if (!strcmp(arg, "stop")) {
        bt_ctrl_yp3_stop();
    } else if (!strcmp(arg, "state")) {
        struct bt_ctrl_dev_status s;
        bt_ctrl_yp3_dev_status(&s);
        dev_log("bt ctrl state: run=%d status=%08lx ctrl=%08lx",
                    s.running, (unsigned long)s.core_status,
                    (unsigned long)s.core_ctrl);
        dev_log("  ke_evt=%08lx irq26=%lu sched=%lu cmd_credits=%u",
                    (unsigned long)s.ke_events, (unsigned long)s.irq_count,
                    (unsigned long)s.sched_count, s.cmd_credits);
        dev_log("  irq bits 0:%lu 1:%lu 2:%lu 3:%lu 4:%lu 5:%lu 6:%lu 7:%lu "
                    "8:%lu 9:%lu 10:%lu 11:%lu 12:%lu 13:%lu 14:%lu 15:%lu",
                    bt_irq_bits[0], bt_irq_bits[1], bt_irq_bits[2],
                    bt_irq_bits[3], bt_irq_bits[4], bt_irq_bits[5],
                    bt_irq_bits[6], bt_irq_bits[7], bt_irq_bits[8],
                    bt_irq_bits[9], bt_irq_bits[10], bt_irq_bits[11],
                    bt_irq_bits[12], bt_irq_bits[13], bt_irq_bits[14],
                    bt_irq_bits[15]);
        dev_log("  ke arena %lu/%u bytes in use, peak %lu",
                    (unsigned long)ke_arena_used * 4, KE_ARENA_BYTES,
                    (unsigned long)ke_arena_peak * 4);
        dev_log("  pool heads %08lx %08lx %08lx %08lx",
                    (unsigned long)REG(POOL_HEAD(0)),
                    (unsigned long)REG(POOL_HEAD(1)),
                    (unsigned long)REG(POOL_HEAD(2)),
                    (unsigned long)REG(POOL_HEAD(3)));
    } else if (!link_dev_cmd(arg)) {
        return false;
    }
    return true;
}
#endif
