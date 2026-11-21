/* SPDX-License-Identifier: GPL-2.0-or-later */
/* The Flipper's RGB LED: not present here. */
#pragma once
#include <furi.h>
typedef enum { LightRed, LightGreen, LightBlue, LightBacklight } Light;
static inline void furi_hal_light_set(Light light, uint8_t value) { (void)light; (void)value; }
