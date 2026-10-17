"""The new object and avian pets must be real distinct, bounded skinned assets."""
import hashlib
import json
import struct
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
IDS = ('rivet', 'paper-heron', 'acorn-custodian', 'puddle',
       'pocket-observatory', 'night-librarian', 'sprig', 'hearthling', 'crumb-golem')


class CatalogueBipeds(unittest.TestCase):
    def test_nine_original_assets_with_actual_legs(self):
        hashes = set()
        for name in IDS:
            with self.subTest(name=name):
                folder = ROOT / 'characters' / (name + '-3d')
                self.assertTrue((folder / 'manifest.json').exists(), name)
                manifest = json.loads((folder / 'manifest.json').read_text())
                self.assertEqual(manifest['asset'], name)
                self.assertEqual(manifest['kind'], 'builtin-3d')
                rig = json.loads((folder / 'rig.json').read_text())
                self.assertEqual(set(rig['contacts']), {'left', 'right'})
                joints = {j['name']: j for j in rig['joints']}
                for side in ('left', 'right'):
                    for part in ('thigh', 'shin', 'foot'):
                        self.assertIn(part + '_' + side, joints)
                    self.assertEqual(joints['shin_' + side]['parent'], 'thigh_' + side)
                self.assertFalse(rig['arms'])
                self.assertEqual(rig['skin'], ''.join(part.title() for part in name.split('-')) + 'Skin')
                self.assertTrue(rig['skinBounds'])
                self.assertTrue(all(group for group in rig['skinBounds']))
                raw = (folder / (name + '.glb')).read_bytes()
                hashes.add(hashlib.sha256(raw).hexdigest())
                size, = struct.unpack_from('<I', raw, 12)
                gltf = json.loads(raw[20:20 + size])
                self.assertFalse(gltf.get('animations'))
                self.assertEqual(len(gltf['skins']), 1)
                self.assertLessEqual(len(gltf['skins'][0]['joints']), 32)
                validation = json.loads((folder / 'validation.json').read_text())
                self.assertLessEqual(validation['triangles'], 5000)
                self.assertLessEqual(validation['max_influences'], 4)
                self.assertGreater(validation['armature_deformation_test_max_distance_meters'], .01)
                self.assertGreaterEqual(validation['continuous_weighted_legs'], 2)
                if 'eye_left' in joints:
                    self.assertGreater(validation['eye_closure_test_max_distance_meters'], .005)
                    self.assertGreater(validation['eye_closure_vertex_count'], 0)
                provenance = json.loads((folder / 'provenance.json').read_text())
                self.assertTrue(provenance['original_geometry'])
                self.assertTrue(provenance['original_texture'])
                for filename in ('LICENSE', 'atlas.png', name + '.blend', 'preview.png'):
                    self.assertTrue((folder / filename).exists(), filename)
        self.assertEqual(len(hashes), len(IDS))


if __name__ == '__main__':
    unittest.main()
