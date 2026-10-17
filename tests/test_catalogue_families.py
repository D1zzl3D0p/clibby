"""Behavioral contracts for four experimental, nonresident body families."""
import importlib.util
import math
from pathlib import Path
import sys
import unittest

SOURCE = Path(__file__).resolve().parents[1] / "experiments/catalogue-families/motion.py"


class CatalogueFamilyTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if SOURCE.exists():
            spec = importlib.util.spec_from_file_location("catalogue_family_motion", SOURCE)
            cls.motion = importlib.util.module_from_spec(spec)
            sys.modules[spec.name] = cls.motion
            spec.loader.exec_module(cls.motion)

    def test_returned_pose_is_immutable(self):
        pose = self.motion.evaluate("pebblekin", 2.0)
        with self.assertRaises(AttributeError):
            pose.root = (1, 0, 0)

    def test_reads_are_deterministic_and_do_not_advance_motion(self):
        for pet in self.motion.PETS:
            first = self.motion.evaluate(pet, 2.7)
            self.motion.evaluate(pet, 4.8)
            self.assertEqual(first, self.motion.evaluate(pet, 2.7))

    def test_all_families_settle_after_finite_action(self):
        for pet in self.motion.PETS:
            self.assertFalse(self.motion.evaluate(pet, 0).active)
            self.assertEqual(self.motion.evaluate(pet, 6), self.motion.evaluate(pet, 600))
            self.assertEqual(self.motion.evaluate(pet, 6), self.motion.evaluate(pet, 1e308))
            self.assertFalse(self.motion.evaluate(pet, 8).active)

    def test_no_family_advertises_unimplemented_flight(self):
        for capabilities in self.motion.CAPABILITIES.values():
            self.assertNotIn("flight", capabilities)
        self.assertNotIn("walk", self.motion.CAPABILITIES["pebblekin"])

    def test_anchored_base_never_moves_during_wobble(self):
        rest = self.motion.evaluate("pebblekin", 0)
        changed = False
        for i in range(121):
            pose = self.motion.evaluate("pebblekin", i / 15)
            self.assertEqual(pose.root, rest.root)
            self.assertEqual(pose.support_polygon, rest.support_polygon)
            self.assertEqual(pose.contacts, ())
            self.assertLessEqual(abs(pose.value("wobble")), .14)
            changed |= abs(pose.value("wobble")) > .05
        self.assertTrue(changed)

    def test_upper_stone_keeps_actual_mesh_contact_during_pivot(self):
        rest_geometry = self.motion.pebble_geometry(self.motion.evaluate("pebblekin", 0))
        moved = False
        for i in range(121):
            geometry = self.motion.pebble_geometry(self.motion.evaluate("pebblekin", i/15))
            middle, _ = geometry["middle"]
            upper, _ = geometry["upper"]
            pivot = geometry["pivot"]
            self.assertIn(pivot, upper)
            level = max(p[2] for p in middle)
            face = [v for v in middle if abs(v[2]-level) < 1e-12]
            self.assertGreaterEqual(len(face), 3, "support must be a visible face, not an unresolved point")
            self.assertAlmostEqual(level, pivot[2])
            self.assertAlmostEqual(min(p[2] for p in upper), pivot[2])
            # Contact lies inside the actual authored convex support face.
            for a,b in zip(face, face[1:]+face[:1]):
                self.assertGreaterEqual((b[0]-a[0])*(pivot[1]-a[1])-(b[1]-a[1])*(pivot[0]-a[0]), -1e-12)
            self.assertEqual(geometry["middle"], rest_geometry["middle"])
            moved |= geometry["upper"] != rest_geometry["upper"]
        self.assertTrue(moved)

    def test_snail_realizes_travel_with_continuous_floor_contact(self):
        previous = self.motion.evaluate("lantern-snail", 0)
        for i in range(1, 121):
            pose = self.motion.evaluate("lantern-snail", i / 15)
            self.assertGreaterEqual(pose.root[0], previous.root[0])
            self.assertEqual(pose.contacts, ())
            self.assertEqual(pose.support_kind, "continuous")
            self.assertTrue(all(p[2] == 0 for p in pose.support_polygon))
            xs = [p[0] for p in pose.support_polygon]
            self.assertAlmostEqual(max(xs) - min(xs), 2)
            self.assertEqual(len(pose.support_polygon), 50)
            self.assertAlmostEqual(abs(pose.support_polygon[0][1]), .055)
            self.assertGreater(abs(pose.support_polygon[12][1]), .25)
            previous = pose
        self.assertAlmostEqual(previous.root[0], .4)
        self.assertGreater(self.motion.evaluate("lantern-snail", 3.5).value("door"), 1)

    def test_margin_morph_is_visible_finite_and_stays_inside_envelope(self):
        base = self.motion.evaluate("margin", 0)
        self.assertNotEqual(base.points, self.motion.evaluate("margin", 2.25).points)
        self.assertNotEqual(base.points, self.motion.evaluate("margin", 4.75).points)
        for i in range(121):
            pose = self.motion.evaluate("margin", i / 15)
            self.assertEqual(len(pose.points), 32)
            for _, (x, y, z) in pose.points:
                self.assertTrue(all(math.isfinite(a) for a in (x, y, z)))
                self.assertLessEqual(abs(x), 1.1)
                self.assertGreaterEqual(z, 0)
                self.assertLessEqual(z, 2.5)

    def test_velvet_keeps_at_least_one_tripod_on_ground(self):
        seen_swing = False
        for i in range(1201):
            pose = self.motion.evaluate("velvet", i / 150)
            self.assertEqual(len(pose.contacts), 6)
            planted = [c for c in pose.contacts if c.planted]
            self.assertGreaterEqual(len(planted), 3)
            self.assertTrue(all(c.position[2] == 0 for c in planted))
            seen_swing |= len(planted) < 6
        self.assertTrue(seen_swing)

    def test_velvet_stance_feet_are_world_pinned(self):
        previous = self.motion.evaluate("velvet", 0)
        for i in range(1, 1201):
            pose = self.motion.evaluate("velvet", i / 150)
            for before, after in zip(previous.contacts, pose.contacts):
                if before.planted and after.planted:
                    self.assertEqual(before.position, after.position)
            previous = pose
        self.assertAlmostEqual(previous.root[0], .4)

    def test_velvet_root_stays_inside_planted_support_hull(self):
        # Every line through two support points defines a hull edge if every
        # other support lies on the same side. The root must be on that side.
        for i in range(1201):
            pose = self.motion.evaluate("velvet", i / 150)
            points = pose.support_polygon
            for a in points:
                for b in points:
                    if a == b:
                        continue
                    cross = lambda p: (b[0]-a[0])*(p[1]-a[1])-(b[1]-a[1])*(p[0]-a[0])
                    signs = [cross(p) for p in points]
                    if min(signs) >= -1e-9:
                        self.assertGreaterEqual(cross(pose.root), -1e-9)

    def test_invalid_time_and_unknown_pet_reject(self):
        for time in (-1, math.nan, math.inf):
            with self.assertRaises(ValueError):
                self.motion.evaluate("velvet", time)
        with self.assertRaises(ValueError):
            self.motion.evaluate("imaginary", 0)
