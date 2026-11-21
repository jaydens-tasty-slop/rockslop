/* SPDX-License-Identifier: GPL-2.0-or-later */
/* The debug screen's lines (bt_dbg_info): the controller, the link, the
 * A2DP stream and its media path, the profiles and the L2CAP channels, as
 * the stack has them; most fit 21 characters (128 pixels of system font).
 * Read without locking from the caller's thread: a value may be a moment
 * old or torn between two updates, which a debug screen can live with. */
#include "bt.h"

static const char *const status_names[] = {
    "off", "on", "connected", "streaming"
};
static const char *const link_names[] = { "none", "paging", "up", "closing" };
static const char *const stream_names[] = {
    "idle", "configured", "opening", "open", "streaming"
};
static const char *const freq_names[] = { "16", "32", "44.1", "48" };
static const char *const mode_names[] = { "mono", "dual", "stereo", "joint" };
static const char *const abs_names[] = { "off", "asked", "armed", "on" };

static const char *psm_name(const struct bt_state *s,
                            const struct l2cap_chan *c)
{
    switch (c->psm) {
    case PSM_SDP:       return "SDP";
    case PSM_RFCOMM:    return "RFCOMM";
    case PSM_HID_CTRL:  return "HID ctrl";
    case PSM_HID_INTR:  return "HID intr";
    case PSM_AVCTP:     return "AVRCP";
    case PSM_AVDTP:     return c == s->av.media ? "Media" : "AVDTP";
    default:            return "?";
    }
}

static void link_info(const struct bt_state *s, bt_dbg_line_fn line)
{
    const struct bt_link *l = &s->link;
    char addr[BT_ADDR_STR];

    if (l->state == LINK_NONE) {
        if (s->timers[T_REPAGE]) {
            bt_addr_format(&s->lost_addr, addr);
            line("Re-paging (%u):", s->repages);
            line("%s", addr);
        }
        return;
    }
    line("Device: %s", l->name[0] ? l->name : "?");
    bt_addr_format(&l->addr, addr);
    line("%s", addr);
    line("COD %06lx handle %x", (unsigned long)l->cod, l->handle);
    line("Link %s, %s, %s", link_names[l->state],
         l->flags & LF_OUTGOING ? "out" : "in",
         l->flags & LF_PERIPHERAL ? "periph" : "central");
    if (l->flags & (LF_AUTH | LF_ENCRYPT | LF_BONDED))
        line("Sec:%s%s%s", l->flags & LF_AUTH ? " auth" : "",
             l->flags & LF_ENCRYPT ? " enc" : "",
             l->flags & LF_BONDED ? " bonded" : "");
    else
        line("Sec: none");
}

static void stream_info(const struct bt_state *s, bt_dbg_line_fn line)
{
    const struct avdtp *a = &s->av;
    const struct bt_media *m = &s->media;

    if (a->state == ST_IDLE)
        return;
    line("A2DP %s SEID %u", stream_names[a->state], a->rseid);
    line("SBC %s kHz %s", freq_names[a->sbc.freq & 3],
         mode_names[a->sbc.mode & 3]);
    line("%u blk %u sb %s", a->sbc.blocks, a->sbc.subbands,
         a->sbc.alloc == BT_SBC_SNR ? "SNR" : "loudness");
    line("Bitpool: %u (%u-%u)", m->bitpool, m->bp_min, m->bp_max);
    if (m->frame_samples)
        line("Bitrate: %lu kbit/s", (unsigned long)m->frame_len * 8 *
             BT_MEDIA_RATE / m->frame_samples / 1000);
    if (m->running) {
        line("Pkt %ux%u B, MTU %u", m->frames, m->frame_len, m->mtu);
        line("Behind: %ld ms", (long)m->behind * 1000 / BT_MEDIA_RATE);
    }
    line("Packets sent: %lu", (unsigned long)m->packets);
    line("Buf waits: %lu ticks", (unsigned long)m->waits);
    line("Clock restarts: %u", m->restarts);
    line("Suspend resumes: %u", a->restarts);
}

static void profile_info(const struct bt_state *s, bt_dbg_line_fn line)
{
    if (s->avctp) {
        if (s->avrcp.abs == ABS_ON)
            line("Abs volume: %u/127", s->avrcp.vol);
        else
            line("Abs volume: %s", abs_names[s->avrcp.abs & 3]);
    }
    if (s->link.state == LINK_UP && s->hfp.battery_ok)
        line("Headset battery: %u%%", s->hfp.battery);
#ifdef HAVE_BLUETOOTH_HID
    if (s->hid.ctrl || s->hid.intr)
        line("HID: %s, keys %04x", s->hid.up ? "up" : "opening",
             s->hid.keys | s->hid.stick);
#endif
    for (int i = 0; i < BT_CHANNELS; i++) {
        const struct l2cap_chan *c = &s->chan[i];
        if (c->lcid)
            line("%s %x>%x mtu %u%s", psm_name(s, c), c->lcid, c->rcid,
                 c->rmtu, c->state == CH_OPEN ? "" : " (setup)");
    }
}

void bt_dbg_info(bt_dbg_line_fn line)
{
    struct bt_state *s = bt;
    char addr[BT_ADDR_STR];

    line("Status: %s", status_names[bt_status()]);
    if (!s)
        return;
    bt_addr_format(&s->local, addr);
    line("Own %s", addr);
    line("ACL free %u/%u, %u B", s->acl_credits, s->acl_total, s->acl_mtu);
    line("Paired: %u, found: %u%s", s->nkeys, s->nfound,
         s->scan != SCAN_IDLE ? " (scan)" : "");
#ifdef HAVE_BLUETOOTH_DRV_CAL
    line("Radio cal: %u B", s->cal_len);
#endif
    link_info(s, line);
    if (s->av.acked != s->av.announced)
        line("Audio %s: unacknowledged",
             s->av.announced ? "ready" : "gone");
    stream_info(s, line);
    profile_info(s, line);
}
