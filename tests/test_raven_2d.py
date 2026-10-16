"""Art geometry checks for the authored 2D raven."""
import json
import unittest
from pathlib import Path
from PIL import Image
ROOT = Path(__file__).resolve().parents[1]

class RavenArtTests(unittest.TestCase):
    def test_reduced_character_and_art_share_the_same_scale(self):
        d = json.loads((ROOT / "characters/raven/manifest.json").read_text())
        self.assertEqual((d["width"], d["height"]), (192, 168))
        torso = next(a for a in d["attachments"] if a["bone"] == "facing")
        self.assertEqual(torso["source"][2:], [104, 55])
        with Image.open(ROOT / "characters/raven/atlas.png") as atlas:
            for a in d["attachments"]:
                x, y, w, h = a["source"]
                self.assertTrue(0 <= x < x+w <= atlas.width and 0 <= y < y+h <= atlas.height)

    def test_visible_hock_changes_direction_and_soles_meet_the_floor(self):
        d = json.loads((ROOT / "characters/raven/manifest.json").read_text())
        bones = {b["id"]: b for b in d["bones"]}
        for side in ("Left", "Right"):
            upper, lower = bones["knee"+side]["rest"], bones["ankle"+side]["rest"]
            self.assertLess(upper["x"], -6)
            self.assertGreater(lower["x"], 4)
            y = sum(bones[n]["rest"]["y"] for n in ("body", "hip"+side, "knee"+side, "ankle"+side))
            contact = next(c for c in d["contacts"] if c["bone"] == "ankle"+side)
            self.assertAlmostEqual(y+contact["point"][1], d["locomotion"]["groundY"])
