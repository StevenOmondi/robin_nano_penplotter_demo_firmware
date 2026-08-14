#!/usr/bin/env python3
"""Render .bin icons to PNG for visual verification."""
import os
import struct
import sys
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
ASSETS = os.path.join(HERE, "..", "assets_icons")
OUT = os.path.join(HERE, "..", "assets_icons", "preview")
os.makedirs(OUT, exist_ok=True)

names = sys.argv[1:] if len(sys.argv) > 1 else sorted(f for f in os.listdir(ASSETS) if f.startswith("bmp_plot") and f.endswith(".bin"))
for n in names:
    with open(os.path.join(ASSETS, n), "rb") as fh:
        d = fh.read()
    w, h = (d[1] >> 2) | ((d[2] & 0x1F) << 6), (d[2] >> 5) | (d[3] << 3)
    img = Image.new("RGB", (w, h))
    px = img.load()
    for y in range(h):
        for x in range(w):
            v = struct.unpack_from("<H", d, 4 + (y * w + x) * 2)[0]
            px[x, y] = ((v >> 11) & 0x1F) << 3, ((v >> 5) & 0x3F) << 2, (v & 0x1F) << 3
    img.save(os.path.join(OUT, n.replace(".bin", ".png")))
    print(f"wrote {n} -> {w}x{h}")
