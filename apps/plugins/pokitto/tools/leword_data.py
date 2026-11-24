#!/usr/bin/env python3
"""Extract LeWord data from the upstream repository for the SD card.

usage: leword_data.py UPSTREAM_DIR OUT_DIR
Writes OUT_DIR/images.bin (see pk_images.py), OUT_DIR/music/ (the word lists) and
OUT_DIR/sfx/*.raw (converted back from the C arrays) to be installed as
.rockbox/rocks/games/pokitto_leword/.
"""
import os, re, shutil, subprocess, sys

src, out = sys.argv[1], sys.argv[2]
os.makedirs(out + "/music", exist_ok=True)
os.makedirs(out + "/sfx", exist_ok=True)
subprocess.check_call([sys.executable, "-I", os.path.dirname(os.path.abspath(__file__)) + "/pk_images.py",
                       src + "/src/images", "--blob", out + "/images.bin"])
# The tunes (LeWord02-04.raw) name no author or licence upstream: not shipped.
for n in ("LeWord00.img", "LeWord01.img"):
    shutil.copyfile(f"{src}/music/{n}", f"{out}/music/{n}")
for n in ("Success", "Ding_00", "Ding_01", "Ding_02", "Ding_03"):
    text = open(f"{src}/src/sounds/{n}.h").read()
    body = text[text.index("{") + 1:text.rindex("}")]
    open(f"{out}/sfx/{n}.raw", "wb").write(bytes(int(v) for v in re.findall(r"\d+", body)))
