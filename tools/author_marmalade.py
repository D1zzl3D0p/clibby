#!/usr/bin/env python3
"""Deterministic original Marmalade attachment recipes, with no animation frames."""
from __future__ import annotations

import argparse
from pathlib import Path
import tempfile

from author_characters import Character, Piece, generate, validate

ROOT = Path(__file__).resolve().parents[1]
INK = '#633c2d'
GINGER = '#db873c'
LIGHT = '#f3b65d'
CREAM = '#ffe0a0'
STRIPE = '#ad592d'
SHADE = '#b86b37'


def marmalade():
    """Four real two-link legs under a long tabby torso; +X is forward."""
    c = Character('marmalade', 'Marmalade', 'articulated', (192, 128), (96, 112))
    c.bone('body', None, 96, 69, 'body')
    c.bone('silhouette', 'body', 0, 0, 'facing')
    c.bone('head', 'silhouette', 42, -13, 'head')
    c.bone('eyes', 'head', 9, -3, 'eyes')
    c.bone('ear-far', 'head', -8, -14, 'earLeft')
    c.bone('ear-near', 'head', 8, -14, 'earRight')
    c.bone('tail', 'silhouette', -40, -6, 'tail')

    legs = []
    # Offset far paws horizontally so all four contacts remain legible at rest.
    for role, x, far, hind in [('hindRight', -24, True, True),
                                ('foreRight', 22, True, False),
                                ('hindLeft', -32, False, True),
                                ('foreLeft', 31, False, False)]:
        hip, knee, ankle = (role + '-' + joint for joint in ('hip', 'knee', 'ankle'))
        dx = 6 if hind else -6
        color = SHADE if far else GINGER
        c.bone(hip, 'body', x, 10)
        c.bone(knee, hip, dx, 14)
        c.bone(ankle, knee, -dx, 14)
        upper = Piece((29, 24), (14, 4))
        upper.line([(14, 4), (14 + dx, 18)], INK, 12)
        upper.line([(14, 4), (14 + dx, 18)], color, 8)
        upper.ellipse((10, 0, 18, 8), color)
        lower = Piece((29, 23), (14, 4))
        lower.line([(14, 4), (14 - dx, 18)], INK, 9)
        lower.line([(14, 4), (14 - dx, 18)], color, 5)
        c.attach(role + '-upper', hip, upper, 1 if far else 4)
        c.attach(role + '-lower', knee, lower, 2 if far else 6)
        c.attach(role + '-paw', ankle, Piece((19, 10), (8, 3))
                 .ellipse((1, 0, 17, 8), color, INK, 1)
                 .poly([(2, 4), (16, 4), (16, 7), (2, 7)], CREAM if not far else LIGHT)
                 .line([(10, 5), (10, 7)], STRIPE)
                 .line([(13, 5), (13, 7)], STRIPE), 3 if far else 8)
        legs.append(dict(role=role, hip=hip, knee=knee, ankle=ankle,
                         contact=[0, 5], bend=-1 if hind else 1))

    c.attach('upright-curled-tail', 'tail', Piece((48, 64), (42, 56))
             .poly([(44, 60), (34, 57), (22, 47), (14, 34), (12, 19),
                    (15, 8), (22, 2), (30, 2), (35, 7), (34, 15),
                    (29, 19), (25, 16), (25, 11), (22, 13), (21, 23),
                    (25, 34), (33, 44), (45, 49)], GINGER, INK, 2)
             .line([(17, 25), (23, 24)], STRIPE, 4)
             .line([(21, 38), (28, 34)], STRIPE, 4)
             .line([(29, 49), (35, 43)], STRIPE, 4), 0)
    c.attach('horizontal-tabby-body', 'silhouette', Piece((96, 48), (48, 23))
             .ellipse((2, 2, 93, 44), GINGER, INK, 2)
             .ellipse((12, 5, 86, 34), LIGHT)
             .ellipse((26, 25, 83, 42), CREAM)
             .poly([(25, 5), (33, 4), (35, 18), (31, 25), (28, 18)], STRIPE)
             .poly([(44, 3), (52, 3), (53, 15), (49, 23), (46, 15)], STRIPE)
             .poly([(63, 5), (70, 7), (69, 20), (65, 25), (63, 17)], STRIPE), 5)
    for identity, near in [('ear-far', False), ('ear-near', True)]:
        c.attach(identity, identity, Piece((22, 29), (10, 24))
                 .poly([(1, 26), (2, 3), (6, 1), (20, 23)], GINGER if near else SHADE, INK, 2)
                 .poly([(6, 8), (7, 21), (15, 22)], '#eaa18a'), 9)
    c.attach('round-cheek-head', 'head', Piece((54, 43), (25, 21))
             .ellipse((2, 1, 47, 39), GINGER, INK, 2)
             .ellipse((8, 4, 43, 29), LIGHT)
             .poly([(17, 2), (22, 2), (24, 12), (21, 15)], STRIPE)
             .poly([(28, 2), (33, 3), (31, 14), (28, 12)], STRIPE)
             .ellipse((25, 22, 50, 37), CREAM, INK, 1)
             .poly([(44, 21), (51, 23), (47, 27)], '#9c5849')
             .line([(46, 27), (43, 31), (38, 31)], INK, 1)
             .line([(29, 27), (20, 25)], INK, 1)
             .line([(29, 30), (19, 32)], INK, 1), 10)
    c.attach('amber-eyes', 'eyes', Piece((23, 13), (11, 6))
             .ellipse((2, 1, 10, 11), INK)
             .ellipse((3, 2, 8, 9), '#a5b66b')
             .line([(6, 3), (6, 9)], '#332b26', 2)
             .ellipse((15, 2, 21, 10), INK)
             .line([(17, 3), (17, 8)], '#a5b66b', 2)
             .ellipse((4, 2, 5, 3), '#fff3d2'), 12)
    c.manifest['interaction'] = [[51, 48], [112, 41], [129, 25], [158, 34],
                                 [169, 68], [143, 87], [139, 114], [57, 114], [48, 85]]
    c.manifest['contacts'] = [dict(bone=leg['ankle'], point=leg['contact']) for leg in legs]
    c.manifest['locomotion'] = dict(gait='quadrupedWalk', legs=legs, groundY=112,
        speed=36, acceleration=150, reachThreshold=6, stride=9, lift=4,
        minStepDuration=.16, maxStepDuration=.24,
        secondary=dict(frequency=3, damping=.8, response=0))
    return c


def generate_marmalade(destination):
    character = marmalade()
    validate(character)
    generate(character, destination)
    (destination / 'ATTRIBUTION.md').write_text(
        '# Marmalade\n\nOriginal Clibby pixel artwork and four-leg anatomy, '
        'authored as editable polygon, line and ellipse recipes in '
        '`tools/author_marmalade.py`. No downloaded art, traced sprites, '
        'AI-generated images or animation frames are included. MIT licensed.\n\n'
        'The atlas contains independently posed attachment pieces. '
        'The preview is the authored standing pose; runtime walking, turning, '
        'blinking and drop responses are procedural. Seated and flop poses '
        'are outside this first anatomy.\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'characters')
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    destination = args.output / 'marmalade'
    if args.check:
        with tempfile.TemporaryDirectory(prefix='marmalade-source-') as directory:
            generated = Path(directory)
            generate_marmalade(generated)
            for path in generated.iterdir():
                existing = destination / path.name
                if not existing.exists() or existing.read_bytes() != path.read_bytes():
                    raise SystemExit(f'Asset differs from editable source: {existing}')
        print('Verified Marmalade: deterministic source matches every package file')
    else:
        generate_marmalade(destination)
        print(f'Generated Marmalade: {destination}')


if __name__ == '__main__':
    main()
