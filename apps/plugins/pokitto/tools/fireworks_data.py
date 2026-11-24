#!/usr/bin/env python3
"""Extract FireWorks sound data from the upstream repository for the SD card.

usage: fireworks_data.py UPSTREAM_DIR OUT_DIR
Writes OUT_DIR/music/Fwork_0{1,2,3}.raw and OUT_DIR/sfx/*.raw (unsigned
8-bit mono, 16 kHz) to be installed as .rockbox/rocks/games/pokitto_fireworks/.
"""
import os, shutil, sys

src, out = sys.argv[1], sys.argv[2]
os.makedirs(out + "/music", exist_ok=True)
os.makedirs(out + "/sfx", exist_ok=True)
for n in ("Fwork_01", "Fwork_02", "Fwork_03"):
    shutil.copyfile(f"{src}/music/{n}.raw", f"{out}/music/{n}.raw")
for n in ("Fireworks", "Whistle"):
    shutil.copyfile(f"{src}/src/sounds/{n}.raw", f"{out}/sfx/{n}.raw")
