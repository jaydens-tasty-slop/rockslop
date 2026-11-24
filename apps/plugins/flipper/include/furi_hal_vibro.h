/* SPDX-License-Identifier: GPL-2.0-or-later */
/* The Flipper's vibration motor: not present here. */
#pragma once
#include <furi.h>
static inline void furi_hal_vibro_on(bool value) { (void)value; }
