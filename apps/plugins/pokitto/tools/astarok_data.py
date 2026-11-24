#!/usr/bin/env python3
"""Copy Trials of Astarok sound data from the upstream repository for the SD card.

usage: astarok_data.py UPSTREAM_DIR OUT_DIR
Writes OUT_DIR/music/Astaro0{1..5}.raw and OUT_DIR/sfx/*.raw (unsigned 8-bit
mono, 16 kHz) to be installed as .rockbox/rocks/games/pokitto_astarok/.
"""
import os, shutil, sys

src, out = sys.argv[1], sys.argv[2]
os.makedirs(out + "/music", exist_ok=True)
os.makedirs(out + "/sfx", exist_ok=True)
for n in ("Astaro01", "Astaro02", "Astaro03", "Astaro04", "Astaro05"):
    shutil.copyfile(f"{src}/music/{n}.raw", f"{out}/music/{n}.raw")
for n in ("01_Coin_8K", "02_Die", "03_One_Up", "04_Beep", "05_Stomp",
          "06_Jump", "08_OpenChest", "10_Fail"):
    shutil.copyfile(f"{src}/src/sounds/{n}.raw", f"{out}/sfx/{n}.raw")
