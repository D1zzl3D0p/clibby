"""Exercise the authoring output, including usable independent IK geometry."""
import json
import math
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / 'tools/author_marmalade.py'


class MarmaladeTests(unittest.TestCase):
    def generate(self, destination):
        result = subprocess.run([sys.executable, str(SCRIPT), '--output', str(destination)],
                                capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        return destination / 'marmalade'

    def test_generation_is_reproducible_and_check_detects_changed_asset(self):
        # A random pixel, stale source package, or ineffective --check breaks this.
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            first = self.generate(root / 'a')
            second = self.generate(root / 'b')
            self.assertEqual({p.name: p.read_bytes() for p in first.iterdir()},
                             {p.name: p.read_bytes() for p in second.iterdir()})
            command = [sys.executable, str(SCRIPT), '--output', str(root / 'a'), '--check']
            self.assertEqual(subprocess.run(command, capture_output=True).returncode, 0)
            (first / 'atlas.png').write_bytes(b'changed')
            self.assertNotEqual(subprocess.run(command, capture_output=True).returncode, 0)

    def test_package_has_four_independent_reachable_floor_contacts(self):
        # Sharing a joint, parenting to facing, or over-folding a swing breaks this.
        with tempfile.TemporaryDirectory() as directory:
            package = self.generate(Path(directory))
            m = json.loads((package / 'manifest.json').read_text())
            self.assertEqual((m['id'], m['controller'], m['locomotion']['gait']),
                             ('marmalade', 'articulated', 'quadrupedWalk'))
            bones = {b['id']: b for b in m['bones']}
            world = {}
            for b in m['bones']:
                parent = world.get(b['parent'], (0, 0))
                self.assertEqual((b['rest']['scaleX'], b['rest']['scaleY']), (1, 1))
                world[b['id']] = (parent[0] + b['rest']['x'], parent[1] + b['rest']['y'])
            legs = m['locomotion']['legs']
            self.assertEqual({leg['role'] for leg in legs},
                             {'foreLeft', 'foreRight', 'hindLeft', 'hindRight'})
            self.assertEqual(len(legs), 4)
            joints = [leg[key] for leg in legs for key in ('hip', 'knee', 'ankle')]
            self.assertEqual(len(set(joints)), 12)
            soles = []
            for leg in legs:
                self.assertEqual(bones[leg['hip']]['parent'], m['bindings']['body'])
                self.assertEqual(bones[leg['knee']]['parent'], leg['hip'])
                self.assertEqual(bones[leg['ankle']]['parent'], leg['knee'])
                hip, ankle = world[leg['hip']], world[leg['ankle']]
                lengths = [math.hypot(bones[leg[k]]['rest']['x'],
                                      bones[leg[k]]['rest']['y']) for k in ('knee', 'ankle')]
                self.assertAlmostEqual(ankle[1] + leg['contact'][1], m['locomotion']['groundY'])
                self.assertGreater(ankle[1] - hip[1] - m['locomotion']['lift'], abs(lengths[0] - lengths[1]) + 1)
                self.assertLess(math.hypot(m['locomotion']['stride'], ankle[1] - hip[1]), sum(lengths))
                self.assertIn({'bone': leg['ankle'], 'point': leg['contact']}, m['contacts'])
                soles.append(ankle[0] + leg['contact'][0])
            self.assertEqual(len(set(soles)), 4)
            self.assertLessEqual(len(m['bones']), 64)
            self.assertLessEqual(len(m['attachments']), 64)
            with Image.open(package / 'atlas.png') as atlas:
                self.assertLessEqual(max(atlas.size), 2048)
                eyes = [a for a in m['attachments'] if a['bone'] == m['bindings']['eyes']]
                self.assertTrue(eyes)
                for a in m['attachments']:
                    x, y, w, h = a['source']
                    self.assertGreater(w * h, 0)
                    self.assertLessEqual(x + w, atlas.width)
                    self.assertLessEqual(y + h, atlas.height)
                    self.assertIsNotNone(atlas.crop((x, y, x + w, y + h)).getbbox())


if __name__ == '__main__':
    unittest.main()
