#!/usr/bin/env python3
"""Copy 1nvader sound data from the upstream repository for the SD card.

usage: nvader_data.py UPSTREAM_DIR OUT_DIR
Writes OUT_DIR/music/1nvad_0{0,1,2}.raw and OUT_DIR/sfx/*.raw (unsigned 8-bit
mono, 16 kHz) to be installed as .rockbox/rocks/games/pokitto_nvader/.
"""
import os, shutil, sys

src, out = sys.argv[1], sys.argv[2]
os.makedirs(out + "/music", exist_ok=True)
os.makedirs(out + "/sfx", exist_ok=True)
for n in ("1nvad_00", "1nvad_01", "1nvad_02"):
    shutil.copyfile(f"{src}/music/{n}.raw", f"{out}/music/{n}.raw")
for n in ("drop_bomb", "Laser", "Mini_Explosion", "Level_Cleared",
          "Bomb_HitGround", "Player_Hit"):
    shutil.copyfile(f"{src}/src/sounds/{n}.raw", f"{out}/sfx/{n}.raw")
