/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Development builds' log ring, serial mirror, key scripts and thread traces,
 * shared by the targets that define HAVE_DEVLOG (devlog.h). */
#include "config.h"
#include "system.h"
#include "kernel.h"
#include "button.h"
#include "vuprintf.h"
#include "devlog.h"
#include "kernel/thread-internal.h"
#ifdef USB_ENABLE_SERIAL
#include "usbstack/usb_serial.h"
#endif
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define LOG_LINE_BYTES 256      /* longest log line ("ticks" ~226) */

static volatile int held_buttons;   /* a key script's press (dev_keys)... */
static volatile long held_until;    /* ...until this tick */
bool dev_usb_enabled;
bool dev_poweroff_dry;
struct dev_sd_stats dev_sd_stats;

void dev_poweroff_check(void)
{
    if (!dev_poweroff_dry)
        return;
    dev_log("power_off() (dry run): back to the host");
    dev_return_now();
}

static uint32_t *ring_head;
static char *ring_data;
static uint32_t ring_size;

void devlog_init(uint32_t *head, char *data, uint32_t size)
{
    ring_head = head;
    ring_data = data;
    ring_size = size;
}

/* n ring bytes from total offset pos */
static void ring_get(uint32_t pos, char *out, size_t n)
{
    while (n) {
        size_t at = pos % ring_size, k = MIN(n, ring_size - at);
        memcpy(out, &ring_data[at], k);
        pos += k;
        out += k;
        n -= k;
    }
}

/* n bytes into the ring at its head; interrupts masked */
static void ring_put(const char *s, size_t n)
{
    while (n) {
        size_t at = *ring_head % ring_size, k = MIN(n, ring_size - at);
        memcpy(&ring_data[at], s, k);
        *ring_head += k;
        s += k;
        n -= k;
    }
}

uint32_t dev_log_head(void)
{
    return ring_head ? *ring_head : 0;
}

size_t dev_log_read(uint32_t *pos, char *out, size_t size)
{
    if (!ring_head)
        return 0;
    int old = disable_irq_save();
    uint32_t head = *ring_head;
    if (head - *pos > ring_size)
        *pos = head - ring_size;        /* older bytes were overwritten */
    size_t n = MIN(head - *pos, size);
    ring_get(*pos, out, n);
    *pos += n;
    restore_irq(old);
    return n;
}

static int raw_push(void *userp, int c)
{
    (void)userp;
    ring_data[(*ring_head)++ % ring_size] = (char)c;
    return 1;
}

void dev_log_raw(const char *fmt, ...)
{
    if (!ring_head)
        return;
    va_list ap;
    va_start(ap, fmt);
    vuprintf(raw_push, NULL, fmt, ap);
    va_end(ap);
}

#ifdef USB_ENABLE_SERIAL
/* Serial output in chunks of at most one full-speed bulk packet (64 B);
 * the shell's thread waits up to 20 x 10 ms per chunk for room, 2 s for a
 * reply's end */
#define SERIAL_CHUNK      64
#define SERIAL_WAIT_TRIES 20
#define SERIAL_END_TRIES  200
#define SERIAL_LINE_BYTES 200   /* longest mirrored log line */

static bool serial_owner_set;
static unsigned int serial_owner;

void dev_serial_owner(unsigned int thread)
{
    serial_owner = thread;
    serial_owner_set = true;
}

/* Nothing goes out until the host has written since the port was last
 * configured: each enumeration gives a new ttyACM with ECHO on, and lines
 * queued before the host set it raw came back as a command (the shell's
 * "err bt:" in place of the reply). */
static bool serial_heard;

void dev_serial_heard(void)
{
    serial_heard = true;
}

void dev_serial_connected(void)
{
    serial_heard = false;
}

static bool serial_mid_line;        /* the last byte queued was no newline */
/* TODO(cleanup): lost shell replies. */
static unsigned serial_reply_drops; /* shell output dropped after waiting */
static unsigned serial_end_newlines;/* reply ends that needed a newline */

/* Queue data for the shell, waiting (up to tries x 10 ms per chunk) for
 * room in the 512 B send buffer instead of dropping it. */
static void serial_send(const char *data, size_t length, int wait_tries)
{
    if (!serial_heard)
        return;
    /* Only the shell's thread waits: a log line from another (the
       Bluetooth stack's, with "bt hci trace on") stalled it 0.4 s a
       line; the ring keeps what the mirror drops. Interrupts never wait
       (the shell's "err busy" comes from the USB interrupt). */
    bool can_wait = get_interrupt_number() == 0 && irq_enabled() &&
                    serial_owner_set && thread_self() == serial_owner;
    /* The others' lines go whole or not at all: one cut after a chunk
       left the stream mid-line, and the shell's "ok" after it was no
       line of its own (the host waited out a reply it had). */
    if (!can_wait && usb_serial_send_space() < (int)length)
        return;
    while (length && usb_serial_send_space() >= 0) {
        size_t n = MIN(length, SERIAL_CHUNK);
        for (int tries = 0; can_wait && usb_serial_send_space() < (int)n &&
                            tries < wait_tries; tries++)
            sleep(HZ / 100 ? HZ / 100 : 1);
        if (usb_serial_send_space() < (int)n) {
            if (can_wait)
                serial_reply_drops++;
            return;     /* host not reading */
        }
        usb_serial_send((const unsigned char *)data, (int)n);
        serial_mid_line = data[n - 1] != '\n';
        data += n;
        length -= n;
    }
}

void dev_serial_write(const char *data, size_t length)
{
    serial_send(data, length, SERIAL_WAIT_TRIES);
}

void dev_serial_end(const char *data, size_t length)
{
    if (serial_mid_line) {
        serial_end_newlines++;
        serial_send("\n", 1, SERIAL_END_TRIES);
    }
    serial_send(data, length, SERIAL_END_TRIES);
}

void dev_serial_state(char *buf, size_t size)
{
    int queued, in_transit;
    unsigned refused;
    usb_serial_dev_state(&queued, &in_transit, &refused);
    snprintf(buf, size, "serial %d B queued, %d in transit, %u refused, "
             "%u reply drops, %u end newlines", queued, in_transit, refused,
             serial_reply_drops, serial_end_newlines);
}
#endif /* USB_ENABLE_SERIAL */

/* Formatted with interrupts on, then copied into the ring masked: masked
 * formatting took milliseconds a line on the YP3 (vuprintf from XIP), and
 * its Bluetooth interrupt must run every 1.25 ms or a packet is lost. */
void dev_log(const char *fmt, ...)
{
    char line[LOG_LINE_BYTES];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(line, sizeof line - 1, fmt, ap);
    va_end(ap);
    if (n < 0)
        n = 0;
    if (n > (int)sizeof line - 2)
        n = sizeof line - 2;
    line[n] = '\n';
    if (ring_head) {
        int old = disable_irq_save();
        ring_put(line, n + 1);
        restore_irq(old);
    }
#ifdef USB_ENABLE_SERIAL
    /* usb_serial_send() is for threads only */
    if (get_interrupt_number() == 0) {
        if (n > SERIAL_LINE_BYTES - 2) {
            n = SERIAL_LINE_BYTES - 2;
            line[n] = '\n';
        }
        serial_send(line, n + 1, SERIAL_WAIT_TRIES);
    }
#endif
}

void dev_log_returns(const char *prefix, const uint32_t *sp,
                     const uint32_t *end, int max)
{
    char line[LOG_LINE_BYTES - 8];
    int len = snprintf(line, sizeof line, "%s:", prefix);
    for (int n = 0; sp < end && n < max && len < (int)sizeof line - 10; sp++)
        if ((*sp & 1) && dev_code_address(*sp & ~1u)) {
            len += snprintf(line + len, sizeof line - len, " %08lx",
                            (unsigned long)(*sp & ~1u));
            n++;
        }
    dev_log("%s", line);
}

#define TRACE_RETURNS 12        /* return addresses logged per thread */

static void thread_trace(const struct thread_entry *t)
{
    const uint32_t *sp = (const uint32_t *)t->context.sp;
    const uint32_t *stack = (const uint32_t *)t->stack;
    const uint32_t *end = (const uint32_t *)((char *)t->stack + t->stack_size);
    /* its stack's high-water mark: the fill (DEADBEEF) no call reached */
    const uint32_t *used = stack;
    while (used < end && *used == DEADBEEF)
        used++;
    char prefix[96];
    snprintf(prefix, sizeof prefix, "thread %s: state %d lr %08lx sp %08lx, stack %u of %u",
             t->name ? t->name : "?", (int)t->state,
             (unsigned long)t->context.lr, (unsigned long)(uintptr_t)sp,
             (unsigned)((end - used) * 4), (unsigned)t->stack_size);
    /* the running thread's saved sp is stale; a wild one is no stack */
    if (t == __running_self_entry() || sp < stack || sp >= end)
        sp = end;
    dev_log_returns(prefix, sp, end, TRACE_RETURNS);
}

void dev_thread_trace(unsigned int thread_id)
{
    thread_trace(__thread_id_entry(thread_id));
}

void dev_thread_trace_name(const char *name)
{
    for (unsigned int i = 0; i < MAXTHREADS; i++) {
        const struct thread_entry *t = __thread_slot_entry(i);
        if (t->state != STATE_KILLED && t->name && !strcmp(t->name, name)) {
            thread_trace(t);
            return;
        }
    }
}

void dev_thread_trace_all(void)
{
    for (unsigned int i = 0; i < MAXTHREADS; i++) {
        const struct thread_entry *t = __thread_slot_entry(i);
        if (t->state != STATE_KILLED)
            thread_trace(t);
    }
}

bool devlog_config_find(const char *text, size_t size, const char *key,
                        char *buf, size_t bufsize)
{
    const char *p = text, *end = text + size;
    size_t klen = strlen(key);
    while (p < end && *p != '\0' && *p != (char)0xff) {
        const char *eol = memchr(p, '\n', end - p);
        if (!eol)
            eol = end;
        if ((size_t)(eol - p) > klen && p[klen] == '=' && !memcmp(p, key, klen)) {
            size_t n = MIN((size_t)(eol - (p + klen + 1)), bufsize - 1);
            memcpy(buf, p + klen + 1, n);
            buf[n] = '\0';
            return true;
        }
        p = eol + 1;
    }
    return false;
}

static const struct { char letter; int button; } key_letters[] = {
    { 'U', BUTTON_UP }, { 'D', BUTTON_DOWN }, { 'L', BUTTON_LEFT },
    { 'R', BUTTON_RIGHT }, { 'S', BUTTON_SELECT },
#ifdef BUTTON_VOL_UP
    { 'V', BUTTON_VOL_UP }, { 'v', BUTTON_VOL_DOWN },
#endif
#ifdef BUTTON_POWER
    { 'P', BUTTON_POWER },
#endif
};

static int key_button(char c)
{
    for (size_t i = 0; i < ARRAYLEN(key_letters); i++)
        if (key_letters[i].letter == c)
            return key_letters[i].button;
    return 0;
}

int dev_held_buttons(void)
{
    return TIME_BEFORE(current_tick, held_until) ? held_buttons : 0;
}

void dev_keys(const char *script)
{
    const char *p = script;
    for (;;) {
        while (*p == ' ' || *p == ',')
            p++;
        if (!*p)
            return;
        const char *c = p;
        int buttons = 0;
        for (int b; (b = key_button(*c)); c++)
            buttons |= b;
        long ticks = 0;
        if (buttons) {
            ticks = HZ * 15 / 100;
            if (*c == '+')
                for (ticks = 0; *c == '+'; c++)
                    ticks += HZ;
            else if (*c == '*')
                ticks = HZ * 6 / 5, c++;
            else if (*c == '~')
                ticks = HZ / 8, c++;
        } else {
            long ms = 0;
            for (; isdigit((unsigned char)*c); c++)
                ms = ms * 10 + *c - '0';
            ticks = ms * HZ / 1000;
        }
        if (c == p || (*c && *c != ' ' && *c != ',')) {
            dev_log("keys: bad token at '%s'", p);
            return;
        }
        if (buttons) {
            held_buttons = buttons;
            held_until = current_tick + ticks;
            sleep(ticks);
            held_buttons = 0;
            ticks = HZ / 5;
        }
        sleep(MAX(ticks, 1));
        p = c;
    }
}
