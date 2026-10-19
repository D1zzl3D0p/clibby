"""Author the six approved original human and near-human companions in Blender."""
from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parent))
from humanoid_asset import Humanoid
from mathutils import Vector
BASE={'skin':(.48,.28,.16,1),'coat':(.22,.32,.30,1),'trousers':(.12,.15,.17,1),'boots':(.07,.065,.06,1),'hair':(.16,.11,.085,1),'ink':(.015,.02,.025,1),'lip':(.23,.12,.10,1),'linen':(.78,.70,.52,1),'metal':(.55,.42,.23,1),'accent':(.49,.22,.12,1),'paper':(.88,.81,.63,1),'glass':(.22,.48,.40,1)}
def make(name,label,height,width,**colors):
    return Humanoid(name,label,dict(BASE,**colors),height,width)
def book(a):
    p=a.prop_joint((-.015,-.065,-.04))
    a.box('held cloth book',p,(.145,.055,.19),'accent','held_prop')
    a.box('book pages',p+Vector((0,-.06,0)),(.125,.016,.17),'paper','held_prop')
    pivot=p+Vector((-.13,-.08,0))
    a.joint('prop_hinge',pivot,pivot+Vector((0,0,.12)),'held_prop')
    a.box('hinged page',p+Vector((0,-.084,0)),(.13,.009,.17),'linen','prop_hinge')
def nell():
    a=make('nell','Nell',2.60,.34,skin=(.30,.16,.105,1),hair=(.63,.64,.58,1),coat=(.28,.35,.43,1),accent=(.38,.16,.14,1))
    a.blob('swept silver bun',(0,.18,2.43),(.14,.13,.14),'hair','head')
    for s in [-1,1]:
        a.box('glasses upper',(s*.08,-.253,a.face_z+.062),(.067,.020,.020),'metal','head')
        a.box('glasses lower',(s*.08,-.253,a.face_z+.005),(.067,.020,.018),'metal','head')
        for dx in [-.064,.064]: a.box('glasses side',(s*.08+dx,-.253,a.face_z+.032),(.018,.020,.036),'metal','head')
    a.box('glasses bridge',(0,-.255,a.face_z+.04),(.028,.018,.015),'metal','head')
    for x in [-.07,.07]: a.box('cardigan facing',(x,-.20,1.75),(.032,.025,.29),'linen','spine')
    book(a)
    return a
def alden():
    a=make('alden','Alden',2.76,.42,skin=(.52,.33,.22,1),hair=(.43,.42,.36,1),coat=(.30,.34,.35,1),accent=(.37,.23,.16,1))
    a.blob('short grey beard',(0,-.14,a.face_z-.12),(.15,.11,.13),'hair','head')
    a.box('breast plate',(0,-.19,1.84),(.27,.065,.25),'metal','spine')
    for s in [-1,1]: a.blob('armour shoulder',(s*a.shoulder_half_span,.005,a.neck-.14),(.125,.20,.10),'coat','upper_arm_left' if s>0 else 'upper_arm_right',8,4)
    a.wedge('short travel cloak',[(-.40,.17,2.13),(.40,.17,2.13),(-.44,.29,1.40),(.44,.29,1.40),(0,.39,1.68)],[(0,1,4),(0,4,2),(1,3,4),(2,4,3)],'accent','spine')
    a.joint('fastening',(.12,-.265,2.03),(.12,-.265,2.13),'spine')
    a.blob('coat fastening',(.12,-.267,2.03),(.055,.035,.055),'linen','fastening',8,4)
    return a
def saffron():
    a=make('saffron','Saffron',2.82,.275,skin=(.67,.43,.29,1),hair=(.23,.105,.065,1),coat=(.27,.18,.32,1),accent=(.55,.29,.13,1))
    a.blob('swept hair',(0,.12,2.61),(.20,.18,.22),'hair','head')
    a.taper('wide crooked hat brim',(0,0,2.79),(.035,0,2.85),.39,.36,'coat','head',10)
    a.tube('crooked hat crown',[(0,0,2.83),(.05,.015,3.02),(.18,.02,3.12),(.28,.02,3.10)],[.23,.14,.07,.005],'coat','head',sides=8)
    a.taper('long layered coat',(0,.02,1.03),(0,.02,1.75),.31,.28,'coat','spine',10)
    a.box('warm coat facing',(0,-.226,1.73),(.06,.02,.34),'accent','spine')
    p=a.prop_joint((0,-.045,.015))
    a.taper('held vial',p-Vector((0,0,.09)),p+Vector((0,0,.08)),.06,.06,'glass','held_prop',8)
    a.taper('vial neck',p+Vector((0,0,.08)),p+Vector((0,0,.16)),.03,.03,'linen','held_prop',8)
    a.box('vial cork',p+Vector((0,0,.17)),(.04,.04,.025),'accent','held_prop')
    return a
def tess():
    a=make('tess','Tess',2.43,.365,skin=(.28,.135,.075,1),hair=(.07,.045,.032,1),coat=(.56,.37,.16,1),accent=(.25,.29,.31,1))
    for x,y in [(-.12,-.02),(0,-.05),(.12,-.02),(-.08,.10),(.08,.10)]: a.blob('cropped curl',(x,y,2.34),(.085,.085,.08),'hair','head',7,4)
    a.box('work apron',(0,-.22,1.46),(.245,.035,.36),'accent','spine')
    a.box('apron pocket',(.06,-.263,1.42),(.11,.012,.09),'coat','spine')
    p=a.prop_joint((-.01,-.05,0))
    a.blob('held mechanism',p,(.115,.06,.105),'metal','held_prop',8,4)
    a.taper('mechanism axle',p+Vector((0,-.05,0)),p+Vector((0,-.12,0)),.025,.025,'ink','held_prop',6)
    for dx,dz in [(-.08,-.07),(.08,-.07),(-.08,.07),(.08,.07)]: a.box('gear tooth',p+Vector((dx,0,dz)),(.035,.05,.035),'metal','held_prop')
    return a
def rowan():
    a=make('rowan','Rowan',2.77,.26,skin=(.48,.45,.28,1),hair=(.20,.23,.12,1),coat=(.32,.39,.24,1),accent=(.51,.37,.18,1))
    for s in [-1,1]:
        a.wedge('long pointed ear',[(s*.15,0,a.face_z-.06),(s*.36,.025,a.face_z+.17),(s*.19,-.025,a.face_z+.09)],[(0,1,2)],'skin','head')
    a.blob('tied back hair',(0,.19,2.55),(.13,.14,.18),'hair','head')
    a.taper('short mantle',(0,.02,1.92),(0,.02,2.17),.37,.22,'accent','spine',8)
    p=a.prop_joint((-.015,-.04,.015))
    a.blob('held seed',p,(.07,.05,.11),'accent','held_prop',8,4)
    a.taper('seed stem',p+Vector((0,0,.09)),p+Vector((.03,0,.15)),.014,.008,'hair','held_prop',5)
    return a
def vesper():
    a=make('vesper','Vesper',2.91,.26,skin=(.76,.72,.66,1),hair=(.065,.065,.085,1),coat=(.15,.14,.22,1),accent=(.39,.13,.18,1))
    a.blob('long swept dark hair',(0,.135,2.62),(.19,.14,.28),'hair','head')
    a.taper('long formal coat',(0,.035,1.11),(0,.035,2.05),.29,.25,'coat','spine',10)
    for s in [-1,1]:
        a.wedge('high collar',[(s*.07,-.06,2.37),(s*.23,.07,2.47),(s*.23,.16,2.17),(s*.10,-.09,2.15)],[(0,1,2,3)],'accent','spine')
    a.box('linen cravat',(0,-.225,2.14),(.055,.02,.13),'linen','spine')
    p=a.prop_joint((-.02,-.065,.015))
    a.blob('held pocket watch',p,(.09,.035,.11),'metal','held_prop',10,5)
    a.blob('watch dial',p+Vector((0,-.035,0)),(.073,.008,.088),'paper','held_prop',10,4)
    a.box('watch hand',p+Vector((0,-.047,.025)),(.008,.006,.04),'ink','held_prop')
    pivot=p+Vector((-.087,0,0))
    a.joint('prop_hinge',pivot,pivot+Vector((0,0,.12)),'held_prop')
    a.blob('hinged watch cover',p+Vector((0,-.055,0)),(.09,.012,.11),'metal','prop_hinge',10,4)
    return a
BUILDERS={f.__name__:f for f in (nell,alden,saffron,tess,rowan,vesper)}
if __name__=='__main__':
    selected=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else BUILDERS
    for name in selected:
        a=BUILDERS[name]()
        a.export(__file__)
