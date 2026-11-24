#!/usr/bin/env python3
"""Copy the Scopa music and sounds from the upstream repository for the SD card.

usage: scopa_data.py UPSTREAM_DIR OUT_DIR
Writes OUT_DIR/images.bin and title.bin (see pk_images.py), OUT_DIR/music/SCOP_08K.raw and SCOP_D01..05.raw, to be installed as
.rockbox/rocks/games/pokitto_scopa/.
"""
import os, shutil, sys

src, out = sys.argv[1], sys.argv[2]
os.makedirs(out + "/music", exist_ok=True)
import subprocess
subprocess.check_call([sys.executable, "-I", os.path.dirname(os.path.abspath(__file__)) + "/pk_images.py",
                       src + "/src/images", "--blob", out + "/all.bin"])
# The game cannot hold all of it at once: the title screen images (bytes
# 67108-134979) get their own file, loaded over the others while it is shown.
blob = open(out + "/all.bin", "rb").read()
os.remove(out + "/all.bin")
assert len(blob) == 145272
open(out + "/images.bin", "wb").write(blob[:67108] + blob[134980:])
open(out + "/title.bin", "wb").write(blob[67108:134980])
for n in ["SCOP_08K"] + ["SCOP_D0%d" % i for i in range(1, 6)]:
    shutil.copyfile(f"{src}/music/{n}.raw", f"{out}/music/{n}.raw")
