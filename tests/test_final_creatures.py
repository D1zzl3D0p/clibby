"""Acceptance tests against the actual final creature GLB exports."""
import hashlib
import json
import unittest
from pathlib import Path
from test_wayfarer_companions import read_glb

ROOT=Path(__file__).resolve().parents[1]
APPENDAGES={
    'tarn': {'wing_fold_left','wing_fold_right','wing_tip_left','wing_tip_right','tail_base','tail_tip'},
    'nacre': {'gather_arm_left','gather_arm_right','fin_left','fin_right','pattern_left','pattern_right'},
    'windle': {'wisp_left','wisp_right','scarf_end'},
}

class FinalCreatures(unittest.TestCase):
    def test_actual_exported_contacts_appendages_and_parents(self):
        for name,required in APPENDAGES.items():
            with self.subTest(name=name):
                folder=ROOT/'characters'/(name+'-3d')
                self.assertTrue((folder/(name+'.glb')).exists(),name+' actual export missing')
                data,rows=read_glb(folder/(name+'.glb'))
                rig=json.loads((folder/'rig.json').read_text())
                nodes={n['name']:n for n in data['nodes'] if 'name' in n}
                joint_names=[data['nodes'][i]['name'] for i in data['skins'][0]['joints']]
                weighted=set(); points=[]; triangles=0
                for mesh in data['meshes']:
                    for pr in mesh['primitives']:
                        a=pr['attributes']; points+=rows(a['POSITION'])
                        triangles+=data['accessors'][pr['indices']]['count']//3
                        for js,ws in zip(rows(a['JOINTS_0']),rows(a['WEIGHTS_0'])):
                            self.assertAlmostEqual(sum(ws),1,places=5)
                            weighted.update(joint_names[j] for j,w in zip(js,ws) if w>.99)
                self.assertTrue(required<=weighted,required-weighted)
                self.assertLess(triangles,5000)
                self.assertGreaterEqual(min(v[1] for v in points),-1e-5)
                self.assertFalse(rig['arms'])
                self.assertEqual(len(rig['contacts']),4 if name=='tarn' else 2)
                hulls={b['joint'] for group in rig['skinBounds'] for b in group}
                self.assertTrue(required<=hulls)
                for role,contact in rig['contacts'].items():
                    x,y,z=contact
                    actual=(x,z,-y)
                    self.assertLess(min(sum((a-b)**2 for a,b in zip(v,actual))**.5 for v in points),.025,role)
                if name=='tarn':
                    for side in ('left','right'):
                        self.assertIn('wing_tip_'+side,{data['nodes'][i]['name'] for i in nodes['wing_fold_'+side]['children']})
                else:
                    self.assertNotIn('upper_arm_left',nodes)

    def test_exported_triangle_hulls_cover_all_actual_influences(self):
        for name in APPENDAGES:
            folder=ROOT/'characters'/(name+'-3d')
            data,rows=read_glb(folder/(name+'.glb'))
            rig=json.loads((folder/'rig.json').read_text())
            skin=data['skins'][0]
            names=[data['nodes'][i]['name'] for i in skin['joints']]
            inverse=rows(skin['inverseBindMatrices'])
            groups={frozenset(b['joint'] for b in group):{b['joint']:b for b in group} for group in rig['skinBounds']}
            for mesh in data['meshes']:
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

    def test_nacre_pattern_is_buried_at_rest_and_revealed_by_translation(self):
        folder=ROOT/'characters'/'nacre-3d'
        data,rows=read_glb(folder/'nacre.glb')
        names=[data['nodes'][i]['name'] for i in data['skins'][0]['joints']]
        mantle_faces=[]
        for mesh in data['meshes']:
            for pr in mesh['primitives']:
                a=pr['attributes']; ps=rows(a['POSITION']); js=rows(a['JOINTS_0']); ws=rows(a['WEIGHTS_0'])
                ix=[r[0] for r in rows(pr['indices'])]
                for start in range(0,len(ix),3):
                    face=ix[start:start+3]
                    if all(any(names[j]=='head' and w>.99 for j,w in zip(js[v],ws[v])) for v in face):
                        mantle_faces.append([ps[v] for v in face])
        for side,sign in [('left',1),('right',-1)]:
            points=[]
            for mesh in data['meshes']:
                for pr in mesh['primitives']:
                    a=pr['attributes']
                    for p,js,ws in zip(rows(a['POSITION']),rows(a['JOINTS_0']),rows(a['WEIGHTS_0'])):
                        if any(names[j]=='pattern_'+side and w>.99 for j,w in zip(js,ws)):
                            points.append(p)
            self.assertTrue(points)
            # Actual GLB patch vertices lie inside the mantle ellipsoid at rest.
            for x,z,negative_y in points:
                ellipsoid=(x/.43)**2+((-negative_y-.04)/.30)**2+((z-.97)/.39)**2
                self.assertLess(ellipsoid,1.0)
                # Intersect an X-axis ray with actual exported mantle triangles.
                # This catches corners protruding beyond its faceted surface.
                intersections=[]
                for first,second,third in mantle_faces:
                    u=[second[i]-first[i] for i in range(3)]
                    v=[third[i]-first[i] for i in range(3)]
                    determinant=u[1]*v[2]-u[2]*v[1]
                    if abs(determinant)<1e-9: continue
                    dy=z-first[1]; dz=negative_y-first[2]
                    alpha=(dy*v[2]-dz*v[1])/determinant
                    beta=(u[1]*dz-u[2]*dy)/determinant
                    if alpha>=-1e-5 and beta>=-1e-5 and alpha+beta<=1+1e-5:
                        intersections.append(first[0]+alpha*u[0]+beta*v[0])
                self.assertGreaterEqual(len(intersections),2)
                self.assertGreater(x,min(intersections)+1e-4)
                self.assertLess(x,max(intersections)-1e-4)
            # Model-X translation exposes every patch vertex beyond mantle's maximum width.
            self.assertGreater(min(sign*p[0]+.18 for p in points),.43)

    def test_tarn_wing_membrane_seam_is_on_actual_tip_hinge_axis(self):
        data,rows=read_glb(ROOT/'characters'/'tarn-3d'/'tarn.glb')
        names=[data['nodes'][i]['name'] for i in data['skins'][0]['joints']]
        owned={}
        for mesh in data['meshes']:
            for pr in mesh['primitives']:
                a=pr['attributes']
                for p,js,ws in zip(rows(a['POSITION']),rows(a['JOINTS_0']),rows(a['WEIGHTS_0'])):
                    for j,w in zip(js,ws):
                        if w>.99:
                            owned.setdefault(names[j],set()).add(tuple(round(v,5) for v in p))
        for side,sign in [('left',1),('right',-1)]:
            seam=owned['wing_fold_'+side]&owned['wing_tip_'+side]
            self.assertGreaterEqual(len(seam),2)
            for point in seam:
                self.assertAlmostEqual(point[0],sign*.36,places=4)
                self.assertAlmostEqual(point[2],-.18,places=4)
            self.assertGreater(max(p[1] for p in seam)-min(p[1] for p in seam),.29)

    def test_measured_deformation_and_reproducible_provenance(self):
        for name,required in APPENDAGES.items():
            folder=ROOT/'characters'/(name+'-3d')
            self.assertTrue((folder/'validation.json').exists(),name+' validation missing')
            val=json.loads((folder/'validation.json').read_text())
            self.assertTrue(required<=set(val['creature_deformation']))
            for joint in required:
                proof=val['creature_deformation'][joint]
                self.assertGreater(proof['weighted_vertices'],3)
                self.assertGreater(proof['displacement_meters'],.005)
                self.assertLess(proof['unrelated_displacement_meters'],1e-5)
            provenance=json.loads((folder/'provenance.json').read_text())
            self.assertEqual(provenance['source_sha256'],hashlib.sha256((ROOT/'tools/author_final_creatures.py').read_bytes()).hexdigest())
            for filename in ('LICENSE','atlas.png','preview.png','front.png','side.png',name+'.blend'):
                self.assertTrue((folder/filename).exists())

if __name__=='__main__': unittest.main()
