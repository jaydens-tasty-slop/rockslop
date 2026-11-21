#!/usr/bin/env python3
"""Extract RGB565 picture arrays (const uint16_t NAME[] = {...};) named on the
command line from a Gamebuino header into one binary file: the pictures
back to back as little-endian 16-bit words (an Image header, when the array
has one, is kept).

usage: mkpics16.py Pictures.h out.dat NAME1 NAME2 ...
"""
import re
import struct
import sys

src = open(sys.argv[1]).read()
with open(sys.argv[2], 'wb') as f:
    for name in sys.argv[3:]:
        m = re.search(r'(?:const|constexpr)\s+uint16_t\s+%s\[\]\s*=\s*\{(.*?)\};' % name, src, re.S)
        body = re.sub(r'//[^\n]*', '', m.group(1))
        px = [int(x, 0) for x in re.findall(r'0[xX][0-9a-fA-F]+|\b\d+\b', body)]
        f.write(struct.pack('<%dH' % len(px), *px))
