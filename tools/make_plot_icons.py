#!/usr/bin/env python3
"""
Generate pen-plotter themed RGB565 .bin icons for the Robin Nano MKS UI.

Format (LVGL v7 LV_IMG_CF_TRUE_COLOR):
  4-byte header: [cf=0x04][w 11b][h 11b]  (bitfield, LE)
  followed by w*h*2 bytes of RGB565 (little-endian).

Palette follows the Elegoo red theme in draw_ui.h:
  BG         0x12141A -> (18,20,26)
  Surface    0x1C1F28 -> (28,31,40)
  SurfaceAlt 0x272B36 -> (39,43,54)
  Accent     0xE60012 -> (230,0,18)   Elegoo red
  AccentDark 0x99000C -> (153,0,12)
  Text       0xF4F7FB -> (244,247,251)
  Muted      0x9AA4B2 -> (154,164,178)
"""

import math
import os
import struct

from PIL import Image, ImageDraw

OUT = os.path.join(os.path.dirname(__file__), "..", "assets_icons")
BG = (18, 20, 26)
SURFACE = (28, 31, 40)
SURFACE_ALT = (39, 43, 54)
RED = (230, 0, 18)
RED_DARK = (153, 0, 12)
RED_LIGHT = (255, 96, 110)
MUTED = (154, 164, 178)
TEXT = (244, 247, 251)

SS = 4  # supersample factor

PALETTE = [BG, SURFACE, SURFACE_ALT, RED, RED_DARK, RED_LIGHT, MUTED, TEXT]


def quantize(c):
    # nearest palette color, distance computed in RGB565 space
    best, best_d = PALETTE[0], 1 << 30
    for p in PALETTE:
        d = 0
        for i in range(3):
            v = ((c[i] >> (3 if i != 1 else 2)) - (p[i] >> (3 if i != 1 else 2)))
            d += v * v
        if d < best_d:
            best, best_d = p, d
    return best


def canvas(w, h):
    img = Image.new("RGB", (w * SS, h * SS), BG)
    return img, ImageDraw.Draw(img)


def finish(img, w, h, path):
    img = img.resize((w, h), Image.LANCZOS)
    raw = bytearray()
    for y in range(h):
        for x in range(w):
            r, g, b = quantize(img.getpixel((x, y)))
            v = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
            raw += struct.pack("<H", v)
    header = bytes([0x04, (w & 0x3F) << 2, ((w >> 6) & 0x1F) | ((h & 0x07) << 5), (h >> 3) & 0xFF])
    with open(path, "wb") as f:
        f.write(header)
        f.write(raw)
    print(f"{os.path.basename(path)}: {w}x{h}, {os.path.getsize(path)} bytes")


def hub_icon(name, draw_fn):
    w, h = 117, 126
    img, d = canvas(w, h)
    d.rounded_rectangle(
        [6 * SS, 6 * SS, (w - 6) * SS, 90 * SS],
        radius=12 * SS,
        fill=SURFACE,
        outline=SURFACE_ALT,
        width=2 * SS,
    )
    draw_fn(img, d)
    finish(img, w, h, os.path.join(OUT, name))


def line(d, p1, p2, color, width):
    d.line([p1[0] * SS, p1[1] * SS, p2[0] * SS, p2[1] * SS], fill=color, width=width * SS)


def poly(d, pts, color):
    d.polygon([(x * SS, y * SS) for x, y in pts], fill=color)


def tri(d, pts, color):
    d.polygon([(x * SS, y * SS) for x, y in pts], fill=color)


def circle(d, c, r, color, width=0):
    if width:
        d.ellipse(
            [(c[0] - r) * SS, (c[1] - r) * SS, (c[0] + r) * SS, (c[1] + r) * SS],
            outline=color,
            width=width * SS,
        )
    else:
        d.ellipse(
            [(c[0] - r) * SS, (c[1] - r) * SS, (c[0] + r) * SS, (c[1] + r) * SS],
            fill=color,
        )


def rect(d, box, color, radius=0, outline=None, width=0):
    if radius:
        d.rounded_rectangle(
            [box[0] * SS, box[1] * SS, box[2] * SS, box[3] * SS],
            radius=radius * SS,
            fill=color,
            outline=outline,
            width=width * SS,
        )
    else:
        d.rectangle([box[0] * SS, box[1] * SS, box[2] * SS, box[3] * SS], fill=color, outline=outline, width=width * SS)


# ---- Words: red "A" being written by a pen nib at bottom right ----
def draw_words(img, d):
    line(d, (58, 18), (30, 86), RED, 11)
    line(d, (58, 18), (86, 86), RED, 11)
    line(d, (42, 56), (74, 56), RED, 10)
    # pen nib writing the right leg
    poly(d, [(80, 58), (96, 82), (88, 88), (72, 68)], RED_LIGHT)
    poly(d, [(80, 58), (96, 82), (92, 82)], RED_DARK)
    # ink dot
    circle(d, (86, 86), 4, RED)


# ---- Art: diagonal paintbrush with red bristles ----
def draw_art(img, d):
    line(d, (24, 12), (66, 58), MUTED, 12)          # handle
    line(d, (66, 58), (74, 68), TEXT, 11)           # ferrule
    tri(d, [(70, 66), (92, 82), (74, 86)], RED)      # bristles
    line(d, (76, 72), (88, 82), RED_LIGHT, 3)
    line(d, (82, 66), (92, 76), RED_DARK, 3)


# ---- Demos: play button ----
def draw_demos(img, d):
    rect(d, (34, 16, 82, 64), SURFACE_ALT, radius=12, outline=MUTED, width=3)
    tri(d, [(45, 27), (72, 40), (45, 53)], RED)


# ---- Calibrate: crosshair with center dot ----
def draw_calibrate(img, d):
    circle(d, (58, 50), 24, RED, 6)
    line(d, (58, 6), (58, 30), MUTED, 6)
    line(d, (58, 70), (58, 90), MUTED, 6)
    line(d, (6, 50), (30, 50), MUTED, 6)
    line(d, (86, 50), (111, 50), MUTED, 6)
    circle(d, (58, 50), 6, RED)
    circle(d, (58, 50), 2, TEXT)


# ---- Controls: three sliders ----
def draw_controls(img, d):
    for x in (24, 58, 92):
        rect(d, (x - 4, 14, x + 4, 84), SURFACE_ALT, radius=4, outline=MUTED, width=2)
    rect(d, (15, 30, 33, 42), RED, radius=5)
    rect(d, (49, 52, 67, 64), RED, radius=5)
    rect(d, (83, 22, 101, 34), RED, radius=5)
    line(d, (15, 36), (33, 36), RED_LIGHT, 2)
    line(d, (49, 58), (67, 58), RED_LIGHT, 2)
    line(d, (83, 28), (101, 28), RED_LIGHT, 2)


# ---- Jog: four-way arrows ----
def draw_jog(img, d):
    line(d, (58, 24), (58, 52), MUTED, 8)
    tri(d, [(46, 28), (70, 28), (58, 12)], RED)
    line(d, (58, 54), (58, 82), MUTED, 8)
    tri(d, [(46, 78), (70, 78), (58, 92)], RED)
    line(d, (22, 50), (50, 50), MUTED, 8)
    tri(d, [(26, 40), (26, 60), (10, 50)], RED)
    line(d, (66, 50), (94, 50), MUTED, 8)
    tri(d, [(90, 40), (90, 60), (106, 50)], RED)
    circle(d, (58, 50), 4, TEXT)


# ---- Home: house ----
def draw_home(img, d):
    poly(d, [(30, 48), (58, 16), (86, 48)], RED)
    rect(d, (34, 46, 82, 84), RED)
    rect(d, (50, 60, 66, 84), BG)
    rect(d, (52, 62, 64, 82), RED_DARK)
    line(d, (6, 88), (111, 88), MUTED, 4)


# ---- Settings: gear ----
def draw_settings(img, d):
    cx, cy, teeth_r = 58, 52, 34
    for i in range(8):
        a = math.radians(i * 45)
        tx = cx + math.cos(a) * teeth_r
        ty = cy + math.sin(a) * teeth_r
        rect(d, (tx - 6, ty - 7, tx + 6, ty + 7), RED, radius=3)
    circle(d, (cx, cy), 26, RED)
    circle(d, (cx, cy), 9, BG)
    circle(d, (cx, cy), 9, RED_DARK, width=3)


# ---- Small pen nib (36x36) for sub-screen panels ----
def pen_small():
    w, h = 36, 36
    img, d = canvas(w, h)
    poly(d, [(8, 10), (20, 5), (33, 29), (22, 34)], RED)
    line(d, (13, 13), (27, 27), RED_LIGHT, 2)
    finish(img, w, h, os.path.join(OUT, "bmp_plot_pen.bin"))


# ---- Small paintbrush (36x36) for the Art screen panel ----
def brush_small():
    w, h = 36, 36
    img, d = canvas(w, h)
    line(d, (4, 4), (20, 20), MUTED, 5)
    line(d, (20, 20), (24, 24), TEXT, 4)
    tri(d, [(22, 22), (32, 32), (22, 34)], RED)
    finish(img, w, h, os.path.join(OUT, "bmp_plot_brush.bin"))


def main():
    os.makedirs(OUT, exist_ok=True)
    for name, fn in [
        ("bmp_plot_words.bin", draw_words),
        ("bmp_plot_art.bin", draw_art),
        ("bmp_plot_demos.bin", draw_demos),
        ("bmp_plot_calibrate.bin", draw_calibrate),
        ("bmp_plot_controls.bin", draw_controls),
        ("bmp_plot_jog.bin", draw_jog),
        ("bmp_plot_home.bin", draw_home),
        ("bmp_plot_settings.bin", draw_settings),
    ]:
        hub_icon(name, fn)
    pen_small()
    brush_small()


if __name__ == "__main__":
    main()
