"""Contracts against the final humanoids' actual exported mesh and skin."""
import hashlib
import json
from pathlib import Path
import unittest
from test_wayfarer_companions import read_glb

ROOT=Path(__file__).resolve().parents[1]
IDS=('maren','pell','serein')

class FinalHumanoids(unittest.TestCase):
    def package(self,name):
        folder=ROOT/'characters'/(name+'-3d')
        self.assertTrue((folder/(name+'.glb')).exists(),name+' export missing')
        return folder

    def test_complete_original_packages(self):
        for name in IDS:
            with self.subTest(name=name):
                folder=self.package(name)
                for f in ('atlas.png','preview.png','front.png','side.png','LICENSE','rig.json','validation.json','provenance.json',name+'.blend'):
                    self.assertTrue((folder/f).exists(),f)
                proof=json.loads((folder/'provenance.json').read_text())
                self.assertTrue(proof['original_geometry'])
                self.assertTrue(proof['original_texture'])
                for key,path in [('source_sha256','tools/author_final_humanoids.py'),('humanoid_helper_sha256','tools/humanoid_asset.py'),('helper_sha256','tools/catalogue_asset.py')]:
                    self.assertEqual(proof[key],hashlib.sha256((ROOT/path).read_bytes()).hexdigest())

    def test_real_three_joint_arms_props_and_rope_ownership(self):
        for name in IDS:
            with self.subTest(name=name):
                folder=self.package(name); gltf,rows=read_glb(folder/(name+'.glb'))
                nodes={n['name']:n for n in gltf['nodes'] if 'name' in n}
                required={'held_prop'}
                pairs=[('hand_left','held_prop')]
                for side in ('left','right'):
                    required.update(p+'_'+side for p in ('upper_arm','forearm','hand'))
                    pairs.extend([('upper_arm_'+side,'forearm_'+side),('forearm_'+side,'hand_'+side)])
                if name=='maren':
                    required.update(('rope_loop','rope_knot'))
                    pairs.extend([('held_prop','rope_loop'),('held_prop','rope_knot')])
                for parent,child in pairs:
                    self.assertIn(child,{gltf['nodes'][i]['name'] for i in nodes[parent]['children']})
                names=[gltf['nodes'][i]['name'] for i in gltf['skins'][0]['joints']]
                weighted=set(); triangles=0
                for mesh in gltf['meshes']:
                    for prim in mesh['primitives']:
                        triangles+=gltf['accessors'][prim['indices']]['count']//3
                        for js,ws in zip(rows(prim['attributes']['JOINTS_0']),rows(prim['attributes']['WEIGHTS_0'])):
                            self.assertAlmostEqual(sum(ws),1,places=5)
                            weighted.update(names[j] for j,w in zip(js,ws) if w>.99)
                self.assertTrue(required<=weighted,required-weighted)
                self.assertLessEqual(triangles,5000)
                validation=json.loads((folder/'validation.json').read_text())
                for joint in required:
                    proof=validation['humanoid_deformation'][joint]
                    self.assertGreater(proof['displacement_meters'],.002)
                    self.assertLess(proof['unrelated_displacement_meters'],1e-5)

    def test_adult_faces_skeletal_openings_and_cloven_ground_geometry(self):
        for name in IDS:
            with self.subTest(name=name):
                folder=self.package(name); gltf,rows=read_glb(folder/(name+'.glb'))
                points=[p for m in gltf['meshes'] for pr in m['primitives'] for p in rows(pr['attributes']['POSITION'])]
                exported={tuple(round(c,5) for c in p) for p in points}
                val=json.loads((folder/'validation.json').read_text())
                self.assertGreaterEqual(val['adult_height_in_heads'],5)
                self.assertLessEqual(val['adult_height_in_heads'],6.5)
                for x,y,z in val['face_mesh_vertices_blender']:
                    self.assertIn((round(x,5),round(z,5),round(-y,5)),exported)
                self.assertGreaterEqual(min(p[1] for p in points),-1e-5)
                anatomy=val['final_anatomy']
                for part,vertices in anatomy.items():
                    self.assertGreater(len(vertices),2,part)
                    for x,y,z in vertices:
                        self.assertIn((round(x,5),round(z,5),round(-y,5)),exported,part)
                if name=='pell':
                    self.assertTrue({'skull_cranium','skull_jaw','left_socket_rim','right_socket_rim','left_orbital_cavity','right_orbital_cavity','left_exposed_shin','right_exposed_shin'}<=set(anatomy))
                    for side in ('left','right'):
                        socket=anatomy[side+'_orbital_cavity']
                        self.assertGreater(max(p[0] for p in socket)-min(p[0] for p in socket),.10)
                        self.assertLess(min(p[1] for p in socket),-.19)
                if name=='serein':
                    self.assertTrue({'left_horn','right_horn','left_pointed_ear','right_pointed_ear','left_inner_clove','left_outer_clove','right_inner_clove','right_outer_clove'}<=set(anatomy))
                    for side in ('left','right'):
                        inner=anatomy[side+'_inner_clove']; outer=anatomy[side+'_outer_clove']
                        self.assertAlmostEqual(min(p[2] for p in inner),0,places=5)
                        self.assertAlmostEqual(min(p[2] for p in outer),0,places=5)
                        self.assertGreater(abs(sum(p[0] for p in inner)/len(inner)-sum(p[0] for p in outer)/len(outer)),.07)

    def test_ground_contacts_intersect_actual_weighted_soles(self):
        def inside(p,triangle):
            a,b,c=[(v[0],v[2]) for v in triangle]
            if any((p[0]-x)**2+(p[1]-z)**2<1e-10 for x,z in (a,b,c)): return True
            cross=lambda u,v:u[0]*v[1]-u[1]*v[0]
            sub=lambda u,v:(u[0]-v[0],u[1]-v[1])
            area=cross(sub(b,a),sub(c,a))
            if abs(area)<1e-12: return False
            u=cross(sub(p,a),sub(c,a))/area; v=cross(sub(b,a),sub(p,a))/area
            return u>=-1e-5 and v>=-1e-5 and u+v<=1.00001
        for name in IDS:
            folder=self.package(name); gltf,rows=read_glb(folder/(name+'.glb'))
            rig=json.loads((folder/'rig.json').read_text())
            names=[gltf['nodes'][i]['name'] for i in gltf['skins'][0]['joints']]
            for side,contact in rig['contacts'].items():
                triangles=[]
                for mesh in gltf['meshes']:
                    for pr in mesh['primitives']:
                        attrs=pr['attributes']; pts=rows(attrs['POSITION']); js=rows(attrs['JOINTS_0']); ws=rows(attrs['WEIGHTS_0']); indices=[r[0] for r in rows(pr['indices'])]
                        for start in range(0,len(indices),3):
                            face=indices[start:start+3]
                            if all(abs(pts[v][1])<1e-5 and any(names[j]=='foot_'+side and w>.99 for j,w in zip(js[v],ws[v])) for v in face):
                                triangles.append([pts[v] for v in face])
                self.assertTrue(any(inside((contact[0],-contact[1]),tri) for tri in triangles),(name,side,'contact misses real weighted sole'))

    def test_actual_triangles_fit_all_influence_hulls(self):
        for name in IDS:
            folder=self.package(name); gltf,rows=read_glb(folder/(name+'.glb'))
            rig=json.loads((folder/'rig.json').read_text()); skin=gltf['skins'][0]
            names=[gltf['nodes'][i]['name'] for i in skin['joints']]; inverse=rows(skin['inverseBindMatrices'])
            groups={frozenset(b['joint'] for b in g):{b['joint']:b for b in g} for g in rig['skinBounds']}
            for mesh in gltf['meshes']:
                for primitive in mesh['primitives']:
                    a=primitive['attributes']; positions=rows(a['POSITION']); joints=rows(a['JOINTS_0']); weights=rows(a['WEIGHTS_0'])
                    indices=[r[0] for r in rows(primitive['indices'])]
                    for start in range(0,len(indices),3):
                        face=indices[start:start+3]; active={j for v in face for j,w in zip(joints[v],weights[v]) if w>0}
                        bounds=groups[frozenset(names[j] for j in active)]
                        for j in active:
                            for v in face:
                                p=positions[v]+(1,); local=[sum(inverse[j][c*4+r]*p[c] for c in range(4)) for r in range(3)]
                                for axis in range(3):
                                    self.assertGreaterEqual(local[axis],bounds[names[j]]['minimum'][axis]-1e-5)
                                    self.assertLessEqual(local[axis],bounds[names[j]]['maximum'][axis]+1e-5)

if __name__=='__main__': unittest.main()
