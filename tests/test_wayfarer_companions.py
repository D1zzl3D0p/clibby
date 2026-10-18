"""Exported next-six anatomy, true skin ownership and differentiated proportions."""
import hashlib
import json
from pathlib import Path
import struct
import unittest

ROOT = Path(__file__).resolve().parents[1]
IDS = ('iona','milo','orris','fen','oona','quillan')

def read_glb(path):
    raw = path.read_bytes()
    size, = struct.unpack_from('<I', raw, 12)
    data = json.loads(raw[20:20+size])
    start = 28 + size
    def rows(index):
        a=data['accessors'][index]; view=data['bufferViews'][a['bufferView']]
        code,width={5121:('B',1),5123:('H',2),5125:('I',4),5126:('f',4)}[a['componentType']]
        count={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4,'MAT4':16}[a['type']]
        offset=start+view.get('byteOffset',0)+a.get('byteOffset',0)
        return [struct.unpack_from('<'+code*count,raw,offset+i*view.get('byteStride',width*count)) for i in range(a['count'])]
    return data, rows

class WayfarerAssets(unittest.TestCase):
    def test_packages_exist(self):
        for name in IDS:
            with self.subTest(name=name):
                folder=ROOT/'characters'/(name+'-3d')
                for filename in ('rig.json','validation.json','provenance.json','LICENSE','atlas.png','preview.png','front.png','side.png',name+'.glb',name+'.blend'):
                    self.assertTrue((folder/filename).exists(), filename)

    def test_actual_exported_arm_prop_and_fin_weights(self):
        for name in IDS:
            with self.subTest(name=name):
                folder=ROOT/'characters'/(name+'-3d')
                if not (folder/'rig.json').exists(): self.fail(name+' export missing')
                rig=json.loads((folder/'rig.json').read_text())
                gltf,rows=read_glb(folder/(name+'.glb'))
                nodes={n['name']:n for n in gltf['nodes'] if 'name' in n}
                skin_names=[gltf['nodes'][i]['name'] for i in gltf['skins'][0]['joints']]
                required=set()
                for side in ('left','right'):
                    for prefix in ('upper_arm_','forearm_','hand_'): required.add(prefix+side)
                    for parent,child in [('upper_arm_','forearm_'),('forearm_','hand_')]:
                        self.assertIn(child+side,{gltf['nodes'][i]['name'] for i in nodes[parent+side].get('children',[])})
                if name=='quillan':
                    self.assertNotIn('held_prop',nodes)
                    for side in ('left','right'):
                        required.add('ear_fin_'+side)
                        self.assertIn('ear_fin_'+side,{gltf['nodes'][i]['name'] for i in nodes['head']['children']})
                else:
                    required.add('held_prop')
                    self.assertIn('held_prop',{gltf['nodes'][i]['name'] for i in nodes['hand_left']['children']})
                if name in ('iona','orris','fen'):
                    required.add('prop_hinge')
                    self.assertIn('prop_hinge',{gltf['nodes'][i]['name'] for i in nodes['held_prop']['children']})
                weighted=set(); positions=[]; triangles=0
                for mesh in gltf['meshes']:
                    for primitive in mesh['primitives']:
                        attrs=primitive['attributes']; positions+=rows(attrs['POSITION'])
                        triangles+=gltf['accessors'][primitive['indices']]['count']//3
                        for js,ws in zip(rows(attrs['JOINTS_0']),rows(attrs['WEIGHTS_0'])):
                            self.assertAlmostEqual(sum(ws),1,places=5)
                            self.assertLessEqual(sum(w>0 for w in ws),2)
                            weighted.update(skin_names[j] for j,w in zip(js,ws) if w>.99)
                self.assertLessEqual(triangles,5000)
                self.assertTrue(required<=weighted,required-weighted)
                hulls={v['joint'] for group in rig['skinBounds'] for v in group}
                self.assertTrue(required<=hulls)
                val=json.loads((folder/'validation.json').read_text())
                self.assertTrue(required<=set(val['humanoid_deformation']))
                for joint in required:
                    proof=val['humanoid_deformation'][joint]
                    self.assertGreater(proof['displacement_meters'],.002,joint)
                    self.assertLess(proof['unrelated_displacement_meters'],1e-5,joint)
                face=val['face_mesh_vertices_blender']
                exported={tuple(round(v,5) for v in p) for p in positions}
                self.assertTrue(all((round(x,5),round(z,5),round(-y,5)) in exported for x,y,z in face))
                self.assertGreaterEqual(val['adult_height_in_heads'],5)
                self.assertLessEqual(val['adult_height_in_heads'],6.5)
                self.assertGreaterEqual(min(p[1] for p in positions),-1e-5)
                self.assertTrue(json.loads((folder/'provenance.json').read_text())['original_geometry'])

    def test_distinct_adult_anatomy_is_in_exported_meshes(self):
        extents={}; hips={}
        for name in IDS:
            folder=ROOT/'characters'/(name+'-3d')
            if not (folder/'rig.json').exists(): self.fail(name+' export missing')
            gltf,rows=read_glb(folder/(name+'.glb'))
            pts=[p for m in gltf['meshes'] for pr in m['primitives'] for p in rows(pr['attributes']['POSITION'])]
            extents[name]=[max(p[i] for p in pts)-min(p[i] for p in pts) for i in range(3)]
            rig=json.loads((folder/'rig.json').read_text())
            hips[name]=next(j['head'][2] for j in rig['joints'] if j['name']=='pelvis')
        self.assertLess(extents['orris'][1],extents['iona'][1]*.87)
        self.assertGreater(extents['orris'][0],extents['iona'][0]*1.20)
        self.assertGreater(extents['quillan'][1],extents['fen'][1]*1.12)
        self.assertLess(hips['orris'],hips['milo']*.78)
        self.assertLess(extents['fen'][0]/extents['fen'][1],extents['orris'][0]/extents['orris'][1]*.8)

    def test_exported_triangle_bounds_and_current_source_provenance(self):
        for name in IDS:
            with self.subTest(name=name):
                folder=ROOT/'characters'/(name+'-3d')
                rig=json.loads((folder/'rig.json').read_text())
                provenance=json.loads((folder/'provenance.json').read_text())
                for key,path in [('source_sha256','tools/author_wayfarer_companions.py'),('humanoid_helper_sha256','tools/humanoid_asset.py'),('helper_sha256','tools/catalogue_asset.py')]:
                    self.assertEqual(provenance[key],hashlib.sha256((ROOT/path).read_bytes()).hexdigest())
                gltf,rows=read_glb(folder/(name+'.glb'))
                skin=gltf['skins'][0]
                names=[gltf['nodes'][i]['name'] for i in skin['joints']]
                inverse=rows(skin['inverseBindMatrices'])
                groups={frozenset(b['joint'] for b in group):{b['joint']:b for b in group} for group in rig['skinBounds']}
                # Every actual triangle's vertices must lie inside each influence
                # hull. Linear skinning then bounds intermediate pixels during
                # arbitrary joint rotations, including mixed elbow/hand seams.
                for mesh in gltf['meshes']:
                    for primitive in mesh['primitives']:
                        attrs=primitive['attributes']
                        positions=rows(attrs['POSITION']); joints=rows(attrs['JOINTS_0']); weights=rows(attrs['WEIGHTS_0'])
                        indices=[r[0] for r in rows(primitive['indices'])]
                        for start in range(0,len(indices),3):
                            face=indices[start:start+3]
                            active={j for v in face for j,w in zip(joints[v],weights[v]) if w>0}
                            bounds=groups[frozenset(names[j] for j in active)]
                            for j in active:
                                matrix=inverse[j]; bound=bounds[names[j]]
                                for v in face:
                                    p=positions[v]+(1,)
                                    local=[sum(matrix[c*4+r]*p[c] for c in range(4)) for r in range(3)]
                                    for axis in range(3):
                                        self.assertGreaterEqual(local[axis],bound['minimum'][axis]-1e-5)
                                        self.assertLessEqual(local[axis],bound['maximum'][axis]+1e-5)

if __name__=='__main__': unittest.main()
