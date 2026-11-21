/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef FLIPPER_INTERNAL_H
#define FLIPPER_INTERNAL_H
#include <stdbool.h>
#include <input/input.h>

/* flipper_core.c */
void flipper_pump(void);            /* poll input, run timers, redraw */
bool flipper_is_main(void);
void flipper_checkpoint(void);      /* abandons the app after a quit timeout */
void flipper_wait_slice(void);      /* one tick of waiting, pumping if main */

/* flipper_notify.c */
void flipper_sound_stop(void);      /* silence the speaker and drop queued notes */
void flipper_sound_close(void);     /* release the mixer channel */

/* flipper_gui.c */
void flipper_gui_dispatch(const InputEvent *ev);
void flipper_gui_redraw(void);      /* draw the viewport if it is dirty */
void flipper_gui_refresh_all(void); /* repaint border and screen after a menu */
void flipper_gui_reset(void);
#endif
