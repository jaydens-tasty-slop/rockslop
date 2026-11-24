#!/usr/bin/env python3
"""Extract RoadTrip data from the upstream repository for the SD card.

usage: roadtrip_data.py UPSTREAM_DIR OUT_DIR
Writes OUT_DIR/images.bin (see pk_images.py), OUT_DIR/music/roadtr{20..32}.raw
(the short radio sounds) and OUT_DIR/sfx/*.raw (converted back from the C
arrays; unsigned 8-bit mono) to be installed as
.rockbox/rocks/games/pokitto_roadtrip/. The tunes (roadtr01-05) name no author
or licence upstream: not shipped.
"""
import os, re, shutil, subprocess, sys

src, out = sys.argv[1], sys.argv[2]
os.makedirs(out + "/music", exist_ok=True)
os.makedirs(out + "/sfx", exist_ok=True)
subprocess.check_call([sys.executable, "-I", os.path.dirname(os.path.abspath(__file__)) + "/pk_images.py",
                       src + "/src/images", "--blob", out + "/images.bin"])
for n in range(20, 33):
    shutil.copyfile(f"{src}/music/roadtr{n:02d}.raw", f"{out}/music/roadtr{n:02d}.raw")
for n in ("Bump", "ButtonPress", "Passing", "Skid", "Beep"):
    text = open(f"{src}/src/sounds/{n}.h").read()
    body = text[text.index("{") + 1:text.rindex("}")]
    open(f"{out}/sfx/{n}.raw", "wb").write(bytes(int(v) for v in re.findall(r"\d+", body)))
