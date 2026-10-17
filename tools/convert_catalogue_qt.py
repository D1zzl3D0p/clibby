#!/usr/bin/env python3
"""Convert a trusted original catalogue GLB to addressable nearest Qt artwork."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
from png_metadata import strip_texture_exif


def convert(source, output, balsam, type_name):
    if not re.fullmatch(r"[A-Z][A-Za-z0-9]{0,63}", type_name):
        raise ValueError("invalid trusted QML type")
    source, output = Path(source).resolve(), Path(output).resolve()
    output.mkdir(parents=True, exist_ok=True)
    rig = json.loads((source.parent / "rig.json").read_text())
    if rig.get("skin") != type_name + "Skin":
        raise ValueError("asset skin/type mismatch")
    expected = {joint["name"] for joint in rig["joints"]}
    subprocess.run([str(balsam), str(source), "-o", str(output)], check=True,
                   env=dict(os.environ, QT_QPA_PLATFORM="offscreen"))
    for texture in (output / "maps").rglob("*.png"):
        strip_texture_exif(texture)
    # Balsam normalizes hyphenated basenames differently from PascalCase types.
    # Keep only the one generated root; the target type is supplied by CMake.
    qml = output / (type_name + ".qml")
    generated = [p for p in output.glob("*.qml") if p != qml]
    if generated:
        if len(generated) != 1:
            raise RuntimeError("ambiguous converted root")
        generated[0].replace(qml)
    if not qml.exists():
        raise RuntimeError("Balsam did not produce a root QML file")
    text = qml.read_text()
    found = set(re.findall(r'objectName: "([^"\n]+)"', text))
    if not expected <= found or rig["skin"] not in found:
        raise RuntimeError(f"Unaddressable asset: {expected - found}")
    text = re.sub(r"^\s*(?:magFilter|minFilter|mipFilter|generateMipmaps):.*$", "", text, flags=re.MULTILINE)
    text = re.sub(r"(Texture\s*\{)", r"\1\n        magFilter: Texture.Nearest\n        minFilter: Texture.Nearest\n        mipFilter: Texture.None", text)
    qml.write_text(text)
    (output / "qmldir").write_text(f"module {type_name}Asset\n{type_name} 1.0 {type_name}.qml\n")
    (output / "conversion.json").write_text(json.dumps({"version": 1, "source": str(source), "type": type_name,
        "joints": sorted(expected), "joint_count": len(expected), "nearest_atlas": True}, indent=2) + "\n")
    return qml


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--balsam", type=Path, required=True)
    parser.add_argument("--type", required=True)
    args = parser.parse_args()
    print(convert(args.source, args.output, args.balsam, args.type))
