#!/usr/bin/env python3
"""Place rockbox.elf into a full 2 MiB flash image.

    package.py ELF BASE.bin OUT.bin

BASE is a full flash image of the player (2 MiB of 0xFF will do); its
bootloader [0, 0xE000) and settings [0x1FC000, 2M) are kept byte-for-byte.
The release region [0xE000, 0x150000) starts erased: Rockbox, then its XIP
codecs (placed by the caller at 0xE0000).

Layout contract with the player's bootloader (the partition table at 0xD000
is left untouched, its FIRM entry points at 0xE000):
  * a FIRM header from its documented fields (firm_header());
  * sections are placed at LMA - 0xC00000 (XIP alias), inside the FIRM span;
  * the first (__yp3_early_code_end - .text) bytes of .text are copied to
    0xE430: the bootloader copies [0xE430, 0xF430) to SRAM 0x804c00 and calls
    0x804c01, which arms recovery and sets up XIP before jumping to flash.
The bootloader checks neither the header's CRC nor the partition table's
FIRM CRC (left stale, so its sector is never written).

The cross tools are $TC followed by their name (default arm-none-eabi-, from
PATH), as in rockslop's mkpkg.py.
"""
import binascii, os, struct, subprocess, sys, tempfile
from pathlib import Path

TC = os.environ.get("TC", "arm-none-eabi-")     # prefix: f"{TC}objcopy"
IMAGE_SIZE = 2 * 1024 * 1024
XIP_ALIAS = 0xC00000
COPY_OFFSET, COPY_MAX = 0xE430, 0x1000
COPY_DEST = 0x804C00
FIRM_START, FIRM_END = 0xE000, 0xE000 + 0x192B18
HEADER = 0x30
RELEASE_END = 0x150000      # end of the XIP codec area: what a release writes
LOADED = (".text", ".vectors", ".data")
OPTIONAL = (".text_rest",)  # code beside functions app.lds puts in SRAM


def sections(elf):
    """The sections with contents that load: {name: (size, vma, lma)}"""
    out = subprocess.check_output([f"{TC}objdump", "-h", elf], text=True)
    rows, row = {}, None
    for line in out.splitlines():
        f = line.split()
        if len(f) >= 7 and f[0].isdigit():
            row = f
        elif row and "LOAD" in line and "CONTENTS" in line:
            rows[row[1]] = (int(row[2], 16), int(row[3], 16), int(row[4], 16))
            row = None
        else:
            row = None
    return rows


def symbol(elf, name):
    for line in subprocess.check_output([f"{TC}nm", elf], text=True).splitlines():
        f = line.split()
        if f[-1] == name:
            return int(f[0], 16)
    raise SystemExit(f"missing symbol {name}")


def firm_header(img):
    """FIRM header: its size, the prefix's destination, entry (Thumb) and
    length, and CRC-16/CCITT (init 0xFFFF) of the 0x1000 bytes after it;
    the other fields stay 0"""
    hdr = bytearray(HEADER)
    crc = binascii.crc_hqx(img[FIRM_START + HEADER:FIRM_START + HEADER + COPY_MAX], 0xFFFF)
    struct.pack_into("<I", hdr, 0x00, HEADER)
    struct.pack_into("<IIII", hdr, 0x10, COPY_DEST, COPY_DEST | 1, COPY_MAX, crc)
    return hdr


def package(elf, base, out):
    img = bytearray(Path(base).read_bytes())
    if len(img) != IMAGE_SIZE:
        raise SystemExit("base image must be 2 MiB")
    img[FIRM_START:RELEASE_END] = b"\xff" * (RELEASE_END - FIRM_START)
    rows = sections(elf)
    if not set(LOADED) <= set(rows) <= set(LOADED + OPTIONAL):
        raise SystemExit(f"expected sections {LOADED} (and {OPTIONAL}), got {sorted(rows)}")
    with tempfile.TemporaryDirectory() as tmp:
        for name, (size, vma, lma) in rows.items():
            start = lma - XIP_ALIAS
            if start < 0x10000 or start + size > FIRM_END:
                raise SystemExit(f"{name} [{start:#x}+{size:#x}] outside FIRM span")
            raw = Path(tmp) / "s.bin"
            subprocess.check_call([f"{TC}objcopy", "-O", "binary", "-j", name, elf, raw])
            data = raw.read_bytes()
            if len(data) != size:
                raise SystemExit(f"{name}: objcopy size mismatch")
            img[start:start + size] = data
    text_size, text_vma, text_lma = rows[".text"]
    copy = symbol(elf, "__yp3_early_code_end") - text_vma
    if not 0 < copy <= COPY_MAX:
        raise SystemExit(f"early prefix size {copy:#x} out of range")
    start = text_lma - XIP_ALIAS
    img[COPY_OFFSET:COPY_OFFSET + copy] = img[start:start + copy]
    img[FIRM_START:FIRM_START + HEADER] = firm_header(img)
    Path(out).write_bytes(img)
    end = max(lma - XIP_ALIAS + size for size, vma, lma in rows.values())
    print(f"{out}: text {text_size // 1024} KiB, image end {end:#x}, prefix {copy} B")


if __name__ == "__main__":
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    package(*sys.argv[1:])
