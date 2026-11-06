"""Knock near-black backgrounds to alpha for UI PNGs under Content/UI/Art."""
from pathlib import Path

from PIL import Image

ART = Path(r"c:\Users\jamdl\OneDrive\Documents\Unreal Projects\GADE7322POE\Content\UI\Art")
THRESHOLD = 18  # RGB below this becomes transparent


def punch_black(path: Path) -> None:
    img = Image.open(path).convert("RGBA")
    pixels = img.load()
    w, h = img.size
    changed = 0
    for y in range(h):
        for x in range(w):
            r, g, b, a = pixels[x, y]
            if r <= THRESHOLD and g <= THRESHOLD and b <= THRESHOLD:
                if a != 0:
                    pixels[x, y] = (0, 0, 0, 0)
                    changed += 1
    img.save(path)
    print(f"{path.name}: punched {changed} px")


def main():
    for png in sorted(ART.glob("*.png")):
        punch_black(png)
    print("DONE transparency")


if __name__ == "__main__":
    main()
