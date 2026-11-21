/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Development builds (HAVE_DEVLOG, which a target's dev build defines: YP3_DEV,
 * TS17_DEV). The host writes a run config before boot and reads a log ring
 * after the player returns to its loader; on USB serial a line-based shell
 * (apps/dev_shell.c) takes commands while it runs. firmware/devlog.c is the
 * shared part; the target places the ring and the config and answers the
 * hooks at the end (firmware/target/.../dev-*.c). Without HAVE_DEVLOG the
 * log calls compile to nothing. */
#ifndef DEVLOG_H
#define DEVLOG_H
#include "config.h"
#ifdef HAVE_DEVLOG
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* The target's ring: SIZE bytes at DATA, and *HEAD, the count of bytes ever
 * written (data[*head % size] comes next). Both live where the return to the
 * loader keeps them; the target resets *HEAD when it is not its own. */
void devlog_init(uint32_t *head, char *data, uint32_t size);
/* A line into the ring and, from a thread, to the shell */
void dev_log(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
/* Formatted straight into the ring, nothing else: exception handlers */
void dev_log_raw(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
/* A message for torture's `msgs` (and the log); apps code only
 * (apps/dev_shell.c). KIND: one word, e.g. "plugin". */
void dev_msg(const char *kind, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));
/* Ring position of the next byte written (counts up from 0) */
uint32_t dev_log_head(void);
/* Up to SIZE log bytes from position *POS (moved up to the oldest byte the
 * ring still holds), advancing *POS; the count */
size_t dev_log_read(uint32_t *pos, char *out, size_t size);

/* Where a thread waits: its saved lr and the return addresses into code on
 * its stack. By id, by name (if there is one), or every thread. */
void dev_thread_trace(unsigned int thread_id);
void dev_thread_trace_name(const char *name);
void dev_thread_trace_all(void);
/* Up to MAX return addresses into code found from SP up to END, as one
 * "PREFIX: ..." line */
void dev_log_returns(const char *prefix, const uint32_t *sp,
                     const uint32_t *end, int max);
/* The open file descriptors (firmware/common/file.c): the count, and with
 * LOG a line with each one's size and flags */
int dev_files_open(bool log);

/* Key script (shell `keys`, run config keys=), in the calling thread.
 * Tokens split by spaces or commas. Letters press buttons together:
 * U D L R S (ring or pad, centre), V v (volume up, down) and P (power)
 * where the player has them. A press lasts 150 ms; X+ holds 1 s, X++ 2 s
 * and so on, X* 1.2 s, X~ 1/8 s; 200 ms released follow it. A number
 * waits that many ms. */
void dev_keys(const char *script);
/* The buttons a key script holds: the target's button driver reports them
 * as pressed, so a script is a user at the keys. The press ends at its
 * tick, not when the script's thread next runs: a busy UI cannot stretch
 * it past the repeat delay into extra presses. */
int dev_held_buttons(void);
/* USB may attach (run config usb=; the target's usb_detect()) */
extern bool dev_usb_enabled;
/* A power-off returns to the host instead (shell `dryoff`, run config
 * poweroff=dry), for unattended runs; the target's power_off() calls
 * dev_poweroff_check() first */
extern bool dev_poweroff_dry;
void dev_poweroff_check(void);
/* Storage driver counters for the shell's timings; zero where the driver
 * keeps none */
extern struct dev_sd_stats {
    uint32_t read_calls, read_sectors, read_us;
    uint32_t write_sectors, write_us, write_busy_polls;
} dev_sd_stats;

#ifdef USB_ENABLE_SERIAL
/* Raw output to the shell (thread context) */
void dev_serial_write(const char *data, size_t length);
/* A reply's closing "ok"/"err" line: on a line of its own (the host reads up
 * to a line that is exactly that) and waited for longer */
void dev_serial_end(const char *data, size_t length);
/* USB_SERIAL_RX_HOOK: a line for the shell (apps/dev_shell.c) */
void dev_serial_rx(const unsigned char *data, int length);
/* The host wrote: serial output starts then (usb_serial.c's connection
 * stops it until the host writes again) */
void dev_serial_heard(void);
void dev_serial_connected(void);
/* The shell's thread: the only one that waits for the host to read */
void dev_serial_owner(unsigned int thread);
/* The send ring's state, for a shell whose replies stopped reaching the
 * host. TODO(cleanup): lost shell replies. */
void dev_serial_state(char *buf, size_t size);
#endif

/* KEY's value in TEXT, "key=value" lines up to SIZE bytes or a NUL or
 * erased (0xff) byte; for the target's dev_config() */
bool devlog_config_find(const char *text, size_t size, const char *key,
                        char *buf, size_t bufsize);

/* --- The target's part (dev-*.c) --- */

/* The run config's value for KEY (from "KEY=value" lines), NUL-terminated
 * in BUF; false when it has none */
bool dev_config(const char *key, char *buf, size_t size);
/* Keep the recovery watchdog fed through long work before the UI runs */
void dev_feed(void);
/* Back to the host's loader soon, keeping the log (shell `rom`, `!rom`) */
void dev_return(void);
/* The same at once, from any context (a dry power-off) */
void dev_return_now(void) __attribute__((noreturn));
/* ADDR (Thumb bit clear) is in code: for return address scans */
bool dev_code_address(uint32_t addr);
/* The PCs the last ticks interrupted, newest first; how many (<= N) */
unsigned dev_trace_copy(uint32_t *out, unsigned n);
/* A free-running microsecond count */
uint32_t dev_usec(void);

/* The target's boot tasks from the run config, before the shared ones
 * (apps/dev_shell.c dev_boot()) */
void dev_target_boot(void);
/* Shell commands of the target's own, after the shared ones: the Ith, NULL
 * past the last */
struct dev_cmd {
    const char *name;
    bool (*fn)(char *arg);
    const char *usage;      /* for `help`; NULL: not listed */
};
const struct dev_cmd *dev_target_cmd(size_t i);
#else
#define dev_log(...) do { } while (0)
#define dev_msg(...) do { } while (0)
#endif
#endif
