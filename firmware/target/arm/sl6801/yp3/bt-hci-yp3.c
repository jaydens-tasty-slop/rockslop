/* SPDX-License-Identifier: GPL-2.0-or-later */
/* HCI shim and bt_drv_* for the ROM Bluetooth controller (bt-yp3.h).
 *
 * The ROM controller speaks RivieraWaves kernel messages, not HCI bytes.
 * This file is the H4 <-> ke_msg boundary:
 *   - bt_drv_send() turns an H4 command into the kernel message the ROM
 *     expects, using per-opcode descriptor tables. ACL data goes to the
 *     link's task with the payload copied into exchange memory. HCI_Reset is
 *     answered here after a controller soft reset.
 *   - controller replies arrive as kernel messages to the HCI task
 *     (BT_KE_TASK_HCI); bt_hci_ke_task_handler() converts them to H4 event
 *     or ACL packets and hands them to the host.
 *
 * Limits: commands for a link go to its generic link task, which serves a
 * single connection only (see "Command transmit"), and the shim reports one
 * command credit at a time. bt_drv_open() runs the RF
 * init (bt_rf_yp3_init) first; the dev shell ("bt hci open-noradio") starts
 * the controller without the radio to exercise the kernel path.
 *
 * The command and event tables are data from stock firmware. */

#include "config.h"
#include "system.h"
#include "sl6801.h"
#include "mmio-sl6801.h"
#include "bt-yp3.h"
#include "bt_drv.h"
#include "dev-yp3.h"
#include <string.h>

/* H4 packet headers, indicator byte included. */
#define H4_CMD_HDR          4   /* indicator, opcode (2), parameter length */
#define H4_ACL_HDR          5   /* indicator, handle + flags (2), length (2) */

/* HCI commands the shim treats specially. */
#define OP_RESET            0x0c03   /* answered by the shim directly */
#define OP_HOST_BUFFER_SIZE 0x0c33   /* parameters realigned */

/* Reply message ids (to BT_KE_TASK_HCI). The event tables are indexed from
 * the first reply id of each task. */
#define EV38_ID_BASE        0xe065   /* task 0x38: ev38[0] */
#define EV39_ID_BASE        0xe427   /* task 0x39: ev39[0] */
/* The two Command Complete replies whose parameters need compaction. */
#define ID_LOCAL_VER_CC     0xe0bb   /* Read_Local_Version (0x1001) */
#define ID_BUFFER_SIZE_CC   0xe0bf   /* Read_Buffer_Size (0x1005) */
#define ID_ACL_RX           0xe47d   /* received ACL data to the host */
#define ID_ACL_TX           0xe426   /* ACL data to a link task */

/* Glue command env (ROM BSS 0x803c28). */
#define GLUE_CMD_ENV        0x00803c28
#define GLUE_CMD_CREDIT     (GLUE_CMD_ENV + 0x07)  /* HCI command credits */

/* HCI event codes and status. */
#define EVT_INQUIRY_RESULT       0x02
#define EVT_CONN_COMPLETE        0x03
#define EVT_DISCONN_COMPLETE     0x05
#define EVT_ROLE_CHANGE          0x12
#define EVT_AUTH_COMPLETE        0x06
#define EVT_ENCRYPT_CHANGE       0x08
#define EVT_REMOTE_FEATURES      0x0b   /* Read Remote Supported Features */
#define EVT_COMMAND_COMPLETE     0x0e
#define EVT_COMMAND_STATUS       0x0f
#define EVT_NUM_COMPLETED_PKTS   0x13
#define EVT_MODE_CHANGE          0x14
#define EVT_MAX_SLOTS_CHANGE     0x1b
#define EVT_INQUIRY_RESULT_RSSI  0x22
#define EVT_EXT_INQUIRY_RESULT   0x2f
#define EVT_USER_CONFIRM_REQ     0x33
#define HCI_SUCCESS              0x00
#define HCI_ERR_UNKNOWN_CMD      0x01
#define HCI_NUM_CMD_PKTS         1      /* Num_HCI_Command_Packets reported */
#define HCI_LINK_TYPE_ACL        0x01   /* Connection Complete link type */

/* HCI ACL header: 12-bit handle, packet boundary flags in bits 12-13,
 * broadcast flags in 14-15; then the 16-bit data length. */
#define HCI_ACL_HDR         4
#define HCI_ACL_HANDLE_MASK 0x0fff
#define HCI_ACL_FLAGS_SHIFT 12
#define HCI_ACL_PB_CONT     0x1000   /* PB 0b01: continuing fragment */

/* ACL data descriptor: the parameters of ID_ACL_RX and ID_ACL_TX. */
struct acl_desc {
    uint16_t handle;    /* +0 */
    uint8_t  flags;     /* +2 PB in bits 0-1, BC in bits 2-3 */
    uint8_t  pad3;
    uint16_t len;       /* +4 */
    uint16_t pad6;
    uint32_t buf;       /* +8 payload address in exchange memory */
};
#define ACL_FLAGS_PB        0x3
#define ACL_FLAGS_BC        0xc

/* Byte offsets in a reply message (from the start of struct bt_ke_msg).
 * Events are built in place in front of the parameters so they can be
 * delivered without a copy: a Command Complete / Status header (code, len,
 * num_cmd, opcode) starts at MSG_CC_EVT, an async event header (code, len)
 * at MSG_ASYNC_EVT. The field packers below use raw offsets on the same
 * basis: p + 16 is the first parameter, p + 14 the async event header. */
#define MSG_PARAM           16       /* sizeof(struct bt_ke_msg) */
#define MSG_CC_EVT          (MSG_PARAM - 5)
#define MSG_ASYNC_EVT       (MSG_PARAM - 2)

static bt_drv_recv_fn bt_recv;

/* ======================================================================
 * HCI command tables (data from stock firmware). The ROM speaks kernel
 * messages: each HCI opcode maps to a ke message id, a source task and (for
 * commands with parameters) a parameter length. The destination task is
 * id>>10. Commands with parameters are allocated, the host payload is copied
 * in, and the message is sent; no-payload commands are sent directly. Two
 * data tables keyed by full opcode. */
struct hci_cmd {
    uint16_t opcode;    /* HCI opcode (OGF << 10 | OCF) */
    uint16_t id;        /* kernel message id; its task is the destination */
    uint8_t src;        /* source task (always BT_KE_TASK_HCI) */
    uint8_t len;        /* kernel parameter length (0: no-payload table) */
};

/* Commands with parameters (id, src, param_len). Sorted by opcode for
 * bsearch. */
static const struct hci_cmd cmd_payload[] = {
    {0x0401, 0xe000, 0x3c,   5}, {0x0403, 0xe002, 0x3c,  10},
    {0x0405, 0xe004, 0x3c,  14}, {0x0406, 0xe400, 0x3c,   4},
    {0x0408, 0xe005, 0x3c,   6}, {0x0409, 0xe006, 0x3c,   7},
    {0x040a, 0xe007, 0x3c,   7}, {0x040b, 0xe008, 0x3c,  22},
    {0x040c, 0xe009, 0x3c,   6}, {0x040d, 0xe00a, 0x3c,  23},
    {0x040e, 0xe00b, 0x3c,   6}, {0x040f, 0xe401, 0x3c,   4},
    {0x0411, 0xe402, 0x3c,   2}, {0x0413, 0xe403, 0x3c,   4},
    {0x0415, 0xe404, 0x3c,   2}, {0x0419, 0xe00c, 0x3c,  10},
    {0x041a, 0xe00d, 0x3c,   6}, {0x041b, 0xe405, 0x3c,   2},
    {0x041c, 0xe406, 0x3c,   4}, {0x041d, 0xe407, 0x3c,   2},
    {0x041f, 0xe408, 0x3c,   2}, {0x0420, 0xe409, 0x3c,   2},
    {0x0428, 0xe40a, 0x3c,  20}, {0x0429, 0xe00e, 0x3c,  24},
    {0x042a, 0xe010, 0x3c,   7}, {0x042b, 0xe011, 0x3c,   9},
    {0x042c, 0xe012, 0x3c,   6}, {0x042d, 0xe013, 0x3c,   6},
    {0x042e, 0xe014, 0x3c,  12}, {0x042f, 0xe015, 0x3c,   6},
    {0x0430, 0xe016, 0x3c,  38}, {0x0433, 0xe017, 0x3c,   6},
    {0x0434, 0xe018, 0x3c,   7}, {0x043d, 0xe40b, 0x3c,  64},
    {0x043e, 0xe00f, 0x3c,  68},
    {0x0801, 0xe40c, 0x3c,   6}, {0x0803, 0xe40d, 0x3c,  10},
    {0x0804, 0xe40e, 0x3c,   2}, {0x0807, 0xe411, 0x3c,  20},
    {0x0809, 0xe412, 0x3c,   2}, {0x080b, 0xe019, 0x3c,   7},
    {0x080c, 0xe413, 0x3c,   2}, {0x080d, 0xe414, 0x3c,   4},
    {0x080f, 0xe01b, 0x3c,   2}, {0x0810, 0xe415, 0x3c,  24},
    {0x0811, 0xe416, 0x3c,   8},
    {0x0c01, 0xe01c, 0x3c,   8}, {0x0c05, 0xe01e, 0x3c,  11},
    {0x0c08, 0xe417, 0x3c,   2}, {0x0c0a, 0xe020, 0x3c,   1},
    {0x0c0d, 0xe022, 0x3c,   7}, {0x0c11, 0xe023, 0x3c, 111},
    {0x0c12, 0xe024, 0x3c,   7}, {0x0c13, 0xe025, 0x3c, 248},
    {0x0c16, 0xe028, 0x3c,   2}, {0x0c18, 0xe02a, 0x3c,   2},
    {0x0c1a, 0xe02c, 0x3c,   1}, {0x0c1c, 0xe02e, 0x3c,   4},
    {0x0c1e, 0xe030, 0x3c,   4}, {0x0c20, 0xe032, 0x3c,   1},
    {0x0c24, 0xe034, 0x3c,   3}, {0x0c26, 0xe036, 0x3c,   2},
    {0x0c27, 0xe418, 0x3c,   2}, {0x0c28, 0xe419, 0x3c,   4},
    {0x0c2a, 0xe038, 0x3c,   1}, {0x0c2c, 0xe03a, 0x3c,   1},
    {0x0c2d, 0xe41a, 0x3c,   4}, {0x0c2f, 0xe03b, 0x3c,   1},
    {0x0c31, 0xe03d, 0x3c,   1}, {0x0c33, 0xe03e, 0x3c,   8},
    {0x0c35, 0xe03f, 0x3c,  30}, {0x0c36, 0xe41b, 0x3c,   2},
    {0x0c37, 0xe41c, 0x3c,   4}, {0x0c3a, 0xe042, 0x3c,   4},
    {0x0c3f, 0xe043, 0x3c,  10}, {0x0c43, 0xe045, 0x3c,   1},
    {0x0c45, 0xe047, 0x3c,   1}, {0x0c47, 0xe049, 0x3c,   1},
    {0x0c49, 0xe04b, 0x3c,   1}, {0x0c52, 0xe04d, 0x3c, 241},
    {0x0c53, 0xe41d, 0x3c,   2}, {0x0c56, 0xe04f, 0x3c,   1},
    {0x0c59, 0xe052, 0x3c,   1}, {0x0c5b, 0xe054, 0x3c,   1},
    {0x0c5f, 0xe41e, 0x3c,   4}, {0x0c60, 0xe055, 0x3c,   7},
    {0x0c68, 0xe41f, 0x3c,   4}, {0x0c6d, 0xe057, 0x3c,   2},
    {0x1401, 0xe420, 0x3c,   2}, {0x1402, 0xe421, 0x3c,   2},
    {0x1403, 0xe422, 0x3c,   2}, {0x1405, 0xe423, 0x3c,   2},
    {0x1406, 0xe424, 0x3c,   2}, {0x1407, 0xe05f, 0x3c,   4},
    {0x1408, 0xe425, 0x3c,   2},
};

/* No-payload commands (id, src 0x3c). Sorted by opcode for bsearch. */
static const struct hci_cmd cmd_nopayload[] = {
    {0x0402, 0xe001, 0x3c, 0}, {0x0404, 0xe003, 0x3c, 0},
    {0x080e, 0xe01a, 0x3c, 0},
    {0x0c09, 0xe01f, 0x3c, 0}, {0x0c0b, 0xe021, 0x3c, 0},
    {0x0c14, 0xe026, 0x3c, 0}, {0x0c15, 0xe027, 0x3c, 0},
    {0x0c17, 0xe029, 0x3c, 0}, {0x0c19, 0xe02b, 0x3c, 0},
    {0x0c1b, 0xe02d, 0x3c, 0}, {0x0c1d, 0xe02f, 0x3c, 0},
    {0x0c1f, 0xe031, 0x3c, 0}, {0x0c23, 0xe033, 0x3c, 0},
    {0x0c25, 0xe035, 0x3c, 0}, {0x0c29, 0xe037, 0x3c, 0},
    {0x0c2b, 0xe039, 0x3c, 0}, {0x0c2e, 0xe03c, 0x3c, 0},
    {0x0c38, 0xe040, 0x3c, 0}, {0x0c39, 0xe041, 0x3c, 0},
    {0x0c42, 0xe044, 0x3c, 0}, {0x0c44, 0xe046, 0x3c, 0},
    {0x0c46, 0xe048, 0x3c, 0}, {0x0c48, 0xe04a, 0x3c, 0},
    {0x0c51, 0xe04c, 0x3c, 0}, {0x0c55, 0xe04e, 0x3c, 0},
    {0x0c57, 0xe050, 0x3c, 0}, {0x0c58, 0xe051, 0x3c, 0},
    {0x0c5a, 0xe053, 0x3c, 0}, {0x0c6c, 0xe056, 0x3c, 0},
    {0x1001, 0xe058, 0x3c, 0}, {0x1002, 0xe059, 0x3c, 0},
    {0x1003, 0xe05a, 0x3c, 0}, {0x1005, 0xe05c, 0x3c, 0},
    {0x1009, 0xe05d, 0x3c, 0}, {0x100b, 0xe05e, 0x3c, 0},
    {0x1801, 0xe060, 0x3c, 0}, {0x1803, 0xe062, 0x3c, 0},
};

/* Binary search of a sorted command table; NULL if the opcode is absent. */
static const struct hci_cmd *hci_cmd_find(const struct hci_cmd *t, size_t n,
                                          uint16_t opcode)
{
    size_t lo = 0, hi = n;
    while (lo < hi) {
        size_t mid = (lo + hi) / 2;
        if (t[mid].opcode == opcode)
            return &t[mid];
        if (t[mid].opcode < opcode)
            lo = mid + 1;
        else
            hi = mid;
    }
    return NULL;
}

/* ======================================================================
 * Host delivery. Write the H4 indicator into the byte in front of hdr and
 * hand [indicator][content] to the recv callback. hdr points into a
 * consumed controller message, whose header ids in front of the HCI bytes
 * are dead by now, or into a caller buffer with a spare first byte; no copy
 * on the interrupt stack. May run in interrupt context.
 * ====================================================================== */
#ifdef YP3_DEV
static bool hci_trace;                  /* "bt hci trace on|off" */
static void hci_dev_hex(const char *dir, const void *pkt, size_t len);
#endif

void bt_ctrl_deliver_host(uint8_t type, uint8_t *hdr, size_t len)
{
    /* status, handle (2), BD_ADDR, link type */
    if (type == BT_H4_EVT && len >= 12 && hdr[0] == EVT_CONN_COMPLETE &&
        !hdr[2] && hdr[11] == HCI_LINK_TYPE_ACL)
        bt_ctrl_acl_tx_reclaim(hdr[3] | hdr[4] << 8);
    /* status, handle (2), reason */
    if (type == BT_H4_EVT && len >= 6 && hdr[0] == EVT_DISCONN_COMPLETE &&
        !hdr[2])
        bt_ctrl_lt_addr_reclaim(hdr[3] | hdr[4] << 8);
    if (type == BT_H4_EVT && len >= 1 &&
        (hdr[0] == EVT_DISCONN_COMPLETE || hdr[0] == EVT_ROLE_CHANGE))
        bt_ctrl_role_switch_end();
    if (!bt_recv)
        return;
    hdr[-1] = type;
#ifdef YP3_DEV
    if (hci_trace)
        hci_dev_hex("rx", hdr - 1, len + 1);
#endif
    bt_recv(hdr - 1, len + 1);
}

/* ======================================================================
 * Command Complete builder. Writes the 5-byte CC header into the message in
 * front of the parameters (MSG_CC_EVT; hci_off = -5) and returns the H4
 * length. Num_HCI_Command_Packets is reported as 1 (one outstanding
 * command): honest flow control for the shim, which keeps no fuller credit
 * count.
 * ====================================================================== */
static unsigned build_cc(uint8_t *p, uint16_t op, uint8_t ret_len)
{
    REG8(GLUE_CMD_CREDIT) = HCI_NUM_CMD_PKTS; /* keep the env field sane */
    p[MSG_CC_EVT]     = EVT_COMMAND_COMPLETE;
    p[MSG_CC_EVT + 1] = ret_len + 3;   /* num_cmd(1) + opcode(2) + ret_len */
    p[MSG_CC_EVT + 2] = HCI_NUM_CMD_PKTS;
    p[MSG_CC_EVT + 3] = op & 0xff;
    p[MSG_CC_EVT + 4] = op >> 8;
    return ret_len + 5;                /* code+len + param_len */
}

/* Command Status: [0x0f][4][status][num_cmd][opcode]. The
 * status is the reply's first parameter; the header overwrites it. */
static unsigned build_status(uint8_t *p, uint16_t op)
{
    uint8_t st = p[MSG_PARAM];
    p[MSG_CC_EVT]     = EVT_COMMAND_STATUS;
    p[MSG_CC_EVT + 1] = 4;             /* status, num_cmd, opcode */
    p[MSG_CC_EVT + 2] = st;
    p[MSG_CC_EVT + 3] = HCI_NUM_CMD_PKTS;
    p[MSG_CC_EVT + 4] = op & 0xff;
    p[MSG_CC_EVT + 5] = op >> 8;
    return 6;                          /* code+len + 4 */
}

/* Parameter compaction. The controller lays reply fields out aligned; HCI
 * wants them packed. A field move copies a little-endian field of width 1,
 * 2 or 4 bytes from src to dst inside the message (byte offsets from the
 * message start, so MSG_PARAM is the first parameter); width 0 stores a zero
 * byte at dst. Moves run in order, each reading its source before writing.
 * The tables record the controller's reply field layouts (from stock
 * firmware). */
struct field_move { uint8_t dst, src, width; };

static void apply_moves(uint8_t *p, const struct field_move *m, size_t n)
{
    for (; n--; m++) {
        uint32_t v = 0;
        for (unsigned i = m->width; i--; )
            v = v << 8 | p[m->src + i];
        unsigned w = m->width ? m->width : 1;
        for (unsigned i = 0; i < w; i++, v >>= 8)
            p[m->dst + i] = v;
    }
}
#define APPLY_MOVES(p, tbl) apply_moves(p, tbl, ARRAYLEN(tbl))

/* The two Command Complete replies with parameters to compact. */
static const struct field_move local_version_moves[] = {
    { 18, 18, 2 }, { 21, 22, 2 }, { 23, 24, 2 },
};
static const struct field_move buffer_size_moves[] = {
    { 17, 18, 2 }, { 19, 20, 1 }, { 20, 22, 2 }, { 22, 24, 2 },
};

/* ======================================================================
 * Event conversion tables (data from stock firmware). A controller reply
 * message's id selects the HCI event. CC/CS events sit 5 bytes before the
 * parameters (hci_off -5, MSG_CC_EVT); async events sit 2 bytes before
 * (hci_off -2, MSG_ASYNC_EVT).
 * ====================================================================== */
/* Command Complete (task 0x38): index = id - EV38_ID_BASE; {opcode,
 * param_len}; {0, 0} marks an unused id.
 * Params arrive in wire order except Read_Local_Version (0x1001) and
 * Read_Buffer_Size (0x1005), compacted below. Other multi-field read CCs
 * (Read_Clock, Read_Link_Quality, ...) are off the A2DP path; build_cc
 * emits them in the controller's field layout. */
static const struct ev38_entry { uint16_t op; uint8_t len; } ev38[] = {
    [0x00]={0x0000,1}, [0x01]={0x0402,1}, [0x02]={0x0403,1},
    [0x03]={0x0404,1}, [0x04]={0x0408,7}, [0x05]={0x040b,7},
    [0x06]={0x040c,7}, [0x07]={0x040d,7}, [0x08]={0x040e,7},
    [0x09]={0x041a,7}, [0x0a]={0x042b,7}, [0x0b]={0x042c,7},
    [0x0c]={0x042d,7}, [0x0d]={0x042e,7}, [0x0e]={0x042f,7},
    [0x0f]={0x0430,7}, [0x10]={0x0433,7}, [0x11]={0x0434,7},
    [0x12]={0x080e,3}, [0x13]={0x080f,1}, [0x14]={0x0c01,1},
    [0x15]={0x0c03,1}, [0x16]={0x0000,1}, [0x17]={0x0c05,1},
    [0x18]={0x0c09,2}, [0x19]={0x0c0a,1}, [0x1a]={0x0c0b,1},
    [0x1b]={0x0c0d,5}, [0x1c]={0x0c11,2}, [0x1d]={0x0c12,3},
    [0x1e]={0x0c13,1}, [0x1f]={0x0c14,249}, [0x20]={0x0c15,3},
    [0x21]={0x0c16,1}, [0x22]={0x0c17,3}, [0x23]={0x0c18,1},
    [0x24]={0x0c19,2}, [0x25]={0x0c1a,1}, [0x26]={0x0c1b,5},
    [0x27]={0x0c1c,1}, [0x28]={0x0c1d,5}, [0x29]={0x0c1e,1},
    [0x2a]={0x0c1f,2}, [0x2b]={0x0c20,1}, [0x2c]={0x0000,0},
    [0x2d]={0x0000,0}, [0x2e]={0x0c23,4}, [0x2f]={0x0c24,1},
    [0x30]={0x0c25,3}, [0x31]={0x0c26,1}, [0x32]={0x0c29,2},
    [0x33]={0x0c2a,1}, [0x34]={0x0c2b,2}, [0x35]={0x0c2c,1},
    [0x36]={0x0c2f,1}, [0x37]={0x0c2e,2}, [0x38]={0x0c31,1},
    [0x39]={0x0c33,1}, [0x3a]={0x0c35,1}, [0x3b]={0x0c38,2},
    [0x3c]={0x0c39,0}, [0x3d]={0x0c3a,1}, [0x3e]={0x0000,0},
    [0x3f]={0x0000,0}, [0x40]={0x0c3f,1}, [0x41]={0x0c42,2},
    [0x42]={0x0c43,1}, [0x43]={0x0c44,2}, [0x44]={0x0c45,1},
    [0x45]={0x0c46,2}, [0x46]={0x0c47,1}, [0x47]={0x0c48,2},
    [0x48]={0x0c49,1}, [0x49]={0x0c51,242}, [0x4a]={0x0c52,1},
    [0x4b]={0x0c55,2}, [0x4c]={0x0c56,1}, [0x4d]={0x0c57,33},
    [0x4e]={0x0c58,2}, [0x4f]={0x0c59,1}, [0x50]={0x0c5a,2},
    [0x51]={0x0c5b,1}, [0x52]={0x0c60,7}, [0x53]={0x0c6c,3},
    [0x54]={0x0c6d,1}, [0x55]={0x0000,0}, [0x56]={0x1001,9},
    [0x57]={0x1002,65}, [0x58]={0x1003,9}, [0x59]={0x1004,11},
    [0x5a]={0x1005,8}, [0x5b]={0x1009,7}, [0x5c]={0x100b,1},
    [0x5d]={0x1407,9}, [0x5e]={0x1801,2}, [0x5f]={0x1802,1},
};

/* Task 0x39: Command Complete (0xe427-0xe438, kind 'C'), Command Status
 * (0xe439-0xe453 'S', 0xe454 's' / 0xe455 'c' take the opcode from the
 * message), and async events (0xe456+, 'P' = code+len passthrough, 'A' =
 * field-compacting packer). index = id - EV39_ID_BASE; kind 0 marks an
 * unused id. code is the HCI event code, len the event parameter length
 * (for 'C' the return-parameter length), op the opcode for 'C'/'S'. */
struct ev39_entry { uint8_t kind; uint8_t code; uint8_t len; uint16_t op; };
static const struct ev39_entry ev39[] = {
    [0x00]={'C',0x0e,8,0x0420},  [0x01]={'C',0x0e,4,0x0809},
    [0x02]={'C',0x0e,5,0x080c},  [0x03]={'C',0x0e,3,0x080d},
    [0x04]={'C',0x0e,3,0x0811},  [0x05]={'C',0x0e,3,0x0c08},
    [0x06]={'C',0x0e,5,0x0c27},  [0x07]={'C',0x0e,3,0x0c28},
    [0x08]={'C',0x0e,4,0x0c2d},  [0x09]={'C',0x0e,5,0x0c36},
    [0x0a]={'C',0x0e,3,0x0c37},  [0x0b]={'C',0x0e,6,0x0c68},
    [0x0c]={'C',0x0e,5,0x1401},  [0x0d]={'C',0x0e,3,0x1402},
    [0x0e]={'C',0x0e,4,0x1403},  [0x0f]={'C',0x0e,4,0x1405},
    [0x10]={'C',0x0e,9,0x1407},  [0x11]={'C',0x0e,14,0x1406},
    [0x12]={'S',0x0f,0,0x1408},  [0x13]={'S',0x0f,0,0x0406},
    [0x14]={'S',0x0f,0,0x0409},  [0x15]={'S',0x0f,0,0x040a},
    [0x16]={'S',0x0f,0,0x040f},  [0x17]={'S',0x0f,0,0x0411},
    [0x18]={'S',0x0f,0,0x0413},  [0x19]={'S',0x0f,0,0x0415},
    [0x1a]={'S',0x0f,0,0x041b},  [0x1b]={'S',0x0f,0,0x041c},
    [0x1c]={'S',0x0f,0,0x041d},  [0x1d]={'S',0x0f,0,0x041f},
    [0x1e]={'S',0x0f,0,0x0428},  [0x1f]={'S',0x0f,0,0x043d},
    [0x20]={'S',0x0f,0,0x0429},  [0x21]={'S',0x0f,0,0x043e},
    [0x22]={'S',0x0f,0,0x042a},  [0x23]={'S',0x0f,0,0x0801},
    [0x24]={'S',0x0f,0,0x0803},  [0x25]={'S',0x0f,0,0x0804},
    [0x28]={'S',0x0f,0,0x0807},  [0x29]={'S',0x0f,0,0x080b},
    [0x2a]={'S',0x0f,0,0x0810},  [0x2b]={'S',0x0f,0,0x0c53},
    [0x2c]={'S',0x0f,0,0x0c5f},  [0x2d]={'s',0x0f,0,0},
    [0x2e]={'c',0x0e,0,0},
    [0x2f]={'A',0x03,11,0}, [0x30]={'P',0x04,10,0}, [0x31]={'A',0x05,4,0},
    [0x32]={'A',0x06,3,0},  [0x33]={'A',0x08,4,0},  [0x34]={'A',0x09,3,0},
    [0x35]={'A',0x0a,4,0},  [0x36]={'A',0x0b,11,0}, [0x37]={'A',0x0c,8,0},
    [0x38]={'A',0x0d,21,0}, [0x39]={'A',0x11,2,0},  [0x3a]={'P',0x12,8,0},
    [0x3b]={'A',0x13,0,0},  [0x3c]={'A',0x14,6,0},  [0x3d]={'P',0x16,6,0},
    [0x3e]={'P',0x17,6,0},  [0x3f]={'P',0x18,23,0}, [0x40]={'P',0x1a,1,0},
    [0x41]={'A',0x1b,3,0},  [0x42]={'A',0x1c,5,0},  [0x43]={'A',0x1d,5,0},
    [0x44]={'A',0x1e,2,0},  [0x45]={'A',0x21,22,0}, [0x46]={'A',0x23,13,0},
    [0x47]={'A',0x2c,17,0}, [0x48]={'A',0x2d,9,0},  [0x49]={'A',0x2e,11,0},
    [0x4a]={'A',0x30,3,0},  [0x4b]={'P',0x31,6,0},  [0x4c]={'P',0x32,9,0},
    [0x4d]={'A',0x33,10,0}, [0x4e]={'P',0x34,6,0},  [0x4f]={'P',0x35,6,0},
    [0x50]={'P',0x36,7,0},  [0x51]={'A',0x38,4,0},  [0x52]={'A',0x39,2,0},
    [0x53]={'P',0x3b,10,0}, [0x54]={'P',0x3c,7,0},  [0x55]={'P',0x3d,14,0},
};

/* Task 0x38 reply ids past its Command Complete block (entries
 * 0x60..0x72): Command Status for inquiry and the connection commands, then
 * async events. index = id - EV38_ID_BASE - EV38_EXT. */
#define EV38_EXT 0x60
static const struct ev39_entry ev38_ext[] = {
    {'C',0x0e,1,0x1803}, {'C',0x0e,1,0x1804}, {'S',0x0f,0,0x0401},
    {'S',0x0f,0,0x0405}, {'S',0x0f,0,0x0409}, {'S',0x0f,0,0x040a},
    {'S',0x0f,0,0x0419}, {'S',0x0f,0,0x0429}, {'S',0x0f,0,0x043e},
    {'S',0x0f,0,0x042a}, {'S',0x0f,0,0x080b}, {'P',0x01,1,0},
    {'A',0x02,0,0},      {'A',0x03,11,0},     {'P',0x07,255,0},
    {'P',0x10,1,0},      {'P',0x15,23,0},     {'A',0x22,0,0},
    {'A',0x2f,0,0},
};

/* Async events whose parameters need compaction (field layouts from stock
 * firmware). len is the HCI parameter length; for PACK_COUNTED events (HCI:
 * a count byte, then that many records) it is the record length and the
 * parameter length is 1 + count * len. Other async events (kind 'P', and
 * the non-A2DP rest) get code and length only. */
#define PACK_COUNTED    0x01
#define PACK_MAX_MOVES  5
struct async_pack {
    uint8_t code, len, flags, nmoves;
    struct field_move move[PACK_MAX_MOVES];
};
static const struct async_pack async_packs[] = {
    { EVT_CONN_COMPLETE, 11, 0, 5,
      { { 17, 18, 2 }, { 19, 21, 4 }, { 23, 25, 2 }, { 25, 27, 1 },
        { 26, 28, 1 } } },
    { EVT_DISCONN_COMPLETE, 4, 0, 2,
      { { 17, 18, 2 }, { 19, 20, 1 } } },
    { EVT_AUTH_COMPLETE, 3, 0, 1,
      { { 17, 18, 2 } } },
    { EVT_ENCRYPT_CHANGE, 4, 0, 2,
      { { 17, 18, 2 }, { 19, 20, 1 } } },
    { EVT_REMOTE_FEATURES, 11, 0, 3,
      { { 17, 18, 2 }, { 19, 20, 4 }, { 23, 24, 4 } } },
    { EVT_NUM_COMPLETED_PKTS, 4, PACK_COUNTED, 2,
      { { 17, 18, 2 }, { 19, 20, 2 } } },
    { EVT_MODE_CHANGE, 6, 0, 3,
      { { 17, 18, 2 }, { 19, 20, 1 }, { 20, 22, 2 } } },
    { EVT_MAX_SLOTS_CHANGE, 3, 0, 2,
      { { 16, 16, 2 }, { 19, 0, 0 } } },
    { EVT_USER_CONFIRM_REQ, 10, 0, 1,
      { { 22, 24, 4 } } },
    { EVT_INQUIRY_RESULT, 14, PACK_COUNTED, 1,
      { { 29, 30, 2 } } },
    { EVT_INQUIRY_RESULT_RSSI, 14, PACK_COUNTED, 1,
      { { 28, 28, 2 } } },
    { EVT_EXT_INQUIRY_RESULT, 254, PACK_COUNTED, 1,
      { { 28, 28, 2 } } },
};

/* Compact an async event in place and write its header at MSG_ASYNC_EVT
 * (code, parameter length); returns the length + 2. len is the table
 * entry's, used for events without a pack descriptor. */
static unsigned pack_async(uint8_t *p, uint8_t code, uint8_t len)
{
    const struct async_pack *e = NULL;
    for (size_t i = 0; i < ARRAYLEN(async_packs); i++)
        if (async_packs[i].code == code) {
            e = &async_packs[i];
            break;
        }
    unsigned plen = len;
    if (e) {
        uint8_t n = p[MSG_PARAM];
        if (!(e->flags & PACK_COUNTED) || n)    /* no records: nothing to do */
            apply_moves(p, e->move, e->nmoves);
        plen = e->flags & PACK_COUNTED ? 1 + n * e->len : e->len;
    } else {
        dev_log("bt hci: async event 0x%02x passthrough (stage 2+)", code);
    }
    p[MSG_ASYNC_EVT] = code;
    p[MSG_ASYNC_EVT + 1] = plen;
    return plen + 2;
}

/* Received ACL data. The controller's message holds a struct acl_desc
 * (handle, packet boundary/broadcast flags, length and a pointer to the
 * payload in exchange memory), with room for the 4-byte HCI ACL header in
 * front of the payload. Deliver it in fragments the stack's receive ring
 * takes, then release the buffer (ROM 0x305ec). Each fragment's header (and
 * the H4 byte before it) overwrites the end of the previous fragment, so
 * those bytes are saved and restored around the delivery. */
#define ACL_RX_FRAG     512
#define ROM_ACL_RX_FREE ((void (*)(void *))BT_THUMB(0x305ec))
static void acl_rx(const struct acl_desc *d)
{
    uint16_t handle = d->handle & HCI_ACL_HANDLE_MASK;
    uint16_t pb = (d->flags & ACL_FLAGS_PB) << HCI_ACL_FLAGS_SHIFT,
             bc = (d->flags & ACL_FLAGS_BC) << HCI_ACL_FLAGS_SHIFT;
    unsigned len = d->len;
    uint8_t *data = (uint8_t *)(uintptr_t)d->buf;
    unsigned off = 0;
    do {
        unsigned n = MIN(len - off, ACL_RX_FRAG);
        uint8_t *h = data + off - HCI_ACL_HDR, save[1 + HCI_ACL_HDR];
        uint16_t hf = handle | bc | (off ? HCI_ACL_PB_CONT : pb);
        memcpy(save, h - 1, sizeof save);
        h[0] = hf; h[1] = hf >> 8; h[2] = n; h[3] = n >> 8;
        bt_ctrl_deliver_host(BT_H4_ACL, h, HCI_ACL_HDR + n);
        memcpy(h - 1, save, sizeof save);
        off += n;
    } while (off < len);
    ROM_ACL_RX_FREE(data);
}

/* Build the event an ev39-style entry describes in the message and deliver
 * it: CC/CS sit 5 bytes before the parameters, async events 2. 's' and 'c'
 * replies carry the opcode as their first parameter. */
static int deliver_entry(uint8_t *p, const struct ev39_entry *e)
{
    unsigned h4;
    switch (e->kind) {
    case 'C': h4 = build_cc(p, e->op, e->len); break;
    case 'S': h4 = build_status(p, e->op); break;
    case 's':                       /* CS, opcode from the message */
        h4 = build_status(p, p[MSG_PARAM] | (p[MSG_PARAM + 1] << 8)); break;
    case 'c':                       /* status-only CC, opcode from message */
        h4 = build_cc(p, p[MSG_PARAM] | (p[MSG_PARAM + 1] << 8), 1); break;
    case 'P':
        p[MSG_ASYNC_EVT] = e->code; p[MSG_ASYNC_EVT + 1] = e->len;
        h4 = e->len + 2; break;
    default:  h4 = pack_async(p, e->code, e->len); break;
    }
    bt_ctrl_deliver_host(BT_H4_EVT, p + (e->kind == 'P' || e->kind == 'A'
                                        ? MSG_ASYNC_EVT : MSG_CC_EVT), h4);
    return KE_MSG_CONSUMED;
}

/* ======================================================================
 * HCI task handler: turn a controller reply message into an H4 event (or
 * ACL data) and deliver it. Always KE_MSG_CONSUMED: the
 * dispatcher frees the message; unknown replies are logged and dropped.
 * ====================================================================== */
int bt_hci_ke_task_handler(uint16_t msgid, struct bt_ke_msg *msg)
{
    uint8_t *p = (uint8_t *)msg;
    unsigned task = BT_KE_MSG_TASK(msgid);

    if (msgid == ID_ACL_RX) {       /* received ACL data */
        acl_rx((const struct acl_desc *)(p + MSG_PARAM));
        return KE_MSG_CONSUMED;
    }

    if (task == BT_KE_TASK_LC) {        /* Command Complete, then ev38_ext */
        unsigned idx = msgid - EV38_ID_BASE;
        if (idx >= EV38_EXT && idx - EV38_EXT < ARRAYLEN(ev38_ext))
            return deliver_entry(p, &ev38_ext[idx - EV38_EXT]);
        if (idx >= ARRAYLEN(ev38) || (!ev38[idx].op && !ev38[idx].len)) {
            dev_log("bt hci: unknown CC reply id %04x", msgid);
            return KE_MSG_CONSUMED;
        }
        if (msgid == ID_LOCAL_VER_CC)
            APPLY_MOVES(p, local_version_moves);
        else if (msgid == ID_BUFFER_SIZE_CC)
            APPLY_MOVES(p, buffer_size_moves);
        unsigned h4 = build_cc(p, ev38[idx].op, ev38[idx].len);
        bt_ctrl_deliver_host(BT_H4_EVT, p + MSG_CC_EVT, h4);
        return KE_MSG_CONSUMED;
    }

    if (task == BT_KE_TASK_LM) {
        unsigned idx = msgid - EV39_ID_BASE;
        if (idx >= ARRAYLEN(ev39) || ev39[idx].kind == 0) {
            dev_log("bt hci: unknown reply id %04x", msgid);
            return KE_MSG_CONSUMED;
        }
        return deliver_entry(p, &ev39[idx]);
    }

    dev_log("bt hci: reply to task %02x id %04x", task, msgid);
    return KE_MSG_CONSUMED;
}

/* ======================================================================
 * Command transmit.
 *
 * Commands whose parameters carry a connection handle or a BD_ADDR
 * (Create_Connection, Disconnect, the authentication/encryption and
 * pairing-reply commands, ...) go to the generic link task (dest = the id's
 * task, instance 0). That is right for a single connection, the only case
 * the shim supports.
 * ====================================================================== */
static bool hci_send_payload(uint16_t opcode, const uint8_t *param,
                             uint8_t plen)
{
    const struct hci_cmd *c = hci_cmd_find(cmd_payload, ARRAYLEN(cmd_payload),
                                           opcode);
    if (!c)
        return false;
    uint8_t len = c->len;
    void *pr = bt_ke_msg_alloc(c->id, BT_KE_MSG_TASK(c->id), c->src, len);
    uint8_t *p = pr;
    memcpy(p, param, plen < len ? plen : len);
    if (plen < len)
        memset(p + plen, 0, len - plen);

    /* Host_Buffer_Size: the controller's message has the two 16-bit counts
     * aligned after the 1-byte SCO MTU (from stock firmware). */
    if (opcode == OP_HOST_BUFFER_SIZE) {
        uint8_t t5 = p[5], t6 = p[6], t3 = p[3], t4 = p[4];
        p[6] = t5; p[7] = t6; p[4] = t3; p[5] = t4;
    }
    bt_ke_msg_send(p);
    return true;
}

/* Send one HCI command to the controller; false if the opcode is not in the
 * tables. */
static bool hci_tx_cmd(uint16_t opcode, uint8_t plen, const uint8_t *param)
{
    bt_ctrl_enter_critical();

    bool ok;
    if (plen == 0) {
        const struct hci_cmd *c = hci_cmd_find(cmd_nopayload,
                                               ARRAYLEN(cmd_nopayload),
                                               opcode);
        ok = c != NULL;
        if (ok)
            bt_ke_msg_send_basic(c->id, BT_KE_MSG_TASK(c->id), c->src);
    } else {
        ok = hci_send_payload(opcode, param, plen);
    }
    bt_ctrl_exit_critical();
    return ok;
}

/* The host stack waits for a command credit, so every command must produce a
 * Command Complete or Command Status. An opcode we do not recognise gets a
 * Command Complete with Unknown_HCI_Command so the credit is returned. */
static void hci_send_cc_error(uint16_t opcode, uint8_t status)
{
    uint8_t cc[7] = { 0, EVT_COMMAND_COMPLETE, 4,     /* [0]: H4 byte */
                      HCI_NUM_CMD_PKTS, opcode & 0xff, opcode >> 8, status };
    bt_ctrl_deliver_host(BT_H4_EVT, cc + 1, sizeof cc - 1);
}

/* The link task that takes a handle's ACL data now (an ACL handle of an
 * in-use link), or BT_LL_NO_TASK. */
static uint16_t acl_dest_task(uint16_t handle)
{
    if (bt_ll_data_state(handle) != BT_LL_ACL)
        return BT_LL_NO_TASK;
    return bt_ll_data_task(handle);
}

/* ACL TX. One fragment per call; the host stack sends PB 0b10 then 0b01.
 * The payload goes into an exchange-memory buffer and a struct acl_desc for
 * it to the handle's link task. */
/* Largest segment: the ACL length the controller advertises in its
 * Read_Buffer_Size reply, 1021 (F-0114), which is also the largest 3-DH5
 * payload and fits a 1036-byte pool 0 buffer (F-0081, F-0112). A host that
 * obeys the reply never sends more; a longer segment is dropped. */
#define ACL_TX_LEN_MAX  1021
static bool hci_send_acl(uint16_t hf, const uint8_t *data, uint16_t dlen)
{
    uint16_t handle = hf & HCI_ACL_HANDLE_MASK;
    uint8_t flags = (hf >> HCI_ACL_FLAGS_SHIFT) & ACL_FLAGS_PB;

    if (dlen > ACL_TX_LEN_MAX) {
        dev_log("bt hci: ACL segment of %u bytes dropped", dlen);
        return true;
    }
    bt_ctrl_enter_critical();

    /* No link to send on drops the segment (a flush). */
    uint16_t task = acl_dest_task(handle);
    void *buf = task != BT_LL_NO_TASK ? bt_ke_acl_payload_alloc() : NULL;
    if (!buf) {
        bt_ctrl_exit_critical();
        return task == BT_LL_NO_TASK;  /* no link: consume; no buffer: retry */
    }
    memcpy(buf, data, dlen);

    struct acl_desc *d = bt_ke_msg_alloc(ID_ACL_TX, task, BT_KE_TASK_HCI,
                                         sizeof *d);
    d->handle = handle;
    d->flags = flags;
    d->len = dlen;
    d->buf = (uint32_t)(uintptr_t)buf;
    bt_ke_msg_send(d);
    bt_ctrl_exit_critical();
    return true;
}

bool bt_drv_send(const void *pkt, size_t len)
{
    const uint8_t *b = pkt;
    if (!bt_ctrl_yp3_running() || len < 1)
        return false;
#ifdef YP3_DEV
    if (hci_trace)
        hci_dev_hex("tx", pkt, len);
#endif

    if (b[0] == BT_H4_CMD) {
        if (len < H4_CMD_HDR)
            return false;
        uint16_t opcode = b[1] | (b[2] << 8);
        uint8_t plen = b[3];
        if ((size_t)plen + H4_CMD_HDR > len)
            return false;
        if (opcode == OP_RESET) {
            /* Soft-reset the controller in place, then answer with Command
             * Complete. */
            bt_ctrl_yp3_soft_reset();
            hci_send_cc_error(OP_RESET, HCI_SUCCESS);
            return true;
        }
        if (!hci_tx_cmd(opcode, plen, b + H4_CMD_HDR)) {
            dev_log("bt hci: unhandled cmd %04x (%u bytes)", opcode, plen);
            hci_send_cc_error(opcode, HCI_ERR_UNKNOWN_CMD);
        }
        return true;
    }
    if (b[0] == BT_H4_ACL) {
        if (len < H4_ACL_HDR)
            return false;
        uint16_t hf = b[1] | (b[2] << 8);
        uint16_t dlen = b[3] | (b[4] << 8);
        if ((size_t)dlen + H4_ACL_HDR > len)
            return false;
        return hci_send_acl(hf, b + H4_ACL_HDR, dlen);
    }
    /* SCO (BT_H4_SCO) is out of scope (A2DP is ACL only). */
    dev_log("bt hci: H4 type %u not handled", b[0]);
    return false;
}

/* ======================================================================
 * Controller <-> shim registration.
 * ====================================================================== */
/* HCI command-interface task (BT_KE_TASK_HCI) descriptor: one instance, one
 * state, and a single KE_MSG_DEFAULT handler that routes every reply id to
 * bt_hci_ke_task_handler. */
static uint8_t hci_inst_state[1];
static int hci_msg_thunk(uint16_t id, void *param, uint16_t dest, uint16_t src)
{
    (void)dest; (void)src;
    return bt_hci_ke_task_handler(id, (struct bt_ke_msg *)
                                      ((uint8_t *)param - MSG_PARAM));
}
static const struct ke_msg_handler hci_mtab[] = {
    { KE_MSG_DEFAULT, 0, hci_msg_thunk }
};
static const struct ke_state_handler hci_sh = {
    hci_mtab, ARRAYLEN(hci_mtab), 0
};
static const struct ke_task_desc hci_desc = {
    &hci_sh, &hci_sh, hci_inst_state, 1, ARRAYLEN(hci_inst_state)
};

/* Create the HCI task (BT_KE_TASK_HCI) in state 0. */
void bt_hci_task_desc_init(void)
{
    bt_ke_task_create(BT_KE_TASK_HCI, &hci_desc);
    bt_ke_state_set(BT_KE_TASK_HCI, 0);
}

/* Hook slots 0x803bbc..0x803bd8: the controller's HCI command-interface
 * callbacks. The shim builds kernel messages itself, so these are not on
 * its path; install a stub that logs any call. */
static int hci_cmd_hook_stub(uint16_t id, void *param, uint16_t d, uint16_t s)
{
    (void)id; (void)param; (void)d; (void)s;   /* id unused without YP3_DEV */
    dev_log("bt hci: cmd-interface stub r0=%04x from %p", id,
                __builtin_return_address(0));
    return 0;
}
void bt_hci_install_cmd_hooks(void)
{
    static const uint32_t slots[] = {
        0x00803bbc, 0x00803bc0, 0x00803bc4, 0x00803bc8,
        0x00803bcc, 0x00803bd0, 0x00803bd4, 0x00803bd8,
    };
    for (unsigned i = 0; i < ARRAYLEN(slots); i++)
        *(volatile uint32_t *)(uintptr_t)slots[i] =
            (uint32_t)(uintptr_t)hci_cmd_hook_stub;
}

/* ======================================================================
 * bt_drv_* HAL (firmware/export/bt_drv.h).
 * ====================================================================== */
bool bt_drv_open(bt_drv_recv_fn recv)
{
    bt_recv = recv;
    if (!bt_rf_yp3_init()) {           /* clocks, RF tables, calibration */
        bt_recv = NULL;
        return false;
    }
    if (!bt_ctrl_yp3_start()) {
        bt_rf_yp3_off();
        bt_recv = NULL;
        return false;
    }
    return true;
}

void bt_drv_close(void)
{
    bt_ctrl_yp3_stop();
    bt_rf_yp3_off();
    bt_recv = NULL;
}

#ifdef YP3_DEV
/* Used by the dev shell to start the controller without the radio (RF clocks
 * only, no calibration). */
static void bt_hci_yp3_open_noradio(bt_drv_recv_fn recv)
{
    bt_recv = recv;
    bt_ctrl_yp3_start();
}

uint8_t bt_hci_yp3_cmd_credits(void)
{
    return bt_ctrl_yp3_running() ? REG8(GLUE_CMD_CREDIT) : 0;
}

/* Log an H4 packet (first HCI_TRACE_BYTES bytes) with a 1 MHz timestamp. */
#define HCI_TRACE_BYTES 32
static void hci_dev_hex(const char *dir, const void *pkt, size_t len)
{
    const uint8_t *b = pkt;
    char hex[3 * HCI_TRACE_BYTES + 1];
    size_t n = len > HCI_TRACE_BYTES ? HCI_TRACE_BYTES : len;
    for (size_t i = 0; i < n; i++) {
        static const char d[] = "0123456789abcdef";
        hex[i * 3]     = d[b[i] >> 4];
        hex[i * 3 + 1] = d[b[i] & 0xf];
        hex[i * 3 + 2] = ' ';
    }
    hex[n * 3] = '\0';
    dev_log("bt %s t=%lu len=%u: %s", dir,
                (unsigned long)REG(SL6801_COUNTER_LO),
                (unsigned)len, hex);
}

/* Log every H4 packet the controller sends up. */
static void bt_hci_dev_recv(const void *pkt, size_t len)
{
    hci_dev_hex("rx", pkt, len);
}

static int hex_nibble(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/* "bt hci open-noradio | open | close | cmd HEX... | trace on|off". */
bool bt_hci_yp3_dev(char *arg)
{
    if (!strncmp(arg, "trace ", 6)) {
        hci_trace = !strcmp(arg + 6, "on");
        dev_log("bt hci trace %s", hci_trace ? "on" : "off");
    } else if (!strcmp(arg, "open-noradio")) {
        bt_hci_yp3_open_noradio(bt_hci_dev_recv);
        dev_log("bt hci open-noradio: controller up, recv logging on");
    } else if (!strcmp(arg, "open")) {
        bool ok = bt_drv_open(bt_hci_dev_recv);
        dev_log("bt hci open: %s", ok ? "ok" : "FAILED");
    } else if (!strcmp(arg, "close")) {
        bt_drv_close();
    } else if (!strncmp(arg, "cmd ", 4)) {
        uint8_t buf[64];
        size_t n = 0;
        for (const char *s = arg + 4; *s && n < sizeof buf; ) {
            while (*s == ' ') s++;
            int hi = hex_nibble(*s);
            if (hi < 0) break;
            int lo = hex_nibble(s[1]);
            if (lo < 0) break;
            buf[n++] = (hi << 4) | lo;
            s += 2;
        }
        if (n < 1)
            return false;
        dev_log("bt hci cmd: sending %u bytes", (unsigned)n);
        bool ok = bt_drv_send(buf, n);
        dev_log("bt hci cmd: send %s", ok ? "ok" : "rejected");
    } else {
        return false;
    }
    return true;
}
#endif
