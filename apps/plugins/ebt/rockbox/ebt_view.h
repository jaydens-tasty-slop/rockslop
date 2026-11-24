/* EBT player screen (WTFPL-compatible, see ../LICENSE). */
#ifndef EBT_VIEW_H
#define EBT_VIEW_H

#include "ebt_types.h"

/* Provided by the front end. */
void view_fill(int x, int y, int w, int h, uint32_t rgb);
void view_text(int x, int y, uint32_t rgb, const char *s);

#define VIEW_FW 6 /* text cell size the layout assumes */
#define VIEW_FH 8

void ebt_view_draw(const struct ebt_status *st, int sel_ch, unsigned mute,
                   int paused);

#endif
