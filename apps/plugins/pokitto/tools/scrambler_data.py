#!/usr/bin/env python3
"""Extract Scrambler sound data from the upstream repository for the SD card.

usage: scrambler_data.py UPSTREAM_DIR OUT_DIR
Writes OUT_DIR/music/Scram_0{1,2}.raw and OUT_DIR/sfx/*.raw (unsigned 8-bit
mono, 16 kHz; the effects are converted back from C arrays) to be installed
as .rockbox/rocks/games/pokitto_scrambler/.
"""
import os, re, shutil, sys

src, out = sys.argv[1], sys.argv[2]
os.makedirs(out + "/music", exist_ok=True)
os.makedirs(out + "/sfx", exist_ok=True)
for n in ("Scram_01", "Scram_02"):
    shutil.copyfile(f"{src}/music/{n}.raw", f"{out}/music/{n}.raw")
for n in ("Explosion_00", "Explosion_01", "Explosion_02", "PlayerBomb",
          "PlayerLaser", "RocketLaunch", "SurfaceToAir", "FuelUp",
          "StageComplete"):
    text = open(f"{src}/src/sounds/{n}.h").read()
    body = text[text.index("{") + 1:text.rindex("}")]
    open(f"{out}/sfx/{n}.raw", "wb").write(bytes(int(v) for v in re.findall(r"\d+", body)))
