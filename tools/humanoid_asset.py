"""Original adult humanoid construction over the existing bounded asset exporter."""
from pathlib import Path
import hashlib
import json
import bpy
from mathutils import Vector
from catalogue_asset import Asset

class Humanoid(Asset):
    def __init__(self,name,label,palette,height,width):
        palette=dict(palette)
        palette.pop('paper' if name=='saffron' else 'glass')
        palette['skin_light']=tuple(min(1,c*1.18+.025) for c in palette['skin'][:3])+(1,)
        super().__init__(name,label,palette)
        self.height,self.width=height,width
        self.hip=height*.46
        self.neck=height*.82
        self.face_z=height-.25
        self.joint('root',(0,0,0),(0,0,.15))
        self.joint('pelvis',(0,0,self.hip),(0,0,self.hip+.17),'root')
        self.joint('spine',(0,0,self.hip+.17),(0,0,self.neck),'pelvis')
        self.joint('head',(0,0,self.neck),(0,0,height-.05),'spine')
        # Geometry, rather than costume alone, differentiates the adult faces.
        face_x,face_y,face_h,jaw,segments={
            'nell':(.205,.175,.225,.92,10),
            'alden':(.215,.180,.225,.99,8),
            'saffron':(.170,.165,.240,.80,10),
            'tess':(.205,.180,.215,.95,10),
            'rowan':(.165,.160,.250,.80,8),
            'vesper':(.160,.165,.265,.78,10),
        }[name]
        face=self.blob('adult face',(0,-.035,self.face_z),(face_x,face_y,face_h),'skin','head',segments,6)
        for v in face.data.vertices:
            if v.co.z<0: v.co.x*=1+(jaw-1)*min(1,-v.co.z/(face_h*.6))
        self.face_vertices=[list(face.matrix_world@v.co) for v in face.data.vertices]
        self.face_bounds={'minimum':[min(p[i] for p in self.face_vertices) for i in range(3)],
                          'maximum':[max(p[i] for p in self.face_vertices) for i in range(3)]}
        nose_width={'nell':.047,'alden':.052,'saffron':.037,'tess':.049,'rowan':.035,'vesper':.034}[name]
        self.blob('nose',(0,-.035-face_y-.035,self.face_z-.015),(nose_width,.067,.056),'skin_light','head',6,3)
        front=-.035-face_y
        self.box('mouth',(0,front-.012,self.face_z-.105),(.045,.012,.012),'lip','head')
        self.eyes(self.face_z+.035,front-.025,.078,.032)
        self.blob('neck',(0,0,self.neck),(.09,.09,.14),'skin','spine',8,4)
        for s in (-1,1):
            self.blob('human ear',(s*face_x,-.005,self.face_z),(.045,.055,.075),'skin','head',6,4)
            self.box('brow',(s*.078,front-.025,self.face_z+.092),(.047,.019,.020),'hair','head')
        self.blob('hair crown',(0,.015,height-.105),(face_x+.010,face_y+.01,.125),'hair','head',10,5)
        self.blob('pelvis clothes',(0,.015,self.hip+.06),(width*.78,.19,.20),'trousers','pelvis')
        self.blob('broad clothed trunk',(0,.025,(self.neck+self.hip)/2),(width,.22,(self.neck-self.hip)/2+.05),'coat','spine',10,5)
        self.contacts=self.biped(hip_z=self.hip,width=width*.48,knee_y=-.065,radius=.082 if width<.32 else .105,
                               foot_scale=(.115,.205,.075),color='trousers',feet_color='boots')
        self.hands={}
        for side,s in [('left',1),('right',-1)]:
            sh=Vector((s*(width+.035),0,self.neck-.10))
            elbow=Vector((s*(width+.10),-.035,self.neck-.43))
            hand=Vector((s*(width+.065),-.255,self.neck-.67))
            upper,lower,palm=[p+'_'+side for p in ('upper_arm','forearm','hand')]
            self.joint(upper,sh,elbow,'spine')
            self.joint(lower,elbow,hand,upper)
            self.joint(palm,hand,hand+Vector((0,0,-.13)),lower)
            self.tube('continuous sleeve '+side,[sh,sh.lerp(elbow,.5),elbow,elbow.lerp(hand,.5),hand],
                      [.10,.095,.08,.075,.065],'coat',upper,
                      [{upper:1},{upper:.85,lower:.15},{upper:.5,lower:.5},{lower:1},{lower:.75,palm:.25}])
            self.blob('mitten hand '+side,hand+Vector((0,-.025,-.06)),(.075,.075,.11),'skin',palm,8,4)
            self.hands[side]=hand

    def prop_joint(self,offset=(0,-.035,-.08)):
        pos=self.hands['left']+Vector(offset)
        self.joint('held_prop',pos,pos+Vector((0,0,.12)),'hand_left')
        return pos

    def export(self,source):
        self.finish(source,self.contacts,arms=True,presentation={'pixels_per_meter':80,'camera_tilt_degrees':15.4,'yaw_bias_degrees':-55})
        skin=bpy.data.objects[self.skin_name]
        arm=skin.parent
        def evaluated():
            bpy.context.view_layer.update()
            ob=skin.evaluated_get(bpy.context.evaluated_depsgraph_get())
            mesh=ob.to_mesh()
            pts=[v.co.copy() for v in mesh.vertices]
            ob.to_mesh_clear()
            return pts
        rest=evaluated()
        checks={}
        for name in ['upper_arm_left','forearm_left','hand_left','upper_arm_right','forearm_right','hand_right']+[n for n in ('held_prop','prop_hinge','fastening') if n in arm.pose.bones]:
            descendants={name}
            for bone in arm.data.bones:
                if any(p.name==name for p in bone.parent_recursive): descendants.add(bone.name)
            owned={v.index for v in skin.data.vertices if any(skin.vertex_groups[g.group].name in descendants and g.weight>0 for g in v.groups)}
            bone=arm.pose.bones[name]
            bone.rotation_mode='XYZ'
            bone.rotation_euler.x=.25
            posed=evaluated()
            displacement=max((rest[i]-posed[i]).length for i in owned)
            unrelated=max((p-q).length for i,(p,q) in enumerate(zip(rest,posed)) if i not in owned)
            assert displacement>.002 and unrelated<1e-5,(name,displacement,unrelated)
            bone.rotation_euler.x=0
            checks[name]={'weighted_vertices':len(owned),'displacement_meters':displacement,'unrelated_displacement_meters':unrelated}
        path=self.out/'validation.json'
        val=json.loads(path.read_text())
        val.update(adult_height_in_heads=self.height/(self.face_bounds['maximum'][2]-self.face_bounds['minimum'][2]),humanoid_deformation=checks,face_mesh_bounds_blender=self.face_bounds,face_mesh_vertices_blender=self.face_vertices)
        path.write_text(json.dumps(val,indent=2)+'\n')
        path=self.out/'provenance.json'
        val=json.loads(path.read_text())
        val['humanoid_helper_sha256']=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
        path.write_text(json.dumps(val,indent=2)+'\n')
