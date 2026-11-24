/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef YP3_CODEC_H
#define YP3_CODEC_H
/* Software volume; line-in gain is the codec aux input gain (FM tuner),
 * codes 0..31 at ~1.8 dB, shown relative to the default 22; mic gain is
 * the built-in mic's path gain code. */
#define AUDIOHW_CAPS (LIN_GAIN_CAP | MIC_GAIN_CAP)
/* Gain code 0 (audiohw-yp3.c) with the PCM at -8 dB at
 * most. Default -24 dB: the loudness of the earlier -6 dB at code 24. */
AUDIOHW_SETTING(VOLUME, "dB", 1, 5, -790, -80, -240);
AUDIOHW_SETTING(LEFT_GAIN,  "dB", 1, 1, 0, 31, 22, (val - 22) * 18)
AUDIOHW_SETTING(RIGHT_GAIN, "dB", 1, 1, 0, 31, 22, (val - 22) * 18)
/* Mic path 2 gain code 0..127 (default 0x54 = 84; step size not measured). */
AUDIOHW_SETTING(MIC_GAIN,   "",   0, 1, 0, 127, 84)
#endif
