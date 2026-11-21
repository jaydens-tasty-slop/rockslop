/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Hands-Free Profile 1.7 audio gateway, only for the headset's battery
 * level: no calls, no voice recognition, no audio (no SCO). The headset
 * sets up the service level connection with its AT commands and then
 * reports its battery as an HF indicator (AT+BIEV, assigned number 2) or
 * Apple's way (AT+XAPL, then AT+IPHONEACCEV). The RFCOMM channel is the
 * headset's, or ours if it has not opened one HFP_WAIT after the stream
 * did: then sdp.c finds its HF channel first, and each of the query and
 * the RFCOMM set-up gets HFP_SETUP. Commands we do not know get OK, those
 * that would start a call or audio ERROR. */
#include "bt.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "system.h"

#define HFP_WAIT        (3 * HZ)
#define HFP_SETUP       (10 * HZ)

/* AG features (+BRSF): HF indicators */
#define AG_FEATURES     0x400
#define HF_IND_BATTERY  2           /* HF indicator assigned number */
#define APPLE_BATTERY   1           /* AT+IPHONEACCEV key: level 0..9 */
#define APPLE_FEATURES  2           /* AT+XAPL: battery reporting */

/* AG indicators: their order is what +CIND? and +CIEV refer to */
#define CIND_TEST "+CIND: (\"service\",(0,1)),(\"call\",(0,1))," \
                  "(\"callsetup\",(0-3)),(\"callheld\",(0-2))," \
                  "(\"signal\",(0-5)),(\"roam\",(0,1)),(\"battchg\",(0-5))"
#define CIND_READ "+CIND: 1,0,0,0,5,0,5"

/* One result line, "\r\n" around it */
static void result(const char *s)
{
    char *o = (char *)rfcomm_data();
    rfcomm_send(snprintf(o, RFCOMM_DATA_MAX + 1, "\r\n%s\r\n", s));
}

static void battery(int percent)
{
    struct hfp *h = &bt->hfp;
    percent = MAX(0, MIN(percent, 100));
    if (!h->battery_ok || h->battery != percent)
        logf("bt: hfp battery %d%%", percent);
    h->battery = percent;
    h->battery_ok = true;
}

/* The next number in a parameter list, skipping one comma */
static int next_int(const char **s)
{
    char *end;
    int v = strtol(*s, &end, 10);
    *s = *end == ',' ? end + 1 : end;
    return v;
}

/* Apple's battery: [count], then count key, value pairs */
static void apple_battery(const char *s)
{
    for (int n = next_int(&s); n > 0 && *s; n--) {
        const char *pair = s;
        int key = next_int(&s), v = next_int(&s);
        if (s == pair)
            break;                      /* not a number */
        if (key == APPLE_BATTERY)
            battery((MAX(0, MIN(v, 9)) + 1) * 10);
    }
}

/* Commands that must fail: calls, voice recognition, audio */
static bool refused(const char *l)
{
    static const char *const no[] = {
        "ATA", "ATD", "AT+CHUP", "AT+BLDN", "AT+BVRA", "AT+BCC", "AT+BCS",
        "AT+VTS", "AT+CHLD",
    };
    for (unsigned i = 0; i < ARRAYLEN(no); i++)
        if (!strncmp(l, no[i], strlen(no[i])))
            return true;
    return false;
}

/* The value of "cmd=value" if l starts with cmd= (cmd given without '='),
 * else NULL */
static const char *set_value(const char *l, const char *cmd)
{
    size_t n = strlen(cmd);
    return strncmp(l, cmd, n) || l[n] != '=' ? NULL : l + n + 1;
}

/* One AT command line from the headset */
static void command(char *l)
{
    char buf[24];
    const char *v;
    for (char *c = l; *c && *c != '='; c++)     /* the name, upper case */
        *c = toupper((unsigned char)*c);

    if (refused(l)) {
        result("ERROR");
        return;
    }
    if (set_value(l, "AT+BRSF")) {
        snprintf(buf, sizeof buf, "+BRSF: %d", AG_FEATURES);
        result(buf);
    } else if (!strcmp(l, "AT+CIND=?")) {
        result(CIND_TEST);
    } else if (!strcmp(l, "AT+CIND?")) {
        result(CIND_READ);
    } else if (set_value(l, "AT+CMER")) {
        logf("bt: hfp service level connection up");
    } else if (!strcmp(l, "AT+BIND=?")) {
        snprintf(buf, sizeof buf, "+BIND: (%d)", HF_IND_BATTERY);
        result(buf);
    } else if (!strcmp(l, "AT+BIND?")) {
        snprintf(buf, sizeof buf, "+BIND: %d,1", HF_IND_BATTERY);
        result(buf);
    } else if ((v = set_value(l, "AT+BIEV"))) {
        if (next_int(&v) == HF_IND_BATTERY)
            battery(next_int(&v));
    } else if (set_value(l, "AT+XAPL")) {
        snprintf(buf, sizeof buf, "+XAPL=iPhone,%d", APPLE_FEATURES);
        result(buf);
    } else if ((v = set_value(l, "AT+IPHONEACCEV"))) {
        apple_battery(v);
    } else if (!strcmp(l, "AT+COPS?")) {
        result("+COPS: 0");
    }
    result("OK");
}

/* Data from the headset: lines end with CR (an LF may follow) */
void hfp_rx(const uint8_t *p, size_t len)
{
    struct hfp *h = &bt->hfp;
    for (; len--; p++) {
        if (*p == '\r' || *p == '\n') {
            if (h->len && h->len < HFP_LINE_MAX) {
                h->line[h->len] = '\0';
                command(h->line);
            }
            h->len = 0;
        } else if (h->len < HFP_LINE_MAX - 1) {
            h->line[h->len++] = *p;
        } else {
            h->len = HFP_LINE_MAX;      /* too long: dropped */
        }
    }
}

void hfp_open(void)
{
    bt->hfp.len = 0;
    bt_timer(T_HFP, 0);
}

void hfp_closed(void)
{
    bt->hfp.battery_ok = false;
}

/* A new link: we may look for the headset's HF service once */
void hfp_link_up(void)
{
    memset(&bt->hfp, 0, sizeof bt->hfp);
}

/* The audio stream is open: give the headset time to connect hands-free */
void hfp_stream_opened(void)
{
    if (!bt->rfcomm.l2 && !bt->hfp.tried)
        bt_timer(T_HFP, HFP_WAIT);
}

/* It has not: find its HF channel. Later: our query or set-up is stuck */
void hfp_timer(void)
{
    struct rfcomm *r = &bt->rfcomm;
    if (bt->link.state != LINK_UP)
        return;
    if (!bt->hfp.tried) {
        if (!r->l2) {
            bt->hfp.tried = true;
            sdp_find_hf();
            bt_timer(T_HFP, HFP_SETUP);
        }
    } else if (bt->sdp_query) {
        logf("bt: hfp sdp query timed out");
        l2cap_close(bt->sdp_query);
    } else if (r->l2 && r->initiator && r->dlc != DLC_OPEN) {
        logf("bt: rfcomm set-up timed out (session %u, dlc %u)",
             r->state, r->dlc);
        l2cap_close(r->l2);
    }
}

/* The SDP query is over (hf_channel 0: no HF service): connect to it */
void hfp_sdp_done(void)
{
    if (bt->hfp.hf_channel && !bt->rfcomm.l2 && bt->link.state == LINK_UP &&
        rfcomm_connect(bt->hfp.hf_channel))
        bt_timer(T_HFP, HFP_SETUP);
}

int bt_battery(void)
{
    struct bt_state *s = bt;
    return s && s->link.state == LINK_UP && s->hfp.battery_ok ?
           s->hfp.battery : -1;
}
