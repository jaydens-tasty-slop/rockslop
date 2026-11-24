/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef LCD_YP3_H
#define LCD_YP3_H

#include <stdbool.h>

/* True once lcd_init_device() has completed and no update has failed. */
bool yp3_lcd_ready(void);

/* PWM3 backlight cold setup (GPIO, clock gate, channel) and 25% duty.
 * Called once from lcd_init_device(). Returns 0 on success. */
int yp3_backlight_cold_setup(void);

#endif
