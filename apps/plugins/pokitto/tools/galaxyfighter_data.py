#!/usr/bin/env python3
"""Copy GalaxyFighter sound data from the upstream repository for the SD card.

usage: galaxyfighter_data.py UPSTREAM_DIR OUT_DIR
Writes OUT_DIR/music/galaxy{1..9,A}.raw and OUT_DIR/sfx/*.raw (unsigned 8-bit
mono, 8 kHz) to be installed as .rockbox/rocks/games/pokitto_galaxyfighter/.
OUT_DIR/images.bin holds the image data (see pk_images.py).
The stage start jingle only exists as a C array and is converted back.
"""
import os, re, shutil, subprocess, sys

src, out = sys.argv[1], sys.argv[2]
os.makedirs(out + "/music", exist_ok=True)
os.makedirs(out + "/sfx", exist_ok=True)
fx = f"{src}/Assets/Sound Effects"
for n in "123456789A":
    shutil.copyfile(f"{src}/music/galaxy{n}.raw", f"{out}/music/galaxy{n}.raw")
for dest, name in (("Coin", "10 Coin"), ("NewLife", "09 NewLife"),
                   ("Shoot", "14 Shoot"), ("ExtraLife", "ExtraLife")):
    shutil.copyfile(f"{fx}/{name}.raw", f"{out}/sfx/{dest}.raw")
subprocess.check_call([sys.executable, "-I", os.path.dirname(os.path.abspath(__file__)) + "/pk_images.py",
                       src + "/src/images", "--blob", out + "/images.bin"])
text = open(f"{src}/src/sounds/SequenceStart.h").read()
body = text[text.index("{") + 1:text.rindex("}")]
open(f"{out}/sfx/SequenceStart.raw", "wb").write(
    bytes(int(v) for v in re.findall(r"\d+", body)))
