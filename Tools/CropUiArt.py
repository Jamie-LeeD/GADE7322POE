"""Crop Content/UI/Art PNGs to opaque content bounds (removes square transparent padding)."""
from pathlib import Path

import numpy as np
from PIL import Image

ART = Path(r"c:\Users\jamdl\OneDrive\Documents\Unreal Projects\GADE7322POE\Content\UI\Art")
PAD = 8
ALPHA_MIN = 10


def crop_one(path: Path) -> None:
    im = Image.open(path).convert("RGBA")
    alpha = np.array(im)[:, :, 3]
    ys, xs = np.where(alpha > ALPHA_MIN)
    if len(xs) == 0:
        print(f"skip empty {path.name}")
        return
    x0 = max(0, int(xs.min()) - PAD)
    x1 = min(im.width, int(xs.max()) + 1 + PAD)
    y0 = max(0, int(ys.min()) - PAD)
    y1 = min(im.height, int(ys.max()) + 1 + PAD)
    cropped = im.crop((x0, y0, x1, y1))
    if cropped.size == im.size:
        print(f"keep {path.name} {im.size}")
        return
    cropped.save(path)
    print(f"{path.name}: {im.size} -> {cropped.size}")


def main():
    for png in sorted(ART.glob("*.png")):
        crop_one(png)
    print("DONE crop")


if __name__ == "__main__":
    main()
