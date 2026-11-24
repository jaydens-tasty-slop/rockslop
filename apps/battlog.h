/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Battery benchmark logger (apps/battlog.c); HAVE_BATTLOG builds only. */
#ifndef BATTLOG_H
#define BATTLOG_H

/* Start logging if /battlog.csv exists (main.c, after the disk is up). */
void battlog_start(void);

#endif
