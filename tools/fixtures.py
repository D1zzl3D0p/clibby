#!/usr/bin/env python3
"""Generate and evaluate the portable desktop-pet fixture (Pillow only for art)."""
from __future__ import annotations

import argparse
import json
import math
from pathlib import Path

SHARED = Path(__file__).resolve().parents[1] / "shared"
SCENARIOS = ("static", "idle-motion", "walk-fixed", "walk-desktop", "hidden",
             "mixed-active", "mixed-rest")


def _finite_vector(value, count):
    return (isinstance(value, list) and len(value) == count
            and all(isinstance(x, (int, float)) and not isinstance(x, bool)
                    and math.isfinite(x) for x in value))


def validate_rig(rig: dict) -> None:
    """Reject malformed assets before either engine starts a benchmark."""
    if rig.get("version") != 1 or len(rig.get("parts", [])) != 12:
        raise ValueError("rig must have version 1 and exactly 12 parts")
    if not _finite_vector(rig.get("atlas_size"), 2) or min(rig["atlas_size"]) <= 0:
        raise ValueError("invalid atlas dimensions")
    if rig.get("surface_size") != [256, 256] or not _finite_vector(rig.get("origin"), 2):
        raise ValueError("invalid surface size or origin")
    polygon = rig.get("hit_polygon")
    if (not isinstance(polygon, list) or len(polygon) < 3
            or any(not _finite_vector(point, 2) or min(point) < 0 or max(point) > 256
                   for point in polygon)):
        raise ValueError("hit polygon must contain at least three finite surface-coordinate points")
    for index, point in enumerate(polygon):
        following = polygon[(index + 1) % len(polygon)]
        after = polygon[(index + 2) % len(polygon)]
        cross = ((following[0] - point[0]) * (after[1] - following[1])
                 - (following[1] - point[1]) * (after[0] - following[0]))
        if cross <= 0:
            raise ValueError("hit polygon must be strictly convex in clockwise surface order")
    atlas = rig.get("atlas")
    if not isinstance(atlas, str) or Path(atlas).is_absolute() or ".." in Path(atlas).parts:
        raise ValueError("atlas must be a safe relative path")
    known = set()
    for part in rig["parts"]:
        name = part.get("id")
        if not isinstance(name, str) or not name or name in known:
            raise ValueError("part names must be nonempty and unique")
        if part.get("parent") is not None and part["parent"] not in known:
            raise ValueError("parents must precede their children")
        for key, count in (("rect", 4), ("position", 2), ("pivot", 2), ("size", 2)):
            if not _finite_vector(part.get(key), count):
                raise ValueError(f"invalid {key} in {name}")
        x, y, width, height = part["rect"]
        if (any(int(v) != v for v in part["rect"]) or min(x, y) < 0
                or min(width, height) <= 0 or x + width > rig["atlas_size"][0]
                or y + height > rig["atlas_size"][1]):
            raise ValueError(f"atlas rectangle out of bounds in {name}")
        if part["size"] != [width, height]:
            raise ValueError("version 1 parts use atlas-native dimensions")
        if not _finite_vector([part.get("rotation")], 1):
            raise ValueError(f"invalid base rotation in {name}")
        known.add(name)
    if known != {"tail", "rear_left", "rear_right", "body", "front_left", "front_right",
                 "head", "ear_left", "ear_right", "eye_left", "eye_right", "muzzle"}:
        raise ValueError("part names differ from the frozen version 1 contract")
    motion = rig.get("motion", {})
    expected = {"hz": 60, "max_catchup_steps": 4, "root_amplitude": 32,
                "root_period_ticks": 240, "stride_distance": 32,
                "tail_stiffness": 36, "tail_damping": 12,
                "tail_angle_limit": 0.6, "tail_velocity_limit": 2}
    if motion != expected:
        raise ValueError("motion constants differ from the frozen version 1 contract")


def load_rig(directory: Path = SHARED) -> dict:
    rig = json.loads((Path(directory) / "rig.json").read_text())
    validate_rig(rig)
    return rig


def phase_at(seconds: float, scenario: str) -> str:
    if scenario not in SCENARIOS:
        raise ValueError(f"unknown scenario: {scenario}")
    if (not isinstance(seconds, (int, float)) or isinstance(seconds, bool)
            or not math.isfinite(seconds) or seconds < 0):
        raise ValueError("seconds must be finite and nonnegative")
    remainder = seconds % 60
    if scenario == "mixed-active":
        return "idle-motion" if remainder < 45 else "walk-desktop"
    if scenario == "mixed-rest":
        return "static" if remainder < 40 else "walk-desktop" if remainder < 55 else "idle-motion"
    return scenario


class MotionState:
    """Reference state with one fixed-duration interval per advance call."""
    def __init__(self, rig=None):
        self.rig = rig if rig is not None else load_rig()
        self.active = 0
        self.walking = 0
        self.distance = 0.0
        self.root_x = 0.0
        self.tail_angle = 0.0
        self.tail_velocity = 0.0
        self.gait = 0.0

    def advance(self, phase: str) -> None:
        if phase not in SCENARIOS[:5]:
            raise ValueError("advance requires a resolved phase")
        if phase in ("static", "hidden"):
            return
        self.active += 1
        if phase in ("walk-fixed", "walk-desktop"):
            self.walking += 1
            old_x = self.root_x
            self.root_x = 32 * math.sin(math.tau * self.walking / 240)
            self.distance += abs(self.root_x - old_x)
            self.gait = math.sin(math.tau * self.distance / 32)
        else:
            self.gait = 0.0
        target = 0.25 * math.sin(math.tau * self.active / 120) + 0.1 * self.gait
        self.tail_velocity = max(-2, min(2, self.tail_velocity + (
            36 * (target - self.tail_angle) - 12 * self.tail_velocity) / 60))
        self.tail_angle = max(-0.6, min(0.6, self.tail_angle + self.tail_velocity / 60))
        if abs(self.tail_angle) == 0.6:
            self.tail_velocity = 0.0

    def pose(self) -> dict:
        transforms = [{"id": p["id"], "x": p["position"][0], "y": p["position"][1],
                       "rotation": p["rotation"], "scale_x": 1.0, "scale_y": 1.0}
                      for p in self.rig["parts"]]
        if self.active:
            by_id = {p["id"]: p for p in transforms}
            breath = math.sin(math.tau * self.active / 180)
            tx = 24 * breath
            ty = 8 * math.sin(math.tau * self.active / 240)
            by_id["body"]["y"] -= 1.5 * breath
            by_id["body"]["scale_y"] += 0.015 * breath
            by_id["head"]["rotation"] += 0.5 * math.atan2(ty, 48 + tx)
            by_id["tail"]["rotation"] += self.tail_angle
            for name, sign in (("rear_left", 1), ("front_right", 1),
                               ("rear_right", -1), ("front_left", -1)):
                by_id[name]["rotation"] += sign * 0.35 * self.gait
            for name in ("eye_left", "eye_right"):
                by_id[name]["x"] += tx / 24 * 1.5
                by_id[name]["y"] += ty / 8 * 0.75
        return {"root_offset": [self.root_x, 0.0], "parts": transforms}


def evaluate_pose(tick: int, scenario: str) -> dict:
    if not isinstance(tick, int) or isinstance(tick, bool) or tick < 0:
        raise ValueError("tick must be a nonnegative integer")
    phase_at(0, scenario)
    state = MotionState()
    for interval in range(tick):
        state.advance(phase_at(interval / 60, scenario))
    return state.pose()


def generate_golden(directory: Path = SHARED) -> None:
    ticks = {
        "static": [0, 600], "hidden": [0, 600],
        "idle-motion": [0, 1, 30, 60, 120, 2700],
        "walk-fixed": [0, 1, 30, 60, 120, 180, 240, 301],
        "walk-desktop": [0, 1, 60, 180, 301],
        "mixed-active": [0, 2699, 2700, 2701, 3599, 3600, 3601, 6300, 6301],
        "mixed-rest": [0, 2399, 2400, 2401, 3299, 3300, 3301, 3599, 3600, 3601, 6000, 6001],
    }
    poses = [{"scenario": scenario, "tick": tick, **evaluate_pose(tick, scenario)}
             for scenario, values in ticks.items() for tick in values]
    (directory / "golden.json").write_text(json.dumps({"version": 1, "poses": poses}, indent=2) + "\n")


def generate_atlas(directory: Path = SHARED) -> None:
    from PIL import Image, ImageDraw

    rig = load_rig(directory)
    factor = 4
    atlas = Image.new("RGBA", tuple(rig["atlas_size"]), (0, 0, 0, 0))
    outline, purple, light, dark = "#34304d", "#ad8ddd", "#e4cff6", "#786399"
    for part in rig["parts"]:
        name = part["id"]
        x, y, w, h = part["rect"]
        tile = Image.new("RGBA", (w * factor, h * factor), (0, 0, 0, 0))
        draw = ImageDraw.Draw(tile)

        def box(coords):
            return tuple(round(v * factor) for v in coords)

        def ellipse(coords, fill, edge=None, width=1):
            draw.ellipse(box(coords), fill=fill, outline=edge, width=width * factor)

        if name.startswith("ear"):
            draw.polygon([tuple(round(v * factor) for v in pt)
                          for pt in [(2, h - 3), (w / 2, 2), (w - 2, h - 3)]], fill=outline)
            draw.polygon([tuple(round(v * factor) for v in pt)
                          for pt in [(5, h - 5), (w / 2, 8), (w - 5, h - 5)]], fill=light)
        elif name.startswith("eye"):
            ellipse((1, 1, w - 1, h - 1), outline)
            ellipse((3, 2, 5, 5), "#ffffff")
        elif name == "muzzle":
            ellipse((1, 1, w - 1, h - 1), light)
            ellipse((11, 3, 17, 7), outline)
            draw.line(box((14, 7, 14, 12)), fill=outline, width=factor)
        else:
            fill = dark if name.startswith("rear") or name == "tail" else purple
            ellipse((1.5, 1.5, w - 1.5, h - 1.5), fill, outline, 2)
            if name == "body":
                ellipse((15, 17, w - 15, h - 6), light)
            elif name == "head":
                ellipse((7, 28, 17, 33), "#d9a4cc")
                ellipse((w - 17, 28, w - 7, 33), "#d9a4cc")
        tile = tile.resize((w, h), Image.Resampling.LANCZOS)
        atlas.alpha_composite(tile, (x, y))
    atlas.save(directory / "atlas.png", optimize=False)


def render_preview(directory: Path = SHARED) -> None:
    """Render the hierarchy once for untimed human inspection."""
    from PIL import Image, ImageDraw

    rig = load_rig(directory)
    atlas = Image.open(directory / "atlas.png").convert("RGBA")
    preview = Image.new("RGBA", (768, 256), "#f2edf7")
    draw = ImageDraw.Draw(preview)
    for x in range(0, 768, 16):
        for y in range(0, 256, 16):
            if (x // 16 + y // 16) % 2:
                draw.rectangle((x, y, x + 15, y + 15), fill="#e1dce8")
    for column, (tick, scenario) in enumerate(((0, "static"), (30, "idle-motion"), (30, "walk-fixed"))):
        pose = evaluate_pose(tick, scenario)
        matrices = {}
        root = (1, 0, 0, 1, rig["origin"][0] + pose["root_offset"][0], rig["origin"][1])
        for part, transform in zip(rig["parts"], pose["parts"]):
            c, s = math.cos(transform["rotation"]), math.sin(transform["rotation"])
            a, b, c0, d, e, f = matrices.get(part["parent"], root)
            sx, sy = transform["scale_x"], transform["scale_y"]
            aa, bb = (a * c + c0 * s) * sx, (b * c + d * s) * sx
            cc, dd = (-a * s + c0 * c) * sy, (-b * s + d * c) * sy
            ee = a * transform["x"] + c0 * transform["y"] + e
            ff = b * transform["x"] + d * transform["y"] + f
            matrices[part["id"]] = (aa, bb, cc, dd, ee, ff)
            px, py = part["pivot"]
            ee -= aa * px + cc * py
            ff -= bb * px + dd * py
            determinant = aa * dd - bb * cc
            inverse = (dd / determinant, -cc / determinant, (cc * ff - dd * ee) / determinant,
                       -bb / determinant, aa / determinant, (bb * ee - aa * ff) / determinant)
            x, y, w, h = part["rect"]
            tile = atlas.crop((x, y, x + w, y + h))
            layer = tile.transform((256, 256), Image.Transform.AFFINE, inverse,
                                   resample=Image.Resampling.BICUBIC)
            preview.alpha_composite(layer, (column * 256, 0))
        draw.text((column * 256 + 12, 230), f"{scenario} / tick {tick}", fill="#34304d")
    preview.convert("RGB").save(directory / "preview.png")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--golden-only", action="store_true", help="requires only Python standard library")
    args = parser.parse_args()
    generate_golden()
    if not args.golden_only:
        generate_atlas()
        render_preview()
    print(f"Generated fixtures in {SHARED}")


if __name__ == "__main__":
    main()
