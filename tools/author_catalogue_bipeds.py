"""Author nine original procedural biped pets in Blender.

Run Blender --background --python tools/author_catalogue_bipeds.py.
Optional arguments after -- are asset IDs to regenerate individually.
"""
from pathlib import Path
import math
import sys
sys.path.insert(0,str(Path(__file__).resolve().parent))
from catalogue_asset import Asset

PALETTE = {
    'body':(.45,.235,.13,1), 'light':(.70,.42,.22,1),
    'cream':(.86,.74,.52,1), 'dark':(.15,.12,.11,1),
    'ink':(.018,.026,.027,1), 'green':(.27,.40,.22,1),
    'leaf':(.43,.57,.28,1), 'accent':(.66,.30,.13,1),
    'blue':(.13,.22,.29,1), 'metal':(.58,.49,.32,1),
}


def base(name,label,hip=.63,head=1.28,palette=None):
    a = Asset(name,label,palette or PALETTE)
    a.joint('root',(0,0,0),(0,0,.15))
    a.joint('pelvis',(0,0,hip),(0,0,hip+.17),'root')
    a.joint('spine',(0,0,hip+.17),(0,0,head),'pelvis')
    a.joint('head',(0,0,head),(0,0,head+.25),'spine')
    return a


def leaf(a,name,start,end,width,color,bone):
    """One broad folded leaf plane, with a quiet darker underside."""
    x,y,z = start
    ex,ey,ez = end
    center = ((x+ex)/2,(y+ey)/2,(z+ez)/2)
    vertices = [start,(center[0]-width,center[1],center[2]),
                (center[0],center[1]-.07,center[2]+.07),
                (center[0]+width,center[1],center[2]),end]
    a.wedge(name,vertices,[(0,1,2),(0,2,3),(1,4,2),(2,4,3),(0,3,4,1)],color,bone)


def rivet():
    a=base('rivet','Rivet',head=1.35)
    a.blob('terracotta motor shell',(0,.02,1.0),(.49,.32,.46),'body','spine')
    a.box('central cream panel',(0,-.29,1.04),(.28,.055,.21),'cream','spine')
    a.box('panel inset',(0,-.354,1.01),(.11,.012,.035),'metal','spine')
    a.blob('rounded head casing',(0,0,1.58),(.43,.32,.29),'light','head')
    a.box('cream face panel',(0,-.295,1.58),(.31,.04,.16),'cream','head')
    a.eyes(1.62,-.344,.17,.043)
    a.box('quiet mouth',(0,-.345,1.51),(.08,.012,.013),'dark','head')
    a.taper('antenna mast',(.14,.04,1.80),(.22,.04,2.06),.025,.019,'metal','head')
    a.blob('antenna cap',(.22,.04,2.06),(.065,.065,.065),'accent','head',8,4)
    for sign in [-1,1]:
        x=sign*.50
        a.blob('shoulder rivet',(x,0,1.17),(.10,.13,.12),'metal','spine')
        a.taper('resting mechanical arm',(x,0,1.13),(x*1.10,-.03,.77),.08,.065,'body','spine')
        a.blob('mitten claw',(x*1.10,-.03,.73),(.105,.12,.10),'dark','spine')
    return a,a.biped(width=.25,radius=.10,feet_color='dark')


def paper_heron():
    p=dict(PALETTE,body=(.81,.78,.67,1),light=(.96,.90,.72,1),accent=(.30,.44,.43,1))
    a=base('paper-heron','Paper Heron',hip=.82,head=1.74,palette=p)
    # A closed folded hull and paired asymmetric crease planes, not a rounded bird.
    a.wedge('folded paper hull',[(-.31,-.36,1.03),(.31,-.36,1.03),(-.31,.39,1.04),(.31,.39,1.04),
             (0,-.19,1.43),(0,.54,1.27),(0,.02,.85)],
             [(0,4,1),(0,2,5,4),(1,4,5,3),(2,6,3,5),(0,6,2),(1,3,6),(0,1,6)],'body','spine')
    for s in [-1,1]:
        a.wedge('tucked paper wing',[(s*.30,-.24,1.29),(s*.34,.48,1.19),(s*.28,.20,.99),(s*.12,.02,1.32)],
                [(0,1,2),(0,3,1),(0,2,3),(1,3,2)],'accent','spine')
    a.tube('angular folded neck',[(0,-.23,1.23),(0,-.49,1.51),(0,-.29,1.78)],
           [.14,.115,.10],'light','head',sides=4)
    a.wedge('paper crown',[(-.14,-.48,1.74),(.14,-.48,1.74),(0,-.19,1.91),(0,-.17,1.68)],
            [(0,1,2),(0,3,1),(0,2,3),(1,3,2)],'light','head')
    a.taper('long folded beak',(0,-.43,1.78),(0,-.91,1.73),.09,.008,'accent','head',4)
    a.eyes(1.80,-.44,.11,.025)
    return a,a.biped(hip_z=.82,width=.17,knee_y=.09,radius=.037,foot_scale=(.10,.19,.035),color='dark',feet_color='dark')


def acorn():
    a=base('acorn-custodian','Acorn Custodian',head=1.25)
    a.blob('amber seed',(0,0,1.12),(.43,.33,.60),'light','spine',12,6)
    a.blob('broad acorn cap',(0,.01,1.64),(.55,.42,.24),'body','head',12,4)
    a.taper('cap stem',(0,.05,1.76),(.08,.08,1.98),.065,.04,'dark','head')
    a.taper('cap brim',(0,0,1.50),(0,0,1.59),.52,.53,'dark','head',12)
    for s in [-1,1]:
        a.taper('twig arm',(s*.37,0,1.25),(s*.55,-.05,.88),.055,.035,'body','spine',6)
        a.taper('twig fork',(s*.52,-.05,.94),(s*.66,-.08,.99),.032,.015,'body','spine',6)
    leaf(a,'single cap leaf',(.07,.04,1.85),(.44,.10,2.00),.12,'green','head')
    a.eyes(1.30,-.31,.16,.035)
    a.box('seed groove',(0,-.327,.94),(.015,.012,.18),'body','spine')
    return a,a.biped(width=.23,radius=.075,foot_scale=(.14,.21,.075),feet_color='body')


def puddle():
    p=dict(PALETTE,body=(.85,.68,.30,1),light=(.95,.83,.48,1),cream=(.90,.87,.72,1),accent=(.68,.29,.09,1))
    a=base('puddle','Puddle',hip=.43,head=1.19,palette=p)
    a.blob('broad duck body',(0,.05,.85),(.45,.46,.43),'cream','spine',12,6)
    a.blob('rounded breast',(0,-.29,.98),(.34,.23,.35),'light','spine')
    a.blob('short neck',(0,-.21,1.24),(.23,.24,.34),'light','head')
    a.blob('duck head',(0,-.28,1.51),(.29,.29,.28),'body','head',12,6)
    a.blob('broad flat bill',(0,-.57,1.43),(.25,.23,.065),'accent','head',8,4)
    a.eyes(1.57,-.47,.205,.034)
    for s in [-1,1]:
        a.blob('folded oval wing',(s*.38,.10,.95),(.12,.34,.24),'body','spine')
    a.wedge('upturned tail',[(-.19,.34,.88),(.19,.34,.88),(.12,.64,1.12),(-.12,.64,1.12),(0,.52,.83)],
            [(0,1,2,3),(0,4,1),(1,4,2),(2,4,3),(3,4,0)],'cream','spine')
    return a,a.biped(hip_z=.43,width=.23,knee_y=.06,radius=.055,foot_scale=(.20,.27,.06),color='accent',feet_color='accent')


def observatory():
    a=base('pocket-observatory','Pocket Observatory',hip=.65,head=1.17)
    a.blob('navy instrument pedestal',(0,.02,.97),(.28,.25,.29),'blue','spine')
    a.taper('telescope barrel',(0,.35,1.47),(0,-.48,1.63),.25,.34,'metal','head',12)
    a.taper('navy barrel sleeve',(0,.20,1.50),(0,-.24,1.58),.265,.31,'blue','head',12)
    a.taper('broad brass lens ring',(0,-.43,1.62),(0,-.53,1.64),.35,.35,'light','head',12)
    a.taper('dark circular lens',(0,-.537,1.64),(0,-.552,1.642),.29,.29,'ink','head',12)
    a.taper('blue lens glass',(0,-.555,1.643),(0,-.56,1.644),.23,.23,'blue','head',12)
    a.blob('single lens glint',(-.09,-.566,1.73),(.055,.012,.04),'cream','head',8,4)
    a.taper('eyepiece',(0,.31,1.48),(0,.57,1.43),.10,.08,'dark','head')
    for s in [-1,1]:
        a.blob('altitude trunnion',(s*.28,.03,1.28),(.11,.10,.12),'metal','spine')
    return a,a.biped(width=.24,radius=.075,color='metal',feet_color='blue')


def librarian():
    a=base('night-librarian','Night Librarian',hip=.52,head=1.0)
    # Whole book looks as one instrument; its head pivot is the book's honest tilt.
    a.box('cloth back cover',(0,.14,1.22),(.49,.065,.64),'blue','head')
    a.box('warm paper block',(.015,0,1.22),(.43,.10,.58),'cream','head')
    a.box('cloth front cover',(0,-.14,1.22),(.49,.055,.64),'blue','head')
    a.box('rounded spine',(-.46,0,1.22),(.07,.17,.64),'green','head')
    for z in [.78,1.63]:
        a.box('brass spine band',(-.47,-.01,z),(.075,.18,.035),'metal','head')
    a.box('quiet title plaque',(.02,-.20,1.35),(.21,.014,.14),'metal','head')
    a.box('plaque inset',(.02,-.216,1.35),(.15,.005,.09),'blue','head')
    a.wedge('bookmark ribbon',[(-.03,-.05,.65),(.07,-.05,.65),(.07,-.10,.37),(.02,-.10,.41),(-.03,-.10,.37)],
            [(0,1,2,3,4)],'accent','head')
    for z in [.89,1.06,1.48,1.62]:
        a.box('page edge group',(.452,0,z),(.008,.093,.012),'metal','head')
    return a,a.biped(hip_z=.52,width=.25,radius=.065,foot_scale=(.13,.21,.07),color='dark',feet_color='body')


def sprig():
    p=dict(PALETTE,body=(.66,.69,.58,1),light=(.86,.83,.67,1))
    a=base('sprig','Sprig',hip=.46,head=1.13,palette=p)
    a.taper('pale ceramic pot',(0,0,.57),(0,0,1.16),.29,.43,'body','spine',10)
    a.taper('thick pot lip',(0,0,1.13),(0,0,1.25),.46,.46,'light','spine',10)
    a.taper('soil surface',(0,0,1.246),(0,0,1.255),.38,.38,'dark','spine',10)
    # Leaf groups are rigid geometry attached to a botanical crown, never ears.
    for i in range(7):
        ang=i*math.tau/7
        end=(math.cos(ang)*.68,math.sin(ang)*.57,1.43+(i%2)*.15)
        leaf(a,'succulent folded outer leaf '+str(i),(0,0,1.24),end,.16,'green' if i%2 else 'leaf','head')
    for i in range(3):
        ang=i*math.tau/3
        leaf(a,'upright crown leaf '+str(i),(0,0,1.27),(math.cos(ang)*.23,math.sin(ang)*.23,1.94),.13,'leaf','head')
    return a,a.biped(hip_z=.46,width=.22,radius=.065,foot_scale=(.14,.21,.065),color='green',feet_color='dark')


def hearthling():
    a=base('hearthling','Hearthling',hip=.57,head=1.25)
    a.blob('charcoal upright trunk',(0,.03,.99),(.34,.28,.49),'dark','spine',12,6)
    a.blob('warm ember belly',(0,-.235,.99),(.24,.085,.35),'accent','spine')
    a.blob('broad salamander head',(0,-.09,1.47),(.43,.34,.23),'dark','head',12,6)
    a.blob('short blunt muzzle',(0,-.36,1.40),(.33,.15,.13),'body','head')
    a.eyes(1.55,-.33,.28,.037,color='cream')
    for s in [-1,1]:
        a.tube('small resting arm',[(s*.27,0,1.17),(s*.40,-.07,.98),(s*.39,-.14,.88)],
               [.09,.07,.055],'dark','spine')
        a.blob('resting hand',(s*.39,-.14,.88),(.09,.09,.05),'body','spine')
    a.joint('tail_base',(0,.18,.72),(0,.67,.49),'pelvis')
    a.joint('tail_tip',(0,.67,.49),(.22,1.15,.54),'tail_base')
    a.tube('curved salamander tail',[(0,.18,.72),(0,.43,.57),(0,.67,.49),(.11,.94,.48),(.22,1.15,.54)],
           [.20,.17,.13,.08,.025],'dark','tail_base',
           [{'tail_base':1},{'tail_base':.9,'tail_tip':.1},{'tail_base':.5,'tail_tip':.5},{'tail_tip':1},{'tail_tip':1}])
    for y,z in [(.13,1.31),(.20,1.12),(.26,.94)]:
        a.wedge('small ember dorsal plate',[(-.055,y,z),(.055,y,z),(0,y+.14,z+.17),(0,y+.19,z-.03)],
                [(0,1,2),(0,3,1),(0,2,3),(1,3,2)],'accent','spine')
    return a,a.biped(hip_z=.57,width=.23,radius=.10,foot_scale=(.17,.23,.08),color='dark',feet_color='body')


def crumb():
    a=base('crumb-golem','Crumb Golem',hip=.62,head=1.34)
    a.blob('uneven biscuit trunk',(0,0,1.03),(.43,.30,.43),'light','spine',7,4)
    a.blob('angular biscuit head',(-.035,0,1.56),(.34,.28,.29),'cream','head',7,4)
    a.eyes(1.62,-.25,.15,.033)
    for s,z in [(-1,1.15),(1,1.03)]:
        a.blob('mismatched stone arm',(s*.47,0,z),(.17,.16,.25),'body' if s<0 else 'cream','spine',6,4)
    for x,z,size in [(-.18,1.11,.065),(.20,.91,.055),(.07,1.29,.038),(-.14,1.74,.04),(.19,1.52,.045)]:
        a.blob('baked chip',(x,-.272,z),(size,.025,size*.75),'body','head' if z>1.4 else 'spine',6,3)
    a.wedge('biscuit brow notch',[(-.14,-.25,1.82),(.08,-.25,1.82),(-.02,-.25,1.73),(-.03,-.12,1.77)],
            [(0,1,2),(0,3,1),(0,2,3),(1,3,2)],'body','head')
    return a,a.biped(width=.25,radius=.12,foot_scale=(.20,.25,.10),color='light',feet_color='body')


BUILDERS = {'rivet':rivet,'paper-heron':paper_heron,'acorn-custodian':acorn,
            'puddle':puddle,'pocket-observatory':observatory,'night-librarian':librarian,
            'sprig':sprig,'hearthling':hearthling,'crumb-golem':crumb}


if __name__ == '__main__':
    selected = sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else list(BUILDERS)
    for name in selected:
        a, contacts = BUILDERS[name]()
        a.finish(__file__,contacts,arms=False,presentation={
            'pixels_per_meter':80,'camera_tilt_degrees':15.4,'yaw_bias_degrees':-35})
