"""Reproducible original wayfarer companions; run with Blender --background --python."""
from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parent))
from humanoid_asset import Humanoid
from mathutils import Vector

BASE={'skin':(.48,.28,.16,1),'coat':(.22,.32,.30,1),'trousers':(.12,.15,.17,1),'boots':(.07,.065,.06,1),'hair':(.16,.11,.085,1),'ink':(.015,.02,.025,1),'lip':(.23,.12,.10,1),'linen':(.78,.70,.52,1),'metal':(.55,.42,.23,1),'accent':(.49,.22,.12,1),'paper':(.88,.81,.63,1),'glass':(.22,.48,.40,1)}

def make(name,label,height,width,anatomy=None,**colors):
    return Humanoid(name,label,dict(BASE,**colors),height,width,**(anatomy or {}))

def iona():
    a=make('iona','Iona',2.67,.255,skin=(.56,.34,.20,1),coat=(.31,.43,.39,1),hair=(.13,.085,.055,1),accent=(.63,.35,.17,1))
    a.blob('short swept hair',(0,.11,2.49),(.18,.15,.17),'hair','head',8,4)
    a.taper('short field coat',(0,.025,1.10),(0,.025,1.78),.28,.255,'coat','spine',8)
    for s in (-1,1):
        a.box('field coat pocket',(s*.16,-.205,1.48),(.065,.035,.09),'accent','spine')
    a.box('linen neck scarf',(0,-.10,2.14),(.12,.10,.055),'linen','spine')
    p=a.prop_joint((-.005,-.075,.015))
    a.box('folded map board',p,(.13,.025,.17),'accent','held_prop')
    a.box('map fixed face',p+Vector((0,-.029,0)),(.115,.008,.155),'paper','held_prop')
    pivot=p+Vector((-.126,-.038,0))
    a.joint('prop_hinge',pivot,pivot+Vector((0,0,.12)),'held_prop')
    a.box('unfolding map panel',p+Vector((0,-.047,0)),(.126,.008,.165),'linen','prop_hinge')
    for dx,dz,sx in [(-.045,-.06,.06),(.02,0,.055),(.07,.07,.035)]:
        a.box('broad ink map route',p+Vector((dx,-.058,dz)),(sx,.004,.009),'ink','prop_hinge')
    a.blob('map destination',p+Vector((.075,-.064,.08)),(.018,.007,.018),'accent','prop_hinge',6,3)
    return a

def milo():
    a=make('milo','Milo',2.78,.30,skin=(.30,.17,.115,1),coat=(.23,.24,.37,1),hair=(.075,.060,.055,1),accent=(.54,.27,.12,1))
    a.taper('long split travel coat',(0,.04,1.00),(0,.04,1.99),.34,.285,'coat','spine',10)
    for s in (-1,1):
        a.box('coat lapel',(s*.09,-.22,2.02),(.045,.025,.19),'linen','spine')
    a.blob('coiled hair knot',(0,.19,2.57),(.13,.13,.13),'hair','head',8,4)
    a.box('coat belt',(0,-.20,1.55),(.26,.035,.045),'accent','spine')
    p=a.prop_joint((-.015,-.055,-.005))
    a.blob('small string instrument lower body',p,(.12,.055,.145),'accent','held_prop',8,5)
    a.blob('small string instrument shoulder',p+Vector((0,0,.10)),(.09,.05,.10),'accent','held_prop',8,4)
    a.box('instrument neck',p+Vector((0,0,.27)),(.032,.035,.16),'hair','held_prop')
    a.blob('instrument sound hole',p+Vector((0,-.058,.035)),(.034,.007,.041),'ink','held_prop',8,4)
    for dx in (-.018,0,.018):
        a.box('silent broad string',p+Vector((dx,-.067,.18)),(.003,.004,.22),'linen','held_prop')
    for s in (-1,1):
        a.box('tuning peg',p+Vector((s*.055,0,.38)),(.023,.021,.019),'metal','held_prop')
    return a

def orris():
    a=make('orris','Orris',2.18,.425,anatomy=dict(hip_ratio=.405,neck_ratio=.79,arm_scale=.78,limb_radius=.100,hand_scale=(.085,.08,.105)),
           skin=(.62,.40,.26,1),coat=(.35,.24,.17,1),hair=(.63,.60,.49,1),accent=(.20,.32,.32,1))
    a.blob('square grey beard',(0,-.14,a.face_z-.16),(.205,.11,.18),'hair','head',8,4)
    for s in (-1,1):
        a.box('spectacles upper',(s*.09,-.265,a.face_z+.067),(.074,.018,.015),'metal','head')
        a.box('spectacles lower',(s*.09,-.265,a.face_z+.003),(.074,.018,.015),'metal','head')
        for dx in (-.07,.07):
            a.box('square spectacles side',(s*.09+dx,-.265,a.face_z+.035),(.014,.018,.045),'metal','head')
    a.box('spectacles bridge',(0,-.268,a.face_z+.045),(.026,.018,.014),'metal','head')
    a.box('clockmaker broad waistcoat',(0,-.20,1.26),(.29,.04,.25),'accent','spine')
    for z in (1.16,1.31,1.46):
        a.blob('waistcoat brass button',(0,-.246,z),(.023,.018,.023),'metal','spine',6,3)
    p=a.prop_joint((-.02,-.075,.015))
    a.box('small carried clock',p,(.105,.045,.13),'accent','held_prop')
    a.blob('clock ivory dial',p+Vector((0,-.05,.02)),(.087,.009,.087),'paper','held_prop',10,4)
    a.box('clock hour hand',p+Vector((0,-.065,.043)),(.008,.005,.034),'ink','held_prop')
    pivot=p+Vector((.112,0,.015))
    a.joint('prop_hinge',pivot,pivot+Vector((.10,0,0)),'held_prop')
    a.taper('winding key axle',pivot-Vector((.01,0,0)),pivot+Vector((.06,0,0)),.022,.022,'metal','prop_hinge',6)
    a.box('winding key bow',pivot+Vector((.065,0,0)),(.018,.027,.05),'metal','prop_hinge')
    return a

def fen():
    a=make('fen','Fen',2.47,.225,anatomy=dict(hip_ratio=.48,neck_ratio=.82,arm_scale=.96,limb_radius=.062,hand_scale=(.057,.065,.145)),
           skin=(.36,.45,.22,1),coat=(.40,.27,.15,1),hair=(.13,.17,.095,1),accent=(.68,.43,.15,1))
    for s in (-1,1):
        a.wedge('goblin swept pointed ear',[(s*.13,0,a.face_z-.07),(s*.35,.025,a.face_z+.14),(s*.20,-.055,a.face_z+.09),(s*.22,.055,a.face_z+.035)],
                [(0,1,2),(0,3,1),(1,3,2),(0,2,3)],'skin','head')
    a.tube('short angular hair ridge',[(0,.015,2.35),(0,.015,2.49),(0,.08,2.51)],[.15,.09,.025],'hair','head',sides=6)
    a.box('tinker leather apron',(0,-.21,1.46),(.16,.025,.27),'accent','spine')
    a.box('apron broad patch',(.045,-.241,1.42),(.08,.012,.08),'coat','spine')
    p=a.prop_joint((0,-.065,-.01))
    a.box('lantern dark chamber',p,(.085,.065,.125),'ink','held_prop')
    a.box('lantern unlit inset',p+Vector((0,-.069,0)),(.063,.008,.095),'linen','held_prop')
    for dz in (-.13,.13):
        a.box('lantern brass rim',p+Vector((0,0,dz)),(.10,.075,.018),'metal','held_prop')
    a.tube('lantern rigid carry loop',[p+Vector((-.065,0,.13)),p+Vector((-.065,0,.22)),p+Vector((.065,0,.22)),p+Vector((.065,0,.13))],[.014]*4,'metal','held_prop',sides=5)
    pivot=p+Vector((-.09,-.079,0))
    a.joint('prop_hinge',pivot,pivot+Vector((0,0,.10)),'held_prop')
    a.box('hinged lantern shutter',p+Vector((0,-.083,0)),(.09,.011,.115),'coat','prop_hinge')
    a.box('shutter handle',p+Vector((.052,-.101,0)),(.014,.015,.038),'metal','prop_hinge')
    return a

def oona():
    a=make('oona','Oona',2.69,.315,skin=(.40,.245,.16,1),coat=(.60,.62,.55,1),hair=(.10,.07,.06,1),accent=(.21,.33,.40,1))
    # Open helmet framing leaves the actual skin face exposed; fixed lamp is a detail.
    a.blob('helmet back shell',(0,.13,a.face_z+.04),(.245,.18,.25),'accent','head',10,5)
    for s in (-1,1):
        a.blob('helmet cheek rim',(s*.205,-.02,a.face_z-.045),(.05,.10,.14),'linen','head',8,4)
    a.blob('helmet top arch',(0,-.02,a.face_z+.215),(.23,.16,.075),'linen','head',10,4)
    a.box('fixed helmet lamp housing',(0,-.18,a.face_z+.22),(.075,.07,.047),'metal','head')
    a.box('fixed helmet lamp face',(0,-.253,a.face_z+.22),(.054,.007,.032),'paper','head')
    a.taper('open helmet neck ring',(0,.01,2.13),(0,.01,2.20),.18,.19,'accent','spine',10)
    a.box('geology suit chest panel',(0,-.22,1.80),(.17,.035,.18),'accent','spine')
    for s in (-1,1):
        a.box('suit cuff',(s*(a.shoulder_half_span+.025),-.20,1.62),(.075,.075,.045),'accent','forearm_left' if s>0 else 'forearm_right')
    p=a.prop_joint((-.005,-.05,.015))
    a.wedge('held faceted mineral',[(p.x+dx,p.y+dy,p.z+dz) for dx,dy,dz in [(-.075,-.01,-.06),(.07,-.02,-.06),(.085,.025,.035),(0,.02,.15),(-.065,.03,.075),(0,-.07,.06)]],
            [(0,1,5),(1,2,5),(2,3,5),(3,4,5),(4,0,5),(0,4,3,2,1)],'metal','held_prop')
    return a

def quillan():
    a=make('quillan','Quillan',2.96,.215,anatomy=dict(hip_ratio=.49,neck_ratio=.83,arm_scale=1.10,limb_radius=.060,hand_scale=(.055,.065,.145)),
           skin=(.48,.50,.62,1),coat=(.27,.30,.44,1),hair=(.34,.35,.48,1),accent=(.58,.53,.65,1),lip=(.25,.26,.36,1))
    a.taper('slender visitor tunic',(0,.02,1.19),(0,.02,2.16),.235,.215,'coat','spine',8)
    a.box('asymmetric tunic stripe',(.08,-.22,1.89),(.026,.015,.29),'accent','spine')
    for side,s in [('left',1),('right',-1)]:
        pivot=Vector((s*.145,.005,a.face_z+.025))
        joint='ear_fin_'+side
        a.joint(joint,pivot,pivot+Vector((0,0,.14)),'head')
        a.wedge('folded articulated ear fin '+side,[(pivot.x+s*dx,pivot.y+dy,pivot.z+dz) for dx,dy,dz in [(0,0,-.055),(.14,.02,.13),(.075,.085,.20),(.015,.065,.07)]],
                [(0,1,2),(0,2,3),(0,3,1),(1,3,2)],'skin',joint)
        a.wedge('ear fin inset '+side,[(pivot.x+s*dx,pivot.y+dy,pivot.z+dz) for dx,dy,dz in [(.02,-.005,.015),(.11,.015,.13),(.062,.045,.15)]],[(0,1,2)],'accent',joint)
    return a

BUILDERS={f.__name__:f for f in (iona,milo,orris,fen,oona,quillan)}
if __name__=='__main__':
    selected=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else BUILDERS
    for name in selected:
        a=BUILDERS[name]()
        a.export(__file__,preview_center=1.4)
