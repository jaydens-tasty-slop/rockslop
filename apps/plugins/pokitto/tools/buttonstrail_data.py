#!/usr/bin/env python3
"""Copy ButtonsTrail sound data from the upstream repository for the SD card.

usage: buttonstrail_data.py UPSTREAM_DIR OUT_DIR
Writes OUT_DIR/music/DarkB_01.raw and OUT_DIR/sfx/Tone_0{0..9}.raw (unsigned
8-bit mono, 11.025 kHz) to be installed as
.rockbox/rocks/games/pokitto_buttonstrail/.
"""
import os, shutil, sys

src, out = sys.argv[1], sys.argv[2]
os.makedirs(out + "/music", exist_ok=True)
os.makedirs(out + "/sfx", exist_ok=True)
shutil.copyfile(f"{src}/music/DarkB_01.raw", f"{out}/music/DarkB_01.raw")
for i in range(10):
    shutil.copyfile(f"{src}/music/Tone_0{i}.raw", f"{out}/sfx/Tone_0{i}.raw")
