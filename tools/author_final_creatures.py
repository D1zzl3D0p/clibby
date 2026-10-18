"""Original final creature recipes. Run in Blender --background --python this_file."""
import sys
import json
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
import bpy
from mathutils import Vector
from catalogue_asset import Asset

PALETTE=dict(body=(.23,.35,.46,1),light=(.49,.63,.69,1),dark=(.10,.17,.23,1),
             ink=(.035,.065,.08,1),warm=(.86,.62,.34,1),fin=(.32,.58,.62,1),
             pale=(.76,.84,.81,1),scarf=(.46,.29,.32,1),pattern=(.89,.55,.31,1))
PRESENTATION=dict(pixels_per_meter=80,camera_tilt_degrees=15.4,camera_target_bias_pixels=-10.8,
                  yaw_bias_degrees=-25,pixel_width=96,pixel_height=96)

def foundation(name,label,hip=.62,head=1.13):
    a=Asset(name,label,PALETTE)
    a.joint('root',(0,0,0),(0,0,.15))
    a.joint('pelvis',(0,0,hip),(0,0,hip+.15),'root')
    a.joint('spine',(0,0,hip+.15),(0,0,head),'pelvis')
    a.joint('head',(0,0,head),(0,0,head+.16),'spine')
    return a

def upright_joint(a,name,point,parent):
    a.joint(name,point,(point[0],point[1],point[2]+.12),parent)

def supports(a,width,hip,radius,color,hem=False):
    contacts={}
    for side,sign in [('left',1),('right',-1)]:
        x=width*sign
        h=(x,0,hip); k=(x,-.08,(hip+.12)/2); f=(x,-.08,.10)
        t,s,p=['%s_%s'%(prefix,side) for prefix in ('thigh','shin','foot')]
        a.joint(t,h,k,'pelvis'); a.joint(s,k,f,t); a.joint(p,f,(x,-.21,.08),s)
        points=[h,tuple((u+v)/2 for u,v in zip(h,k)),k,tuple((u+v)/2 for u,v in zip(k,f)),f,(x,-.10,.06)]
        a.tube('continuous leg '+side,points,[radius,radius*.9,radius*.8,radius*.7,radius*.7,radius*.65],color,t,
               [{t:1},{t:.9,s:.1},{t:.5,s:.5},{s:1},{s:.5,p:.5},{p:1}])
        if hem:
            a.wedge('weighted trailing hem '+side,[(x-.105,-.17,0),(x+.105,-.17,0),(x+.08,.06,.13),(x-.08,.06,.13),
                    (x-.11,-.12,.31),(x+.11,-.12,.31),(x,-.17,0)],[(0,6,5,4),(6,1,5),(1,2,5),(2,3,4,5),(3,0,4),(0,3,2,1)],color,p)
            contacts[side]=[x,-.17,0]
        else:
            a.blob('ventral support pad '+side,(x,-.10,.045),(.095,.12,.045),color,p,10,6)
            contacts[side]=[x,-.10,0]
    return contacts

def tarn():
    a=foundation('tarn','Tarn',.55,.94)
    a.blob('angular ribcage',(0,.10,.68),(.27,.43,.28),'body','spine',8,4)
    a.blob('narrow breast',(0,-.29,.69),(.20,.24,.25),'light','spine',8,4)
    a.tube('rising neck',[(0,-.28,.71),(0,-.37,.90),(0,-.42,1.10)],[.16,.13,.12],'body','head')
    a.blob('long dragon skull',(0,-.49,1.10),(.17,.26,.13),'body','head',8,4)
    a.box('angular muzzle',(0,-.69,1.045),(.125,.16,.065),'light','head')
    for side,sgn in [('left',1),('right',-1)]:
        a.taper('swept horn '+side,(sgn*.12,-.39,1.18),(sgn*.19,-.20,1.38),.055,.008,'warm','head',5)
        a.blob('eye '+side,(sgn*.144,-.59,1.15),(.025,.045,.025),'warm','head',8,4)
    contacts={}; chains={}
    for role in ('foreLeft','foreRight','hindLeft','hindRight'):
        fore=role.startswith('fore'); x=.22 if role.endswith('Left') else -.22
        y=-.26 if fore else .40; z=.57 if fore else .52
        h=(x,y,z); k=(x,y+(.09 if fore else -.11),.29); f=(x,y,.105)
        t,s,p=['%s_%s'%(prefix,role) for prefix in ('thigh','shin','foot')]
        a.joint(t,h,k,'spine' if fore else 'pelvis'); a.joint(s,k,f,t); a.joint(p,f,(x,y-.18,.07),s)
        a.tube('continuous leg '+role,[h,Vector(h).lerp(Vector(k),.5),k,Vector(k).lerp(Vector(f),.5),f],
               [.095,.09,.068,.055,.048],'body',t,[{t:1},{t:.9,s:.1},{t:.5,s:.5},{s:1},{s:.5,p:.5}])
        a.blob('clawed ground pad '+role,(x,y-.05,.055),(.085,.14,.055),'dark',p,10,6)
        for dx in (-.045,0,.045):
            a.taper('claw '+role,(x+dx,y-.13,.045),(x+dx,y-.22,.025),.022,.004,'warm',p,5)
        contacts[role]=[x,y-.05,0]
        chains[role]=dict(hip=t,knee=s,ankle=p,bend=-1 if fore else 1)
    for side,sgn in [('left',1),('right',-1)]:
        fold='wing_fold_'+side; tip='wing_tip_'+side
        upright_joint(a,fold,(sgn*.18,.06,.94),'spine')
        upright_joint(a,tip,(sgn*.36,.18,1.03),fold)
        a.tube('folded wing spar '+side,[(sgn*.18,.06,.94),(sgn*.36,.18,1.03),(sgn*.39,.55,.94)],
               [.045,.036,.015],'light',fold,[{fold:1},{fold:.5,tip:.5},{tip:1}],6)
        a.wedge('proximal wing membrane '+side,[(sgn*.18,.06,.94),(sgn*.36,.18,1.03),(sgn*.36,.18,.73)],[(0,1,2),(2,1,0)],'fin',fold)
        a.wedge('distal scalloped wing membrane '+side,[(sgn*.36,.18,1.03),(sgn*.39,.55,.94),(sgn*.30,.69,.67),(sgn*.36,.18,.73)],[(0,1,2,3),(3,2,1,0)],'body',tip)
    upright_joint(a,'tail_base',(0,.39,.68),'pelvis'); upright_joint(a,'tail_tip',(0,.78,.54),'tail_base')
    a.tube('jointed dragon tail',[(0,.39,.68),(0,.63,.59),(0,.78,.54),(0,1.0,.61)],
           [.13,.10,.065,.008],'body','tail_base',[{'tail_base':1},{'tail_base':1},{'tail_base':.5,'tail_tip':.5},{'tail_tip':1}],8)
    return a,contacts,chains

def nacre():
    a=foundation('nacre','Nacre',.55,1.03)
    # Broad hovering mantle remains honestly supported by two real ventral arms.
    a.blob('broad cuttle mantle',(0,.04,.97),(.43,.30,.39),'pale','head',12,6)
    a.blob('flattened cuttle head',(0,-.23,.76),(.32,.24,.16),'body','head',10,4)
    for side,sgn in [('left',1),('right',-1)]:
        fin='fin_'+side; gather='gather_arm_'+side; pattern='pattern_'+side
        upright_joint(a,fin,(sgn*.34,.06,1.00),'head')
        a.wedge('wide rippling mantle fin '+side,[(sgn*.30,-.19,.93),(sgn*.59,-.12,1.01),(sgn*.62,.15,1.06),
                (sgn*.46,.37,1.10),(sgn*.25,.25,1.13)],[(0,1,2,3,4),(4,3,2,1,0)],'fin',fin)
        upright_joint(a,gather,(sgn*.24,-.28,.75),'head')
        for offset in (0,.07):
            a.tube('free curling arm '+side+str(offset),[(sgn*(.24+offset),-.28,.75),(sgn*(.36+offset),-.36,.55),
                   (sgn*(.47+offset),-.40,.43),(sgn*(.40+offset),-.47,.47)], [.055,.045,.033,.015],'light',gather,sides=7)
        upright_joint(a,pattern,(sgn*.27,-.025,1.10),'head')
        # Embedded at rest; ±.18m model-X translates the actual coloured patch outside.
        a.box('concealed amber mantle pattern '+side,(sgn*.27,-.025,1.10),(.012,.045,.070),'pattern',pattern)
        a.blob('cuttle eye '+side,(sgn*.29,-.38,.80),(.065,.035,.055),'warm','head',8,4)
        a.box('horizontal eye slit '+side,(sgn*.29,-.412,.80),(.045,.009,.012),'ink','head')
    return a,supports(a,.15,.55,.065,'body'),None

def windle():
    a=foundation('windle','Windle',.64,1.29)
    # Open tapered side panels leave negative space between two actual hem supports.
    a.taper('tapered apparition torso',(0,0,.50),(0,0,1.15),.21,.15,'pale','spine',7)
    a.blob('quiet featureless head',(0,-.015,1.36),(.125,.12,.18),'pale','head',9,5)
    a.box('shadow face opening',(0,-.121,1.36),(.08,.012,.06),'body','head')
    for sgn in (-1,1):
        a.box('soft eye notch',(sgn*.034,-.138,1.375),(.012,.01,.009),'light','head')
    for side,sgn in [('left',1),('right',-1)]:
        wisp='wisp_'+side
        upright_joint(a,wisp,(sgn*.13,0,1.07),'spine')
        a.wedge('layered side wisp '+side,[(sgn*.12,-.035,1.10),(sgn*.28,-.015,1.02),(sgn*.43,-.025,.72),
                (sgn*.32,-.035,.82),(sgn*.27,-.04,.64),(sgn*.19,-.05,.89)],[(0,1,2,3,4,5),(5,4,3,2,1,0)],'light',wisp)
        a.wedge('outer trailing veil '+side,[(sgn*.12,.04,.84),(sgn*.27,.06,.72),(sgn*.31,.08,.30),
                (sgn*.22,.08,.39),(sgn*.19,.05,.23)],[(0,1,2,3,4),(4,3,2,1,0)],'pale','spine')
    a.blob('scarf collar',(0,-.01,1.17),(.185,.15,.055),'scarf','head',10,4)
    upright_joint(a,'scarf_end',(-.13,.04,1.18),'head')
    a.wedge('weighted scarf ribbon',[(-.13,.04,1.18),(-.25,.10,1.11),(-.29,.16,.76),(-.18,.14,.81),(-.15,.10,1.06)],
            [(0,1,2,3,4),(4,3,2,1,0)],'scarf','scarf_end')
    return a,supports(a,.115,.64,.07,'pale',hem=True),None

APPENDAGES=dict(tarn=['wing_fold_left','wing_fold_right','wing_tip_left','wing_tip_right','tail_base','tail_tip'],
                nacre=['gather_arm_left','gather_arm_right','fin_left','fin_right','pattern_left','pattern_right'],
                windle=['wisp_left','wisp_right','scarf_end'])

def deformation(a):
    skin=bpy.data.objects[a.skin_name]; arm=skin.parent
    def evaluated():
        bpy.context.view_layer.update()
        obj=skin.evaluated_get(bpy.context.evaluated_depsgraph_get()); mesh=obj.to_mesh()
        points=[v.co.copy() for v in mesh.vertices]; obj.to_mesh_clear(); return points
    rest=evaluated(); proofs={}
    for name in APPENDAGES[a.id]:
        descendants={name}
        for bone in arm.data.bones:
            if any(parent.name==name for parent in bone.parent_recursive): descendants.add(bone.name)
        owned={v.index for v in skin.data.vertices if any(skin.vertex_groups[g.group].name in descendants and g.weight>0 for g in v.groups)}
        direct={v.index for v in skin.data.vertices if any(skin.vertex_groups[g.group].name==name and g.weight>.99 for g in v.groups)}
        assert direct,name
        bone=arm.pose.bones[name]; bone.rotation_mode='QUATERNION'
        if name.startswith('pattern_'):
            delta=Vector((.18 if name.endswith('left') else -.18,0,0))
            bone.location=bone.bone.matrix_local.to_3x3().inverted()@delta
        else:
            from mathutils import Quaternion
            axis=Vector((0,1,0)) if a.id=='windle' else Vector((0,0,1))
            local=bone.bone.matrix_local.to_3x3().inverted()@axis
            bone.rotation_quaternion=Quaternion(local,.40)
        posed=evaluated()
        moved=max((rest[i]-posed[i]).length for i in direct)
        unrelated=max((p-q).length for i,(p,q) in enumerate(zip(rest,posed)) if i not in owned)
        assert moved>.005,(name,moved)
        assert unrelated<1e-5,(name,unrelated)
        proofs[name]=dict(weighted_vertices=len(direct),displacement_meters=moved,unrelated_displacement_meters=unrelated,
                          moving_hull_blender_minimum=[min(posed[i][axis] for i in owned) for axis in range(3)],
                          moving_hull_blender_maximum=[max(posed[i][axis] for i in owned) for axis in range(3)])
        bone.location=(0,0,0); bone.rotation_quaternion=(1,0,0,0)
        assert max((p-q).length for p,q in zip(rest,evaluated()))<1e-5
    path=a.out/'validation.json'; val=json.loads(path.read_text()); val['creature_deformation']=proofs
    val['real_contact_geometry']='four clawed pads' if a.id=='tarn' else 'two ventral support arms' if a.id=='nacre' else 'two weighted robe hem supports'
    path.write_text(json.dumps(val,indent=2)+'\n')

if __name__=='__main__':
    names=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else ['tarn','nacre','windle']
    for name in names:
        a,contacts,chains=globals()[name]()
        a.finish(__file__,contacts,legs=chains,gait='quadrupedWalk' if chains else None,arms=False,presentation=PRESENTATION)
        deformation(a)
