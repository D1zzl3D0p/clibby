"""Original final companion recipes; Blender --background --python this_file."""
from pathlib import Path
import sys
import math
import json
import bpy
from mathutils import Vector
sys.path.insert(0,str(Path(__file__).resolve().parent))
from catalogue_asset import Asset
from humanoid_asset import Humanoid

PALETTE={'skin':(.59,.38,.25,1),'coat':(.16,.29,.30,1),'trousers':(.19,.20,.20,1),'boots':(.09,.075,.06,1),'hair':(.64,.61,.53,1),'ink':(.018,.026,.025,1),'lip':(.29,.16,.12,1),'linen':(.76,.68,.48,1),'metal':(.44,.40,.28,1),'accent':(.48,.29,.13,1),'paper':(.71,.73,.40,1)}

class FinalHumanoid(Asset):
    """Independent construction preserves every earlier helper and recipe hash."""
    def __init__(self,name,label,height,width,colors):
        palette=dict(PALETTE,**colors)
        palette['skin_light']=tuple(min(1,c*1.18+.025) for c in palette['skin'][:3])+(1,)
        super().__init__(name,label,palette)
        self.height,self.width=height,width
        self.hip,self.neck=height*.46,height*.82
        self.face_z=height-.25
        self.anatomy={}
        self.joint('root',(0,0,0),(0,0,.15))
        self.joint('pelvis',(0,0,self.hip),(0,0,self.hip+.17),'root')
        self.joint('spine',(0,0,self.hip+.17),(0,0,self.neck),'pelvis')
        self.joint('head',(0,0,self.neck),(0,0,height-.05),'spine')
        if name=='pell':
            cranium=self.mark('skull_cranium',self.blob('actual ivory skull vault',(0,.01,self.face_z+.075),(.184,.160,.158),'skin','head',10,6))
            jaw=self.mark('skull_jaw',self.blob('separate angular mandible',(0,-.045,self.face_z-.145),(.145,.122,.065),'skin','head',8,4))
            self.face_vertices=self.vertices(cranium)+self.vertices(jaw)
            for side,s in [('left',1),('right',-1)]:
                cx=s*.083; cy=-.158; cz=self.face_z+.008
                points=[(cx+.066*math.cos(i*math.tau/12),cy,cz+.075*math.sin(i*math.tau/12)) for i in range(13)]
                self.mark(side+'_socket_rim',self.tube('open orbital bone rim '+side,points,[.026]*13,'skin','head',sides=5))
                self.taper('zygomatic cheekbone '+side,(s*.14,-.12,self.face_z-.035),(s*.09,-.14,self.face_z-.105),.035,.025,'skin','head',6)
            self.eyes(self.face_z+.005,-.185,.083,.056)
            for side in ('left','right'):
                eye=next(ob for ob in self.parts if ob.name=='eye '+side)
                self.mark(side+'_orbital_cavity',eye)
            self.wedge('nasal aperture',[(0,-.175,self.face_z-.035),(-.027,-.175,self.face_z-.084),(.027,-.175,self.face_z-.084)],[(0,1,2)],'ink','head')
            for x in (-.08,-.04,0,.04,.08):
                self.box('separate visible tooth',(x,-.158,self.face_z-.095),(.013,.018,.027),'skin_light','head')
            self.taper('exposed cervical vertebrae',(0,0,self.neck-.03),(0,0,self.face_z-.18),.045,.05,'skin','head',6)
        else:
            dims=(.185,.170,.235) if name=='maren' else (.163,.157,.240)
            face=self.blob('adult weathered face' if name=='maren' else 'adult faun face',(0,-.035,self.face_z),dims,'skin','head',10,6)
            self.face_vertices=self.vertices(face)
            self.blob('broad adult nose',(0,-.035-dims[1]-.032,self.face_z-.016),(.049 if name=='maren' else .034,.061,.055),'skin_light','head',6,4)
            self.box('quiet mouth',(0,-.035-dims[1]-.012,self.face_z-.10),(.047,.012,.014),'lip','head')
            self.eyes(self.face_z+.033,-.035-dims[1]-.025,.075,.030)
            self.blob('visible neck',(0,0,self.neck),(.087,.09,.14),'skin','spine',8,4)
            self.blob('silver swept crown' if name=='maren' else 'curled chestnut crown',(0,.025,height-.108),(dims[0]+.015,dims[1]+.01,.12),'hair','head',10,5)
            for s in (-1,1):
                self.box('adult expressive brow',(s*.075,-.035-dims[1]-.021,self.face_z+.09),(.045,.018,.018),'hair','head')
                if name=='maren':
                    self.blob('human ear',(s*.183,0,self.face_z),(.043,.051,.072),'skin','head',6,4)
                    self.box('weathered cheek crease',(s*.115,-.184,self.face_z-.065),(.034,.009,.010),'lip','head')
        self.face_bounds={'minimum':[min(p[i] for p in self.face_vertices) for i in range(3)],'maximum':[max(p[i] for p in self.face_vertices) for i in range(3)]}
        self.blob('clothed pelvis',(0,.025,self.hip+.05),(width*.78,.17,.19),'trousers','pelvis',10,5)
        self.blob('adult fitted trunk',(0,.025,(self.neck+self.hip)/2),(width,.20,(self.neck-self.hip)/2+.035),'coat','spine',10,5)
        radius=.058 if name=='pell' else .082
        self.contacts=self.biped(hip_z=self.hip,width=width*.48,knee_y=-.065,radius=radius,foot_scale=(.105,.19,.07),color='skin' if name=='pell' else 'trousers',feet_color='boots')
        if name=='maren':
            for side,s in [('left',1),('right',-1)]:
                self.box('flat worn boot sole '+side,(s*width*.48,-.10,.018),(.09,.17,.018),'boots','foot_'+side)
        if name in ('pell','serein'):
            for ob in list(self.parts):
                if ob.name.startswith('planted foot') or (name=='pell' and ob.name.startswith('continuous leg')):
                    self.parts.remove(ob); bpy.data.objects.remove(ob,do_unlink=True)
            if name=='pell':
                self.weighted_tubes=[n for n in self.weighted_tubes if not n.startswith('continuous leg')]
                for side,s in [('left',1),('right',-1)]:
                    x=s*width*.48; hip=Vector((x,0,self.hip)); knee=Vector((x,-.065,(self.hip+.13)/2)); ankle=Vector((x,0,.13))
                    self.taper('exposed femur '+side,hip,knee,.048,.037,'skin','thigh_'+side,7)
                    self.mark(side+'_exposed_shin',self.taper('actual tibia '+side,knee,ankle,.032,.024,'skin','shin_'+side,7))
                    self.taper('separate fibula '+side,knee+Vector((s*.043,0,-.03)),ankle+Vector((s*.034,0,.025)),.016,.012,'skin','shin_'+side,5)
                    self.blob('bony patella '+side,knee+Vector((0,-.03,0)),(.048,.04,.055),'skin_light','shin_'+side,6,4)
                    self.box('skeletal travel shoe '+side,(x,-.10,.045),(.10,.19,.045),'boots','foot_'+side)
            else:
                for side,s in [('left',1),('right',-1)]:
                    x=s*width*.48
                    for kind,dx in [('inner',-.055*s),('outer',.055*s)]:
                        self.mark(side+'_'+kind+'_clove',self.box('separate cloven hoof '+side+' '+kind,(x+dx,-.10,.065),(.044,.185,.065),'boots','foot_'+side))
                    self.contacts[side]=[x+.055*s,-.10,0]
        self.hands={}
        for side,s in [('left',1),('right',-1)]:
            sh=Vector((s*(width+.035),0,self.neck-.10)); elbow=Vector((s*(width+.10),-.035,self.neck-.43)); hand=Vector((s*(width+.065),-.255,self.neck-.67))
            upper,lower,palm=[p+'_'+side for p in ('upper_arm','forearm','hand')]
            self.joint(upper,sh,elbow,'spine'); self.joint(lower,elbow,hand,upper); self.joint(palm,hand,hand+Vector((0,0,-.13)),lower)
            self.tube('continuous sleeve '+side,[sh,sh.lerp(elbow,.5),elbow,elbow.lerp(hand,.5),hand],[.09,.085,.07,.065,.053],'coat',upper,[{upper:1},{upper:.85,lower:.15},{upper:.5,lower:.5},{lower:1},{lower:.75,palm:.25}])
            if name=='pell':
                self.box('skeletal palm '+side,hand+Vector((0,-.012,-.035)),(.045,.026,.045),'skin',palm)
                for dx in (-.035,0,.035):
                    self.taper('distinct skeletal digits '+side,hand+Vector((dx,-.025,-.055)),hand+Vector((dx,-.055,-.145)),.014,.010,'skin',palm,5)
            else:
                self.blob('hand '+side,hand+Vector((0,-.025,-.06)),(.069,.07,.105),'skin',palm,8,4)
            self.hands[side]=hand

    @staticmethod
    def vertices(ob):
        return [list(ob.matrix_world@v.co) for v in ob.data.vertices]

    def mark(self,name,ob):
        self.anatomy[name]=self.vertices(ob)
        return ob

    prop_joint=Humanoid.prop_joint

    def export(self,source):
        Humanoid.export(self,source,preview_center=1.38)
        skin=bpy.data.objects[self.skin_name]; arm=skin.parent
        def evaluate():
            bpy.context.view_layer.update()
            obj=skin.evaluated_get(bpy.context.evaluated_depsgraph_get()); mesh=obj.to_mesh()
            pts=[v.co.copy() for v in mesh.vertices]; obj.to_mesh_clear(); return pts
        rest=evaluate(); path=self.out/'validation.json'; data=json.loads(path.read_text())
        for name in ('rope_loop','rope_knot'):
            if name not in arm.pose.bones: continue
            owned={v.index for v in skin.data.vertices if any(skin.vertex_groups[g.group].name==name and g.weight>0 for g in v.groups)}
            bone=arm.pose.bones[name]; bone.rotation_mode='XYZ'; bone.rotation_euler.x=.25; posed=evaluate()
            displacement=max((rest[i]-posed[i]).length for i in owned)
            unrelated=max((p-q).length for i,(p,q) in enumerate(zip(rest,posed)) if i not in owned)
            assert displacement>.002 and unrelated<1e-5
            bone.rotation_euler.x=0
            data['humanoid_deformation'][name]={'weighted_vertices':len(owned),'displacement_meters':displacement,'unrelated_displacement_meters':unrelated}
        data['final_anatomy']=self.anatomy
        path.write_text(json.dumps(data,indent=2)+'\n')


def maren():
    a=FinalHumanoid('maren','Maren',2.68,.32,dict(skin=(.56,.36,.25,1),coat=(.15,.30,.32,1),hair=(.63,.62,.54,1)))
    a.taper('tidekeeper oilskin coat',(0,.03,1.02),(0,.03,1.94),.33,.30,'coat','spine',10)
    a.box('worn rope belt',(0,-.19,1.46),(.29,.034,.038),'accent','spine')
    a.box('open weathered linen collar',(0,-.14,2.10),(.13,.09,.07),'linen','spine')
    a.blob('tied silver hair',(0,.17,2.47),(.16,.12,.13),'hair','head',8,4)
    p=a.prop_joint((0,-.055,-.035))
    # Each authored loop is attached to a real pivot; there is no free rope simulation.
    a.tube('hand held rope bight',[p+Vector((-.06,0,.055)),p+Vector((0,0,.085)),p+Vector((.07,0,.035))],[.025]*3,'linen','held_prop',sides=6)
    pivot=p+Vector((0,0,-.04)); a.joint('rope_loop',pivot,pivot+Vector((0,0,.12)),'held_prop')
    for inset in (0,.045):
        points=[p+Vector((.14*math.cos(i*math.tau/16),inset,-.18+.19*math.sin(i*math.tau/16))) for i in range(17)]
        a.tube('attached broad rope coil',points,[.024]*17,'linen','rope_loop',sides=6)
    knot=p+Vector((.075,-.015,.005)); a.joint('rope_knot',knot,knot+Vector((0,0,.10)),'held_prop')
    a.tube('bound inspection knot',[knot+Vector((-.04,0,-.035)),knot+Vector((.035,-.025,.015)),knot+Vector((-.035,-.035,.055)),knot+Vector((.025,.005,-.025))],[.028]*4,'accent','rope_knot',sides=6)
    return a


def pell():
    a=FinalHumanoid('pell','Pell',2.77,.255,dict(skin=(.80,.77,.63,1),coat=(.24,.23,.29,1),hair=(.34,.32,.29,1),trousers=(.31,.28,.25,1),accent=(.43,.35,.22,1)))
    a.taper('travelling coat skirt',(0,.045,1.03),(0,.045,1.91),.29,.25,'coat','spine',10)
    for s in (-1,1):
        a.box('turned travelling lapel',(s*.12,-.19,1.98),(.052,.033,.17),'accent','spine')
    a.box('bound pebble coat pocket',(.16,-.202,1.48),(.072,.028,.095),'accent','spine')
    p=a.prop_joint((0,-.055,-.035))
    a.blob('smooth held river pebble',p+Vector((0,-.02,.04)),(.074,.055,.060),'metal','held_prop',10,5)
    return a


def serein():
    a=FinalHumanoid('serein','Serein',2.79,.27,dict(skin=(.59,.39,.25,1),coat=(.32,.41,.22,1),hair=(.25,.13,.07,1),trousers=(.37,.26,.16,1),boots=(.14,.105,.065,1),accent=(.56,.35,.17,1),paper=(.60,.66,.26,1)))
    a.taper('short orchard tunic',(0,.025,1.18),(0,.025,1.95),.29,.26,'coat','spine',10)
    a.box('orchard woven sash',(0,-.19,1.53),(.24,.035,.055),'accent','spine')
    for side,s in [('left',1),('right',-1)]:
        a.mark(side+'_horn',a.tube('curved faun horn '+side,[(s*.115,.02,2.66),(s*.18,.045,2.83),(s*.195,.09,2.96),(s*.16,.12,3.00)],[.052,.047,.03,.008],'linen','head',sides=6))
        a.mark(side+'_pointed_ear',a.wedge('pointed faun ear '+side,[(s*.145,0,a.face_z-.055),(s*.35,.015,a.face_z+.10),(s*.225,-.075,a.face_z+.055),(s*.23,.06,a.face_z+.035)],[(0,1,2),(0,3,1),(1,3,2),(0,2,3)],'skin','head'))
    p=a.prop_joint((0,-.055,-.035))
    a.wedge('bound orchard leaf',[(p.x+dx,p.y+dy,p.z+dz) for dx,dy,dz in [(0,0,-.045),(-.09,0,.07),(-.025,0,.21),(.08,0,.10),(0,-.025,.08)]],[(0,1,4),(1,2,4),(2,3,4),(3,0,4)],'paper','held_prop')
    a.taper('leaf central stem',p+Vector((0,-.028,-.065)),p+Vector((-.025,-.028,.18)),.009,.004,'accent','held_prop',5)
    return a

BUILDERS={f.__name__:f for f in (maren,pell,serein)}
if __name__=='__main__':
    selected=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else BUILDERS
    for name in selected:
        BUILDERS[name]().export(__file__)
