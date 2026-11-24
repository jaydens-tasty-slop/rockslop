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
#ifndef _CPU_H_
#define _CPU_H_

#include "hal.h"

#define MEMORY_SIZE        0x140 /* bytes: 0x280 RAM nibbles, two per byte */

#define MEM_RAM_ADDR        0x000
#define MEM_RAM_SIZE        0x280

#define MEM_DISPLAY1_ADDR     0xE00
#define MEM_DISPLAY1_SIZE     0x050

#define MEM_DISPLAY2_ADDR     0xE80
#define MEM_DISPLAY2_SIZE     0x050

#define DISPLAY_RAM_SIZE      ((MEM_DISPLAY1_SIZE + MEM_DISPLAY2_SIZE) / 2)

#define MEM_IO_ADDR       0xF00
#define MEM_IO_SIZE       0x080

#define ROM_WORDS         6144 /* 12-bit words in the P1 ROM */

typedef struct {
  u4_t factor_flag_reg;
  u4_t mask_reg;
  bool_t triggered; /* 1 if triggered, 0 otherwise */
  u8_t vector;
} interrupt_t;

/* Complete CPU state; this is also the body of the save file. */
typedef struct {
  u13_t pc;
  u12_t x;
  u12_t y;
  u4_t a;
  u4_t b;
  u5_t np;
  u8_t sp;
  u4_t flags;
  u32_t tick_counter;
  u32_t clk_timer_timestamp;
  u32_t prog_timer_timestamp;
  bool_t prog_timer_enabled;
  u8_t prog_timer_data;
  u8_t prog_timer_rld;
  bool_t halted;
  bool_t pset_pending;
  u8_t previous_cycles;
  interrupt_t interrupts[6];
  u8_t memory[MEMORY_SIZE];
  u8_t display_ram[DISPLAY_RAM_SIZE];
} cpu_state_t;

typedef enum {
  PIN_K00 = 0x0,
  PIN_K01 = 0x1,
  PIN_K02 = 0x2,
  PIN_K03 = 0x3,
  PIN_K10 = 0X4,
  PIN_K11 = 0X5,
  PIN_K12 = 0X6,
  PIN_K13 = 0X7,
} pin_t;

typedef enum {
  PIN_STATE_LOW = 0,
  PIN_STATE_HIGH = 1,
} pin_state_t;

typedef enum {
  INT_PROG_TIMER_SLOT = 0,
  INT_SERIAL_SLOT = 1,
  INT_K10_K13_SLOT = 2,
  INT_K00_K03_SLOT = 3,
  INT_STOPWATCH_SLOT = 4,
  INT_CLOCK_TIMER_SLOT = 5,
  INT_SLOT_NUM,
} int_slot_t;

#ifdef __cplusplus
 extern "C" {
#endif

/* The CPU runs at 32768 ticks per second of emulated time. */
#define CPU_TICK_HZ 32768

void cpu_get_state(cpu_state_t *cpustate);
void cpu_set_state(const cpu_state_t *cpustate);

void cpu_set_input_pin(pin_t pin, pin_state_t state);

/* Replay the display RAM through hw_set_lcd_pin() (after cpu_set_state). */
void cpu_refresh_display(void);

void cpu_reset(void);

/* rom: ROM_WORDS 12-bit words, which must stay valid. */
bool_t cpu_init(const u12_t *rom);

u32_t cpu_get_ticks(void);

/* Run until the tick counter reaches until (wrapping compare). Returns 1 if
 * an unknown opcode was hit (a bad ROM). */
int cpu_run(u32_t until);

#ifdef __cplusplus
}
#endif

#endif /* _CPU_H_ */
