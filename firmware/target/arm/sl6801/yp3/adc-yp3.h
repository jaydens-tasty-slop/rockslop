/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef ADC_YP3_H
#define ADC_YP3_H

#include <stdbool.h>

/* General-purpose ADC controller (not the codec's audio ADC; adc-yp3.c has
 * the rest of its registers). Per channel, 0x10 apart: +0x24 holds the
 * latest sample. Channel 1 is the Up/Down key ladder on GPIO3.0; the button
 * scan reads its sample directly.
 * Not from a datasheet (there is no documentation): these definitions were
 * worked out on the hardware and from the SoC's behaviour; the names are
 * ours. */
#define ADC_BASE        0x40096000u
#define ADC_CH_SAMPLE(ch) (ADC_BASE + 0x24u + (ch) * 0x10u)
#define ADC_KEY_CHANNEL 1u
#define ADC_KEY_SAMPLE  ADC_CH_SAMPLE(ADC_KEY_CHANNEL)

/* True once adc_init() completed the controller sequence. */
bool yp3_adc_ready(void);
/* ADC interrupt (IRQ20) handler, installed at runtime by adc_init(). */
void yp3_adc_irq(void);

#endif
