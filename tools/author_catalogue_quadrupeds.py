"""Original species-authored pixel 3D companions; Blender is authoring-only.

Run Blender --background --python tools/author_catalogue_quadrupeds.py.
All four use the real continuous four-contact quadruped skin contract.
No hop, sit, flop or neck-retraction animations are baked or claimed.
"""
from pathlib import Path
import math
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from catalogue_asset import Asset


def p(point):
    """Author x-forward/Z-up shapes in the same space as selected woodland B."""
    return (point[1], -point[0], point[2])


def ell(a, name, position, scale, color, bone, segments=12, rings=6):
    return a.blob(name, p(position), (scale[1], scale[0], scale[2]), color, bone,
                  segments=segments, rings=rings)


def tube(a, name, points, radii, color, bone, weights=None, sides=8):
    return a.tube(name, [p(point) for point in points], radii, color, bone,
                  weights=weights, sides=sides)


def foundation(a, pelvis, shoulder, neck, head):
    a.joint('root', (0, 0, 0), (0, 0, .15))
    a.joint('pelvis', p(pelvis), p(shoulder), 'root')
    a.joint('spine', p(shoulder), p(neck), 'pelvis')
    a.joint('head', p(neck), p(head), 'spine')


def eyes(a, x, depth, z, size=.055):
    for side, y in [('left', -depth), ('right', depth)]:
        a.joint('eye_' + side, p((x, y, z)), p((x, y, z + .08)), 'head')
        ell(a, 'small eye ' + side, (x, y, z), (size, .035, size * .85), 'ink', 'eye_' + side, 8, 4)


def legs(a, fore, hind, depth, widths, paw_scale, color, feet_color):
    """Continuous ring-weighted limbs, with exact ground-level sole contacts."""
    contacts, chains = {}, {}
    for role in ('foreLeft', 'foreRight', 'hindLeft', 'hindRight'):
        is_fore = role.startswith('fore')
        x, hip_z, knee_offset = fore if is_fore else hind
        y = -depth if role.endswith('Left') else depth
        hip = (x, y, hip_z)
        knee = (x + knee_offset, y, hip_z * .53)
        ankle = (x - .025, y, paw_scale[2] * 1.7)
        foot = (x + .06, y, paw_scale[2])
        thigh, shin, paw = ('thigh_' + role, 'shin_' + role, 'foot_' + role)
        a.joint(thigh, p(hip), p(knee), 'spine' if is_fore else 'pelvis')
        a.joint(shin, p(knee), p(ankle), thigh)
        a.joint(paw, p(ankle), p((x + .22, y, paw_scale[2])), shin)
        mix = lambda u, v: tuple((s + t) * .5 for s, t in zip(u, v))
        points = [hip, mix(hip, knee), knee, mix(knee, ankle), ankle, foot]
        upper = widths[0] if is_fore else widths[1]
        weights = [{thigh: 1}, {thigh: .9, shin: .1}, {thigh: .5, shin: .5},
                   {shin: 1}, {shin: .5, paw: .5}, {paw: 1}]
        tube(a, 'continuous leg ' + role, points,
             [upper, upper * .85, upper * .65, upper * .5, upper * .43, min(upper * .5, paw_scale[2] * .9)],
             color, thigh, weights)
        ell(a, 'ground paw ' + role, foot, paw_scale, feet_color, paw)
        contacts[role] = list(p((foot[0], y, 0)))
        chains[role] = {'hip': thigh, 'knee': shin, 'ankle': paw, 'bend': -1 if is_fore else 1}
    return contacts, chains


def finish(a, contacts, chains):
    a.finish(Path(__file__), contacts, legs=chains, gait='quadrupedWalk',
             presentation={'pixels_per_meter': 80, 'camera_tilt_degrees': 15.4,
                           'camera_target_bias_pixels': -10.8, 'yaw_bias_degrees': -35,
                           'pixel_width': 96, 'pixel_height': 96})


def pip():
    a = Asset('pip', 'Pip', {
        'fur': (.60, .36, .18, 1), 'cream': (.91, .78, .56, 1),
        'ear': (.30, .15, .08, 1), 'ink': (.035, .025, .019, 1),
        'chest': (.79, .57, .32, 1), 'nose': (.12, .08, .06, 1),
        'patch': (.43, .24, .12, 1), 'light': (.74, .48, .24, 1)}, 'PipSkin')
    foundation(a, (-.48, 0, .94), (.30, 0, 1.02), (.79, 0, 1.20), (1.02, 0, 1.48))
    ell(a, 'long compact hound body', (-.08, 0, .96), (.92, .38, .37), 'fur', 'spine')
    ell(a, 'shoulder', (.48, 0, 1.02), (.38, .40, .43), 'cream', 'spine')
    ell(a, 'haunch', (-.67, 0, .93), (.36, .37, .40), 'fur', 'pelvis')
    ell(a, 'neck', (.74, 0, 1.18), (.27, .31, .37), 'cream', 'head')
    ell(a, 'hound skull', (.96, 0, 1.43), (.33, .33, .31), 'fur', 'head')
    ell(a, 'forward broad dog muzzle', (1.25, 0, 1.32), (.32, .255, .18), 'cream', 'head')
    ell(a, 'wide charcoal nose', (1.50, 0, 1.37), (.09, .18, .09), 'nose', 'head')
    ell(a, 'subtle brow', (1.08, 0, 1.62), (.25, .30, .1), 'light', 'head')
    eyes(a, 1.12, .31, 1.49)
    for side, y in [('left', -.30), ('right', .30)]:
        a.joint('ear_' + side, p((.79, y, 1.58)), p((.61, y * 1.4, 1.05)), 'head')
        ell(a, 'pendant long ear ' + side, (.69, y * 1.32, 1.30), (.22, .13, .39), 'ear', 'ear_' + side)
    a.joint('tail_base', p((-.88, 0, 1.05)), p((-1.31, 0, 1.18)), 'pelvis')
    a.joint('tail_tip', p((-1.31, 0, 1.18)), p((-1.61, .05, 1.43)), 'tail_base')
    tube(a, 'relaxed tapering dog tail', [(-.88, 0, 1.05), (-1.13, 0, 1.08), (-1.4, .03, 1.25), (-1.61, .05, 1.43)],
         [.14, .13, .095, .035], 'fur', 'tail_base',
         [{'tail_base': 1}, {'tail_base': .7, 'tail_tip': .3}, {'tail_tip': 1}, {'tail_tip': 1}])
    finish(a, *legs(a, (.59, .96, -.13), (-.66, .91, .18), .29, (.16, .20), (.20, .145, .10), 'fur', 'cream'))


def biscuit():
    a = Asset('biscuit', 'Biscuit', {
        'fur': (.49, .30, .16, 1), 'light': (.65, .43, .23, 1),
        'muzzle': (.58, .38, .22, 1), 'ear': (.34, .19, .10, 1),
        'ink': (.045, .033, .025, 1), 'foot': (.31, .20, .12, 1),
        'cream': (.74, .55, .34, 1), 'shadow': (.41, .25, .14, 1)}, 'BiscuitSkin')
    foundation(a, (-.50, 0, .78), (.35, 0, .88), (.83, 0, .99), (1.10, 0, 1.15))
    ell(a, 'barrel shaped capybara torso', (-.22, 0, .84), (1.0, .52, .55), 'fur', 'spine')
    ell(a, 'round rump', (-.75, 0, .85), (.48, .53, .53), 'fur', 'pelvis')
    ell(a, 'heavy shoulder', (.43, 0, .90), (.48, .51, .51), 'light', 'spine')
    ell(a, 'broad blunt rectangular head', (1.00, 0, 1.06), (.58, .39, .34), 'light', 'head')
    ell(a, 'broad flat capybara muzzle', (1.40, 0, 1.01), (.22, .385, .245), 'muzzle', 'head', 8, 4)
    for side, y in [('left', -.30), ('right', .30)]:
        a.joint('ear_' + side, p((.74, y, 1.30)), p((.74, y, 1.51)), 'head')
        ell(a, 'tiny round ear ' + side, (.74, y, 1.41), (.12, .10, .13), 'ear', 'ear_' + side, 8, 4)
        ell(a, 'nostril ' + side, (1.57, y * .64, 1.13), (.035, .04, .03), 'ink', 'head', 8, 4)
    eyes(a, 1.13, .375, 1.24, .048)
    finish(a, *legs(a, (.65, .69, -.09), (-.72, .70, .15), .37, (.17, .23), (.205, .16, .095), 'fur', 'foot'))


def clover():
    a = Asset('clover', 'Clover', {
        'fur': (.75, .68, .52, 1), 'light': (.91, .84, .67, 1),
        'cream': (.95, .90, .77, 1), 'ear': (.59, .38, .29, 1),
        'ink': (.045, .033, .028, 1), 'nose': (.45, .25, .20, 1),
        'shadow': (.53, .48, .37, 1), 'foot': (.81, .73, .58, 1)}, 'CloverSkin')
    foundation(a, (-.45, 0, .90), (.30, 0, .88), (.65, 0, 1.07), (.89, 0, 1.33))
    ell(a, 'compact rabbit torso', (-.12, 0, .91), (.69, .38, .43), 'fur', 'spine')
    ell(a, 'large upright rabbit haunches', (-.57, 0, .90), (.54, .45, .58), 'fur', 'pelvis')
    ell(a, 'slim forechest', (.46, 0, .86), (.29, .30, .39), 'light', 'spine')
    ell(a, 'rabbit head', (.82, 0, 1.26), (.35, .32, .33), 'light', 'head')
    ell(a, 'short split muzzle', (1.06, 0, 1.15), (.18, .25, .14), 'cream', 'head')
    ell(a, 'tiny rose nose', (1.22, 0, 1.20), (.04, .055, .035), 'nose', 'head', 8, 4)
    eyes(a, .99, .305, 1.35, .059)
    for side, y in [('left', -.26), ('right', .26)]:
        a.joint('ear_' + side, p((.71, y, 1.47)), p((.49, y * 1.80, .92)), 'head')
        ell(a, 'folded lop ear ' + side, (.70, y * 1.65, 1.15), (.20, .14, .43), 'fur', 'ear_' + side)
        ell(a, 'inner ear plane ' + side, (.72, y * 2.15, 1.13), (.11, .025, .28), 'ear', 'ear_' + side)
    a.joint('tail_base', p((-.99, 0, .99)), p((-1.15, 0, 1.07)), 'pelvis')
    ell(a, 'short cotton tail', (-1.07, 0, 1.02), (.21, .22, .22), 'cream', 'tail_base')
    finish(a, *legs(a, (.51, .73, -.085), (-.57, .94, .26), .27, (.12, .245), (.24, .13, .09), 'fur', 'foot'))


def mossback():
    a = Asset('mossback', 'Mossback', {
        'shell': (.32, .34, .27, 1), 'scute': (.43, .44, .32, 1),
        'rim': (.23, .25, .19, 1), 'skin': (.52, .48, .29, 1),
        'light': (.63, .58, .35, 1), 'moss': (.24, .36, .17, 1),
        'ink': (.035, .038, .027, 1), 'cream': (.64, .61, .43, 1)}, 'MossbackSkin')
    foundation(a, (-.47, 0, .56), (.29, 0, .57), (.82, 0, .62), (1.10, 0, .73))
    ell(a, 'low plastron', (-.13, 0, .49), (.95, .62, .17), 'cream', 'pelvis')
    # Closed faceted dome: a broad flat lower rim keeps the silhouette tortoise-like.
    vertices = []
    for height, radius in [(.48, 1), (.66, 1.01), (.98, .79), (1.18, .38)]:
        for i in range(12):
            angle = i * math.tau / 12
            vertices.append(p((-.16 + math.cos(angle) * radius, math.sin(angle) * radius * .66, height)))
    faces = []
    for ring in range(3):
        for i in range(12):
            faces.append((ring * 12 + i, ring * 12 + (i + 1) % 12,
                          (ring + 1) * 12 + (i + 1) % 12, (ring + 1) * 12 + i))
    faces += [tuple(reversed(range(12))), tuple(range(36, 48))]
    a.wedge('single rigid domed shell', vertices, faces, 'shell', 'spine')
    # Several large inset scutes; no tiny tessellation or noisy moss texture.
    for x, y, z, scale in [(-.18, 0, 1.18, (.37, .29, .05)), (.40, 0, 1.07, (.27, .27, .07)),
                           (-.72, 0, 1.06, (.24, .28, .07)), (-.13, -.45, .94, (.32, .13, .08)),
                           (-.13, .45, .94, (.32, .13, .08))]:
        ell(a, 'broad shell scute', (x, y, z), scale, 'scute', 'spine', 6, 4)
    ell(a, 'one moss patch', (-.57, -.23, 1.13), (.25, .20, .065), 'moss', 'spine', 8, 4)
    tube(a, 'short exposed neck', [(.63, 0, .58), (.89, 0, .63), (1.12, 0, .70)],
         [.19, .17, .17], 'skin', 'head')
    ell(a, 'small tortoise head', (1.11, 0, .71), (.29, .22, .20), 'light', 'head')
    ell(a, 'tortoise blunt beak', (1.34, 0, .67), (.11, .19, .10), 'skin', 'head', 8, 4)
    eyes(a, 1.20, .205, .78, .043)
    finish(a, *legs(a, (.56, .48, -.07), (-.69, .46, .10), .44, (.19, .20), (.20, .18, .075), 'skin', 'skin'))


if __name__ == '__main__':
    selected = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
    recipes = {'pip': pip, 'biscuit': biscuit, 'clover': clover, 'mossback': mossback}
    for name, recipe in recipes.items():
        if not selected or name in selected:
            recipe()
