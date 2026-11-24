#!/usr/bin/env python3
"""Extract the Space Cab speech, sound and level files from the upstream repository.

usage: spacecab_data.py UPSTREAM_DIR OUT_DIR
Writes OUT_DIR/music/SPCAB_FILES/*.raw (speech and effects: unsigned 8-bit
mono, 16 kHz; SPCAB0nn.raw are level files), to be installed as
.rockbox/rocks/games/pokitto_spacecab/. The level summary SPCABLVL.raw is
rebuilt by the game and is not copied.
"""
import glob, os, shutil, sys

src, out = sys.argv[1], sys.argv[2]
os.makedirs(out + "/music/SPCAB_FILES", exist_ok=True)
for f in sorted(glob.glob(f"{src}/music/SPCAB_FILES/*.raw")):
    if not f.endswith("SPCABLVL.raw"):
        shutil.copyfile(f, f"{out}/music/SPCAB_FILES/{os.path.basename(f)}")
