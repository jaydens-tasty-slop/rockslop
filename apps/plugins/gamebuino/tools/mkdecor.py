#!/usr/bin/env python3
"""Extract the large indexed images (const uint8_t NAME[] = {...};) named on
the command line from a Gamebuino graphics header into one binary file, each
image (header included) padded to the size of the largest, and print the
record size.

usage: mkdecor.py graphicAssets.h out.dat sp1DecorData sp2DecorData ...
"""
import re
import sys

src = open(sys.argv[1]).read()
recs = []
for name in sys.argv[3:]:
    m = re.search(r'const\s+uint8_t\s+%s\[\]\s*=\s*\{(.*?)\};' % name, src, re.S)
    body = re.sub(r'//[^\n]*', '', m.group(1))
    recs.append(bytes(int(x, 0) for x in re.findall(r'0[xX][0-9a-fA-F]+|\b\d+\b', body)))
size = max(len(r) for r in recs)
with open(sys.argv[2], 'wb') as f:
    for r in recs:
        f.write(r + bytes(size - len(r)))
print(size)
