#!/usr/bin/env python3
"""Rasterise the .ouas session-file icon into the formats the installers need.

The SVG sources in src/app/res/icons/mime are the master artwork: file-ouas.svg is
the detailed drawing and file-ouas-16.svg the simplified variant used at 16 and 24
pixels. This script renders them into

  * file-ouas.ico  - embedded in the Windows executable and pointed at by the
                     DefaultIcon of the registered ProgID, and
  * file-ouas.icns - copied into the macOS bundle for CFBundleTypeIconFile, and
  * PNG files      - installed into the Linux hicolor mimetype directories.

Usage: python tools/gen_file_icons.py
"""

from __future__ import annotations

import io
import re
import sys
from pathlib import Path

import resvg_python
from PIL import Image

ICON_DIR = Path(__file__).resolve().parent.parent / "src" / "app" / "res" / "icons" / "mime"
LINUX_ICON_DIR = (
    Path(__file__).resolve().parent.parent
    / ".github"
    / "linux"
    / "usr"
    / "share"
    / "icons"
    / "hicolor"
)
LINUX_ICON_NAME = "application-x-ouaexp-session.png"

# The detailed drawing loses clarity below 32 pixels.
SMALL_VARIANT_MAX = 24

ICO_SIZES = (16, 24, 32, 48, 64, 128, 256)
ICNS_SIZES = (16, 32, 64, 128, 256, 512, 1024)
LINUX_SIZES = (16, 24, 32, 48, 64, 128, 256)


def render(svg: str, size: int) -> Image.Image:
    svg = re.sub(r'width="\d+" height="\d+"', f'width="{size}" height="{size}"', svg, count=1)
    png = resvg_python.svg_to_png(svg)
    return Image.open(io.BytesIO(bytes(png))).convert("RGBA")


def main() -> int:
    detailed = (ICON_DIR / "file-ouas.svg").read_text(encoding="utf-8")
    simplified = (ICON_DIR / "file-ouas-16.svg").read_text(encoding="utf-8")

    def frame(size: int) -> Image.Image:
        return render(simplified if size <= SMALL_VARIANT_MAX else detailed, size)

    # Pillow scales the base image down for every requested size unless append_images
    # supplies that size itself, so the base is the largest frame and each rendered
    # frame is appended to override the scaling with a fresh render.
    frames = [frame(size) for size in ICO_SIZES]
    frames[-1].save(
        ICON_DIR / "file-ouas.ico",
        format="ICO",
        sizes=[(size, size) for size in ICO_SIZES],
        append_images=frames,
    )
    print(f"wrote {ICON_DIR / 'file-ouas.ico'} {ICO_SIZES}")

    # Pillow writes the modern ICNS slots only; the smallest is ic11, the 16-point
    # @2x entry stored as a 32-pixel bitmap. Finder shows that slot at small sizes,
    # so it carries the simplified drawing like the small ICO and Linux sizes, while
    # every larger slot gets a fresh render of the detailed artwork instead of a
    # downscale of the base image.
    icns_frames = [
        render(simplified, size) if size <= 2 * SMALL_VARIANT_MAX else render(detailed, size)
        for size in ICNS_SIZES
        if size >= 32
    ]
    icns_frames[-1].save(
        ICON_DIR / "file-ouas.icns",
        format="ICNS",
        append_images=icns_frames[:-1],
    )
    print(f"wrote {ICON_DIR / 'file-ouas.icns'}")

    for size in LINUX_SIZES:
        destination = LINUX_ICON_DIR / f"{size}x{size}" / "mimetypes" / LINUX_ICON_NAME
        destination.parent.mkdir(parents=True, exist_ok=True)
        frame(size).save(destination, format="PNG")
        print(f"wrote {destination} ({size}x{size})")

    return 0


if __name__ == "__main__":
    sys.exit(main())
