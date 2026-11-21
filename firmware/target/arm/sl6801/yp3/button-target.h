/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef YP3_BUTTON_TARGET_H
#define YP3_BUTTON_TARGET_H
#include <stdbool.h>
#define BUTTON_UP 0x01
#define BUTTON_DOWN 0x02
#define BUTTON_LEFT 0x04
#define BUTTON_RIGHT 0x08
#define BUTTON_SELECT 0x10
#define BUTTON_MAIN 0x1f
/* Centre held 3 s powers off; plugins turn that off while they run. */
void button_set_power_hold(bool enable);
#endif
