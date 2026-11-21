/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SDP server (Core spec Vol 3, Part B): the A2DP source, AVRCP target
 * (1.3, for the play status) and controller (1.4, for absolute volume)
 * and hands-free audio gateway records. Answers service search, service
 * attribute and service search attribute requests from static records;
 * long attribute lists go out in parts, the continuation state being the
 * offset reached. As a client we only ask for the headset's hands-free
 * RFCOMM channel (hfp.c). */
#include "bt.h"
#include <string.h>

/* PDU IDs */
#define SDP_ERROR_RSP           0x01
#define SDP_SEARCH_REQ          0x02
#define SDP_SEARCH_RSP          0x03
#define SDP_ATTR_REQ            0x04
#define SDP_ATTR_RSP            0x05
#define SDP_SEARCH_ATTR_REQ     0x06
#define SDP_SEARCH_ATTR_RSP     0x07

/* PDU header: PDU ID, transaction ID, parameter length */
#define SDP_HDR                 5

/* Error codes */
#define ERR_BAD_HANDLE          0x0002
#define ERR_BAD_SYNTAX          0x0003
#define ERR_BAD_CONT            0x0005

/* Data elements: a header byte, type << 3 | size index, then for size
 * index 5, 6 or 7 an 8, 16 or 32-bit length, then the data. Size index
 * 0..4 means 1, 2, 4, 8 or 16 data bytes (none for nil). */
#define DE_TYPE_SHIFT           3
#define DE_SIZE_MASK            7
#define DE_NIL                  0
#define DE_UINT                 1
#define DE_UUID                 3
#define DE_SEQ                  6
#define DE_SIZE_1               0
#define DE_SIZE_2               1
#define DE_SIZE_4               2
#define DE_SIZE_LEN8            5
#define DE_SIZE_LEN16           6
#define DE_HDR(type, size)      ((type) << DE_TYPE_SHIFT | (size))
#define DE_UINT8                DE_HDR(DE_UINT, DE_SIZE_1)
#define DE_UINT16               DE_HDR(DE_UINT, DE_SIZE_2)
#define DE_UINT32               DE_HDR(DE_UINT, DE_SIZE_4)
#define DE_UUID16               DE_HDR(DE_UUID, DE_SIZE_2)
#define DE_SEQ8                 DE_HDR(DE_SEQ, DE_SIZE_LEN8)
#define DE_SEQ16                DE_HDR(DE_SEQ, DE_SIZE_LEN16)
#define SEQ16_HDR               3       /* DE_SEQ16 and its length */

/* Data elements for the records */
#define U8(v)       DE_UINT8, (v)
#define U16(v)      DE_UINT16, (v) >> 8, (v) & 0xff
#define U32(v)      DE_UINT32, (v) >> 24, ((v) >> 16) & 0xff, \
                    ((v) >> 8) & 0xff, (v) & 0xff
#define UUID16(v)   DE_UUID16, (v) >> 8, (v) & 0xff
#define SEQ(n)      DE_SEQ8, (n)

/* Attribute IDs */
#define ATTR_RECORD_HANDLE      0x0000  /* ServiceRecordHandle */
#define ATTR_CLASS_ID_LIST      0x0001  /* ServiceClassIDList */
#define ATTR_PROTOCOL_LIST      0x0004  /* ProtocolDescriptorList */
#define ATTR_BROWSE_GROUP_LIST  0x0005  /* BrowseGroupList */
#define ATTR_PROFILE_LIST       0x0009  /* BluetoothProfileDescriptorList */
#define ATTR_NETWORK            0x0301  /* Network (HFP: call rejection) */
#define ATTR_FEATURES           0x0311  /* SupportedFeatures */
#define ATTR_ID_LEN             3       /* U16(attribute ID) */

/* 16-bit UUIDs (Assigned Numbers) */
#define UUID_RFCOMM             0x0003
#define UUID_AVCTP              0x0017
#define UUID_AVDTP              0x0019
#define UUID_L2CAP              0x0100
#define UUID_PUBLIC_BROWSE      0x1002  /* PublicBrowseRoot */
#define UUID_AUDIO_SOURCE       0x110a
#define UUID_AV_REMOTE_TARGET   0x110c  /* A/V_RemoteControlTarget */
#define UUID_A2DP               0x110d  /* AdvancedAudioDistribution */
#define UUID_AV_REMOTE          0x110e  /* A/V_RemoteControl */
#define UUID_HANDSFREE          0x111e
#define UUID_HANDSFREE_AG       0x111f  /* HandsfreeAudioGateway */
#define UUID_GENERIC_AUDIO      0x1203

/* Protocol and profile versions, major << 8 | minor */
#define VER_AVDTP               0x0103
#define VER_A2DP                0x0103
#define VER_AVCTP               0x0102  /* with AVRCP 1.3 */
#define VER_AVRCP               0x0103  /* play status */
#define VER_AVCTP_CT            0x0103  /* with AVRCP 1.4 */
#define VER_AVRCP_CT            0x0104  /* absolute volume */
#define VER_HFP                 0x0107  /* HF indicators */

/* SupportedFeatures: A2DP source player; AVRCP category 1, the controller
 * also category 2 (absolute volume); the audio gateway none */
#define FEAT_A2DP               0x0001
#define FEAT_AVRCP_TG           0x0001
#define FEAT_AVRCP_CT           0x0003
#define FEAT_HFP_AG             0x0000

#define HANDLE_A2DP             0x00010001
#define HANDLE_AVRCP_TG         0x00010002
#define HANDLE_AVRCP_CT         0x00010003
#define HANDLE_HFP_AG           0x00010004
#define HANDLE_LEN              4
#define REC_HANDLE  (ATTR_ID_LEN + 1)   /* offset of a record's handle */

/* Each record: attribute ID / value pairs in ascending ID order. Sequence
 * lengths count bytes: U16() and UUID16() take 3 each, SEQ() 2. */
#define RECORD(handle, class, psm, proto, proto_ver, profile, profile_ver, \
               features)                                                \
    U16(ATTR_RECORD_HANDLE), U32(handle),                               \
    U16(ATTR_CLASS_ID_LIST), SEQ(3), UUID16(class),                     \
    U16(ATTR_PROTOCOL_LIST), SEQ(16),                                   \
        SEQ(6), UUID16(UUID_L2CAP), U16(psm),                           \
        SEQ(6), UUID16(proto), U16(proto_ver),                          \
    U16(ATTR_BROWSE_GROUP_LIST), SEQ(3), UUID16(UUID_PUBLIC_BROWSE),    \
    U16(ATTR_PROFILE_LIST), SEQ(8), SEQ(6), UUID16(profile),            \
        U16(profile_ver),                                               \
    U16(ATTR_FEATURES), U16(features)

static const uint8_t rec_a2dp[] = {
    RECORD(HANDLE_A2DP, UUID_AUDIO_SOURCE, PSM_AVDTP, UUID_AVDTP, VER_AVDTP,
           UUID_A2DP, VER_A2DP, FEAT_A2DP)
};
static const uint8_t rec_avrcp_tg[] = {
    RECORD(HANDLE_AVRCP_TG, UUID_AV_REMOTE_TARGET, PSM_AVCTP, UUID_AVCTP,
           VER_AVCTP, UUID_AV_REMOTE, VER_AVRCP, FEAT_AVRCP_TG)
};
static const uint8_t rec_avrcp_ct[] = {
    RECORD(HANDLE_AVRCP_CT, UUID_AV_REMOTE, PSM_AVCTP, UUID_AVCTP,
           VER_AVCTP_CT, UUID_AV_REMOTE, VER_AVRCP_CT, FEAT_AVRCP_CT)
};
static const uint8_t rec_hfp_ag[] = {
    U16(ATTR_RECORD_HANDLE), U32(HANDLE_HFP_AG),
    U16(ATTR_CLASS_ID_LIST), SEQ(6), UUID16(UUID_HANDSFREE_AG),
        UUID16(UUID_GENERIC_AUDIO),
    U16(ATTR_PROTOCOL_LIST), SEQ(12),
        SEQ(3), UUID16(UUID_L2CAP),
        SEQ(5), UUID16(UUID_RFCOMM), U8(HFP_AG_CHANNEL),
    U16(ATTR_BROWSE_GROUP_LIST), SEQ(3), UUID16(UUID_PUBLIC_BROWSE),
    U16(ATTR_PROFILE_LIST), SEQ(8), SEQ(6), UUID16(UUID_HANDSFREE),
        U16(VER_HFP),
    U16(ATTR_NETWORK), U8(0),
    U16(ATTR_FEATURES), U16(FEAT_HFP_AG),
};

#define RECORD_UUIDS 6

static const struct {
    const uint8_t *attrs;
    uint8_t len;
    uint16_t uuids[RECORD_UUIDS];   /* every UUID in the record */
} records[] = {
    { rec_a2dp, sizeof rec_a2dp,
      { UUID_AUDIO_SOURCE, UUID_L2CAP, UUID_AVDTP, UUID_PUBLIC_BROWSE,
        UUID_A2DP } },
    { rec_avrcp_tg, sizeof rec_avrcp_tg,
      { UUID_AV_REMOTE_TARGET, UUID_L2CAP, UUID_AVCTP, UUID_PUBLIC_BROWSE,
        UUID_AV_REMOTE } },
    { rec_avrcp_ct, sizeof rec_avrcp_ct,
      { UUID_AV_REMOTE, UUID_L2CAP, UUID_AVCTP, UUID_PUBLIC_BROWSE } },
    { rec_hfp_ag, sizeof rec_hfp_ag,
      { UUID_HANDSFREE_AG, UUID_GENERIC_AUDIO, UUID_L2CAP, UUID_RFCOMM,
        UUID_PUBLIC_BROWSE, UUID_HANDSFREE } },
};
#define NRECORDS ((int)(sizeof records / sizeof records[0]))

#define MAX_UUIDS   12      /* in a service search pattern */
#define MAX_RANGES  8       /* in an attribute ID list */
#define LIST_SIZE   320     /* every record with every attribute fits */

/* Our continuation state: 2 bytes, the offset reached in the list */
#define CONT_LEN    2

/* Size of the data element at p (header + data), 0 if malformed; *hdr
 * gets the header size */
static size_t de_size(const uint8_t *p, size_t avail, size_t *hdr)
{
    if (!avail)
        return 0;
    unsigned type = p[0] >> DE_TYPE_SHIFT, idx = p[0] & DE_SIZE_MASK;
    size_t h = 1, n;
    if (type == DE_NIL)
        n = 0;
    else if (idx < DE_SIZE_LEN8)
        n = 1u << idx;
    else {
        h = 1 + (1u << (idx - DE_SIZE_LEN8));
        if (avail < h)
            return 0;
        n = idx == DE_SIZE_LEN8 ? p[1] :
            idx == DE_SIZE_LEN16 ? get_be16(p + 1) : get_be32(p + 1);
    }
    if (n > avail - h)
        return 0;
    *hdr = h;
    return h + n;
}

/* Parse a data element sequence; returns its total size, 0 if bad */
static size_t de_seq(const uint8_t *p, size_t avail, const uint8_t **body,
                     size_t *len)
{
    size_t hdr, n = de_size(p, avail, &hdr);
    if (!n || (p[0] >> DE_TYPE_SHIFT) != DE_SEQ)
        return 0;
    *body = p + hdr;
    *len = n - hdr;
    return n;
}

/* The Bluetooth Base UUID 00000000-0000-1000-8000-00805F9B34FB after its
 * first 4 bytes, which hold a 16 or 32-bit UUID */
static const uint8_t base_uuid[12] = {
    0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb
};

/* Service search pattern -> 32-bit UUIDs (~0 for non-base 128-bit ones) */
static int parse_uuids(const uint8_t *p, size_t len, uint32_t *uuids)
{
    int n = 0;
    size_t hdr, sz;
    for (; len; p += sz, len -= sz) {
        if (!(sz = de_size(p, len, &hdr)) ||
            (p[0] >> DE_TYPE_SHIFT) != DE_UUID || n == MAX_UUIDS)
            return -1;
        size_t ulen = sz - hdr;
        const uint8_t *u = p + hdr;
        if (ulen == 2)
            uuids[n++] = get_be16(u);
        else if (ulen == 4)
            uuids[n++] = get_be32(u);
        else if (ulen == 16)
            uuids[n++] = memcmp(u + 4, base_uuid, sizeof base_uuid) ?
                         ~0u : get_be32(u);
        else
            return -1;
    }
    return n;
}

/* Every UUID of the pattern is in the record */
static bool record_matches(int r, const uint32_t *uuids, int n)
{
    for (int i = 0; i < n; i++) {
        int j = 0;
        while (j < RECORD_UUIDS && records[r].uuids[j] != uuids[i])
            j++;
        if (j == RECORD_UUIDS)
            return false;
    }
    return n > 0;
}

/* Attribute ID list -> ranges. Each element is an ID (uint16) or a range
 * (uint32: first ID << 16 | last ID). */
static int parse_ranges(const uint8_t *p, size_t len, uint16_t (*ranges)[2])
{
    int n = 0;
    size_t hdr, sz;
    for (; len; p += sz, len -= sz) {
        if (!(sz = de_size(p, len, &hdr)) || n == MAX_RANGES)
            return -1;
        if (p[0] == DE_UINT16) {
            ranges[n][0] = ranges[n][1] = get_be16(p + 1);
        } else if (p[0] == DE_UINT32) {
            ranges[n][0] = get_be16(p + 1);
            ranges[n][1] = get_be16(p + 3);
        } else {
            return -1;
        }
        n++;
    }
    return n;
}

/* One record's attribute list (a sequence) filtered by the ranges */
static size_t put_record(uint8_t *out, int r, uint16_t (*ranges)[2], int nr)
{
    const uint8_t *a = records[r].attrs, *end = a + records[r].len;
    size_t n = SEQ16_HDR, hdr;
    while (a < end) {
        uint16_t id = get_be16(a + 1);
        size_t sz = ATTR_ID_LEN +
                    de_size(a + ATTR_ID_LEN, end - a - ATTR_ID_LEN, &hdr);
        for (int i = 0; i < nr; i++) {
            if (id >= ranges[i][0] && id <= ranges[i][1]) {
                memcpy(out + n, a, sz);
                n += sz;
                break;
            }
        }
        a += sz;
    }
    out[0] = DE_SEQ16;
    put_be16(out + 1, n - SEQ16_HDR);
    return n;
}

/* Send a PDU with len bytes of parameters, already at bt->tx + BT_HDR +
 * SDP_HDR */
static void sdp_send(struct l2cap_chan *c, uint8_t pdu, uint16_t tid, size_t len)
{
    uint8_t *o = bt->tx + BT_HDR;
    o[0] = pdu;
    put_be16(o + 1, tid);
    put_be16(o + 3, len);
    l2cap_send(c, len + SDP_HDR);
}

static void sdp_error(struct l2cap_chan *c, uint16_t tid, uint16_t err)
{
    put_be16(bt->tx + BT_HDR + SDP_HDR, err);
    sdp_send(c, SDP_ERROR_RSP, tid, 2);
}

/* Send part of an attribute list, continuing at the offset in cont: the
 * byte count (2 bytes), the bytes, then the continuation state (its length,
 * 0 or CONT_LEN, and the offset) */
static void send_list(struct l2cap_chan *c, uint8_t pdu, uint16_t tid,
                      const uint8_t *list, size_t total, size_t max,
                      const uint8_t *cont)
{
    uint8_t *o = bt->tx + BT_HDR + SDP_HDR;
    size_t off = 0;
    if (cont[0] == CONT_LEN)
        off = get_be16(cont + 1);
    else if (cont[0])
        off = ~0u;
    if (off > total) {
        sdp_error(c, tid, ERR_BAD_CONT);
        return;
    }
    size_t room = MIN((size_t)c->rmtu, BT_TX_SIZE - BT_HDR) - SDP_HDR - 2 -
                  (1 + CONT_LEN);
    size_t n = MIN(MIN(total - off, max), room);
    put_be16(o, n);
    memcpy(o + 2, list + off, n);
    o += 2 + n;
    if (off + n < total) {
        o[0] = CONT_LEN;
        put_be16(o + 1, off + n);
    } else {
        o[0] = 0;
    }
    sdp_send(c, pdu, tid, 2 + n + (o[0] ? 1 + CONT_LEN : 1));
}

/* One request PDU */
static void sdp_request(struct l2cap_chan *c, const uint8_t *p, size_t len)
{
    uint32_t uuids[MAX_UUIDS];
    uint16_t ranges[MAX_RANGES][2];
    uint8_t list[LIST_SIZE];
    const uint8_t *seq, *rest;
    size_t seqlen, sz, total = SEQ16_HDR, max;
    int nu = 0, nr, r;

    if (len < SDP_HDR)
        return;
    uint8_t pdu = p[0];
    uint16_t tid = get_be16(p + 1);
    size_t plen = get_be16(p + 3);
    p += SDP_HDR;
    if (plen > len - SDP_HDR)
        goto bad;
    len = plen;

    /* service search pattern or service record handle */
    if (pdu == SDP_SEARCH_REQ || pdu == SDP_SEARCH_ATTR_REQ) {
        if (!(sz = de_seq(p, len, &seq, &seqlen)) ||
            (nu = parse_uuids(seq, seqlen, uuids)) < 0)
            goto bad;
    } else if (pdu == SDP_ATTR_REQ && len >= HANDLE_LEN) {
        sz = HANDLE_LEN;
    } else {
        goto bad;
    }
    /* maximum record count or attribute byte count (2 bytes), then (the
     * attribute ID list and) the continuation state, at least 1 byte */
    rest = p + sz;
    len -= sz;
    if (len < 3)
        goto bad;
    max = get_be16(rest);

    if (pdu == SDP_SEARCH_REQ) {
        /* total and current record count, the handles, no continuation */
        uint8_t *o = bt->tx + BT_HDR + SDP_HDR;
        int n = 0;
        for (r = 0; r < NRECORDS; r++)
            if (record_matches(r, uuids, nu) && n < (int)max)
                put_be32(o + 4 + HANDLE_LEN * n++,
                         get_be32(records[r].attrs + REC_HANDLE));
        put_be16(o, n);
        put_be16(o + 2, n);
        o[4 + HANDLE_LEN * n] = 0;
        sdp_send(c, SDP_SEARCH_RSP, tid, 5 + HANDLE_LEN * n);
        return;
    }

    /* attribute ID list, then the continuation state */
    if (!(sz = de_seq(rest + 2, len - 2, &seq, &seqlen)) ||
        (nr = parse_ranges(seq, seqlen, ranges)) < 0 || len - 2 - sz < 1)
        goto bad;
    const uint8_t *cont = rest + 2 + sz;

    if (pdu == SDP_ATTR_REQ) {
        uint32_t handle = get_be32(p);
        for (r = 0; r < NRECORDS; r++)
            if (get_be32(records[r].attrs + REC_HANDLE) == handle)
                break;
        if (r == NRECORDS) {
            sdp_error(c, tid, ERR_BAD_HANDLE);
            return;
        }
        total = put_record(list, r, ranges, nr);
        send_list(c, SDP_ATTR_RSP, tid, list, total, max, cont);
        return;
    }
    /* service search attribute: a sequence of the matching records' lists */
    for (r = 0; r < NRECORDS; r++)
        if (record_matches(r, uuids, nu))
            total += put_record(list + total, r, ranges, nr);
    list[0] = DE_SEQ16;
    put_be16(list + 1, total - SEQ16_HDR);
    send_list(c, SDP_SEARCH_ATTR_RSP, tid, list, total, max, cont);
    return;
bad:
    sdp_error(c, tid, ERR_BAD_SYNTAX);
}

/* Client: a service search attribute request for the hands-free service's
 * protocol descriptor list; its RFCOMM channel is all we take from the
 * answer (one part is enough), then the channel closes */
#define QUERY_TID   1
#define QUERY_MAX   0x0100      /* attribute bytes we take */

void sdp_find_hf(void)
{
    if (!bt->sdp_query)
        bt->sdp_query = l2cap_open(PSM_SDP);
}

static void query_send(struct l2cap_chan *c)
{
    static const uint8_t req[] = {
        SEQ(3), UUID16(UUID_HANDSFREE),
        QUERY_MAX >> 8, QUERY_MAX & 0xff,
        SEQ(3), U16(ATTR_PROTOCOL_LIST),
        0,                              /* no continuation state */
    };
    memcpy(bt->tx + BT_HDR + SDP_HDR, req, sizeof req);
    sdp_send(c, SDP_SEARCH_ATTR_REQ, QUERY_TID, sizeof req);
}

/* The RFCOMM protocol descriptor: its UUID, then the channel (uint8) */
static void query_rsp(const uint8_t *p, size_t len)
{
    static const uint8_t rfcomm[] = { UUID16(UUID_RFCOMM), DE_UINT8 };
    if (len < SDP_HDR || p[0] != SDP_SEARCH_ATTR_RSP) {
        logf("bt: hfp sdp answer %02x", len ? p[0] : 0);
        return;
    }
    for (size_t i = SDP_HDR; i + sizeof rfcomm < len; i++) {
        if (!memcmp(p + i, rfcomm, sizeof rfcomm)) {
            bt->hfp.hf_channel = p[i + sizeof rfcomm];
            logf("bt: hfp sdp: hf channel %u", bt->hfp.hf_channel);
            return;
        }
    }
    logf("bt: hfp sdp: no hf channel (%u bytes)", (unsigned)len);
}

void sdp_event(struct l2cap_chan *c, int ev, const uint8_t *p, size_t len)
{
    if (c != bt->sdp_query) {
        if (ev == CHAN_DATA)
            sdp_request(c, p, len);
        return;
    }
    switch (ev) {
    case CHAN_OPEN:
        query_send(c);
        break;
    case CHAN_DATA:
        query_rsp(p, len);
        l2cap_close(c);
        break;
    case CHAN_CLOSED:
        bt->sdp_query = NULL;
        hfp_sdp_done();
        break;
    }
}
