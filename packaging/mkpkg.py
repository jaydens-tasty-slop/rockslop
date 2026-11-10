#!/usr/bin/env python3
"""Build a rockslop release package: a plain .tar.xz.

    MANIFEST              format=N, target=..., metadata, expect= lines
    SHA256SUMS            sha256sum format, one line per file below
    flash/0x0000e000.bin  a flash image, written at the offset in its name
    card/.rockbox/...     the SD card tree, copied to the card root

Usage (run from anywhere; paths are ordinary paths):

  YP3, from a rockbox-yp3 release build (yp3dev/distro.sh):
    mkpkg.py yp3 --elf build/distro/build/rockbox.elf --base build/base.bin \\
        --codecs build/distro/build/lib/rbcodec/codecs/xip/codecs.flash \\
        --card build/distro/card --version 4.0-yp3 -o rockbox-yp3-4.0.tar.xz

    --base is a full 2 MiB stock flash image; its bootloader and partition
    table are recorded as expect= lines, so the flasher refuses a player
    whose boot sectors differ. The ELF is placed by yp3dev/package.py
    (found next to this checkout, or pass --placer).

  TS17, from a flat binary (objcopy -O binary of rockbox.elf) or the ELF:
    mkpkg.py ts17 --bin rockbox.bin --card CARD --version V -o out.tar.xz
    mkpkg.py ts17 --elf rockbox.elf --card CARD --version V -o out.tar.xz

  Anything else:
    mkpkg.py raw --target yp3 --flash 0xE000:image.bin [--flash ...] \\
        [--expect 0:0xE000:base.bin] [--card DIR] --version V -o out.tar.xz

--fast uses xz preset 6 instead of 9e (for test builds).
"""
import argparse
import datetime
import hashlib
import io
import lzma
import os
import subprocess
import sys
import tarfile
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
# The package layout's version (rockslop's pkg::FORMAT): bump both together
# when a flasher that ignored a change would do the wrong thing.
FORMAT = 1  # rockslop refuses formats it doesn't read
YP3_IMAGE = 2 * 1024 * 1024
YP3_REGION = (0xE000, 0x150000)     # release firmware + XIP codecs
YP3_CODECS = 0xE0000                # XIP_CODEC_AREA_START - 0xC00000
YP3_BOOT = [(0x0, 0xD000), (0xD000, 0x1000)]   # bootloader, partition table
TS17_SYS = 0x14000                  # fw0_sys
TS17_SYS_MAX = 0x1A7000
WRITABLE = {"yp3": (0xE000, 0x1FC000), "ts17": (0x14000, 0x1BB000)}
BOOTFILES = {".rockbox/rockbox.yp3", ".rockbox/rockbox.ts17"}   # Rockbox's BOOTFILE


def die(msg):
    sys.exit(f"mkpkg: {msg}")


def num(s):
    return int(s, 0)


def sha(b):
    return hashlib.sha256(b).hexdigest()


def yp3_flash(a):
    placer = Path(a.placer) if a.placer else HERE.parent.parent / "yp3dev" / "package.py"
    if not placer.exists():
        die(f"{placer} not found; pass --placer path/to/yp3dev/package.py")
    base = Path(a.base).read_bytes()
    if len(base) != YP3_IMAGE:
        die("--base must be a full 2 MiB flash image")
    with tempfile.TemporaryDirectory() as tmp:
        out = Path(tmp) / "image.bin"
        subprocess.run([sys.executable, str(placer), a.elf, a.base, str(out)], check=True)
        img = bytearray(out.read_bytes())
    if a.codecs:
        c = Path(a.codecs).read_bytes()
        if YP3_CODECS + len(c) > YP3_REGION[1]:
            die("codecs.flash does not fit below 0x150000")
        img[YP3_CODECS:YP3_CODECS + len(c)] = c
    lo, hi = YP3_REGION
    region = bytes(img[lo:hi]).rstrip(b"\xff")
    region += b"\xff" * (-len(region) % 4096)
    expect = [(off, n, sha(base[off:off + n])) for off, n in YP3_BOOT]
    return [(lo, region)], expect


def ts17_flash(a):
    if a.elf:
        tc = os.environ.get("TC", "arm-none-eabi-")
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp) / "rockbox.bin"
            subprocess.run([f"{tc}objcopy", "-O", "binary", a.elf, str(out)], check=True)
            data = out.read_bytes()
    else:
        data = Path(a.bin).read_bytes()
    if len(data) > TS17_SYS_MAX:
        die(f"image is {len(data)} bytes; fw0_sys holds {TS17_SYS_MAX}")
    return [(TS17_SYS, data)], []


def raw_flash(a):
    segs = []
    for f in a.flash or []:
        off, path = f.split(":", 1)
        segs.append((num(off), Path(path).read_bytes()))
    expect = []
    for e in a.expect or []:
        off, n, path = e.split(":", 2)
        off, n = num(off), num(n)
        data = Path(path).read_bytes()
        if len(data) < off + n:
            die(f"--expect {e}: file too short")
        expect.append((off, n, sha(data[off:off + n])))
    return segs, expect


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("kind", choices=("yp3", "ts17", "raw"))
    p.add_argument("-o", "--out", required=True)
    p.add_argument("--version", required=True)
    p.add_argument("--title", default="")
    p.add_argument("--card", help="directory whose contents go on the SD card (holds .rockbox)")
    p.add_argument("--fast", action="store_true")
    p.add_argument("--elf")
    p.add_argument("--bin")
    p.add_argument("--base")
    p.add_argument("--codecs")
    p.add_argument("--placer")
    p.add_argument("--target", choices=("yp3", "ts17"))
    p.add_argument("--flash", action="append", help="OFFSET:FILE")
    p.add_argument("--expect", action="append", help="OFFSET:SIZE:FILE (hash that range of FILE)")
    a = p.parse_args()

    if a.kind == "yp3":
        if not (a.elf and a.base):
            die("yp3 needs --elf and --base")
        target, (segs, expect) = "yp3", yp3_flash(a)
    elif a.kind == "ts17":
        if bool(a.elf) == bool(a.bin):
            die("ts17 needs one of --elf or --bin")
        target, (segs, expect) = "ts17", ts17_flash(a)
    else:
        if not a.target or not a.flash:
            die("raw needs --target and at least one --flash")
        target, (segs, expect) = a.target, raw_flash(a)

    lo, hi = WRITABLE[target]
    for off, data in segs:
        if off < lo or off + len(data) > hi or off % 4096:
            die(f"flash image at {off:#x}+{len(data):#x} is outside {lo:#x}..{hi:#x} or unaligned")

    card = []
    if a.card:
        root = Path(a.card)
        if not (root / ".rockbox").is_dir():
            die(f"{root} has no .rockbox directory")
        for dirpath, dirnames, filenames in os.walk(root):
            dirnames.sort()
            for n in sorted(dirnames + filenames):
                full = Path(dirpath) / n
                if full.is_symlink():
                    die(f"{full}: symlinks are not allowed in the card tree")
                rel = full.relative_to(root).as_posix()
                if rel in BOOTFILES:
                    # the firmware goes to flash; a copy on the card makes
                    # Rockbox offer to reboot into it from there (ROLO),
                    # which fails on these players
                    print(f"mkpkg: leaving out {rel}: the firmware is in flash", file=sys.stderr)
                    continue
                card.append((rel, full))
        card.sort(key=lambda x: x[0])

    epoch = int(os.environ.get("SOURCE_DATE_EPOCH", datetime.datetime.now(datetime.timezone.utc).timestamp()))
    built = datetime.datetime.fromtimestamp(epoch, datetime.timezone.utc).strftime("%Y-%m-%d")
    manifest = [f"format={FORMAT}", f"target={target}", f"version={a.version}", f"built={built}"]
    if a.title:
        manifest.append(f"title={a.title}")
    manifest += [f"expect=0x{off:x} 0x{n:x} {h}" for off, n, h in expect]
    sums = [f"{sha(data)}  flash/0x{off:08x}.bin" for off, data in segs]
    for rel, full in card:
        if full.is_file():
            h = hashlib.sha256()
            with open(full, "rb") as f:
                for chunk in iter(lambda: f.read(1 << 20), b""):
                    h.update(chunk)
            sums.append(f"{h.hexdigest()}  card/{rel}")

    def info(name, size, d=False):
        t = tarfile.TarInfo(name)
        t.size = size
        t.mtime = epoch
        t.mode = 0o755 if d else 0o644
        t.type = tarfile.DIRTYPE if d else tarfile.REGTYPE
        t.uid = t.gid = 0
        t.uname = t.gname = ""
        return t

    def add_bytes(tar, name, data):
        tar.addfile(info(name, len(data)), io.BytesIO(data))

    preset = 6 if a.fast else 9 | lzma.PRESET_EXTREME
    out = Path(a.out)
    tmp = out.with_name(out.name + ".part")
    with tarfile.open(tmp, "w:xz", preset=preset, format=tarfile.PAX_FORMAT) as tar:
        add_bytes(tar, "MANIFEST", ("\n".join(manifest) + "\n").encode())
        add_bytes(tar, "SHA256SUMS", ("\n".join(sums) + "\n").encode())
        tar.addfile(info("flash", 0, d=True))
        for off, data in segs:
            add_bytes(tar, f"flash/0x{off:08x}.bin", data)
        if card:
            tar.addfile(info("card", 0, d=True))
        for rel, full in card:
            if full.is_dir():
                tar.addfile(info(f"card/{rel}", 0, d=True))
            else:
                with open(full, "rb") as f:
                    tar.addfile(info(f"card/{rel}", full.stat().st_size), f)
    tmp.replace(out)
    nfiles = sum(1 for _, f in card if f.is_file())
    print(f"{out}: {target} {a.version}, flash {sum(len(d) for _, d in segs)} bytes in {len(segs)} image(s), "
          f"card {nfiles} files, {out.stat().st_size} bytes")


if __name__ == "__main__":
    main()
