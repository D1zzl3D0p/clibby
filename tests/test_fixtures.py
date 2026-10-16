"""Contract regressions: suspension, interval boundaries, spring and asset validation."""
import copy
import json
import math
from pathlib import Path
import unittest

from tools.fixtures import SCENARIOS, evaluate_pose, phase_at, validate_rig, load_rig


SHARED = Path(__file__).resolve().parents[1] / "shared"


class FixtureTests(unittest.TestCase):
    def test_hit_polygon_contains_transformed_alpha_across_motion_extrema(self):
        from PIL import Image

        rig = load_rig()
        with Image.open(SHARED / rig["atlas"]) as image:
            alpha = image.convert("RGBA").getchannel("A")
            pixels = {
                part["id"]: [(x + 0.5 - part["pivot"][0], y + 0.5 - part["pivot"][1])
                             for y in range(part["rect"][3])
                             for x in range(part["rect"][2])
                             if alpha.getpixel((part["rect"][0] + x, part["rect"][1] + y)) > 0]
                for part in rig["parts"]
            }
        fixtures = json.loads((SHARED / "golden.json").read_text())["poses"]
        fixtures += [{"scenario": scenario, "tick": tick, **evaluate_pose(tick, scenario)}
                     for scenario in ("idle-motion", "walk-fixed", "walk-desktop")
                     for tick in range(0, 721, 30)]
        for pose in fixtures:
            # Absolute surface coordinates: origin is already included in polygon.
            offset = pose["root_offset"][0] if pose["scenario"] == "walk-fixed" else 0
            polygon = [(x + offset, y) for x, y in rig["hit_polygon"]]
            matrices = {None: (1, 0, 0, 1, rig["origin"][0] + offset, rig["origin"][1])}
            for part, local in zip(rig["parts"], pose["parts"]):
                a, b, c, d, e, f = matrices[part["parent"]]
                co, si = math.cos(local["rotation"]), math.sin(local["rotation"])
                aa, bb = (a * co + c * si) * local["scale_x"], (b * co + d * si) * local["scale_x"]
                cc, dd = (-a * si + c * co) * local["scale_y"], (-b * si + d * co) * local["scale_y"]
                ee, ff = e + a * local["x"] + c * local["y"], f + b * local["x"] + d * local["y"]
                matrices[part["id"]] = (aa, bb, cc, dd, ee, ff)
                for px, py in pixels[part["id"]]:
                    x, y = aa * px + cc * py + ee, bb * px + dd * py + ff
                    self.assertTrue(all((qx - ox) * (y - oy) - (qy - oy) * (x - ox) >= -1e-8
                                        for (ox, oy), (qx, qy) in zip(polygon, polygon[1:] + polygon[:1])),
                                    f"{pose['scenario']} tick {pose['tick']} {part['id']} pixel {(x, y)} outside polygon")

    def test_rig_rejects_missing_motion_parts_and_invalid_hit_polygon(self):
        rig = load_rig()
        for polygon in ([], [[0, 0], [1, 1]], [[0, 0], [1, float("nan")], [2, 0]],
                        [[-1, 0], [1, 1], [2, 0]]):
            broken = copy.deepcopy(rig)
            broken["hit_polygon"] = polygon
            with self.subTest(polygon=polygon), self.assertRaises(ValueError):
                validate_rig(broken)
        broken = copy.deepcopy(rig)
        broken["parts"][-1]["id"] = "renamed_muzzle"
        with self.assertRaises(ValueError):
            validate_rig(broken)

    def test_static_and_hidden_are_frozen(self):
        for scenario in ("static", "hidden"):
            self.assertEqual(evaluate_pose(0, scenario), evaluate_pose(600, scenario))

    def test_mixed_boundaries(self):
        for seconds, expected in ((44.999, "idle-motion"), (45, "walk-desktop"),
                                  (59.999, "walk-desktop"), (60, "idle-motion")):
            self.assertEqual(phase_at(seconds, "mixed-active"), expected)
        for seconds, expected in ((39.999, "static"), (40, "walk-desktop"),
                                  (55, "idle-motion"), (60, "static")):
            self.assertEqual(phase_at(seconds, "mixed-rest"), expected)

    def test_completed_interval_boundary(self):
        self.assertEqual(evaluate_pose(2700, "mixed-active"), evaluate_pose(2700, "idle-motion"))
        self.assertEqual(evaluate_pose(2700, "mixed-active")["root_offset"], [0, 0])
        self.assertAlmostEqual(evaluate_pose(2701, "mixed-active")["root_offset"][0],
                               32 * math.sin(math.tau / 240), places=12)

    def test_mixed_rest_freezes_and_resumes_without_replay(self):
        self.assertEqual(evaluate_pose(2400, "mixed-rest"), evaluate_pose(0, "static"))
        self.assertEqual(evaluate_pose(2401, "mixed-rest"), evaluate_pose(1, "walk-desktop"))
        frozen = evaluate_pose(3600, "mixed-rest")
        self.assertEqual(frozen, evaluate_pose(6000, "mixed-rest"))
        self.assertNotEqual(frozen, evaluate_pose(6001, "mixed-rest"))

    def test_first_idle_step_spring_and_tracking(self):
        parts = {p["id"]: p for p in evaluate_pose(1, "idle-motion")["parts"]}
        # First semi-implicit Euler step: v=36*target/60; q=v/60.
        self.assertAlmostEqual(parts["tail"]["rotation"],
                               0.2 + 0.25 * math.sin(math.tau / 120) / 100, places=14)
        self.assertGreater(parts["eye_left"]["x"], -13)
        self.assertLess(parts["body"]["y"], 0)
        self.assertGreater(parts["head"]["rotation"], 0)

    def test_walk_peak_and_presentation_independence(self):
        self.assertAlmostEqual(evaluate_pose(60, "walk-fixed")["root_offset"][0], 32)
        self.assertAlmostEqual(evaluate_pose(180, "walk-fixed")["root_offset"][0], -32)
        self.assertEqual(evaluate_pose(301, "walk-fixed"), evaluate_pose(301, "walk-desktop"))

    def test_invalid_inputs(self):
        for tick in (-1, 1.5, True, "4"):
            with self.assertRaises((TypeError, ValueError)):
                evaluate_pose(tick, "static")
        for seconds in (-1, float("inf"), float("nan")):
            with self.assertRaises((TypeError, ValueError)):
                phase_at(seconds, "static")
        for function, value in ((evaluate_pose, 0), (phase_at, 0)):
            with self.assertRaises(ValueError):
                function(value, "typo")

    def test_rig_names_parent_order_bounds_and_finiteness(self):
        rig = load_rig()
        validate_rig(rig)
        self.assertEqual(len(rig["parts"]), 12)
        modifications = [
            ("id", rig["parts"][1]["id"]), ("parent", "muzzle"),
            ("rect", [255, 0, 44, 20]), ("position", [float("nan"), 0]),
            ("size", [-1, 20]),
        ]
        for key, value in modifications:
            broken = copy.deepcopy(rig)
            broken["parts"][0][key] = value
            with self.subTest(key=key), self.assertRaises(ValueError):
                validate_rig(broken)

    def test_golden_is_checked_in_and_matches_reference(self):
        golden = json.loads((SHARED / "golden.json").read_text())
        self.assertEqual(golden["version"], 1)
        self.assertEqual({p["scenario"] for p in golden["poses"]}, set(SCENARIOS))
        for entry in golden["poses"]:
            actual = evaluate_pose(entry["tick"], entry["scenario"])
            self.assertEqual(actual["root_offset"], entry["root_offset"])
            self.assertEqual(actual["parts"], entry["parts"])

    def test_all_golden_transforms_are_finite_and_tail_bounded(self):
        golden = json.loads((SHARED / "golden.json").read_text())
        for entry in golden["poses"]:
            for part in entry["parts"]:
                for key in ("x", "y", "rotation", "scale_x", "scale_y"):
                    self.assertTrue(math.isfinite(part[key]))
                if part["id"] == "tail":
                    self.assertLessEqual(abs(part["rotation"] - 0.2), 0.6)


if __name__ == "__main__":
    unittest.main()
