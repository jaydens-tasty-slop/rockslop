/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Simulator Bluetooth controller (bt_drv.h): H4 over TCP to a host
 * process that runs a virtual controller, e.g. yp3dev/bt_peer.py (Bumble).
 * $ROCKBOX_BT_HCI is host:port, default 127.0.0.1:6402. A reader thread
 * delivers packets in the simulator's interrupt context. */
#include <SDL.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "config.h"
#include "debug.h"
#include "system.h"
#include "bt_drv.h"

#define DEFAULT_HCI     "127.0.0.1:6402"
#define HOST_MAX        64          /* host part of $ROCKBOX_BT_HCI, incl. NUL */

/* HCI packet headers after the H4 indicator: event {code, len}, ACL
 * {handle, len u16}, SCO {handle, len} */
#define EVT_HDR         2
#define ACL_HDR         4
#define SCO_HDR         3
#define H4_MAX          (1 + ACL_HDR + 65535)   /* largest H4 packet */

static int sock = -1;
static SDL_Thread *reader;
static bt_drv_recv_fn recv_fn;
static unsigned char rx_buf[H4_MAX];

/* Read exactly n bytes; false on error or end of stream */
static bool read_all(unsigned char *p, size_t n)
{
    while (n) {
        ssize_t r = read(sock, p, n);
        if (r <= 0)
            return false;
        p += r;
        n -= r;
    }
    return true;
}

/* Reassemble H4 packets from the stream and hand each to recv_fn as if
 * from an interrupt; ends when the socket closes (bt_drv_close) or on junk */
static int reader_thread(void *arg)
{
    (void)arg;
    for (;;) {
        size_t hdr, len;
        if (!read_all(rx_buf, 1))
            break;
        switch (rx_buf[0]) {
        case BT_H4_EVT: hdr = EVT_HDR; break;
        case BT_H4_ACL: hdr = ACL_HDR; break;
        case BT_H4_SCO: hdr = SCO_HDR; break;
        default:
            DEBUGF("bt-sdl: bad H4 type %02x\n", rx_buf[0]);
            return 0;
        }
        if (!read_all(rx_buf + 1, hdr))
            break;
        /* the length field is the header's last byte(s) */
        len = rx_buf[0] == BT_H4_EVT ? rx_buf[2] :
              rx_buf[0] == BT_H4_ACL ? (size_t)(rx_buf[3] | rx_buf[4] << 8) :
              rx_buf[3];
        if (!read_all(rx_buf + 1 + hdr, len))
            break;
        sim_enter_irq_handler();
        recv_fn(rx_buf, 1 + hdr + len);
        sim_exit_irq_handler();
    }
    return 0;
}

/* Connect to $ROCKBOX_BT_HCI ("host:port") and start the reader */
bool bt_drv_open(bt_drv_recv_fn recv)
{
    const char *spec = getenv("ROCKBOX_BT_HCI");
    char host[HOST_MAX];
    struct addrinfo hints, *res;
    int one = 1;

    if (!spec)
        spec = DEFAULT_HCI;
    const char *colon = strrchr(spec, ':');
    if (!colon || (size_t)(colon - spec) >= sizeof host)
        return false;
    memcpy(host, spec, colon - spec);
    host[colon - spec] = '\0';
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host, colon + 1, &hints, &res))
        return false;
    sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock >= 0 && connect(sock, res->ai_addr, res->ai_addrlen)) {
        close(sock);
        sock = -1;
    }
    freeaddrinfo(res);
    if (sock < 0) {
        DEBUGF("bt-sdl: no controller at %s\n", spec);
        return false;
    }
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &one, sizeof one);
    recv_fn = recv;
    reader = SDL_CreateThread(reader_thread, "bt-hci", NULL);
    return true;
}

void bt_drv_close(void)
{
    if (sock < 0)
        return;
    shutdown(sock, SHUT_RDWR);
    SDL_WaitThread(reader, NULL);
    close(sock);
    sock = -1;
}

bool bt_drv_send(const void *pkt, size_t len)
{
    const unsigned char *p = pkt;
    while (len && sock >= 0) {
        ssize_t n = send(sock, p, len, MSG_NOSIGNAL);
        if (n <= 0)
            return false;
        p += n;
        len -= n;
    }
    return !len;
}
