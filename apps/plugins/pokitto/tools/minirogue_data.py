#!/usr/bin/env python3
"""Copy Mini Rogue sound data from the upstream repository for the SD card.

usage: minirogue_data.py UPSTREAM_DIR OUT_DIR
Writes OUT_DIR/music/MiniR_0{0..4}.raw and OUT_DIR/sfx/*.raw (unsigned 8-bit
mono, 11.025 kHz) to be installed as .rockbox/rocks/games/pokitto_minirogue/.
"""
import os, shutil, sys

src, out = sys.argv[1], sys.argv[2]
os.makedirs(out + "/music", exist_ok=True)
os.makedirs(out + "/sfx", exist_ok=True)
for n in ("MiniR_00", "MiniR_01", "MiniR_02", "MiniR_03", "MiniR_04"):
    shutil.copyfile(f"{src}/music/{n}.raw", f"{out}/music/{n}.raw")
for n in ("EvilLaugh", "RollDice", "Positive", "Negative", "FistPunch1",
          "FistPunch2", "Welcome"):
    shutil.copyfile(f"{src}/src/sounds/{n}.raw", f"{out}/sfx/{n}.raw")
