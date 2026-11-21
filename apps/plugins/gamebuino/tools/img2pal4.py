#!/usr/bin/env python3
"""Rewrite Gamebuino META RGB565 image arrays (const uint16_t NAME[] = {...};)
as the layer's 4-bit palette format (colour mode 2): the six header words,
a 16 entry RGB565 palette, then packed 4-bit pixels (high nibble first, rows
padded to bytes).  Everything else in the file is copied unchanged.  Images
with more than 16 colours are left as they are.

usage: img2pal4.py in.h out.h
"""
import re
import sys

ARRAY = re.compile(r'const\s+uint16_t\s+(\w+)\[\]\s*=\s*\{(.*?)\};', re.S)


def convert(m):
    body = re.sub(r'//[^\n]*', '', m.group(2))
    vals = [int(x, 0) for x in re.findall(r'0[xX][0-9a-fA-F]+|\d+', body)]
    w, h, frames, loop, transp, mode = [v & 0xFFFF for v in vals[:6]]
    px = [v & 0xFFFF for v in vals[6:]]
    if mode != 0 or len(px) != w * h * frames:
        return m.group(0)
    pal = sorted(set(px))
    if len(pal) > 16:
        return m.group(0)
    tidx = pal.index(transp) if transp in pal else 0xFFFF
    idx = {c: i for i, c in enumerate(pal)}
    pal += [0] * (16 - len(pal))
    data = bytearray()
    for f in range(frames):
        for y in range(h):
            row = [idx[c] for c in px[(f * h + y) * w:(f * h + y + 1) * w]]
            if len(row) & 1:
                row.append(0)
            data += bytes((row[i] << 4) | row[i + 1] for i in range(0, len(row), 2))
    if len(data) & 1:
        data.append(0)
    words = [w, h, frames, loop, tidx, 2] + pal
    words += [data[i] | (data[i + 1] << 8) for i in range(0, len(data), 2)]
    lines = []
    for i in range(0, len(words), 12):
        lines.append('  ' + ', '.join('0x%04x' % x for x in words[i:i + 12]) + ',')
    return 'const uint16_t %s[] = {\n%s\n};' % (m.group(1), '\n'.join(lines))


src = open(sys.argv[1]).read()
open(sys.argv[2], 'w').write(ARRAY.sub(convert, src))
