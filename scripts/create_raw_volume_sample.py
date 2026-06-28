#!/usr/bin/env python3

import json
import math
import struct
from pathlib import Path


WIDTH = 64
HEIGHT = 64
DEPTH = 32
SPACING = 1.0
BACKGROUND_VALUE = 18.0


def clamp(value, lower, upper):
    return max(lower, min(upper, value))


def voxel_value(x, y, z):
    x_norm = x / (WIDTH - 1)
    y_norm = y / (HEIGHT - 1)
    z_norm = z / (DEPTH - 1)

    value = BACKGROUND_VALUE
    value += 150.0 * x_norm
    value += 65.0 * (1.0 - y_norm)
    value += 40.0 * z_norm

    if 9 <= x <= 24 and 36 <= y <= 54 and 4 <= z <= 14:
        value += 70.0

    if 42 <= x <= 58 and 10 <= y <= 24 and 16 <= z <= 28:
        value += 115.0

    diagonal_band = 40.0 if x >= int(0.65 * y) + 14 and z <= 18 else 0.0
    value += diagonal_band

    if (x + (2 * y) + (3 * z)) % 17 == 0:
        value += 25.0

    return clamp(value, 0.0, 255.0)


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
