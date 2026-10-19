"""Export the approved Clibby vector geometry into checked-in runtime assets."""
from pathlib import Path
import json
import runpy
import shutil
import subprocess

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
DESTINATION = ROOT / "assets" / "brand"


def main():
    source = runpy.run_path(str(ROOT / "art-studies/clibby/build.py"))
    source["write_assets"]()
    DESTINATION.mkdir(parents=True, exist_ok=True)
    for filename in ("mark.svg", "mark-reversed.svg", "menu-template.svg", "app-icon.svg"):
        shutil.copyfile(ROOT / "art-studies/clibby" / filename, DESTINATION / filename)
    for filename, sizes in {
        "mark": (32, 64, 256), "mark-reversed": (32, 64, 256),
        "menu-template": (32, 64), "app-icon": (16, 32, 64, 128, 256, 512, 1024),
    }.items():
        for size in sizes:
            subprocess.run(["rtk", "proxy", "rsvg-convert", "-w", str(size), "-h", str(size),
                            "-o", str(DESTINATION / f"{filename}-{size}.png"),
                            str(DESTINATION / f"{filename}.svg")], check=True, capture_output=True)
    shutil.copyfile(DESTINATION / "mark-reversed-256.png", DESTINATION / "mark-reversed.png")
    shutil.copyfile(DESTINATION / "app-icon-1024.png", DESTINATION / "app-icon.png")
    Image.open(DESTINATION / "app-icon.png").save(DESTINATION / "clibby.icns", format="ICNS")
    tokens = {
        "name": "Clibby", "slug": "clibby",
        "colours": {"ink": source["INK"], "lilac": source["LILAC"], "cream": source["CREAM"]},
        "geometry": {"viewBox": [0, 0, 256, 256], "outline": source["OUTLINE"],
                     "eyes": [{"cx": x, "cy": 145, "rx": 10, "ry": 18} for x in (116, 156)]},
        "spacing": [4, 8, 12, 16, 24, 32],
        "radii": {"control": 8, "panel": 12, "tile": 54},
    }
    (DESTINATION / "tokens.json").write_text(json.dumps(tokens, indent=2) + "\n")
    print(f"Exported Clibby vector, PNG, macOS icon and design tokens to {DESTINATION}")


if __name__ == "__main__":
    main()
