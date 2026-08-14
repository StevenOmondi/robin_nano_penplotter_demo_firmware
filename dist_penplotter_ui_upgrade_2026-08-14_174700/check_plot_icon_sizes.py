#!/usr/bin/env python3
"""
Verify every icon in assets_icons fits its SPI flash slot.

Reproduces the RLE scheme used by SPIFlashStorage.cpp (rle_compress over
uint16 words): replicate runs [count][pixel], literal runs [negative][data].
The 9 KB per-image budget is PER_PIC_MAX_SPACE_TFT35 (9216 bytes).
"""

import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ASSETS = os.path.join(HERE, "..", "assets_icons")
SLOT = 9216  # PER_PIC_MAX_SPACE_TFT35


def rle_size(words):
    n = len(words)
    count = out = index = i = 0
    max_run = 32767
    while count < n:
        index = count
        pixel = words[index]
        index += 1
        while index < n and (index - count) < max_run and words[index] == pixel:
            index += 1
        if index - count == 1:
            while index < n and (index - count) < max_run and (words[index] != words[index - 1] or (index > 1 and words[index] != words[index - 2])):
                index += 1
            while index < n and words[index] == words[index - 1]:
                index -= 1
            out += 1 + (index - count)
            for _ in range(count, index):
                pass
        else:
            out += 2
        count = index
    return out


def main():
    files = sorted(f for f in os.listdir(ASSETS) if f.startswith("bmp_plot") and f.endswith(".bin"))
    if not files:
        sys.exit("no icons found")
    ok = True
    for f in files:
        with open(os.path.join(ASSETS, f), "rb") as fh:
            data = fh.read()
        w, h = (data[1] >> 2) | ((data[2] & 0x1F) << 6), (data[2] >> 5) | (data[3] << 3)
        words = struct.unpack("<%dH" % (w * h), data[4:])
        size = rle_size(words)
        status = "OK " if size <= SLOT else "FAIL"
        if size > SLOT:
            ok = False
        print(f"{status} {f}: {w}x{h} raw={len(words)*2}B rle={size}B (slot {SLOT}B)")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
