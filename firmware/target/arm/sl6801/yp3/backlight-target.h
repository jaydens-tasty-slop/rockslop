/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef YP3_BACKLIGHT_TARGET_H
#define YP3_BACKLIGHT_TARGET_H
#include <stdbool.h>

bool backlight_hw_init(void);
void backlight_hw_on(void);
void backlight_hw_off(void);
void backlight_hw_brightness(int brightness);

#ifdef HAVE_LCD_SLEEP
void lcd_awake(void);
#endif

#endif
