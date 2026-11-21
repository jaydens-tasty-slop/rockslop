/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef FLIPPER_ELEMENTS_H
#define FLIPPER_ELEMENTS_H
#include <gui/canvas.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Only elements_button_center is provided: a framed label at the bottom. */
void elements_button_center(Canvas* canvas, const char* str);

#ifdef __cplusplus
}
#endif
#endif
