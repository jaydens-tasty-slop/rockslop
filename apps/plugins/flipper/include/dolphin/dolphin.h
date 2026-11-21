/* SPDX-License-Identifier: GPL-2.0-or-later */
/* No dolphin (XP) service here. */
#include <furi.h>
typedef enum { DolphinDeedPluginGameStart, DolphinDeedPluginGameWin } DolphinDeed;
static inline void dolphin_deed(DolphinDeed deed) { (void)deed; }
