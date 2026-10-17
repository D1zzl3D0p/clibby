"""Production species geometry must preserve the four-foot skin contract."""
import json
import struct
from pathlib import Path
import unittest

from tools.measure_raven_bounds import measure

ROOT = Path(__file__).resolve().parents[1]
PETS = ('pip', 'biscuit', 'clover', 'mossback')
ROLES = {'foreLeft', 'foreRight', 'hindLeft', 'hindRight'}


class CatalogueQuadrupedTests(unittest.TestCase):
    def test_every_species_has_a_deforming_continuous_four_leg_skin(self):
        for pet in PETS:
            with self.subTest(pet=pet):
                directory = ROOT / 'characters' / (pet + '-3d')
                self.assertTrue((directory / (pet + '.glb')).is_file(), 'Missing skinned species')
                raw = (directory / (pet + '.glb')).read_bytes()
                length = struct.unpack_from('<I', raw, 12)[0]
                gltf = json.loads(raw[20:20 + length])
                for mesh in gltf['meshes']:
                    for primitive in mesh['primitives']:
                        position = gltf['accessors'][primitive['attributes']['POSITION']]
                        self.assertGreaterEqual(position['min'][1], -1e-5, 'Skin must not penetrate its authored floor')
                rig = json.loads((directory / 'rig.json').read_text())
                self.assertEqual(rig['gait'], 'quadrupedWalk')
                self.assertEqual(set(rig['contacts']), ROLES)
                unions = [{entry['joint'] for entry in group} for group in measure(directory / (pet + '.glb'))]
                for role in ROLES:
                    self.assertEqual(rig['contacts'][role][2], 0)
                    self.assertEqual(set(rig['legs'][role]), {'hip', 'knee', 'ankle', 'bend'})
                    for first, second in [('thigh_', 'shin_'), ('shin_', 'foot_')]:
                        self.assertTrue(any({first + role, second + role} <= group for group in unions))
                validation = json.loads((directory / 'validation.json').read_text())
                self.assertLessEqual(validation['max_influences'], 2)
                self.assertEqual(validation['continuous_weighted_legs'], 4)
                self.assertGreater(validation['eye_closure_test_max_distance_meters'], .005)
                self.assertGreater(validation['eye_closure_vertex_count'], 0)
                self.assertGreater(validation['armature_deformation_test_max_distance_meters'], .03)

    def test_species_have_distinct_rest_geometry_and_honest_optional_roles(self):
        dimensions = []
        for pet in PETS:
            directory = ROOT / 'characters' / (pet + '-3d')
            self.assertTrue((directory / 'rig.json').is_file(), 'Missing species rig')
            rig = json.loads((directory / 'rig.json').read_text())
            joints = {joint['name']: joint for joint in rig['joints']}
            heights = [joints['thigh_' + role]['head'][2] for role in sorted(ROLES)]
            widths = [rig['contacts'][role][0] for role in sorted(ROLES)]
            dimensions.append(tuple(round(value, 3) for value in heights + widths))
            self.assertIn('eye_left', joints)
            self.assertIn('eye_right', joints)
            if pet in ('biscuit', 'mossback'):
                self.assertNotIn('tail_base', joints)
            if pet == 'mossback':
                self.assertNotIn('ear_left', joints)
                self.assertLess(max(heights), .6)
            if pet == 'clover':
                self.assertGreater(joints['thigh_hindLeft']['head'][2], joints['thigh_foreLeft']['head'][2])
        self.assertEqual(len(set(dimensions)), len(PETS), 'Species must differ in anatomy, not only palette')
