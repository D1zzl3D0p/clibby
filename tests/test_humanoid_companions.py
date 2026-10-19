"""Exported humanoids must have measured adults, articulated arms and owned props."""
import json
import struct
import unittest
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
IDS = ('nell','alden','saffron','tess','rowan','vesper')
class HumanoidAssets(unittest.TestCase):
    def test_original_adult_skins_and_bound_articulation(self):
        hashes=set()
        face_ratios=set()
        for name in IDS:
            with self.subTest(name=name):
                folder=ROOT/'characters'/(name+'-3d')
                rig=json.loads((folder/'rig.json').read_text())
                self.assertTrue(rig['arms'])
                self.assertEqual(set(rig['contacts']),{'left','right'})
                joints={j['name']:j for j in rig['joints']}
                for side in ('left','right'):
                    self.assertEqual(joints['forearm_'+side]['parent'],'upper_arm_'+side)
                    self.assertEqual(joints['hand_'+side]['parent'],'forearm_'+side)
                if name!='alden':
                    self.assertEqual(joints['held_prop']['parent'],'hand_left')
                if name in ('nell','vesper'):
                    self.assertEqual(joints['prop_hinge']['parent'],'held_prop')
                if name=='alden': self.assertEqual(joints['fastening']['parent'],'spine')
                validation=json.loads((folder/'validation.json').read_text())
                self.assertGreaterEqual(validation['adult_height_in_heads'],5)
                self.assertLessEqual(validation['adult_height_in_heads'],6.5)
                self.assertLessEqual(validation['triangles'],5000)
                self.assertEqual(validation['continuous_weighted_legs'],2)
                for joint,proof in validation['humanoid_deformation'].items():
                    self.assertGreater(proof['weighted_vertices'],0,joint)
                    self.assertGreater(proof['displacement_meters'],.002,joint)
                    self.assertLess(proof['unrelated_displacement_meters'],1e-5,joint)
                raw=(folder/(name+'.glb')).read_bytes()
                import hashlib
                hashes.add(hashlib.sha256(raw).hexdigest())
                size,=struct.unpack_from('<I',raw,12)
                gltf=json.loads(raw[20:20+size])
                skin_names=[gltf['nodes'][i]['name'] for i in gltf['skins'][0]['joints']]
                skin=set(skin_names)
                nodes={n['name']:n for n in gltf['nodes'] if 'name' in n}
                for side in ('left','right'):
                    for parent,child in [('upper_arm_','forearm_'),('forearm_','hand_')]:
                        children={gltf['nodes'][i]['name'] for i in nodes[parent+side].get('children',[])}
                        self.assertIn(child+side,children)
                start=20+size+8
                def rows(index):
                    a=gltf['accessors'][index]
                    view=gltf['bufferViews'][a['bufferView']]
                    code,width={5121:('B',1),5123:('H',2),5126:('f',4)}[a['componentType']]
                    count={'VEC3':3,'VEC4':4}[a['type']]
                    offset=start+view.get('byteOffset',0)+a.get('byteOffset',0)
                    return [struct.unpack_from('<'+code*count,raw,offset+i*view.get('byteStride',width*count)) for i in range(a['count'])]
                weighted=set()
                positions=[]
                for mesh in gltf['meshes']:
                    for primitive in mesh['primitives']:
                        attrs=primitive['attributes']
                        positions+=rows(attrs['POSITION'])
                        for js,ws in zip(rows(attrs['JOINTS_0']),rows(attrs['WEIGHTS_0'])):
                            self.assertAlmostEqual(sum(ws),1,places=5)
                            self.assertLessEqual(sum(w>0 for w in ws),2)
                            weighted.update(skin_names[j] for j,w in zip(js,ws) if w>.99)
                self.assertTrue(set(validation['humanoid_deformation'])<=weighted)
                # Match actual face vertices to exported positions; metadata alone cannot
                # satisfy the distinct adult-face requirement.
                exported={tuple(round(v,5) for v in p) for p in positions}
                face=validation['face_mesh_vertices_blender']
                for x,y,z in face:
                    self.assertIn((round(x,5),round(z,5),round(-y,5)),exported)
                extent=[max(p[i] for p in face)-min(p[i] for p in face) for i in range(3)]
                face_ratios.add(round(extent[0]/extent[2],3))
                self.assertGreater(max(p[1] for p in positions),2.3)
                self.assertGreaterEqual(min(p[1] for p in positions),-1e-5)
                if name!='alden':
                    self.assertIn('held_prop',{gltf['nodes'][i]['name'] for i in nodes['hand_left']['children']})
                self.assertTrue(set(validation['humanoid_deformation'])<=skin)
                bounds={v['joint'] for group in rig['skinBounds'] for v in group}
                self.assertTrue(set(validation['humanoid_deformation'])<=bounds)
                provenance=json.loads((folder/'provenance.json').read_text())
                self.assertTrue(provenance['original_geometry'])
                self.assertTrue(provenance['original_texture'])
                for file in ('LICENSE','atlas.png',name+'.blend','preview.png','front.png','side.png'):
                    self.assertTrue((folder/file).exists(),file)
        self.assertEqual(len(hashes),6)
        self.assertGreaterEqual(len(face_ratios),5)

    def test_shoulders_join_the_chest_and_arms_rest_beside_it(self):
        # This catches the old wide socket / tapered ellipsoid mismatch in
        # the exported mesh, rather than trusting authoring metadata.
        from test_wayfarer_companions import read_glb
        spans = {}
        for name in IDS + ('iona','milo','orris','fen','oona','quillan'):
            with self.subTest(name=name):
                folder = ROOT/'characters'/(name+'-3d')
                rig = json.loads((folder/'rig.json').read_text())
                joints = {j['name']:j for j in rig['joints']}
                shoulder = joints['upper_arm_left']['head']
                elbow = joints['forearm_left']['head']
                spans[name] = 2*shoulder[0]
                self.assertLessEqual(elbow[0]-shoulder[0], .04,
                                     'resting upper arm splays away from chest')
                gltf, rows = read_glb(folder/(name+'.glb'))
                names = [gltf['nodes'][i]['name'] for i in gltf['skins'][0]['joints']]
                chest = []
                for mesh in gltf['meshes']:
                    for primitive in mesh['primitives']:
                        attrs = primitive['attributes']
                        for point, js, ws in zip(rows(attrs['POSITION']), rows(attrs['JOINTS_0']), rows(attrs['WEIGHTS_0'])):
                            if any(names[j]=='spine' and w>.99 for j,w in zip(js,ws)):
                                chest.append(point)
                # glTF +Y is Blender +Z. A real chest ring must reach the
                # arm socket; a low belly maximum cannot satisfy this.
                socket_chest = [p for p in chest if abs(p[1]-shoulder[2])<.075 and abs(p[2])<.15]
                self.assertTrue(socket_chest)
                self.assertGreaterEqual(max(p[0] for p in socket_chest), shoulder[0]-.025,
                                        'sleeve socket sits beyond upper chest')
        self.assertGreater(spans['alden'], spans['rowan']*1.4)
        self.assertGreater(spans['orris'], spans['iona']*1.4)
        self.assertLess(spans['quillan'], spans['milo']*.8)
if __name__=='__main__': unittest.main()
