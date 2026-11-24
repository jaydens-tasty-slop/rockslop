/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef CAPTURE_YP3_H
#define CAPTURE_YP3_H

/* ADC capture of the aux (FM) inputs, looped to the DAC via the mixer. */
void yp3_capture_start(void);
void yp3_capture_stop(void);
void yp3_capture_start_nomonitor(void);
void yp3_capture_set_gain(unsigned left, unsigned right);
void yp3_capture_set_mic_gain(unsigned gain);
void yp3_capture_levels(int *peak_l, int *peak_r, int *avg_l, int *avg_r);
extern struct yp3_capture_stats {
    unsigned long periods, consumed, overruns, underruns;
#ifdef YP3_DEV
    /* the mixer's calls for the next period: the longest gap between two
     * (or since the monitor started) and those over two periods (dev fmmon
     * resets both), and the 1 MHz counter at the last one (0: no monitor
     * yet) */
    unsigned long gap_max_us, late, last_us;
#endif
} yp3_capture_stats;

#endif
