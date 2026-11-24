#!/usr/bin/env python3
"""Copy MineSweeper sound effects from the upstream repository for the SD card.

usage: minesweeper_data.py UPSTREAM_DIR OUT_DIR
Writes OUT_DIR/music/mineswe{1..7}.raw (unsigned 8-bit mono, 16 kHz) to be
installed as .rockbox/rocks/games/pokitto_minesweeper/.
"""
import glob, os, shutil, sys

src, out = sys.argv[1], sys.argv[2]
os.makedirs(out + "/music", exist_ok=True)
for n in range(1, 8):
    f = glob.glob(f"{src}/src/sounds/mineswe{n} - *.raw")[0]
    shutil.copyfile(f, f"{out}/music/mineswe{n}.raw")
