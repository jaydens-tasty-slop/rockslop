#!/usr/bin/env python3
"""Extract Lights Out sound data from the upstream repository for the SD card.

usage: lightsout_data.py UPSTREAM_DIR OUT_DIR
The upstream repository only has the sounds as C arrays; they are converted
back to raw unsigned 8-bit mono 16 kHz: OUT_DIR/music/lights{1,2}.raw (the
intro and end tunes) and OUT_DIR/sfx/Switch.raw, to be installed as
.rockbox/rocks/games/pokitto_lightsout/.
"""
import os, re, sys

src, out = sys.argv[1], sys.argv[2]
os.makedirs(out + "/music", exist_ok=True)
os.makedirs(out + "/sfx", exist_ok=True)
for name, dest in (("Intro", "music/lights1"), ("End", "music/lights2"),
                   ("Switch", "sfx/Switch")):
    text = open(f"{src}/src/sounds/{name}.h").read()
    body = text[text.index("{") + 1:text.rindex("}")]
    data = bytes(int(v) for v in re.findall(r"\d+", body))
    open(f"{out}/{dest}.raw", "wb").write(data)
