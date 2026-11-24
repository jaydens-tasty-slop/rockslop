#!/usr/bin/env python3
"""Extract StackShift data from the upstream repository for the SD card.

usage: stackshift_data.py UPSTREAM_DIR OUT_DIR
Writes OUT_DIR/images.bin (see pk_images.py) and OUT_DIR/music/Stack_01..08.raw, to be
installed as .rockbox/rocks/games/pokitto_stackshift/.
"""
import os, shutil, subprocess, sys

src, out = sys.argv[1], sys.argv[2]
os.makedirs(out + "/music", exist_ok=True)
subprocess.check_call([sys.executable, "-I", os.path.dirname(os.path.abspath(__file__)) + "/pk_images.py",
                       src + "/src/images", "--blob", out + "/images.bin"])
for n in range(1, 9):
    shutil.copyfile(f"{src}/music/Stack_{n:02d}.raw", f"{out}/music/Stack_{n:02d}.raw")
