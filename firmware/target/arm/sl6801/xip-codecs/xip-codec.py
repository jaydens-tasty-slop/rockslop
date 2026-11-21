#!/usr/bin/env python3
"""XIP codec images (codec-xip.lds).

    xip-codec.py split OBJCOPY CODEC.elf OUT.flash
    xip-codec.py pack OUT.flash CODEC.flash...

split: OUT.flash is the codec's image for its flash slot (the header's CPU
address minus the XIP alias 0xC00000).
pack: the codec flash area image, erased bytes between the codecs, ending in
the directory the loader finds them by (struct codec_xip_dir, named after
each CODEC.flash); write it at the offset printed.
"""
import struct, subprocess, sys, tempfile
from pathlib import Path

XIP_ALIAS = 0xC00000
XIP_MAGIC = 0x52435850          # CODEC_XIP_MAGIC
HEADER = struct.Struct("<IHHIII IIII IIII")  # struct codec_xip_header
AREA = 0x0E0000                 # XIP_CODEC_AREA_START - XIP_ALIAS (yp3.h)
AREA_SIZE = 0x70000             # XIP_CODEC_AREA_SIZE (yp3.h)
DIR_MAGIC = 0x44435850          # CODEC_XIP_DIR_MAGIC (codecs.h)
DIR_SIZE = 0x100                # CODEC_XIP_DIR_SIZE
DIR_HEAD = struct.Struct("<II")
DIR_ENTRY = struct.Struct("<12sI")


def header(img, name):
    (magic, _target, _api, load, end, _build_id, _entry, _run, _ci, _api_size,
     fstart, fend, dload, dend) = HEADER.unpack_from(img)
    if magic != XIP_MAGIC:
        sys.exit(f"{name}: not an XIP codec (magic {magic:#x})")
    return load, end, fstart - XIP_ALIAS, fend - XIP_ALIAS, dend


def split(objcopy, elf, flash_out):
    with tempfile.TemporaryDirectory() as tmp:
        raw = Path(tmp) / "image.bin"
        subprocess.check_call([objcopy, "-O", "binary", elf, str(raw)])
        img = raw.read_bytes()
    load, end, start, stop, dend = header(img, elf)
    if stop - start != len(img):
        sys.exit(f"{elf}: image {len(img):#x} B, header says {stop - start:#x}")
    Path(flash_out).write_bytes(img)
    print(f"{Path(flash_out).name}: flash {start:#08x}..{stop:#08x} ({len(img)} B); "
          f"arena: RAM image {dend - load} B, bss {end - dend} B, "
          f"code end {end:#x}")


def pack(out, *images):
    area = bytearray(b"\xff" * AREA_SIZE)
    used = [(AREA + AREA_SIZE - DIR_SIZE, AREA + AREA_SIZE, "the directory")]
    entries = []
    for name in images:
        img = Path(name).read_bytes()
        _, _, start, stop, _ = header(img, name)
        if start < AREA or stop > AREA + AREA_SIZE:
            sys.exit(f"{name}: flash {start:#x}..{stop:#x} outside the codec area")
        for s, e, other in used:
            if start < e and s < stop:
                sys.exit(f"{name}: flash {start:#x}..{stop:#x} overlaps {other}")
        used.append((start, stop, name))
        area[start - AREA:stop - AREA] = img
        codec = Path(name).stem.encode()
        if len(codec) >= DIR_ENTRY.size - 4:
            sys.exit(f"{name}: name too long for the directory")
        entries.append(DIR_ENTRY.pack(codec, start + XIP_ALIAS))
    directory = DIR_HEAD.pack(DIR_MAGIC, len(entries)) + b"".join(entries)
    if len(directory) > DIR_SIZE:
        sys.exit(f"{len(entries)} codecs do not fit the directory")
    area[AREA_SIZE - DIR_SIZE:AREA_SIZE - DIR_SIZE + len(directory)] = directory
    Path(out).write_bytes(bytes(area))
    print(f"{Path(out).name}: write at flash {AREA:#08x}..{AREA + AREA_SIZE:#08x} "
          f"({len(images)} codecs)")


if __name__ == "__main__":
    cmds = {"split": (split, 3), "pack": (pack, None)}
    if len(sys.argv) < 3 or sys.argv[1] not in cmds or \
            cmds[sys.argv[1]][1] not in (None, len(sys.argv) - 2):
        sys.exit(__doc__)
    cmds[sys.argv[1]][0](*sys.argv[2:])
