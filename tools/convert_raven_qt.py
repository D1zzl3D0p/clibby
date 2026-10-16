#!/usr/bin/env python3
"""Convert our trusted, original GLB to editable Qt joints at build time."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess

if __package__:
    from .png_metadata import strip_texture_exif
else:
    from png_metadata import strip_texture_exif


def convert(source, output, balsam):
    source, output = Path(source).resolve(), Path(output).resolve()
    output.mkdir(parents=True, exist_ok=True)
    rig = json.loads((source.parent / "rig.json").read_text())
    expected = {j["name"] for j in rig["joints"]}
    subprocess.run([str(balsam), str(source), "-o", str(output)], check=True,
                   env=dict(os.environ, QT_QPA_PLATFORM="offscreen"))
    for texture in (output / "maps").rglob("*.png"):
        strip_texture_exif(texture)
    qml = output / "Raven.qml"
    if not qml.exists():
        raise RuntimeError("Balsam did not produce Raven.qml")
    text = qml.read_text()
    found = set(re.findall(r'objectName: "([^"\n]+)"', text))
    if not expected <= found:
        raise RuntimeError(f"Missing addressable deform joints: {expected - found}")
    # The original atlas is deliberately nearest sampled in both directions.
    text = re.sub(r"^\s*(?:magFilter|minFilter|mipFilter|generateMipmaps):.*$", "", text, flags=re.MULTILINE)
    text = re.sub(r'(Texture\s*\{)', r'\1\n        magFilter: Texture.Nearest\n        minFilter: Texture.Nearest\n        mipFilter: Texture.None', text)
    qml.write_text(text)
    (output / "qmldir").write_text("module RavenAsset\nRaven 1.0 Raven.qml\n")
    (output / "conversion.json").write_text(json.dumps({"version":1,"joints":sorted(expected),
        "source":str(source),"joint_count":len(expected),"nearest_atlas":True}, indent=2)+"\n")
    return qml


if __name__ == "__main__":
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--source", required=True, type=Path)
    p.add_argument("--output", required=True, type=Path)
    p.add_argument("--balsam", required=True, type=Path)
    a=p.parse_args()
    print(convert(a.source,a.output,a.balsam))
