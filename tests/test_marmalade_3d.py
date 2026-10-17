"""Exported quadruped asset contract."""
import json
import unittest
from pathlib import Path
from tools.measure_raven_bounds import measure

ROOT = Path(__file__).resolve().parents[1] / "characters/marmalade-3d"
ROLES = {"foreLeft", "foreRight", "hindLeft", "hindRight"}

class Marmalade3DTests(unittest.TestCase):
    def test_exported_skin_covers_every_limb_seam(self):
        self.assertTrue((ROOT / "marmalade.glb").exists(), "Missing production skinned cat")
        bounds = measure(ROOT / "marmalade.glb")
        unions = [{entry["joint"] for entry in group} for group in bounds]
        for role in ROLES:
            for seam in (("thigh_", "shin_"), ("shin_", "foot_")):
                self.assertTrue(any({prefix + role for prefix in seam} <= group for group in unions), role + str(seam))

    def test_rig_has_four_ground_contacts_and_no_biped_arms(self):
        self.assertTrue((ROOT / "rig.json").exists(), "Missing quadruped rig")
        rig = json.loads((ROOT / "rig.json").read_text())
        self.assertEqual(set(rig["contacts"]), ROLES)
        self.assertEqual(rig["gait"], "quadrupedWalk")
        names = {j["name"] for j in rig["joints"]}
        self.assertFalse(any(n.startswith("arm") or n.startswith("wing") for n in names))
        for role, contact in rig["contacts"].items():
            self.assertEqual(contact[2], 0)
            for prefix in ("thigh_", "shin_", "foot_"):
                self.assertIn(prefix + role, names)
