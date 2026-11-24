/*
 * TamaLIB - A hardware agnostic Tamagotchi P1 emulation library
 *
 * Copyright (C) 2021 Jean-Christophe Rona <jc@rona.fr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#ifndef _HAL_H_
#define _HAL_H_

#include "hal_types.h"

#ifndef NULL
	#define NULL 0
#endif

/* The part of the TamaLIB hardware abstraction layer that is left: the
 * LCD, icons and buzzer. Timing is driven by the caller (cpu_run). */
typedef struct {
	void (*set_lcd_matrix)(u8_t x, u8_t y, bool_t val);
	void (*set_lcd_icon)(u8_t icon, bool_t val);

	/* set_frequency() changes the output frequency of the sound, while
	 * play_frequency() decides whether the sound should be heard or not. */
	void (*set_frequency)(u32_t freq);
	void (*play_frequency)(bool_t en);
} hal_t;

extern hal_t *g_hal;

#endif /* _HAL_H_ */
