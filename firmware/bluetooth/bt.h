/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef BT_H
#define BT_H
/* Internal interface of the Bluetooth host stack (public API: bluetooth.h).
 *
 *   bt.c      thread, power, RX ring, public API, paired devices file
 *   hci.c     HCI commands and events, credits, ACL, security, inquiry
 *   l2cap.c   L2CAP basic mode: signalling, channels
 *   sdp.c     SDP server, and the one query we make (hfp.c)
 *   avdtp.c   AVDTP signalling, A2DP source stream
 *   avrcp.c   AVCTP/AVRCP: passthrough keys, absolute volume
 *   rfcomm.c  RFCOMM: one multiplexer session, one data channel
 *   hfp.c     Hands-Free audio gateway: the headset's battery level
 *   hid.c     HID host: a game controller or keyboard as buttons
 *   media.c   PCM_SINK_BT and the media path (interrupt context)
 *   sbc_enc.c portable SBC encoder (bt_sbc.h), unless the target has one
 *   debug.c   the debug screen's lines (bt_dbg_info)
 *
 * Everything runs in the Bluetooth thread except the driver's receive
 * callback (any context: flow control and the RX ring) and the media tick
 * task, which pulls PCM, encodes SBC and sends media packets on its own.
 * All state lives in one buflib allocation made at power-on. Include this
 * header first.
 *
 * There is one ACL link at a time (bt->link); every L2CAP channel runs on
 * it. Outgoing PDUs are built in bt->tx (thread) or bt->media.buf (media
 * tick) with the payload at BT_HDR, leaving room for the headers that
 * acl_send() writes in front. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "config.h"
#ifdef SIMULATOR
#define LOGF_ENABLE             /* trace the stack in the simulator */
#endif
#include "logf.h"
#if defined(HAVE_DEVLOG) && !defined(SIMULATOR)
/* Development builds: trace the stack into the dev log */
#include "devlog.h"
#undef logf
#define logf(...) dev_log(__VA_ARGS__)
#define BT_MEDIA_STATS          /* media.c: periodic summary */
#elif defined(LOGF_ENABLE)
#define BT_MEDIA_STATS
#endif
#include "kernel.h"
#include "bluetooth.h"
#include "bt_drv.h"
#include "bt_sbc.h"

#define BT_STACK_SIZE   2560    /* thread stack, bytes */
#define BT_RX_RING      2048    /* received H4 packets */
#define BT_MTU          672     /* our receive MTU on every channel */

/* Header sizes; BT_HDR is all of them, in front of an L2CAP payload */
#define H4_HDR          1       /* packet indicator (BT_H4_*) */
#define ACL_HDR         4       /* HCI ACL: handle and flags, data length */
#define L2CAP_HDR       4       /* L2CAP basic frame: length, channel ID */
#define BT_HDR          (H4_HDR + ACL_HDR + L2CAP_HDR)

#define BT_TX_SIZE      (BT_HDR + 400)  /* the thread's outgoing PDUs */
#define BT_MEDIA_SIZE   1024    /* one media packet with all headers */
#define BT_MEDIA_RATE   44100   /* the one rate the PCM sink takes */
#define BT_MAX_PAIRED   8       /* link keys kept (bluetooth.dat) */
#define BT_MAX_FOUND    8       /* inquiry results kept */
#define BT_CHANNELS     6       /* L2CAP channels on the link */
#define LINK_KEY_LEN    16

/* PSMs (Bluetooth Assigned Numbers) */
#define PSM_SDP         0x0001
#define PSM_RFCOMM      0x0003
#define PSM_HID_CTRL    0x0011
#define PSM_HID_INTR    0x0013
#define PSM_AVCTP       0x0017
#define PSM_AVDTP       0x0019

/* Thread messages */
enum {
    BT_EV_RX = 1,           /* RX ring has packets */
    BT_MSG_OFF,
    BT_MSG_SCAN,            /* data: start */
    BT_MSG_CONNECT,         /* address in bt->connect_addr */
    BT_MSG_DISCONNECT,
    BT_MSG_FORGET,          /* address in bt->forget_addr */
    BT_MSG_STREAM,          /* the PCM sink started or stopped (data) */
    BT_MSG_VOLUME,          /* the Rockbox volume changed (avrcp.c) */
};

/* Thread timers (bt->timers[]) */
enum {
    T_SEC,                  /* security: retry after a collision, or give up */
    T_AVDTP,                /* delayed discover, or a signal timed out */
    T_SUSPEND,              /* suspend the stream after idle */
    T_AVRCP,                /* connect AVRCP if the headset did not, or
                             * our AVRCP command timed out */
    T_HFP,                  /* connect hands-free if the headset did not */
    T_PLAY,                 /* poll the play status the headset waits on */
    T_REPAGE,               /* page the device whose link was lost */
    T_ANNOUNCE,             /* post the audio state again: unacknowledged */
    T_RESTART,              /* restart the controller: it kept a link */
#ifdef HAVE_BLUETOOTH_HID
    T_HID,                  /* the input device's channels are not up */
#endif
    T_COUNT
};

/* Link flags */
#define LF_OUTGOING     0x01    /* we paged */
#define LF_AUTH         0x02    /* authenticated */
#define LF_ENCRYPT      0x04    /* encrypted */
#define LF_SEC_BUSY     0x08    /* authentication/encryption in progress */
#define LF_KEY_RETRY    0x10    /* stored key failed once: pair afresh */
#define LF_PERIPHERAL   0x20    /* the remote is central */
#define LF_BONDED       0x40    /* paired before this link, not on it */
#define LF_SILENT       0x80    /* dropped: the remote stopped answering */

/* The ACL link */
struct bt_link {
    bt_addr_t addr;
    uint16_t handle;            /* HCI connection handle */
    uint8_t state;              /* LINK_* */
    uint8_t flags;              /* LF_* */
    uint8_t ident;              /* last L2CAP signalling identifier */
    uint32_t cod;               /* class of device, 0 if unknown */
    char name[BT_NAME_MAX];     /* "" until known */
};
/* LINK_PAGING: being set up (we paged, or accepted its request);
 * LINK_CLOSING: HCI Disconnect sent */
enum { LINK_NONE, LINK_PAGING, LINK_UP, LINK_CLOSING };

/* A paired device. bluetooth.dat holds the array as in memory, so a layout
 * change needs a new KEYS_MAGIC (bt.c). */
struct bt_key {
    bt_addr_t addr;
    uint8_t type;               /* HCI link key type */
    uint8_t key[LINK_KEY_LEN];
    uint32_t cod;
    char name[BT_NAME_MAX];
};

/* An inquiry result, with what paging it needs */
struct bt_found {
    struct bt_device dev;
    uint8_t psrm;               /* page scan repetition mode; bit 7: name
                                 * requested (hci.c) */
    uint16_t clock;             /* clock offset | valid flag (bit 15) */
};

/* L2CAP channel */
struct l2cap_chan {
    uint16_t lcid;              /* our channel ID, 0: free */
    uint16_t rcid;              /* the remote's channel ID */
    uint16_t psm;
    uint16_t rmtu;              /* largest SDU the remote takes */
    uint8_t state;              /* CH_* */
    uint8_t flags;              /* CHF_* */
    uint8_t ident;              /* identifier of the pending request (ours,
                                 * or the remote's while CH_W4_SEC) */
};
/* CH_W4_CONN: our Connection Request sent; CH_W4_SEC: the remote's
 * request waits for the link to be secured; CH_CONFIG: configuring both
 * directions (CHF_*) */
enum { CH_W4_CONN, CH_W4_SEC, CH_CONFIG, CH_OPEN };
#define CHF_OUT_DONE    0x01    /* our configuration accepted */
#define CHF_IN_DONE     0x02    /* theirs accepted */

/* Upper layer events (l2cap -> sdp/avdtp/avrcp): configured both ways,
 * an SDU arrived (p, len), or closed (lcid is 0 by then) */
enum { CHAN_OPEN, CHAN_DATA, CHAN_CLOSED };

/* AVDTP stream; ST_OPENING: Open accepted, the media channel not yet up */
enum { ST_IDLE, ST_CONFIGURED, ST_OPENING, ST_OPEN, ST_STREAMING };
struct avdtp {
    struct l2cap_chan *sig, *media;     /* signalling and media channels */
    uint8_t state;              /* ST_* */
    uint8_t rseid;              /* the sink's SEID */
    uint8_t label;              /* our next transaction label */
    uint8_t pending;            /* signal we wait a response for, 0: none */
    uint8_t plabel;             /* its transaction label */
    uint8_t retries;            /* times it was sent again (queries, our
                                 * signalling channel request) */
    uint8_t cand[16], ncand;    /* sink SEIDs still to try (discovery);
                                 * PipeWire lists more than 4 before SBC */
    bool want;                  /* the PCM sink is playing */
    bool announced;             /* SYS_BT_AUDIO_CONNECTED sent */
    volatile bool acked;        /* the state the apps acknowledged
                                 * (bt_audio_acknowledge(), UI thread) */
    uint8_t restarts;           /* STARTs after the sink suspended */
    uint8_t cfg[4];             /* SBC codec information in use */
    struct bt_sbc_params sbc;
};

/* AVRCP controller (avrcp.c): absolute volume. ABS_ASKED: capabilities
 * or the volume notification requested; ABS_ARMED: registered, our first
 * volume on its way; ABS_ON: the headset takes the volume. */
enum { ABS_NONE, ABS_ASKED, ABS_ARMED, ABS_ON };
struct avrcp {
    uint8_t abs;                /* ABS_* */
    uint8_t label;              /* our next transaction label */
    uint8_t pending;            /* PDU of our command in flight, 0: none */
    uint8_t plabel;             /* its label */
    uint8_t retries;            /* times it was sent again */
    uint8_t nlabel;             /* label of the volume notification */
    uint8_t vol;                /* the headset's volume, 0..127 */
    uint8_t sent;               /* the last volume we sent */
    volatile bool vol_posted;   /* BT_MSG_VOLUME queued */
    bool play_reg;              /* the headset waits for a play status change */
    uint8_t play_label;         /* ...on this label */
    uint8_t play;               /* the play status it was told */
};

/* RFCOMM (rfcomm.c): the session on its L2CAP channel (RS_*) and its one
 * data channel (DLC_*), at DLCI dlci. With credit based flow control the
 * remote may send rx_credits more frames, we tx_credits. */
enum { RS_NONE, RS_W4_L2CAP, RS_W4_UA, RS_W4_SABM, RS_OPEN };
enum { DLC_NONE, DLC_W4_PN, DLC_W4_UA, DLC_OPEN };
struct rfcomm {
    struct l2cap_chan *l2;
    uint8_t state;              /* RS_* */
    bool initiator;             /* we started the session */
    uint8_t dlc;                /* DLC_* */
    uint8_t dlci;
    bool cfc;                   /* credit based flow control */
    uint8_t tx_credits, rx_credits;
    uint16_t mtu;               /* largest information field the remote takes */
};

/* Hands-free audio gateway (hfp.c) */
#define HFP_LINE_MAX    64      /* AT command line kept, with its NUL */
struct hfp {
    bool tried;                 /* we looked for the headset's HF service */
    uint8_t hf_channel;         /* its RFCOMM channel (SDP), 0: none */
    bool battery_ok;            /* the headset reported battery */
    uint8_t battery;            /* percent */
    uint8_t len;                /* bytes in line; HFP_LINE_MAX: too long */
    char line[HFP_LINE_MAX];
};

/* HID host (hid.c): the input device's channels and the keys it holds
 * (K_* bits) */
struct hid {
    struct l2cap_chan *ctrl, *intr;     /* control, interrupt */
    bool up;                    /* both channels open */
    uint16_t keys;              /* held */
    uint16_t stick;             /* held by the left stick */
};

/* Media path (media.c), shared with the tick task */
struct bt_media {
    /* (the PCM sink's own state is in media.c statics: the sink may be
     * selected while Bluetooth is off) */
    volatile bool running;      /* tick task sends */
    long t0;                    /* pacing: samples sent since tick t0 */
    int32_t sent;
    uint16_t seq;               /* RTP sequence number */
    uint32_t ts;                /* RTP timestamp */
    uint16_t cid;               /* the sink's media channel ID */
    uint16_t frame_len, frame_samples;  /* SBC frame: bytes, samples */
    uint16_t mtu;               /* largest media packet (RTP header on) */
    uint8_t frames;             /* SBC frames per packet */
    uint8_t nframes;            /* frames in buf so far */
    uint8_t bitpool, bp_min, bp_max;    /* adapted to the link */
    bool bp_rose;               /* the last change was a step up */
    long bp_tick;               /* last bitpool change */
    long calm;                  /* last time the link fell behind */
    long calm_wait;             /* kept up this long: step up */
    long busy;                  /* last time many ACL buffers were in use */
    int32_t behind;             /* samples behind at the last tick */
    /* since power-on (the debug screen; BT_MEDIA_STATS logs their growth) */
    uint32_t packets;           /* media packets sent */
    uint32_t waits;             /* ticks that waited for ACL buffers */
    uint16_t restarts;          /* clock restarts: fell too far behind */
#ifdef BT_MEDIA_STATS
    long st_tick;               /* start of the summary period */
    int32_t st_behind;          /* most samples behind in it */
    uint32_t st_packets, st_waits;  /* the counts above at its start */
    uint16_t st_restarts;
    uint8_t st_credits;         /* fewest free ACL buffers seen */
#endif
    int16_t pcm_stage[16 * 8 * 2];  /* one frame of PCM: 16 blocks x 8
                                     * subbands x 2 channels at most */
    uint8_t buf[BT_MEDIA_SIZE]; /* the media packet, payload at BT_HDR */
};

struct bt_state {
    long stack[BT_STACK_SIZE / sizeof(long)];
    struct event_queue queue;
    struct semaphore ready;     /* power-on result */
    struct semaphore credit;    /* a credit came back */
    unsigned int thread;
    bool up;                    /* controller initialised */
    bool usb;                   /* storage is with USB: defer writes */
    bool keys_dirty;            /* keys[] differ from the file */
    volatile bool rx_posted;    /* BT_EV_RX queued, ring not yet drained */
    volatile bool tx_busy;      /* the thread is sending ACL fragments */
    volatile uint8_t cmd_credits;   /* commands the controller takes now */
    volatile uint16_t acl_credits;  /* free controller ACL buffers */
    uint16_t acl_total, acl_mtu;    /* controller ACL buffers: count, size */
    uint8_t init_step;          /* next set-up command (hci.c) */
    uint16_t init_op;           /* set-up command we wait for */
    uint8_t scan;               /* SCAN_* */
    bt_addr_t local;            /* our BD_ADDR */
    bt_addr_t connect_addr;     /* BT_MSG_CONNECT argument */
    bt_addr_t forget_addr;      /* BT_MSG_FORGET argument */
    bool connect_pending;       /* connect once the old link is down */
    bt_addr_t lost_addr;        /* T_REPAGE: the device whose link was lost */
    uint8_t repages;            /* ...paged this often since */
    long timers[T_COUNT];       /* deadlines (current_tick), 0: off */

    struct bt_link link;
    struct l2cap_chan chan[BT_CHANNELS];
    uint16_t next_cid;          /* the next local CID l2cap tries */
    struct avdtp av;
    struct l2cap_chan *avctp;   /* AVCTP channel (AVRCP) */
    struct avrcp avrcp;
    struct rfcomm rfcomm;
    struct hfp hfp;
#ifdef HAVE_BLUETOOTH_HID
    struct hid hid;
#endif
    struct l2cap_chan *sdp_query;   /* our SDP request's channel */
    char code[BT_CODE_MAX];     /* to type on a keyboard to pair, or "" */

    uint8_t nkeys, nfound;
    struct bt_key keys[BT_MAX_PAIRED];  /* most recently used first */
    struct bt_found found[BT_MAX_FOUND];
#ifdef HAVE_BLUETOOTH_DRV_CAL
    uint32_t cal[BT_DRV_CAL_MAX / sizeof(uint32_t)];   /* lent to the driver */
    uint16_t cal_len;           /* calibration bytes in cal[] */
    bool cal_dirty;             /* cal[] differs from the file */
#endif

    volatile uint16_t rx_head, rx_tail; /* RX ring offsets (bt.c) */
    uint16_t rx_pdu_len, rx_pdu_want;   /* ACL reassembly */
    uint8_t rx_ring[BT_RX_RING];
    uint8_t rx_pdu[BT_MTU + L2CAP_HDR]; /* the L2CAP PDU being reassembled */
    uint8_t tx[BT_TX_SIZE];     /* the thread's outgoing PDU */
    struct bt_media media;
};
/* SCAN_INQUIRY: inquiry running; SCAN_NAMES: asking found devices' names */
enum { SCAN_IDLE, SCAN_INQUIRY, SCAN_NAMES };

extern struct bt_state *bt;

/* bt.c */
void bt_init_done(bool ok);         /* controller set-up finished */
void bt_restart(void);              /* driver closed and opened, set up again */
void bt_timer(int t, long ticks);   /* ticks 0: cancel */
void bt_update_status(void);
struct bt_key *bt_key_find(const bt_addr_t *addr);
void bt_key_store(const bt_addr_t *addr, const uint8_t *key, uint8_t type);
void bt_key_used(const bt_addr_t *addr);
void bt_key_forget(const bt_addr_t *addr);
void bt_name_learned(const bt_addr_t *addr, const char *name, size_t max);
const char *bt_name_of(const bt_addr_t *addr);

/* hci.c */
void hci_start(void);
void hci_event(const uint8_t *p, size_t len);
void hci_acl(const uint8_t *p, size_t len);
void hci_scan(bool start);
void hci_connect(const bt_addr_t *addr);
void hci_disconnect(void);
void hci_drop_silent(void);         /* disconnect: the remote stopped answering */
void hci_power_off(void);           /* disconnect: we are powering off */
void hci_secure(void);
void hci_sec_timer(void);
void hci_repage_timer(void);
void hci_restart_timer(void);
bool hci_rx_flow(const uint8_t *p, size_t len);  /* driver context */
/* Send the len-byte L2CAP payload at buf + BT_HDR on channel cid */
bool acl_send(uint8_t *buf, uint16_t cid, size_t len, bool media);

/* l2cap.c */
void l2cap_rx(const uint8_t *pdu, size_t len);
void l2cap_link_up(void);
void l2cap_link_secured(bool ok);
void l2cap_link_down(void);
struct l2cap_chan *l2cap_open(uint16_t psm);
void l2cap_close(struct l2cap_chan *c);
void l2cap_send(struct l2cap_chan *c, size_t len);  /* bt->tx + BT_HDR */

/* sdp.c, avdtp.c, avrcp.c, rfcomm.c: channel events */
void sdp_event(struct l2cap_chan *c, int ev, const uint8_t *p, size_t len);
void avdtp_event(struct l2cap_chan *c, int ev, const uint8_t *p, size_t len);
void avrcp_event(struct l2cap_chan *c, int ev, const uint8_t *p, size_t len);
void rfcomm_event(struct l2cap_chan *c, int ev, const uint8_t *p, size_t len);
void sdp_find_hf(void);
bool avdtp_accept(void);
bool avdtp_close(void);
void avdtp_link_secured(void);
void avdtp_timer(int t);
void avdtp_announce_timer(void);
void avdtp_stream_want(bool play);
#define REPAGE_STOP 20   /* re-pages of a lost link: about 5 minutes */
void avrcp_timer(void);
void avrcp_play_timer(void);
void avrcp_volume(void);

/* rfcomm.c: hfp.c's data channel. rfcomm_send() sends the len bytes it
 * wrote at rfcomm_data(), at most RFCOMM_DATA_MAX. */
#define RFCOMM_DATA_MAX 192
bool rfcomm_connect(uint8_t channel);
uint8_t *rfcomm_data(void);
void rfcomm_send(size_t len);

/* hfp.c */
#define HFP_AG_CHANNEL  1       /* our RFCOMM server channel (SDP record) */
void hfp_link_up(void);
void hfp_stream_opened(void);
void hfp_timer(void);
void hfp_sdp_done(void);
void hfp_open(void);
void hfp_rx(const uint8_t *p, size_t len);
void hfp_closed(void);

/* hid.c */
#ifdef HAVE_BLUETOOTH_HID
bool hid_device(void);              /* the link is to an input device */
bool hid_accept(uint16_t psm);
void hid_event(struct l2cap_chan *c, int ev, const uint8_t *p, size_t len);
void hid_link_secured(void);
void hid_timer(void);
#define HID_KEYBOARD(cod)   BT_COD_KEYBOARD(cod)    /* pairs with a code */
#else
#define hid_device()        false
#define HID_KEYBOARD(cod)   false
#endif

/* media.c */
bool media_open(void);
void media_close(void);
void media_start(void);
void media_stop(void);

/* byte order helpers */
static inline uint16_t get_le16(const uint8_t *p) { return p[0] | p[1] << 8; }
static inline uint16_t get_be16(const uint8_t *p) { return p[0] << 8 | p[1]; }
static inline uint32_t get_le32(const uint8_t *p)
{ return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }
static inline uint32_t get_be32(const uint8_t *p)
{ return (uint32_t)p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3]; }
static inline void put_le16(uint8_t *p, uint16_t v) { p[0] = v; p[1] = v >> 8; }
static inline void put_be16(uint8_t *p, uint16_t v) { p[0] = v >> 8; p[1] = v; }
static inline void put_le32(uint8_t *p, uint32_t v)
{ p[0] = v; p[1] = v >> 8; p[2] = v >> 16; p[3] = v >> 24; }
static inline void put_be32(uint8_t *p, uint32_t v)
{ p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v; }

#endif /* BT_H */
