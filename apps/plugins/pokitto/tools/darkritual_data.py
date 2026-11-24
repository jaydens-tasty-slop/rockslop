#!/usr/bin/env python3
"""Extract the Dark Ritual sound files and image data from the upstream repository.

usage: darkritual_data.py UPSTREAM_DIR OUT_DIR
Writes OUT_DIR/music/darkrit*.raw (unsigned 8-bit mono, 16 kHz) and
OUT_DIR/images.bin (see pk_images.py), to be
installed as .rockbox/rocks/games/pokitto_darkritual/.
"""
import glob, os, shutil, subprocess, sys

src, out = sys.argv[1], sys.argv[2]
os.makedirs(out + "/music", exist_ok=True)
for f in sorted(glob.glob(f"{src}/distributable/music/darkrit*.raw")):
    shutil.copyfile(f, f"{out}/music/{os.path.basename(f)}")
subprocess.check_call([sys.executable, "-I", os.path.dirname(os.path.abspath(__file__)) + "/pk_images.py",
                       src + "/src/images", "--blob", out + "/images.bin"])
