#!/usr/bin/env python3
"""Build the fallback fonts in assets/fonts/fallback/ (fontTools required).

The UI's own fonts (Orbitron, Share Tech Mono) cover Latin-1. Everything else
falls back, glyph by glyph, to Noto, and past Noto to GNU Unifont:

  NotoSans{,Arabic,Hebrew,Devanagari,Thai}-{Regular,Bold}.ttf
      whole script blocks (Latin, Greek, Cyrillic; Arabic; Hebrew;
      Devanagari; Thai), layout tables kept for shaping
  NotoSans{SC,TC,JP,KR}-{Regular,Bold}.otf
      only the CJK characters the translations use (and the language
      picker's names), from Noto Sans CJK's per-region subset fonts
  unifont.otf.xz
      GNU Unifont, the whole Basic Multilingual Plane, xz-compressed

Usage:
  packaging/fonts.py --noto /usr/share/fonts/noto --cjk DIR --unifont unifont-X.otf

DIR holds NotoSans{SC,TC,JP,KR}-{Regular,Bold}.otf from
https://github.com/notofonts/noto-cjk (Sans/SubsetOTF/<region>/).
Rerun it when a CJK translation changes.
"""
import argparse
import lzma
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
OUT = ROOT / "assets" / "fonts" / "fallback"
LANG = ROOT / "assets" / "lang"

SCRIPTS = {
    "NotoSans": "U+0020-007E,U+00A0-02FF,U+0300-036F,U+0370-03FF,U+0400-052F,"
    "U+1E00-1EFF,U+2000-206F,U+20A0-20CF,U+2100-214F,U+2190-21FF,U+2212,U+25CC",
    "NotoSansArabic": "U+0600-06FF,U+0750-077F,U+08A0-08FF,U+FB50-FDFF,U+FE70-FEFF,U+200C-200F,U+25CC",
    "NotoSansHebrew": "U+0590-05FF,U+FB1D-FB4F,U+200C-200F,U+25CC",
    "NotoSansDevanagari": "U+0900-097F,U+A8E0-A8FF,U+1CD0-1CFF,U+200C-200D,U+25CC",
    "NotoSansThai": "U+0E00-0E7F,U+25CC",
}
CJK = {"SC": "chinese-simp", "TC": "chinese-trad", "JP": "japanese", "KR": "korean"}
# the language picker shows these in every language
PICKER = "简体中文繁體中文日本語한국어"


def is_cjk(c):
    o = ord(c)
    return (
        0x2E80 <= o <= 0x9FFF  # radicals, punctuation, kana, Han
        or 0xAC00 <= o <= 0xD7AF  # Hangul syllables
        or 0x1100 <= o <= 0x11FF
        or 0xF900 <= o <= 0xFAFF
        or 0xFF00 <= o <= 0xFFEF  # full-width forms
    )


def translations(lang):
    """The translated text (not the English keys) of one language."""
    out = []
    path = LANG / f"{lang}.txt"
    if path.exists():
        for line in path.read_text(encoding="utf-8").splitlines():
            if line.startswith("= "):
                out.append(line[2:])
    return "".join(out)


def subset(src, dst, unicodes=None, text=None):
    args = [
        "pyftsubset",
        str(src),
        f"--output-file={dst}",
        "--layout-features=*",
        "--no-hinting",
        "--name-IDs=0,1,2,3,4,5,6,13,14",
        "--notdef-outline",
    ]
    if unicodes:
        args.append(f"--unicodes={unicodes}")
    if text:
        args.append(f"--text={text}")
    subprocess.run(args, check=True)
    print(f"{dst.name}: {dst.stat().st_size} bytes")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--noto", required=True, type=pathlib.Path)
    ap.add_argument("--cjk", type=pathlib.Path)
    ap.add_argument("--unifont", type=pathlib.Path)
    a = ap.parse_args()
    OUT.mkdir(parents=True, exist_ok=True)
    for name, ranges in SCRIPTS.items():
        for w in ("Regular", "Bold"):
            subset(a.noto / f"{name}-{w}.ttf", OUT / f"{name}-{w}.ttf", unicodes=ranges)
    if a.cjk:
        for region, lang in CJK.items():
            chars = "".join(sorted({c for c in translations(lang) + PICKER if is_cjk(c)}))
            for w in ("Regular", "Bold"):
                subset(a.cjk / f"NotoSans{region}-{w}.otf", OUT / f"NotoSans{region}-{w}.otf", text=chars)
    if a.unifont:
        data = lzma.compress(a.unifont.read_bytes(), preset=9 | lzma.PRESET_EXTREME)
        (OUT / "unifont.otf.xz").write_bytes(data)
        print(f"unifont.otf.xz: {len(data)} bytes")
    return 0


if __name__ == "__main__":
    sys.exit(main())
