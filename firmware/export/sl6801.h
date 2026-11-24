/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef SL6801_H
#define SL6801_H
#define SL6801_RAM_BASE 0x00800000
#define SL6801_RAM_SIZE 0x00080000
/* Preserve the ROM working region until its standalone ABI is understood. */
#define SL6801_CORE_RAM_BASE 0x00804c00
#define SL6801_MODULE_BASE 0x00860000
#define SL6801_FLASH_BASE 0x00c10000
#define SL6801_FLASH_SIZE 0x001ec000
/* M4 alignment requirement; no architectural L1 cache assumed. */
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
#define CACHEALIGN_BITS 2
#define CACHEALIGN_SIZE 4
/* The SD DMA moves words: buffering keeps file data word-aligned against
 * its file offset, so whole-sector reads skip the bounce buffer (one
 * command per 2 KiB: FLAC stuttered with the card busy 60% of the time) */
#define STORAGE_WANTS_ALIGN
#endif

/*
 * Register map of the SoC blocks that several drivers share. Addresses are
 * plain integer constants so that assembly can use them too; C code reads
 * them through its own 32-bit accessor. Bit masks carry a u suffix and are
 * for C only, except values the assembly uses (PMU bridge, core select,
 * NOR clock fields),
 * which are plain integers. Registers or bits
 * whose role is not established are named by address or id and say
 * "role unknown". Per-driver private registers live in the drivers.
 *
 * Not from a datasheet (there is no documentation): these definitions were
 * worked out on the hardware and from the SoC's behaviour; the names are
 * ours.
 */

/* ---- SPI NOR flash controller (0x30000000) ----
 * Offsets from SPI_NOR_BASE. Only the registers the startup and clock code
 * use. */
#define SPI_NOR_BASE            0x30000000
/* 1 = enabled (set by the ROM initialiser). */
#define SPI_NOR_CONTROL         0x00
/* XIP read mode: the command word the controller uses for XIP fetches, the
 * low byte being the flash opcode (values in xip-setup.inc). */
#define SPI_NOR_XIP_READ        0x20
/* The "XIP mapping register"; field meaning unknown. The
 * bootloader leaves 0 or 0x30; Rockbox must set 0x40 before its first XIP fetch. */
#define SPI_NOR_XIP_MAP         0x24
#define SPI_NOR_XIP_MAP_BOOTLOADER 0x30
#define SPI_NOR_XIP_MAP_ON      0x40

/* ---- System control words (0x40000000) ---- */
/* System word 0x40000000 (bit 6 is set in normal running). The
 * ROM's USB mode sets bit 0, then bit 6. Role of either bit unknown. */
#define SYS_REG_00              0x40000000
/* Memory timing words: slow values below 1020 mV
 * core voltage; both are reset by the watchdog and the RESET pin.
 * Fields in power-yp3.c. */
#define MEM_TIMING_A            0x40000004u
#define MEM_TIMING_B            0x40000008u
/* Core select: bit 0 reads 1 on the secondary core (CPU1), 0 on CPU0. The
 * ROM reset code sends the secondary core to the entry stored at
 * 0x30000600; our startup code uses it to keep
 * Rockbox on CPU0. */
#define SYS_CPU_SELECT          0x4000000c
#define SYS_CPU_SELECT_SECONDARY 1
/* System word 0x40000020, bit 2 (audio-related); the audio
 * driver sets it around the codec reset clear (audiohw-yp3.c). */
#define SYS_REG_20              0x40000020u

/* ---- Free-running 1 MHz counter (timer block 0x40082000) ----
 * Clocked from the 24 MHz clock divided by 24 (clock register 0x40080100 =
 * 0x1701); runs from boot and is never reprogrammed by us.
 * One tick = 1 us; the low word wraps after ~71.6 minutes. For a coherent
 * 64-bit value read HI, LO, HI and retry if the two HI reads differ. */
#define SL6801_COUNTER_LO       0x40082104  /* bits 31:0, microseconds */
#define SL6801_COUNTER_HI       0x40082108  /* bits 63:32 */

/* ---- Clock/reset controller (0x40080000) ---- */
/* PLL2, the CPU/AHB source: MHz = 48 * ((v >> 8) & 0xff) / (v & 0xf, 0 -> 1),
 * bit 28 ready, enable bits 0xc0010000. Cold value 0xd0010801 = 384 MHz
 * (set by the bootloader; we never change it). */
#define CLK_PLL2                0x40080000
#define CLK_PLL2_ENABLE         0xc0010000u
/* Audio PLL, two words.
 * Word 0: bits 31:30 enable (set 0xc0000000 to start), bit 28 locked (poll
 * after enabling), low bits 0x3040 select the 22.579 MHz MCLK (44.1 kHz
 * family) or 0x3060 the 24.576 MHz one (48 kHz family); the bits in mask
 * 0xfc1f0000 are kept and not otherwise understood.
 * Word 1: poll bit 11 clear before programming (taken as busy);
 * field [30:14] takes 0x186c2 (44.1 kHz family) or 0x3126 (48 kHz family),
 * bit 31 and 0x400 are set; field meanings beyond that unknown.
 * Word 0 is clock id 0 (bits 31:30 enable, bit 28 ready). */
#define CLK_AUDIO_PLL0          0x40080010
#define CLK_AUDIO_PLL1          0x40080014
#define AUDIO_PLL0_ENABLE       0xc0000000u     /* word 0 bits 31:30 */
/* Enable word for several clock ids: bit 4 = clock 0x29
 * (PLL2 / (bits 2:0 + 1)), bit 5 = clock 8 (source 8 of selector 0x3f),
 * bit 6 = clock 9, bit 16 = clock 0x35; bit 12 clear selects the constant
 * 24 MHz clock 0x3b. Cold 0x51 (bits 0, 4, 6); 0x71 once the LCD
 * driver has set bit 5. */
#define CLK_ROOT                0x40080030
#define CLK_ROOT_COLD           0x51u       /* cold boot value */
#define CLK_ROOT_CLK29          (1u << 4)   /* clock 0x29, set cold */
#define CLK_ROOT_SOURCE8_ENABLE (1u << 5)
#define CLK_ROOT_CLK9           (1u << 6)   /* clock 9, set cold */
/* CPU clock: PLL2 / divider. Divider field: see set_cpu_frequency(); bit 31
 * reads 1 while a new setting is being applied. Cold value 5
 * = 192 MHz. */
#define CLK_CPU                 0x40080040
#define CLK_CPU_SRC_MASK        0x3u
#define CLK_CPU_DIV_MASK        0x3cu       /* bits 5:2: divider - 1 */
#define CLK_CPU_DIV_SHIFT       2
#define CLK_CPU_BUSY            (1u << 31)
/* AHB bus clock: cold 0xb1 = mux 1, divider 12 -> 32 MHz (bits 7:4 =
 * divider - 1). Changed only at power-off. */
#define CLK_AHB                 0x40080048
#define CLK_AHB_SRC_MASK        0x3u
#define CLK_AHB_DIV_MASK        0xf0u

/* Clock gates, one bit per gate id: request word 0x40080070 + 4 * (id / 32), acknowledge word
 * 0x40080060 + 4 * (id / 32), bit id % 32. Enable = set the request bit,
 * set the ack bit, then poll the ack bit; disable clears ack, then
 * request, without polling. The gate ids the drivers use are below
 * (CODEC and CODEC_RAM: roles inferred from the codec open). */
#define CLK_GATE_ACK_BASE       0x40080060
#define CLK_GATE_REQ_BASE       0x40080070
#define CLK_GATE_ACK(id)        (CLK_GATE_ACK_BASE + 4 * ((id) / 32))
#define CLK_GATE_REQ(id)        (CLK_GATE_REQ_BASE + 4 * ((id) / 32))
#define CLK_GATE_BIT(id)        (1u << ((id) % 32))
/* Gate ids. CODEC_RAM is on only while the codec open clears 0x9400. */
#define CLK_GATE_ID_CODEC_RAM   0x20
/* General DMA controller: enable it before configuring a DMA channel. */
#define CLK_GATE_ID_DMA         0x21
#define CLK_GATE_ID_BT          0x22    /* Bluetooth baseband and RF */
#define CLK_GATE_ID_USB         0x23
#define CLK_GATE_ID_SD          0x24
#define CLK_GATE_ID_CODEC       0x25
#define CLK_GATE_ID_PWM         0x44    /* backlight PWM */
#define CLK_GATE_ID_I2C0        0x50    /* I2C controller 0 (FM tuner) */
#define CLK_GATE_ID_ADC         0x54
/* The audio trim bank: enable it with selector 0x1b before reading trim
 * words. */
#define CLK_GATE_ID_TRIM        0x58
#define CLK_GATE_ID_LCD         0x5c

/* Peripheral clock selectors, named by selector id. Common layout: bit 0
 * enable, source field at bit 4 (and up), divider field position varies. */
#define CLK_SEL_ENABLE          (1u << 0)
/* 0x11: SD card clock. Source 10 when bit 4 is clear
 * (configured 24 MHz, unmeasured); divider field [19:16] plus bit 8 (cold
 * setup: /64). */
#define CLK_SEL_11              0x40080080
/* 0x12 (selector 18): bit 0 set during USB bring-up, as
 * the boot ROM's USB mode has it; what it clocks is unknown. */
#define CLK_SEL_12              0x40080084
/* 0x13: bit 0 is set by the codec open. What it clocks is unknown. */
#define CLK_SEL_13              0x40080088
/* 0x2b (43): SPI NOR / XIP clock. Bits 11:8 divisor - 1, bits 5:4 source
 * (3 = PLL2, 384 MHz), bit 0 gate. Cold 0x801 under 0xf31; we run 0x631
 * (54.9 MHz, nor-clock-yp3.c). */
#define CLK_SEL_2B              0x400800a0
#define CLK_SEL_2B_GATE         0x001       /* bit 0 */
#define CLK_SEL_2B_SOURCE_MASK  0x030       /* bits 5:4 */
#define CLK_SEL_2B_SOURCE_PLL2  0x030       /* source 3: PLL2, 384 MHz */
#define CLK_SEL_2B_DIV_MASK     0xf00       /* bits 11:8: divisor - 1 */
#define CLK_SEL_2B_DIV(n)       (((n) - 1) << 8)
/* 0x24: bit 0 here (the same id as SD clock gate 36). The SD cold setup
 * enables it; what it clocks is unconfirmed. */
#define CLK_SEL_24              0x400800b8
/* 0x3a: bit 0 set in the FM tuner I2C setup right after gate 0x50; what it clocks is not established. */
#define CLK_SEL_3A              0x400800c0
/* 0x1b: audio trim bank clock (0x40099000), enable bit 0. */
#define CLK_SEL_1B              0x400800dc
/* 0x1c: FM tuner reference, out on GPIO3.11. Source field
 * [5:4]: 1 ~32.000 kHz, 2 12 MHz (used), 3 24 MHz; bit 0 enable. */
#define CLK_SEL_1C              0x400800e4
#define CLK_SEL_1C_SOURCE_MASK  (3u << 4)
#define CLK_SEL_1C_SOURCE(n)    ((n) << 4)
/* 0x1d: bit 0 enable, set by the radio calibration; what it clocks is not established. */
#define CLK_SEL_1D              0x400800e8
/* 0x27: PWM clock (backlight PWM channel 3). Bit 4 source (cleared), bit 0
 * enable. */
#define CLK_SEL_27              0x400800f4
/* 0x3f: LCD controller clock. Bit 4 selects source 8 (96 MHz), divider
 * field [11:8] = n - 1 (5: 96 / 6 = 16 MHz), bit 0 enable. */
#define CLK_SEL_3F              0x4008010c
#define CLK_SEL_3F_SOURCE8      (1u << 4)
#define CLK_SEL_3F_DIV_MASK     (0xfu << 8)
#define CLK_SEL_3F_DIV(n)       (((n) - 1) << 8)

/* ---- GPIO / pad controller (0x40081000) ----
 * Four banks of 32 pads; bank n's registers start at GPIO_BASE + 0x40 * n
 * (the per-bank layout and the board pin map are in yp3/gpio-yp3.h). Bank
 * 0 pads 0-5 are the NOR flash pads (boot code, xip-setup.inc). */
#define GPIO_BASE               0x40081000
/* Pin-enable mask, outside the bank layout; the LCD bring-up ORs in 0xffd0. */
#define GPIO_PIN_ENABLE         0x40081404u

/* ---- Watchdog (0x40083000) ----
 * Offsets from WDT_BASE. Register roles are known only as far as the
 * recovery configuration (recovery-hw-yp3.S) establishes them. */
#define WDT_BASE                0x40083000
#define WDT_CTRL                0x00    /* bit 0 set by the configuration */
#define WDT_ACK                 0x04    /* write 1 ("ack"); role unknown */
#define WDT_REFRESH             0x10    /* write WDT_REFRESH_KEY to feed */
#define WDT_MODE                0x14
#define WDT_SELECTOR            0x18    /* write 0 = disabled; bit 0 enables */
#define WDT_CAL                 0x1c    /* calibration/timeout */
#define WDT_REFRESH_KEY         0x28000001
/* Mode 0x2001 resets the SoC (0x2004 raises IRQ 0 instead, 0x2006
 * did not reset); selector 0x71 with cal 0x7fff00 gives ~16 s. Other
 * selectors were inconsistent (1 expires at once). Field meanings unknown. */
#define WDT_MODE_RESET          0x2001
/* 0x71 encodes a request of 20: ((floor(log2 n) + 3) << 4) | enable. */
#define WDT_SELECTOR_16S        0x71

/* ---- PWM block (0x40084000) ----
 * Channel n's registers start at PWM_BASE + 0x20 + 0x20 * n; the backlight
 * uses channel 3 (backlight-yp3.c, which holds the register layout). */
#define PWM_BASE                0x40084000
#define PWM_CH_BASE(ch)         (PWM_BASE + 0x20 + 0x20 * (ch))

/* ---- PMU bridge (0x40085000, "P" below) ----
 * The 8-bit PMU registers are reached through an indexed bridge; only one
 * client may use it at a time (pmu-yp3.h). The rest of the block holds
 * power-sequencer ports used directly (power-yp3.c). */
#define PMU_BASE                0x40085000
/* Calendar RTC words (field layout in rtc-yp3.c). */
#define PMU_RTC_DATE            0x40085004
#define PMU_RTC_TIME            0x40085008
/* Command word: PMU_BRIDGE_CMD_WRITE or _READ | index << 8. Write it, read
 * it back to confirm, then OR in PMU_BRIDGE_START; START reads back set
 * while the transfer runs. The meaning of the other
 * bits in 0x40000060 is unknown. */
#define PMU_BRIDGE_CMD          0x40085104
#define PMU_BRIDGE_CMD_WRITE    0x40000060
#define PMU_BRIDGE_CMD_READ     0x40000061  /* bit 0 = read */
#define PMU_BRIDGE_CMD_INDEX_SHIFT 8
#define PMU_BRIDGE_START        0x80000000  /* bit 31: start / busy */
#define PMU_BRIDGE_WDATA        0x40085108  /* byte to write */
#define PMU_BRIDGE_RDATA        0x4008510c  /* byte read, low 8 bits */
/* Status: any of bits 15:12 set = the last transfer failed (checked after
 * every read and write). */
#define PMU_BRIDGE_STATUS       0x40085110
#define PMU_BRIDGE_STATUS_ERROR 0xf000
/* Power-sequencer ports in the PMU block, written directly (not through the
 * bridge). Named by offset: roles are not
 * established beyond what is said. */
/* P+0x84: the PMU's measurement of its slow internal oscillator against the
 * crystal, in Hz, read-only; 0 until measured (rtc-yp3.c). */
#define PMU_PORT_SLOW_CLOCK     (PMU_BASE + 0x84)
/* P+0x98: core LDO control and voltage selector (fields in
 * power-yp3.c). */
#define PMU_PORT_CORE_LDO       (PMU_BASE + 0x98)
/* P+0xa0 holds the core LDO's trim field [19:16]; the core LDO enable sets
 * P+0xa4 bit 4. */
#define PMU_PORT_A0             (PMU_BASE + 0xa0)
#define PMU_PORT_A4             (PMU_BASE + 0xa4)
/* Power-sequencer port P+0xbc (the "emergency port"), role unknown. The
 * validated recovery config writes ((PMU[0x20] | 0x80) << 8) | 0x20 here
 * before enabling the watchdog (recovery-flags-yp3.c builds that word,
 * recovery-hw-yp3.S writes it). */
#define PMU_EMERGENCY           0x400850bc
/* Power-sequencer port P+0xd8, role unknown. Arming the recovery watchdog
 * writes it back with bit 1 cleared. */
#define PMU_D8                  0x400850d8

/* I2C controller 0 (0x40089000): the FM tuner bus; clock gate 0x50. The
 * register layout is in fmradio-i2c-yp3.c. */
#define I2C0_BASE               0x40089000

/* ---- General DMA controller (0x40001000) ----
 * Eight channels of 0x40 bytes; channel n raises IRQ 60 + n. Needs
 * clock gate 0x21 (sl6801_clock_gate_enable(CLK_GATE_ID_DMA)). Users:
 * LCD channel 0, PCM playback channel 2, capture channel 3. */
#define DMA_CH_BASE(ch)         (0x40001000 + 0x40 * (ch))
#define DMA_CH_CONTROL(ch)      (DMA_CH_BASE(ch) + 0x00)
#define DMA_CH_SOURCE(ch)       (DMA_CH_BASE(ch) + 0x04)  /* source address */
#define DMA_CH_DEST(ch)         (DMA_CH_BASE(ch) + 0x08)  /* dest address */
#define DMA_CH_COUNT(ch)        (DMA_CH_BASE(ch) + 0x0c)
#define DMA_CH_INTMASK(ch)      (DMA_CH_BASE(ch) + 0x20)
#define DMA_CH_STATUS(ch)       (DMA_CH_BASE(ch) + 0x24)
/* Control: a transfer starts when bit 29 is cleared and bit 30 set (what bit 29 does on its own is unknown). The bits in
 * DMA_CONFIG_MASK hold the channel configuration (request number, widths,
 * burst, direction) and are replaced as
 * a whole at channel setup. */
#define DMA_CONTROL_BIT29       (1u << 29)
#define DMA_CONTROL_ENABLE      (1u << 30)
#define DMA_CONFIG_MASK         0x633bf3bfu
/* Configuration fields: the request number of a peripheral source or
 * destination, and which side is memory. Bits 9:8 and 21:20 are the two
 * sides' transfer widths (code 2 = 32 bits, as both audio channels set
 * them); which field belongs to which side is not settled. Bits 7 and 19
 * are set by every user; their role is unknown. */
#define DMA_CFG_SRC_REQ(n)      ((uint32_t)(n))         /* bits 4:0 */
#define DMA_CFG_SRC_MEM         (1u << 5)
#define DMA_CFG_BIT7            (1u << 7)
#define DMA_CFG_WIDTH_9_8(w)    ((uint32_t)(w) << 8)
#define DMA_CFG_DST_REQ(n)      ((uint32_t)(n) << 12)   /* bits 16:12 */
#define DMA_CFG_DST_MEM         (1u << 17)
#define DMA_CFG_BIT19           (1u << 19)
#define DMA_CFG_WIDTH_21_20(w)  ((uint32_t)(w) << 20)
#define DMA_WIDTH_32            2u
/* Request numbers */
#define DMA_REQ_DAC             1u      /* destination side */
#define DMA_REQ_ADC             1u      /* source side */
#define DMA_REQ_LCD             9u      /* destination side */
/* Count: bits 17:0 = transfer length in bytes. The top byte is two 4-bit
 * fields (bits 31:28 and 27:24), probably burst sizes less one
 * (unconfirmed). Values in use: 0x31 for the LCD, 0xff for audio. */
#define DMA_COUNT_BYTES_MASK    0x3ffffu
#define DMA_COUNT_TOP(v)        ((v) << 24)
/* Count top byte of the audio channels (PCM playback and capture): both
 * fields at their maximum. */
#define DMA_COUNT_TOP_AUDIO     0xffu
/* Interrupt mask word: keep bits 13:2, set bit 19
 * (0x20 << 14) and bit 1. Bit 1 matches the completion status bit; the
 * meaning of each bit is otherwise unknown. */
#define DMA_INTMASK_KEEP        0x3ffcu
#define DMA_INTMASK_SET         ((0x20u << 14) | 2u)
/* Status (write 1 to clear): bit 1 transfer complete, bit 0 half done. */
#define DMA_STATUS_HALF         (1u << 0)
#define DMA_STATUS_COMPLETE     (1u << 1)

/* ---- Codec (0x40009000) ----
 * Only the word both audio drivers touch is here; each driver keeps the
 * registers only it uses (audiohw-yp3.c playback, capture-yp3.c capture). */
/* Codec control. CODEC_CTRL_OPEN_BITS (bits 31, 5, 1) are set by every
 * codec open, playback or capture; role unknown. */
#define CODEC_CTRL              0x40009000
#define CODEC_CTRL_OPEN_BITS    0x80000022u
/* DAC data port: one 32-bit word per 16-bit stereo frame. */
#define CODEC_DAC_PORT          0x4000910c

/* ---- Bluetooth RF block (0x40010000) ----
 * Digital part at +0x0000, analog part and register file at +0x1000. The
 * register offsets live in bt-rf-yp3.c. */
#define RF_BASE                 0x40010000

/* ---- Bluetooth core (0x40020000) ----
 * RivieraWaves-style BT core: control, interrupt control, status, ack. The
 * fields are described in bt-ctrl-yp3.c. */
#define BT_CORE_CTRL            0x40020000      /* RWBTCNTL */
#define BT_CORE_INTCNTL         0x4002000c
#define BT_CORE_STATUS          0x40020010
#define BT_CORE_ACK             0x40020018      /* write 1 to clear status */
#define BT_CORE_2C              0x4002002c      /* unidentified */
#define BT_CORE_RADIOPWRUPDN    0x40020080
#define BT_CORE_EM_WIN          0x400200c0      /* exchange-memory window */
#define BT_CORE_118             0x40020118      /* role unknown */
#define BT_CORE_1B0             0x400201b0      /* role unknown */
#define BT_CORE_1E4             0x400201e4      /* role unknown */

/* ---- Bluetooth exchange memory (0x40030000) ----
 * 32 KiB of RAM shared by the BT core and the CPU: hop tables, per-link
 * control structures, data buffers. */
#define BT_EM_BASE              0x40030000
#define BT_EM_END               0x40038000
#define BT_EM_WIN_AREA          0x400313a8      /* BT_CORE_EM_WIN target */
#define BT_EM_CS_BASE           0x400302b0      /* link control structures */

/* ---- USB controller (0x40040000) ----
 * A Mentor MUSB 1.900 core (HWVERS reads 0x0784) with MUSB's register
 * layout. IRQ 41 is the controller, IRQ 42
 * its DMA engine (USB_IRQ, USB_DMA_IRQ: irq-yp3.h). */
#define USB_BASE                0x40040000
/* MUSB POWER, a byte register at offset 0x01. Bit 6 SOFTCONN switches the
 * D+ pull-up: while set, the host sees the device. */
#define USB_REG_POWER           0x01
#define USB_POWER_SOFTCONN      0x40u
/* Vendor word outside the MUSB map (the "mode word"): the ROM's USB mode
 * sets bit 12 and then waits ~50 ms. */
#define USB_MODE_WORD           (USB_BASE + 0x300)

/* ---- Audio trim bank (0x40099000) ----
 * Efuse-like calibration words. Word n is at 0x20 + 4n; the bank must first
 * be enabled (clock gate 0x58, selector CLK_SEL_1B). Words used so far: 0x24 and 0x28 by
 * power-yp3.c (core LDO and PMU 0x11 trims), 0x28 and 0x2c by
 * audiohw-yp3.c (codec calibration). */
#define TRIM_BASE               0x40099000u
#define TRIM_WORD_24            (TRIM_BASE + 0x24u)
#define TRIM_WORD_28            (TRIM_BASE + 0x28u)
#define TRIM_WORD_2C            (TRIM_BASE + 0x2cu)

/* ---- Cortex-M4 NVIC (0xe000e000 system control space) ----
 * Set-enable, clear-enable and clear-pending are arrays of 32-bit words,
 * one bit per IRQ (word irq / 32, bit irq % 32); writing 0 bits has no
 * effect. Priority is one byte per IRQ, upper bits significant (0xff =
 * lowest). */
#define NVIC_ISER_BASE          0xe000e100
#define NVIC_ICER_BASE          0xe000e180
#define NVIC_ISPR_BASE          0xe000e200
#define NVIC_ICPR_BASE          0xe000e280
#define NVIC_IPR_BASE           0xe000e400
#define NVIC_ISER(irq)          (NVIC_ISER_BASE + 4 * ((irq) / 32))
#define NVIC_ICER(irq)          (NVIC_ICER_BASE + 4 * ((irq) / 32))
#define NVIC_ISPR(irq)          (NVIC_ISPR_BASE + 4 * ((irq) / 32))
#define NVIC_ICPR(irq)          (NVIC_ICPR_BASE + 4 * ((irq) / 32))
#define NVIC_IPR(irq)           (NVIC_IPR_BASE + (irq))
#define NVIC_BIT(irq)           (1u << ((irq) % 32))
/* Words in each of those arrays: 8 x 32 bits cover the Cortex-M4 NVIC's
 * maximum of 240 external IRQs, so clearing all of them clears whatever
 * the bootloader left. */
#define NVIC_WORDS              8

/* System control block and FP context control (ARMv7-M architecture), used
 * by the startup and early recovery code. */
#define SCB_CPUID               0xe000ed00
#define SCB_ICSR                0xe000ed04
#define SCB_VTOR                0xe000ed08
#define SCB_SHPR3               0xe000ed20      /* system handler priorities
                                                   12..15 (byte 0: DebugMon) */
#define SCB_DFSR                0xe000ed30      /* debug fault status */
#define SCB_CPACR               0xe000ed88      /* coprocessor access */
#define FPU_FPCCR               0xe000ef34

/* Debug registers (ARMv7-M architecture), used by the dev data watchpoint:
 * DEMCR enables DWT and the DebugMonitor exception, DWT comparator 0 matches
 * a data address. */
#define SCB_DEMCR               0xe000edfc
#define DWT_COMP0               0xe0001020
#define DWT_MASK0               0xe0001024
#define DWT_FUNCTION0           0xe0001028

/* ---- Cortex-M4 exception frame (ARMv7-M architecture) ----
 * On exception entry the core pushes eight words: r0-r3, r12, lr, pc and
 * xPSR, in that order from the lowest address (without FP context, which
 * Rockbox never enables). Indices are in 32-bit words. */
#define EXC_FRAME_PC            6
#define EXC_FRAME_WORDS         8

#endif
