#!/usr/bin/env python3
"""Extract the Abbaye des Morts data files from the upstream Pokitto port.

usage: abbaye_data.py UPSTREAM_DIR OUT_DIR
Writes OUT_DIR/abbaye/{map.bin,enemies.bin,gfx/*.gfx,music/*.raw,sfx/*.raw}
to be installed as .rockbox/rocks/games/pokitto_abbaye/. Music and effects
are unsigned 8-bit mono (8 kHz music, effects as stored upstream).
"""
import glob, os, shutil, sys

src, out = sys.argv[1], sys.argv[2]
for sub in ("gfx", "music", "sfx"):
    os.makedirs(f"{out}/abbaye/{sub}", exist_ok=True)
for f in ("map.bin", "enemies.bin"):
    shutil.copyfile(f"{src}/sdcard/{f}", f"{out}/abbaye/{f}")
for f in glob.glob(f"{src}/sdcard/gfx/*.gfx"):
    shutil.copyfile(f, f"{out}/abbaye/gfx/{os.path.basename(f)}")
for f in glob.glob(f"{src}/sdcard/music/*.raw"):
    shutil.copyfile(f, f"{out}/abbaye/music/{os.path.basename(f)}")
for f in glob.glob(f"{src}/sfx/*.raw"):
    shutil.copyfile(f, f"{out}/abbaye/sfx/{os.path.basename(f)}")
