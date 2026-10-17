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


    def test_signature_appendages_have_real_weighted_geometry_and_isolated_motion(self):
        expected = {
            'rivet': {'antenna': 'head', 'signature_arm_left': 'spine'},
            'paper-heron': {'wing_fold_left': 'spine', 'wing_fold_right': 'spine'},
            'acorn-custodian': {'cap': 'head'},
            'puddle': {'wing_fold_left': 'spine', 'wing_fold_right': 'spine'},
            'night-librarian': {'cover_front': 'head', 'cover_back': 'head', 'bookmark': 'head'},
            'sprig': {'leaf_present': 'head'},
            'crumb-golem': {'signature_arm_left': 'spine'},
        }
        for name, appendages in expected.items():
            with self.subTest(name=name):
                folder = ROOT / 'characters' / (name + '-3d')
                rig = json.loads((folder / 'rig.json').read_text())
                joints = {j['name']: j for j in rig['joints']}
                bounds = {item['joint'] for group in rig['skinBounds'] for item in group}
                validation = json.loads((folder / 'validation.json').read_text())
                raw = (folder / (name + '.glb')).read_bytes()
                size, = struct.unpack_from('<I', raw, 12)
                gltf = json.loads(raw[20:20 + size])
                binary_start = 20 + size + 8
                skin_joints = [gltf['nodes'][i]['name'] for i in gltf['skins'][0]['joints']]

                def accessor_rows(index):
                    accessor = gltf['accessors'][index]
                    view = gltf['bufferViews'][accessor['bufferView']]
                    code, width = {5121: ('B', 1), 5123: ('H', 2), 5126: ('f', 4)}[accessor['componentType']]
                    count = {'VEC4': 4}[accessor['type']]
                    offset = binary_start + view.get('byteOffset', 0) + accessor.get('byteOffset', 0)
                    stride = view.get('byteStride', count * width)
                    return [struct.unpack_from('<' + code * count, raw, offset + i * stride)
                            for i in range(accessor['count'])]

                weighted = set()
                for mesh in gltf['meshes']:
                    for primitive in mesh['primitives']:
                        attrs = primitive['attributes']
                        for indices, weights in zip(accessor_rows(attrs['JOINTS_0']),
                                                    accessor_rows(attrs['WEIGHTS_0'])):
                            weighted.update(skin_joints[index] for index, weight in zip(indices, weights)
                                            if weight > .99)
                for joint, parent in appendages.items():
                    self.assertEqual(joints[joint]['parent'], parent)
                    self.assertIn(joint, bounds)
                    self.assertIn(joint, weighted)
                    proof = validation['signature_appendage_deformation'][joint]
                    self.assertGreater(proof['weighted_vertices'], 0)
                    self.assertGreater(proof['displacement_meters'], .005)
                    self.assertLess(proof['unrelated_displacement_meters'], 1e-5)
                self.assertFalse(rig['arms'])


if __name__ == '__main__':
    unittest.main()
