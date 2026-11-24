/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef BT_YP3_H
#define BT_YP3_H
/* Internal interface of the YP3 Bluetooth driver. The
 * controller is the boot ROM's RivieraWaves dual-mode core; these files add
 * the radio set-up, the ROM hooks and an HCI shim:
 *   bt-rf-yp3.c    BT clocks, RF register tables and calibration
 *   bt-ctrl-yp3.c  ROM controller: hook slots, exchange memory, IRQ 26,
 *                  kernel scheduling in interrupt context
 *   bt-hci-yp3.c   H4 packets <-> ROM kernel messages; implements bt_drv_*
 *                  (firmware/export/bt_drv.h)
 *   bt-sbc-yp3.c   bt_sbc_* (firmware/export/bt_sbc.h) on the ROM encoder */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Radio: enable the BT clocks, load the RF register tables and calibrate.
 * Independent of the controller, but the controller and the exchange memory
 * need the clocks it enables. Takes several seconds (yields); thread context
 * only. False on failure (clocks off again). */
bool bt_rf_yp3_init(void);
/* Gate the BT clocks off again (stop the controller first). */
void bt_rf_yp3_off(void);
/* Enable only the BT clocks (gate 34, selector 0x1d, clock 0x29), no RF
 * calibration: bt-ctrl needs them on before touching the controller core and
 * exchange memory. False if a gate did not acknowledge. */
bool bt_yp3_clocks_on(void);

/* Settings store (PSMP, flash 0x1fc000, read through XIP): copy the
 * value of setting `name` (e.g. "freq_drift", "bt_addr", "bt_name") into
 * buf. Returns its length, or -1 if absent, longer than size or corrupt. */
int bt_yp3_psmp_read(const char *name, void *buf, size_t size);

/* RF data tables (bt-rf-tables-yp3.c; sources cited there). */
#define BT_RF_CELLS         330     /* RF register-file cells */
#define BT_RF_STATIC_CELLS  244     /* cells 0..243 hold static values */
#define BT_RF_FIELDS        30
#define BT_RF_SINE_POINTS   256     /* quarter wave */
#define BT_RF_RAMP_WORDS    7
struct bt_rf_field {
    uint8_t cell;       /* field entry i lives in cell + i */
    uint8_t shift;
    uint32_t mask;      /* before the shift */
};
extern const struct bt_rf_field bt_rf_fields[BT_RF_FIELDS];
extern const uint32_t bt_rf_cells[BT_RF_STATIC_CELLS];
extern const int16_t bt_rf_sine[BT_RF_SINE_POINTS];
extern const uint32_t bt_rf_ramp[BT_RF_RAMP_WORDS];

/* Controller: start or stop the ROM controller. Start turns the BT clocks
 * on if needed; the caller owns them and gates them after a failed start or
 * a stop (bt_drv_open/close). The interface between the controller and the
 * HCI shim follows. */
bool bt_ctrl_yp3_start(void);
void bt_ctrl_yp3_stop(void);

/* ---- Controller <-> HCI shim interface (owned by bt-ctrl-yp3.c) ----
 *
 * The ROM controller is a RivieraWaves kernel: it speaks kernel messages
 * (ke_msg), not HCI bytes. bt-ctrl-yp3.c owns the ROM kernel ABI, the hook
 * slots, exchange memory and the interrupt/scheduler plumbing; bt-hci-yp3.c
 * turns H4 packets into ke_msgs and back. These calls are the boundary.
 *
 * All ROM kernel entry points below are Thumb (call via a pointer with bit0
 * set: BT_THUMB). Task ids are (type | idx<<8); message ids are
 * (task<<10 | n), so a message's id names the task type that defines it. The
 * HCI command-interface task is BT_KE_TASK_HCI; host commands go to tasks
 * 0x38 / 0x39 as their message ids say, and replies come back as messages
 * to BT_KE_TASK_HCI whose id (task 0x38 or 0x39 range) selects the event. */

#define BT_THUMB(addr)   ((addr) | 1u)  /* ROM function pointer */

/* ROM kernel task types. Roles follow this driver's traced call sites (the
 * ROM has no symbols): 0x38 takes the controller-wide HCI commands (message
 * ids 0xe0xx); 0x39 has one instance per link env and takes the
 * per-connection ones (0xe4xx); 0x3b is the link driver that exchanges LMP
 * PDUs (0xecxx). The "LC"/"LM" labels are this port's; the RivieraWaves
 * names may be the other way round. */
#define BT_KE_TASK_LC    0x38
#define BT_KE_TASK_LM    0x39
#define BT_KE_TASK_LD    0x3b
#define BT_KE_TASK_HCI   0x3c   /* HCI command-interface task */
#define BT_KE_TASK_TYPE(tid)     ((tid) & 0xff)
#define BT_KE_TASK_IDX(tid)      ((tid) >> 8)
#define BT_KE_TASK_ID(type, idx) ((type) | (idx) << 8)
#define BT_KE_MSG_TASK(msgid)    ((msgid) >> 10)  /* task type of a msg id */

/* ke_msg header as the ROM lays it out. The HCI framing
 * bytes sit just before the parameter area. */
struct bt_ke_msg {
    struct bt_ke_msg *next;   /* +0  kernel list link */
    uint8_t  hci_type;        /* +4  H4 indicator (BT_H4_*) */
    int8_t   hci_off;         /* +5  signed offset of H4 header vs. param */
    uint16_t hci_len;         /* +6  H4 payload length */
    uint16_t id;              /* +8  message id */
    uint16_t dest_id;         /* +10 destination task */
    uint16_t src_id;          /* +12 source task */
    uint16_t param_len;       /* +14 parameter length */
    uint8_t  param[];         /* +16 parameters */
};

/* Allocate a ke_msg with param_len bytes (ROM ke_msg_alloc 0x309ec) and send
 * it (ROM ke_msg_send 0x30a30). alloc returns the param pointer; the header
 * is the 16 bytes in front of it. */
void *bt_ke_msg_alloc(uint16_t id, uint16_t dest, uint16_t src, uint16_t len);
void  bt_ke_msg_send(void *param);
void  bt_ke_msg_send_basic(uint16_t id, uint16_t dest, uint16_t src);
void  bt_ke_msg_free(void *param);
/* Allocate an ACL payload buffer in exchange memory (ROM 0x305b8). */
void *bt_ke_acl_payload_alloc(void);
/* At Connection Complete of the ACL link handle: return the ACL TX buffers
 * a link lost earlier kept (unless another link is up). */
void bt_ctrl_acl_tx_reclaim(uint16_t handle);
/* At Disconnection Complete of the ACL link handle: return the LT_ADDRs
 * links lost earlier kept (unless another link is up). */
void bt_ctrl_lt_addr_reclaim(uint16_t handle);
/* Connection-handle lookups the ACL TX path uses (ROM 0x3075c / 0x3076c).
 * bt_ll_data_state() gives the handle's kind: BT_LL_ACL for an ACL handle
 * (0x80 + link index), BT_LL_NONE for an unknown one, else the type byte of
 * the matching synchronous (SCO/eSCO) channel record. bt_ll_data_task()
 * gives the handle's link-manager task instance, or BT_LL_NO_TASK unless
 * it is an in-use link. */
#define BT_LL_ACL       1
#define BT_LL_NONE      3
#define BT_LL_NO_TASK   0xff
uint32_t bt_ll_data_state(uint16_t handle);
uint16_t bt_ll_data_task(uint16_t handle);

/* Run bt-ctrl's critical section (masks IRQ 26 and the scheduler line). Used
 * by bt_drv_send, which may be called from a tick interrupt. */
void bt_ctrl_enter_critical(void);
void bt_ctrl_exit_critical(void);

/* True once bt_ctrl_yp3_start() has brought the kernel up. */
bool bt_ctrl_yp3_running(void);

/* Soft-reset the controller in place. The HCI shim calls this when the host
 * sends HCI_Reset. False if the controller is not running. */
bool bt_ctrl_yp3_soft_reset(void);

/* The HCI command-interface task handler (bt-hci-yp3.c), task
 * BT_KE_TASK_HCI. */
int  bt_hci_ke_task_handler(uint16_t msgid, struct bt_ke_msg *msg);

/* For bt-ctrl's controller start: install the HCI command-interface
 * callbacks; create bt-hci's ROM task (BT_KE_TASK_HCI). */
void bt_hci_install_cmd_hooks(void);
void bt_hci_task_desc_init(void);
/* bt-hci's event path delivers one finished H4 packet through this; the
 * byte before hdr is overwritten with the H4 indicator. */
void bt_ctrl_deliver_host(uint8_t type, uint8_t *hdr, size_t len);
/* A role switch ended (Role Change or Disconnection Complete, F-0107) */
void bt_ctrl_role_switch_end(void);

/* RivieraWaves kernel descriptor layout (offsets from stock firmware),
 * shared so bt-hci can build the HCI task's descriptor. A task's handler for
 * a message is looked up in the table for the instance's current state, then
 * in the default table; within a table an exact id wins over a
 * KE_MSG_DEFAULT entry. */
struct ke_msg_handler   { uint16_t id; uint16_t pad; void *func; };
struct ke_state_handler { const struct ke_msg_handler *tab; uint16_t nb;
                          uint16_t pad; };      /* nb entries at tab */
struct ke_task_desc {
    const struct ke_state_handler *state_handler;    /* +0 [state_max] */
    const struct ke_state_handler *default_handler;  /* +4 */
    const uint8_t *current_state;                     /* +8 [idx_max] */
    uint16_t state_max;                               /* +0xc */
    uint16_t idx_max;                                 /* +0xe instances */
};
#define KE_MSG_DEFAULT  0xffff  /* ke_msg_handler id matching any message */
/* ke_task_create (ROM 0x3089c) and ke_state_set (ROM 0x3092c), wrapped for
 * bt-hci. */
void bt_ke_task_create(uint32_t type, const struct ke_task_desc *desc);
void bt_ke_state_set(uint16_t task, uint8_t state);

/* Kernel message handler (ROM ABI) and its return values. */
typedef int (*ke_msg_func)(uint16_t id, void *param, uint16_t dest,
                           uint16_t src);
#define KE_MSG_CONSUMED 0       /* dispatcher frees the message */
#define KE_MSG_NO_FREE  1       /* handler kept or forwarded it */
#define KE_MSG_SAVED    2       /* requeued until the task state changes */

#ifdef YP3_DEV
/* Snapshot of controller status for "bt ctrl state" (bt-dev-yp3.c). */
struct bt_ctrl_dev_status {
    uint32_t core_status;     /* 0x40020010 */
    uint32_t core_ctrl;       /* 0x40020000 */
    uint32_t ke_events;       /* pending ke event bits (ROM BSS 0x8041e4) */
    uint32_t irq_count;       /* IRQ 26 count since start */
    uint32_t sched_count;     /* scheduler-line run count */
    uint8_t  cmd_credits;     /* HCI command credits reported to the host */
    bool     running;
};
void bt_ctrl_yp3_dev_status(struct bt_ctrl_dev_status *out);
/* bt-hci's command-credit byte (glue command env), 0 when stopped. */
uint8_t bt_hci_yp3_cmd_credits(void);
#endif

#ifdef YP3_DEV
/* Dev shell "bt PART ARGS" (apps/dev_shell.c, bt-dev-yp3.c). Each part's
 * handler lives in that part's file; false = bad usage. */
bool bt_yp3_dev_cmd(char *arg);
bool bt_rf_yp3_dev(char *arg);
bool bt_sbc_yp3_dev(char *arg);
bool bt_ctrl_yp3_dev(char *arg);
bool bt_hci_yp3_dev(char *arg);
#endif

#endif /* BT_YP3_H */
