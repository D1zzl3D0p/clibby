"""Original low-poly model recipes and 96px Blender motion-study renderer.

Run with Blender --background --python this_file -- --pet all. Generated files
belong in ignored results/, never in the production character catalogue.
"""
from pathlib import Path
import argparse
import json
import math
import sys
import time

import bpy
from mathutils import Vector

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from motion import PETS, evaluate, CAPABILITIES, pebble_geometry

COLORS = {
    "slate": "777c85", "slateLight": "a6ada9", "slateDark": "444852",
    "ink": "292b3b", "inkLight": "414456", "cream": "e5d8b0",
    "brown": "6b4c3d", "ochre": "af8352", "gold": "dfb66a",
    "leaf": "778567", "eye": "272b30", "moth": "beb18e",
}


def material(name, color):
    rgb = tuple(int(color[i:i+2], 16) / 255 for i in (0, 2, 4))
    mat = bpy.data.materials.new(name)
    mat.diffuse_color = (*rgb, 1)
    mat.use_nodes = True
    shader = mat.node_tree.nodes.get("Principled BSDF")
    shader.inputs["Base Color"].default_value = (*rgb, 1)
    shader.inputs["Roughness"].default_value = .95
    return mat


def mesh(name, vertices, faces, color):
    data = bpy.data.meshes.new(name)
    data.from_pydata(vertices, [], faces)
    data.update()
    obj = bpy.data.objects.new(name, data)
    bpy.context.collection.objects.link(obj)
    obj.data.materials.append(MATERIALS[color])
    return obj


def ell(name, location, scale, color, segments=12, rings=6):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=segments, ring_count=rings,
                                      radius=1, location=location)
    obj = bpy.context.object
    obj.name = name
    obj.scale = scale
    obj.data.materials.append(MATERIALS[color])
    return obj


def tube(name, points, radii, color, sides=6):
    vertices = []
    for i, (p, radius) in enumerate(zip(points, radii)):
        tangent = Vector(points[min(i + 1, len(points)-1)]) - Vector(points[max(0, i-1)])
        tangent.normalize()
        reference = Vector((0, 0, 1)) if abs(tangent.z) < .9 else Vector((0, 1, 0))
        first = tangent.cross(reference).normalized()
        second = tangent.cross(first).normalized()
        for side in range(sides):
            a = side * math.tau / sides
            vertices.append(tuple(Vector(p) + radius * (first * math.cos(a) + second * math.sin(a))))
    faces = [tuple(range(sides-1, -1, -1))]
    for i in range(len(points)-1):
        for side in range(sides):
            a, b = i*sides+side, i*sides+(side+1)%sides
            faces.append((a, b, b+sides, a+sides))
    faces.append(tuple(range((len(points)-1)*sides, len(points)*sides)))
    return mesh(name, vertices, faces, color)


def stone(name, location, scale, color):
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=1, radius=1, location=location)
    obj = bpy.context.object
    obj.name = name
    obj.scale = scale
    obj.rotation_euler = (.13, -.09, .18)
    obj.data.materials.append(MATERIALS[color])
    return obj


def pebblekin(pose):
    bottom = pose.support_polygon
    upper = tuple((x*1.04+.03, y*1.04, .35) for x,y,_ in bottom)
    vertices = bottom + upper + ((.08, .03, .52),)
    faces = [tuple(range(7,-1,-1))]
    for i in range(8):
        n = (i+1)%8
        faces.extend(((i,n,n+8,i+8), (i+8,n+8,16)))
    mesh("flat anchored slate footprint", vertices, faces, "slateDark")
    geometry = pebble_geometry(pose)
    mesh("middle warm slate", *geometry["middle"], "slate")
    mesh("upper pivoting slate", *geometry["upper"], "slateLight")
    yaw, wobble = pose.value("yaw"), pose.value("wobble")
    pivot = Vector(geometry["anchor"]) + Vector((0,0,geometry["lift"]))
    rotation = __import__("mathutils").Euler((0, wobble, yaw)).to_matrix()
    # A pair of tiny mineral seams suggests attention, without a mascot smile.
    for x in (-.13, .13):
        p = pivot + rotation @ Vector((x, -.305, .38))
        eye = ell("mineral eye", tuple(p), (.035, .025, .025), "slateDark", 8, 4)
        eye.rotation_euler = (0, wobble, yaw)


def lantern_snail(pose):
    root = pose.root[0]
    # Continuous sole mesh: every lower strip vertex stays exactly on Z=0.
    vertices, faces = [], []
    for i in range(25):
        x, _, z = pose.point(f"body{i}")
        width = .055 + .21 * math.sin(math.pi * i / 24) ** .55
        vertices.extend(((x, -width, 0), (x, width, 0),
                         (x, -width*.72, z), (x, width*.72, z)))
        if i:
            k = i*4
            faces.extend(((k-4, k, k+1, k-3), (k-2, k+2, k, k-4),
                          (k-3, k+1, k+3, k-1), (k-1, k+3, k+2, k-2)))
    faces.extend(((0, 1, 3, 2), (96, 98, 99, 97)))
    mesh("unbroken floor-contact foot", vertices, faces, "leaf")
    hx, hy, hz = pose.point("head")
    tube("smooth neck", ((root+.35, 0, .15), (root+.65, 0, .24), (hx, hy, hz)),
         (.20, .17, .15), "leaf", 8)
    ell("snail head", (hx, hy, hz), (.20, .19, .16), "leaf")
    for side in (-1, 1):
        tip = (hx+.08, side*.17, hz+.32+.09*pose.value("peek"))
        tube("eye stalk", ((hx, side*.11, hz+.07), tip), (.045, .026), "leaf")
        ell("stalk eye", tip, (.05, .045, .045), "eye", 8, 4)
    ell("rounded shell", (root-.23, .025, .75), (.65, .36, .67), "ochre")
    spiral = []
    for i in range(32):
        u = i/31
        angle = .35 + math.tau * 1.35 * u
        radius = .05 + .45 * u
        spiral.append((root-.23 + radius*math.cos(angle), -.315,
                       .76 + radius*math.sin(angle)))
    tube("broad shell spiral", spiral, (.033,)*32, "brown", 5)
    # A door hinged to the shell; the warm interior is a material, no light.
    hinge = Vector((root+.06, -.34, .38))
    theta = pose.value("door")
    vertices = []
    for x, y, z in ((0, 0, 0), (.30, 0, 0), (.30, 0, .40), (0, 0, .40)):
        vertices.append(tuple(hinge + Vector((x*math.cos(theta), -x*math.sin(theta), z))))
    mesh("hinged lantern door", vertices, ((0, 1, 2, 3),), "brown")
    mesh("warm window", ((root+.08, -.347, .41), (root+.31, -.347, .41),
                         (root+.31, -.347, .72), (root+.08, -.347, .72)),
         ((0, 1, 2, 3),), "gold")


def margin(pose):
    contour = [p for _, p in pose.points]
    vertices = [(x, y+depth, z) for depth in (-.12, .12) for x, y, z in contour]
    faces = [tuple(range(32)), tuple(range(63, 31, -1))]
    faces.extend((i, i+32, (i+1)%32+32, (i+1)%32) for i in range(32))
    mesh("bounded ink contour", vertices, faces, "ink")
    # Cream negative-space-like eye marks on the front planar surface.
    h = .98 + .22*pose.value("exclamation")
    for x in (-.17, .17):
        ell("ink eye cutout", (x, -.135, h), (.055, .025, .09), "cream", 8, 4)
    ell("ink punctuation dot", (0, 0, .075), (.17, .14, .075), "ink", 10, 4)


def velvet(pose):
    root = pose.root[0]
    for contact in pose.contacts:
        hip = pose.point(contact.name + "Hip")
        knee = pose.point(contact.name + "Knee")
        tube(contact.name + " articulated leg", (hip, knee, contact.position),
             (.035, .026, .018), "brown", 5)
    ell("tapered moth abdomen", (root-.18, 0, .48), (.61, .24, .27), "brown")
    ell("velvet thorax", (root+.35, 0, .52), (.31, .28, .30), "moth")
    ell("moth head", (root+.61, 0, .54), (.19, .22, .19), "moth")
    for side in (-1, 1):
        ell("moth eye", (root+.70, side*.17, .58), (.065, .045, .065), "eye", 8, 4)
        points = ((root+.68, side*.11, .68), (root+.83, side*.25, .87),
                  (root+.89, side*.40, .93))
        tube("feathered antenna stem", points, (.023, .018, .012), "brown", 5)
        for i in range(4):
            f = i/4
            p = (root+.76+.12*f, side*(.19+.18*f), .78+.12*f)
            tube("antenna barb", (p, (p[0]-.08, p[1]+side*.07, p[2]+.035)),
                 (.012, .006), "brown", 4)
        # Original folded polygon wings open on an explicit reveal goal.
        opening = .29 + pose.value("wing")
        local = ((.40, 0, .62), (-.77, 0, .67), (-.99, .73, .48),
                 (-.16, .94, .55), (.45, .48, .62))
        wing = [(root+x, side*(.10+y*math.sin(opening)), z+y*math.cos(opening))
                for x, y, z in local]
        mesh("folded moth wing", wing, ((0, 1, 2), (0, 2, 3), (0, 3, 4)), "cream")
        # Broad bands survive at native pixel density; no intricate veins.
        for f in (.38, .70):
            x = root-.64+.65*f
            y = side*(.10+.72*math.sin(opening))
            z = .56+.72*math.cos(opening)
            ell("wing broad spot", (x, y, z), (.16, .035, .095), "ochre", 8, 4)


def setup():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = 4
    scene.render.resolution_x = scene.render.resolution_y = 96
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGBA"
    scene.render.film_transparent = True
    scene.render.filter_size = .01
    scene.view_settings.view_transform = "Standard"
    scene.view_settings.look = "None"
    scene.view_settings.exposure = .15
    scene.world.color = (.55, .55, .55)
    for name, location, energy, size in (("key", (-3,-4,6), 500, 4),
                                         ("fill", (4,2,4), 220, 4)):
        data = bpy.data.lights.new(name, "AREA")
        data.energy, data.shape, data.size = energy, "DISK", size
        obj = bpy.data.objects.new(name, data)
        bpy.context.collection.objects.link(obj)
        obj.location = location
        obj.rotation_euler = (Vector((0,0,.8))-obj.location).to_track_quat("-Z","Y").to_euler()
    data = bpy.data.cameras.new("orthographic 96px")
    cam = bpy.data.objects.new("orthographic 96px", data)
    bpy.context.collection.objects.link(cam)
    cam.location = (3,-8,3.5)
    cam.rotation_euler = (Vector((0,0,1))-cam.location).to_track_quat("-Z","Y").to_euler()
    data.type, data.ortho_scale = "ORTHO", 3.8
    scene.camera = cam
    scene.render.fps = 15


def build(pet, t):
    for obj in list(bpy.data.objects):
        if obj.type == "MESH":
            data = obj.data
            bpy.data.objects.remove(obj, do_unlink=True)
            if data.users == 0:
                bpy.data.meshes.remove(data)
    pose = evaluate(pet, t)
    {"pebblekin": pebblekin, "lantern-snail": lantern_snail,
     "margin": margin, "velvet": velvet}[pet](pose)
    return pose


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--pet", choices=(*PETS, "all"), default="all")
    parser.add_argument("--frames", type=int, default=120)
    parser.add_argument("--output", type=Path, default=HERE.parents[1]/"results/catalogue-families-2026-10-16")
    args = parser.parse_args(sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else [])
    if args.frames not in (1, 120):
        parser.error("only one-frame previews or complete 120-frame studies are supported")
    setup()
    global MATERIALS
    MATERIALS = {name: material(name, color) for name, color in COLORS.items()}
    start = time.monotonic()
    for pet in PETS if args.pet == "all" else (args.pet,):
        directory = args.output/pet
        (directory/"frames").mkdir(parents=True, exist_ok=True)
        poses = []
        for i in range(args.frames):
            pose = build(pet, i/15)
            bpy.context.scene.frame_set(i)
            bpy.context.scene.render.filepath = str(directory/"frames"/f"{i:04}.png")
            bpy.ops.render.render(write_still=True)
            if i == 0:
                bpy.ops.wm.save_as_mainfile(filepath=str(directory/"source.blend"))
            poses.append({"time": i/15, "root": pose.root, "active": pose.active,
                          "contacts": [{"name": c.name, "position": c.position, "planted": c.planted}
                                       for c in pose.contacts],
                          "support": pose.support_polygon, "values": dict(pose.values)})
        (directory/"poses.json").write_text(json.dumps(poses, indent=2)+"\n")
    (args.output/"render_stats.json").write_text(json.dumps({
        "frames_per_pet": args.frames, "fps": 15, "pixel_grid": [96,96],
        "elapsed_offline_seconds": time.monotonic()-start,
        "experimental_not_resident": True, "capabilities": {p: sorted(c) for p,c in CAPABILITIES.items()},
        "source": str(HERE/"render.py")}, indent=2)+"\n")


if __name__ == "__main__":
    main()
