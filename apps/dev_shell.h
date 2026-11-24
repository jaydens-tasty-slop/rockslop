/* SPDX-License-Identifier: GPL-2.0-or-later */
/* The dev shell's hooks in apps code (apps/dev_shell.c); HAVE_DEVLOG only. */
#ifndef DEV_SHELL_H
#define DEV_SHELL_H

#include <stdarg.h>
#include <stdbool.h>

/* The run config's settings overrides (bluetooth=), right after
 * settings_load() and before the drivers they steer start (main.c) */
void dev_settings_loaded(void);
/* Boot tasks from the run config, the shell among them (main.c) */
void dev_boot(void);
/* SYS_DEV_RUN in the UI thread: run the plugin or screen the shell asked
 * for; false for a UI ping (nothing ran). */
bool dev_ui_run(void);
/* pcmbuf found no next chunk: an underrun, or TRACK_END (see dev_shell.c) */
void dev_pcm_starved(bool track_end);
/* pcmbuf_play_stop(): playback stopped or restarts */
void dev_pcm_stopped(void);
/* A splash or a yes/no prompt (its lines may be language IDs): logged and
 * counted for `health`, so a run sees the messages a user would. */
void dev_splash(const char *fmt, va_list ap);
void dev_yesno(const char **lines, int count);
/* The heap into the log: free and largest block, with BLOCKS every
 * allocation (shell `mem`; playback refusing for want of memory) */
void dev_log_heap(bool blocks);
/* A list drawn (gui_synclist_draw): its title and selection for `sel`, so
 * a key walk can see where it is. */
struct gui_synclist;
void dev_list_drawn(struct gui_synclist *list);

#endif
