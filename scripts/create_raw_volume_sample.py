#!/usr/bin/env python3

import json
import math
import struct
from pathlib import Path


WIDTH = 64
HEIGHT = 64
DEPTH = 32
SPACING = 1.0
INSIDE_VALUE = 240.0
OUTSIDE_VALUE = 35.0
SPHERE_RADIUS = 1.02


def voxel_value(x, y, z):
    center_x = (WIDTH - 1) / 2.0
    center_y = (HEIGHT - 1) / 2.0
    center_z = (DEPTH - 1) / 2.0

    dx = (x - center_x) / center_x
    dy = (y - center_y) / center_y
    dz = (z - center_z) / center_z
    distance = math.sqrt((dx * dx) + (dy * dy) + (dz * dz))

    return INSIDE_VALUE if distance <= SPHERE_RADIUS else OUTSIDE_VALUE


def main():
    output_dir = Path("data") / "samples" / "raw_volume"
    output_dir.mkdir(parents=True, exist_ok=True)

    metadata_path = output_dir / "volume.json"
    raw_path = output_dir / "volume.raw"

    metadata = {
        "width": WIDTH,
        "height": HEIGHT,
        "depth": DEPTH,
        "spacingX": SPACING,
        "spacingY": SPACING,
        "spacingZ": SPACING,
    }

    metadata_path.write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")

    with raw_path.open("wb") as raw_file:
        for z in range(DEPTH):
            for y in range(HEIGHT):
                for x in range(WIDTH):
                    raw_file.write(struct.pack("<f", voxel_value(x, y, z)))

    print(metadata_path)
    print(raw_path)


if __name__ == "__main__":
    main()
