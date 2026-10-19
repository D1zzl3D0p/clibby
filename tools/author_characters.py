#!/usr/bin/env python3
"""Reproduce the original example characters from editable pixel shape recipes.

Each recipe is a single attachment, not an animation frame. Coordinates below
are integer design pixels; Pillow draws without antialiasing. All pose/motion is
owned by the runtime. Run from any directory with the project's Pillow venv.
"""
from __future__ import annotations

import argparse
import json
import math
import tempfile
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
OUTLINE = '#292a35'


class Piece:
    def __init__(self, size, pivot):
        self.image = Image.new('RGBA', size)
        self.draw = ImageDraw.Draw(self.image)
        self.pivot = pivot

    def ellipse(self, box, fill, outline=None, width=2):
        self.draw.ellipse(box, fill=fill, outline=outline, width=width)
        return self

    def poly(self, points, fill, outline=None, width=2):
        self.draw.polygon(points, fill=fill)
        if outline:
            self.draw.line(points + [points[0]], fill=outline, width=width, joint='curve')
        return self

    def line(self, points, fill, width=1):
        self.draw.line(points, fill=fill, width=width, joint='curve')
        return self


class Character:
    def __init__(self, identity, name, controller, size, origin):
        self.manifest = dict(version=1, id=identity, name=name, controller=controller,
                             width=size[0], height=size[1], origin=list(origin),
                             atlas='atlas.png', motionEnvelope=16,
                             bones=[], attachments=[], bindings={}, contacts=[])
        self.pieces = []

    def bone(self, identity, parent, x, y, binding=None, rotation=0):
        self.manifest['bones'].append(dict(id=identity, parent=parent,
            rest=dict(x=x, y=y, rotation=rotation, scaleX=1, scaleY=1)))
        if binding:
            self.manifest['bindings'][binding] = identity

    def attach(self, name, bone, piece, layer):
        self.pieces.append((name, piece))
        self.manifest['attachments'].append(dict(bone=bone, source=[],
            pivot=list(piece.pivot), offset=[0, 0], drawOrder=layer))


def soft_creature():
    """Flat silhouette with two actual hip/knee/ankle chains for grounded IK."""
    c = Character('soft-creature', 'Mallow', 'soft', (128, 144), (64, 125))
    cream, ink = '#f3eee1', '#141414'
    c.bone('trunk', None, 64, 86, 'body')
    # Visual quarter-view branch can turn independently of planted IK limbs.
    c.bone('silhouette', 'trunk', 0, 0, 'facing')
    c.bone('face', 'silhouette', 3, -25, 'head')
    c.bone('eyes', 'face', 0, 1, 'eyes')
    c.bone('ear-l', 'face', -9, -13, 'earLeft', -.08)
    c.bone('ear-r', 'face', 8, -13, 'earRight', .08)
    c.bone('tail', 'silhouette', -16, 17, 'tail')
    for side, sign in [('l', -1), ('r', 1)]:
        # Both knees flex gently forward, with two sqrt(3^2+11^2) links.
        # Rest ankles stay y=122 and contacts y=125; hips retain their width.
        c.bone('hip-'+side, 'trunk', sign*9, 14)
        c.bone('knee-'+side, 'hip-'+side, 3, 11)
        c.bone('foot-'+side, 'knee-'+side, -3, 11,
               'footLeft' if side == 'l' else 'footRight')
        c.attach('thigh-'+side, 'hip-'+side, Piece((23, 19), (11, 3))
            .line([(11,3),(14,14)], cream, 7)
            .ellipse((8,0,14,6), cream)
            .ellipse((11,11,17,17), cream), 2)
        c.attach('shin-'+side, 'knee-'+side, Piece((23, 19), (11, 3))
            .line([(11,3),(8,14)], cream, 7)
            .ellipse((8,0,14,6), cream)
            .ellipse((5,11,11,17), cream), 3)
        c.attach('small-foot-'+side, 'foot-'+side, Piece((15, 7), (7, 3))
            .ellipse((1,1,13,6), cream), 4)
    c.attach('curved-tail', 'tail', Piece((42, 25), (35, 6))
        .poly([(35,2),(39,5),(37,13),(31,19),(20,22),(10,21),
               (3,17),(1,12),(3,8),(7,8),(8,13),(13,15),(22,15),
               (29,12),(32,7)], cream), 0)
    c.attach('soft-body', 'silhouette', Piece((45, 56), (22, 24))
        .ellipse((2,1,42,54), cream), 5)
    c.attach('left-ear', 'ear-l', Piece((15, 32), (7, 28))
        .ellipse((2,1,12,30), cream), 8)
    c.attach('right-ear', 'ear-r', Piece((15, 28), (7, 24))
        .ellipse((2,1,12,26), cream), 8)
    c.attach('round-head', 'face', Piece((49, 39), (24, 19))
        .ellipse((1,1,47,37), cream), 10)
    c.attach('black-eyes', 'eyes', Piece((49, 39), (24, 20))
        .ellipse((12,15,17,25), ink)
        .ellipse((31,15,36,25), ink), 12)
    c.manifest['interaction'] = [[47,40],[64,38],[83,41],[89,87],
                                  [82,128],[45,128],[39,92]]
    c.manifest['contacts'] = [dict(bone='foot-'+s, point=[0,3]) for s in ('l','r')]
    c.manifest['locomotion'] = dict(
        legs=[dict(hip='hip-'+s, knee='knee-'+s, ankle='foot-'+s,
                   contact=[0,3], bend=-1) for s in ('l','r')],
        groundY=125, speed=50, acceleration=200, reachThreshold=8,
        stride=12, lift=4, minStepDuration=.18, maxStepDuration=.24,
        secondary=dict(frequency=3, damping=.8, response=0))
    return c


def werewolf():
    c = Character('werewolf', 'Bramble', 'articulated', (144, 176), (74, 162))
    fur, hi, shade, chest = '#626f87', '#91a0b5', '#414c65', '#b5bfcc'
    amber, rose = '#eabd58', '#ae7d88'
    c.bone('spine', None, 74, 88, 'body')
    c.bone('silhouette', 'spine', 0, 0, 'facing')
    c.bone('skull', 'silhouette', 0, -34, 'head')
    c.bone('eyes', 'skull', 0, -3, 'eyes')
    c.bone('ear-l', 'skull', -18, -17, 'earLeft')
    c.bone('ear-r', 'skull', 17, -17, 'earRight')
    c.bone('tail', 'silhouette', -17, 19, 'tail')
    for side, sign in [('l', -1), ('r', 1)]:
        c.bone('upper-arm-'+side, 'silhouette', sign*23, -10)
        c.bone('forearm-'+side, 'upper-arm-'+side, sign*4, 21)
        c.bone('thigh-'+side, 'spine', sign*13, 24)
        c.bone('shin-'+side, 'thigh-'+side, 4, 21)
        c.bone('paw-'+side, 'shin-'+side, -3, 19, 'footLeft' if side=='l' else 'footRight')
    c.attach('bushy-tail', 'tail', Piece((51, 47), (44, 8))
        .poly([(44,2),(49,9),(45,22),(35,30),(19,34),(7,29),(2,22),(0,11),
               (9,15),(6,6),(19,16),(29,18),(37,12)], shade, OUTLINE)
        .poly([(3,13),(10,18),(11,12),(21,23),(16,29),(7,24)], hi), 0)
    for side in ('l','r'):
        c.attach('thigh-'+side,'thigh-'+side, Piece((27,32),(13,5))
            .poly([(5,1),(20,1),(25,11),(21,29),(8,31),(2,20)],fur,OUTLINE)
            .poly([(6,5),(13,4),(16,22),(10,26),(5,17)],hi),2)
        c.attach('hock-'+side,'shin-'+side, Piece((19,29),(9,4))
            .poly([(4,1),(15,2),(17,8),(10,16),(13,25),(5,27),(1,15)],shade,OUTLINE)
            .line([(12,8),(7,15),(9,22)],hi,2),3)
        c.attach('paw-'+side,'paw-'+side, Piece((28,15),(13,3))
            .poly([(7,1),(19,1),(22,6),(26,8),(26,13),(2,13),(1,8)],fur,OUTLINE)
            .line([(6,9),(5,12)],chest,2).line([(13,9),(12,12)],chest,2)
            .line([(20,9),(20,12)],chest,2),4)
        c.attach('upper-arm-'+side,'upper-arm-'+side, Piece((21,31),(10,5))
            .poly([(3,3),(13,1),(19,8),(17,18),(20,24),(12,28),(4,24),(1,13)],fur,OUTLINE)
            .line([(5,8),(6,17),(9,22)],hi,3),6)
        c.attach('clawed-hand-'+side,'forearm-'+side, Piece((23,34),(11,5))
            .poly([(4,1),(17,1),(20,11),(18,19),(21,25),(19,31),(15,27),
                   (12,33),(9,27),(5,31),(3,24),(1,13)],shade,OUTLINE)
            .poly([(6,3),(12,3),(13,17),(7,20),(4,14)],fur)
            .line([(5,25),(5,29)],chest,2).line([(11,27),(11,31)],chest,2)
            .line([(18,25),(18,28)],chest,2),9)
    c.attach('torso','silhouette',Piece((51,57),(25,25))
        .poly([(10,2),(25,0),(40,2),(48,15),(42,38),(35,53),(15,53),(8,38),(2,15)],fur,OUTLINE)
        .poly([(35,9),(44,17),(38,38),(30,46),(33,25)],shade),7)
    c.attach('chest-ruff','silhouette',Piece((51,57),(25,25))
        .poly([(9,4),(25,1),(42,5),(39,16),(43,19),(35,22),(33,31),
               (27,27),(25,39),(20,28),(14,32),(14,23),(7,20),(11,15)],chest,OUTLINE)
        .poly([(16,6),(25,4),(34,7),(28,14),(25,24),(22,15)],'#d3d8dd'),8)
    for side in ('l','r'):
        c.attach('ear-'+side,'ear-'+side,Piece((23,33),(11,29))
            .poly([(2,31),(1,3),(5,1),(19,17),(21,30)],fur,OUTLINE)
            .poly([(5,7),(15,20),(16,26),(6,27)],rose)
            .line([(2,8),(3,23)],hi,2),10)
    c.attach('head','skull',Piece((65,53),(32,25))
        .poly([(12,3),(26,0),(41,1),(53,8),(56,20),(62,26),(55,28),
               (62,34),(52,36),(49,44),(40,49),(24,49),(14,43),(9,36),
               (2,34),(9,28),(2,25),(9,20)],fur,OUTLINE)
        .poly([(11,10),(23,5),(28,11),(21,17),(12,20)],hi)
        .poly([(42,5),(51,12),(52,23),(45,19),(38,12)],shade),11)
    c.attach('eyes','eyes',Piece((65,53),(32,22))
        .poly([(14,19),(25,20),(26,25),(15,25)],OUTLINE)
        .poly([(39,20),(50,18),(49,25),(38,25)],OUTLINE)
        .line([(17,21),(23,22)],amber,2).line([(41,22),(47,21)],amber,2),12)
    c.attach('face','skull',Piece((65,53),(32,25))
        .poly([(22,28),(32,23),(43,28),(48,34),(42,42),(22,42),(17,34)],chest,OUTLINE)
        .poly([(27,27),(37,27),(36,32),(32,34),(28,32)],OUTLINE)
        .line([(32,34),(32,37),(27,39),(23,37)],OUTLINE)
        .line([(32,37),(37,39),(42,37)],OUTLINE)
        .poly([(23,38),(25,39),(24,42)],'#fff3db')
        .poly([(39,39),(41,38),(40,42)],'#fff3db'),12)
    c.manifest['interaction'] = [[50,25],[98,25],[113,63],[117,125],[103,165],[46,165],[29,124],[33,66]]
    c.manifest['contacts'] = [dict(bone='paw-'+s, point=[0,10]) for s in ('l','r')]
    c.manifest['bindings'].update(armLeft='upper-arm-l', armRight='upper-arm-r',
                                  forearmLeft='forearm-l', forearmRight='forearm-r')
    c.manifest['locomotion'] = dict(
        legs=[dict(hip='thigh-'+side, knee='shin-'+side, ankle='paw-'+side,
                   contact=[0,10], bend=-1)
              for side in ('l','r')],
        groundY=162, speed=70, acceleration=240, reachThreshold=13,
        stride=18, lift=6, minStepDuration=.18, maxStepDuration=.26,
        secondary=dict(frequency=3, damping=.8, response=0))
    return c


LICENSE = '''MIT License

Copyright (c) 2026 Better Pets contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
'''


def generate(character, destination):
    destination.mkdir(parents=True, exist_ok=True)
    atlas = Image.new('RGBA', (512,256))
    x, y, row = 2, 2, 0
    for (_, piece), attachment in zip(character.pieces, character.manifest['attachments']):
        w,h = piece.image.size
        if x+w+2 > atlas.width:
            x, y, row = 2, y+row+2, 0
        assert y+h < atlas.height
        atlas.alpha_composite(piece.image,(x,y))
        attachment['source'] = [x,y,w,h]
        x, row = x+w+2, max(row,h)
    manifest = character.manifest
    atlas.save(destination/'atlas.png')
    (destination/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    # Independent rest-pose composition for inspecting seam/layer decisions.
    world = {}
    for bone in manifest['bones']:
        r=bone['rest']; parent=world.get(bone['parent'],(0,0,0))
        a=parent[2]
        world[bone['id']]=(parent[0]+r['x']*math.cos(a)-r['y']*math.sin(a),
                           parent[1]+r['x']*math.sin(a)+r['y']*math.cos(a),a+r['rotation'])
    preview=Image.new('RGBA',(manifest['width'],manifest['height']))
    for index,att in sorted(enumerate(manifest['attachments']),key=lambda pair:pair[1]['drawOrder']):
        piece=character.pieces[index][1]; bx,by,angle=world[att['bone']]
        # Rotate a padded tile around its bone pivot, maintaining local origin.
        tile=Image.new('RGBA',(192,192)); px,py=piece.pivot
        tile.alpha_composite(piece.image,(96-px,96-py))
        tile=tile.rotate(-math.degrees(angle),resample=Image.Resampling.NEAREST)
        preview.alpha_composite(tile,(round(bx)-96,round(by)-96))
    preview.save(destination/'preview.png')
    (destination/'LICENSE').write_text(LICENSE)
    art_iteration = (
        'Flat silhouette revision for grounded IK review; reusable art pieces and '
        'two-link limb anatomy, with no authored walk animation.\n'
        if manifest['id'] == 'soft-creature' else
        'This is the first art iteration; procedural action poses and '
        'silhouette/seam polish remain reviewable.\n')
    (destination/'ATTRIBUTION.md').write_text(
        f'# {manifest["name"]}\n\nOriginal Clibby pixel artwork and anatomy, '
        'authored as editable polygon/ellipse recipes in `tools/author_characters.py`. '
        'No downloaded art, traced game sprites, AI-generated images, or animation frame '
        'sequences are included. MIT licensed. ' + art_iteration)


def validate(character):
    m=character.manifest
    assert len(m['bones'])<=64 and len(m['attachments'])<=64
    ids={b['id'] for b in m['bones']}
    assert len(ids)==len(m['bones'])
    assert all(a['bone'] in ids for a in m['attachments'])
    assert all(v in ids for v in m['bindings'].values())
    assert all(b['parent'] is None or b['parent'] in ids for b in m['bones'])
    assert all(p.image.getbbox() is not None for _,p in character.pieces)
    for _,piece in character.pieces:
        w,h=piece.image.size
        assert 0<=piece.pivot[0]<w and 0<=piece.pivot[1]<h
        # Hard pixels stay hard; accidental filtering must not enter the source.
        assert not any(piece.image.getchannel('A').histogram()[1:255])
    assert all(0<=x<=m['width'] and 0<=y<=m['height'] for x,y in m['interaction'])
    assert all(contact['bone'] in ids for contact in m['contacts'])


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=ROOT/'characters')
    parser.add_argument('--check',action='store_true',
                        help='compare committed package bytes with a fresh deterministic generation')
    args=parser.parse_args()
    for character in (soft_creature(),werewolf()):
        validate(character)
        identity=character.manifest['id']
        if args.check:
            with tempfile.TemporaryDirectory(prefix='clibby-art-') as directory:
                generated=Path(directory)/identity
                generate(character,generated)
                for path in generated.iterdir():
                    existing=args.output/identity/path.name
                    if not existing.exists() or existing.read_bytes()!=path.read_bytes():
                        raise SystemExit(f'Asset differs from editable source: {existing}')
            print(f'Verified {identity}: deterministic source matches every package file')
        else:
            generate(character,args.output/identity)
            print(f'Generated {identity}: {len(character.pieces)} attachments')


if __name__=='__main__':
    main()
