#!/usr/bin/env python3
"""Extract OBS sound data from the upstream repository for the SD card.

usage: obs_data.py UPSTREAM_DIR OUT_DIR
Writes OUT_DIR/music/OBS_0{0,1,2}.raw and OUT_DIR/sfx/*.raw (unsigned 8-bit
mono, 16 kHz) to be installed as .rockbox/rocks/games/obs/. The sound
effects the upstream repository only has as C arrays are converted back.
"""
import os, re, shutil, sys

src, out = sys.argv[1], sys.argv[2]
os.makedirs(out + "/music", exist_ok=True)
os.makedirs(out + "/sfx", exist_ok=True)
for n in ("OBS_00", "OBS_01", "OBS_02"):
    shutil.copyfile(f"{src}/music/{n}.raw", f"{out}/music/{n}.raw")
for n in ("Laser", "Mini_Explosion", "Player_Hit", "Game_Over",
          "Typewriter", "Health"):
    text = open(f"{src}/src/sounds/{n}.h").read()
    body = text[text.index("{") + 1:text.rindex("}")]
    data = bytes(int(v) for v in re.findall(r"\d+", body))
    open(f"{out}/sfx/{n}.raw", "wb").write(data)
