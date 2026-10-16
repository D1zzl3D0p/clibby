"""Original low-poly werewolf, authored/exported by Blender (no runtime dependency).

Run: blender --background --python tools/author_werewolf.py
The character uses Z-up, -Y forward and meters; glTF exporter converts to Y-up.
"""
from pathlib import Path
import json
import hashlib
import math
import struct
import bpy
from mathutils import Vector

OUT = Path(__file__).resolve().parents[1] / "characters" / "werewolf-3d"
COLORS = {
    "fur": (0.32, 0.19, 0.105, 1), "fur_light": (0.47, 0.30, 0.16, 1),
    "cream": (0.78, 0.64, 0.43, 1), "shirt": (0.49, 0.035, 0.045, 1),
    "nose": (0.065, 0.046, 0.036, 1), "eye": (0.95, 0.60, 0.12, 1),
    "claw": (0.83, 0.76, 0.60, 1), "inner_ear": (0.55, 0.34, 0.25, 1),
}
JOINTS = []
PARTS = []


def joint(name, head, tail, parent=None):
    JOINTS.append({"name": name, "head": list(head), "tail": list(tail), "parent": parent})


def shade(obj, color, bone, blend=None):
    obj.data.materials.append(MATERIAL)
    uv = obj.data.uv_layers.active or obj.data.uv_layers.new(name="Atlas")
    uv.name = "Atlas"
    uv.active_render = True
    n = list(COLORS).index(color)
    for loop in uv.data:
        loop.uv = ((n + 0.5) / 8, 0.5)
    vg = obj.vertex_groups.new(name=bone)
    vg.add(list(range(len(obj.data.vertices))), 1, "REPLACE")
    if blend:
        other, fraction = blend
        second = obj.vertex_groups.new(name=other)
        for v in obj.data.vertices:
            vg.add([v.index], 1 - fraction, "REPLACE")
            second.add([v.index], fraction, "REPLACE")
    PARTS.append(obj)
    return obj


def blob(name, position, scale, color, bone, segments=8, rings=4):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=segments, ring_count=rings, location=position)
    obj = bpy.context.object
    obj.name = name
    obj.scale = scale
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return shade(obj, color, bone)


def taper(name, start, end, radius1, radius2, color, bone, vertices=8):
    direction = Vector(end) - Vector(start)
    bpy.ops.mesh.primitive_cone_add(vertices=vertices, radius1=radius1, radius2=radius2,
                                   depth=direction.length, location=(Vector(start) + Vector(end)) / 2)
    obj = bpy.context.object
    obj.name = name
    obj.rotation_euler = direction.to_track_quat("Z", "Y").to_euler()
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    return shade(obj, color, bone)


def wedge(name, vertices, faces, color, bone):
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    return shade(obj, color, bone)


def look_at(obj, target):
    obj.rotation_euler = (Vector(target) - obj.location).to_track_quat("-Z", "Y").to_euler()


def validate_glb(path):
    raw = path.read_bytes()
    magic, version, length = struct.unpack_from("<III", raw)
    assert magic == 0x46546C67 and version == 2 and length == len(raw)
    count, kind = struct.unpack_from("<II", raw, 12)
    assert kind == 0x4E4F534A
    data = json.loads(raw[20:20 + count])
    primitives = [p for m in data["meshes"] for p in m["primitives"]]
    triangles = sum(data["accessors"][p["indices"]]["count"] // 3 for p in primitives)
    vertices = sum(data["accessors"][p["attributes"]["POSITION"]]["count"] for p in primitives)
    assert triangles <= 4000
    assert len(data.get("materials", [])) <= 4
    assert len(data["skins"]) == 1 and len(data["skins"][0]["joints"]) <= 32
    assert not data.get("animations")
    for p in primitives:
        assert "JOINTS_0" in p["attributes"] and "WEIGHTS_0" in p["attributes"]
        assert "JOINTS_1" not in p["attributes"]
    # Decode all exported weights instead of only trusting Blender groups.
    binary_start = 20 + count + 8
    for accessor in data["accessors"]:
        if accessor.get("componentType") != 5126:
            continue
        components = {"SCALAR":1,"VEC2":2,"VEC3":3,"VEC4":4,"MAT4":16}[accessor["type"]]
        view = data["bufferViews"][accessor["bufferView"]]
        offset = binary_start + view.get("byteOffset", 0) + accessor.get("byteOffset", 0)
        stride = view.get("byteStride", components * 4)
        for i in range(accessor["count"]):
            values = struct.unpack_from("<" + "f" * components, raw, offset + i * stride)
            assert all(math.isfinite(v) for v in values), "Non-finite exported accessor"
    max_influences = 0
    for p in primitives:
        accessor = data["accessors"][p["attributes"]["WEIGHTS_0"]]
        view = data["bufferViews"][accessor["bufferView"]]
        assert accessor["componentType"] == 5126 and accessor["type"] == "VEC4"
        offset = binary_start + view.get("byteOffset", 0) + accessor.get("byteOffset", 0)
        stride = view.get("byteStride", 16)
        for i in range(accessor["count"]):
            weights = struct.unpack_from("<4f", raw, offset + i * stride)
            assert all(math.isfinite(w) and 0 <= w <= 1 for w in weights)
            assert abs(sum(weights) - 1) < 1e-5
            max_influences = max(max_influences, sum(w > 0 for w in weights))
    assert len(data["images"]) == 1 and "bufferView" in data["images"][0]
    image_view = data["bufferViews"][data["images"][0]["bufferView"]]
    image_offset = binary_start + image_view.get("byteOffset", 0)
    assert raw[image_offset:image_offset + 8] == b"\x89PNG\r\n\x1a\n"
    assert struct.unpack_from(">II", raw, image_offset + 16) == (256, 256)
    for sampler in data.get("samplers", []):
        assert sampler.get("magFilter") == 9728, "Atlas must enlarge with nearest sampling"
    names = [data["nodes"][i]["name"] for i in data["skins"][0]["joints"]]
    assert set(names) == {j["name"] for j in JOINTS}
    return {"triangles": triangles, "exported_vertices": vertices,
            "materials": len(data["materials"]), "deform_joints": len(names),
            "max_influences": max_influences, "atlas": [256, 256], "animation_clips": 0,
            "joint_order": names, "embedded_images": len(data["images"])}


OUT.mkdir(parents=True, exist_ok=True)
bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
# Author a single original, deliberately uncomplicated atlas with 8 color strips.
atlas = bpy.data.images.new("Werewolf original color atlas", 256, 256, alpha=True)
pixels = []
for y in range(256):
    for x in range(256):
        pixels.extend(list(COLORS.values())[min(x // 32, 7)])
atlas.pixels = pixels
atlas.filepath_raw = str(OUT / "atlas.png")
atlas.file_format = "PNG"
atlas.save()
atlas.pack()
MATERIAL = bpy.data.materials.new("Original atlas - nearest")
MATERIAL.use_nodes = True
nodes = MATERIAL.node_tree.nodes
shader = nodes.get("Principled BSDF")
shader.inputs["Roughness"].default_value = 0.85
texture = nodes.new("ShaderNodeTexImage")
texture.image = atlas
texture.interpolation = "Closest"
MATERIAL.node_tree.links.new(texture.outputs["Color"], shader.inputs["Base Color"])

joint("root", (0, 0, 0), (0, 0, 0.2))
joint("pelvis", (0, 0, 1), (0, 0, 1.22), "root")
joint("spine", (0, 0, 1.22), (0, -0.035, 1.65), "pelvis")
joint("head", (0, -0.035, 1.65), (0, -0.09, 1.98), "spine")
for side, sign in (("left", 1), ("right", -1)):
    joint("ear_" + side, (sign * 0.22, -0.01, 2.06), (sign * 0.29, 0.01, 2.38), "head")
    joint("upper_arm_" + side, (sign * 0.33, 0, 1.58), (sign * 0.49, 0.01, 1.22), "spine")
    joint("forearm_" + side, (sign * 0.49, 0.01, 1.22), (sign * 0.57, -0.045, 0.94), "upper_arm_" + side)
    joint("hand_" + side, (sign * 0.57, -0.045, 0.94), (sign * 0.61, -0.065, 0.77), "forearm_" + side)
    joint("thigh_" + side, (sign * 0.18, 0, 1.04), (sign * 0.22, -0.08, 0.60), "pelvis")
    joint("shin_" + side, (sign * 0.22, -0.08, 0.60), (sign * 0.22, 0.12, 0.25), "thigh_" + side)
    joint("foot_" + side, (sign * 0.22, 0.12, 0.25), (sign * 0.22, -0.16, 0.085), "shin_" + side)
joint("tail_base", (0, 0.12, 1.03), (0, 0.50, 0.87), "pelvis")
joint("tail_tip", (0, 0.50, 0.87), (0, 0.84, 0.65), "tail_base")

blob("Haunches", (0, 0.03, 1.01), (0.32, 0.23, 0.27), "fur", "pelvis")
blob("Torso", (0, 0, 1.38), (0.36, 0.24, 0.36), "fur", "spine", 10, 5)
blob("Chest ruff", (0, -0.214, 1.43), (0.18, 0.065, 0.28), "cream", "spine")
blob("Neck", (0, -0.015, 1.70), (0.23, 0.19, 0.21), "fur_light", "head")
blob("Broad head", (0, -0.06, 1.94), (0.35, 0.26, 0.29), "fur", "head", 10, 5)
blob("Brow", (0, -0.22, 2.04), (0.31, 0.115, 0.12), "fur_light", "head")
blob("Muzzle", (0, -0.31, 1.88), (0.235, 0.23, 0.145), "cream", "head")
blob("Nose", (0, -0.516, 1.915), (0.11, 0.065, 0.068), "nose", "head")
blob("Jaw", (0, -0.32, 1.775), (0.19, 0.17, 0.055), "fur_light", "head")
# Original narrow eyes and asymmetrical cheek tufts distinguish the face.
for side, s in (("left", 1), ("right", -1)):
    joint("eye_" + side, (s * .167, -.285, 2.03), (s * .167, -.285, 2.08), "head")
    blob("Eye socket " + side, (s * 0.167, -0.285, 2.03), (0.09, 0.042, 0.052), "nose", "eye_" + side)
    blob("Amber eye " + side, (s * 0.17, -0.323, 2.032), (0.045, 0.009, 0.028), "eye", "eye_" + side, 6, 3)
    blob("Pupil " + side, (s * 0.165, -0.332, 2.033), (0.014, 0.006, 0.026), "nose", "head", 6, 3)
    x = s * 0.24
    wedge("Ear " + side, [(x-s*.1,-.09,2.08),(x+s*.13,-.05,2.10),(x+s*.08,0,2.39),
                           (x-s*.1,.08,2.08),(x+s*.13,.09,2.10)],
          [(0,1,2),(3,2,4),(0,3,4,1),(0,2,3),(1,4,2)], "fur", "ear_" + side)
    wedge("Ear inset " + side, [(x-s*.045,-.096,2.13),(x+s*.07,-.066,2.14),(x+s*.062,-.011,2.32)],
          [(0,1,2)], "inner_ear", "ear_" + side)
    taper("Cheek tuft " + side, (s*.27,-.035,1.87), (s*.42,-.03,1.75), .12, .005, "fur_light", "head", 5)
    taper("Upper arm " + side, (s*.33,0,1.58), (s*.49,.01,1.22), .16,.12,"fur","upper_arm_"+side)
    blob("Elbow " + side, (s*.49,.01,1.22),(.12,.12,.13),"fur","forearm_"+side)
    taper("Forearm " + side,(s*.49,.01,1.22),(s*.57,-.045,.94),.12,.13,"fur_light","forearm_"+side)
    blob("Paw " + side,(s*.59,-.065,.85),(.155,.13,.16),"fur","hand_"+side)
    for i in range(3):
        px=s*.59+(i-1)*.065
        taper("Hand claw",(px,-.13,.78),(px,-.20,.725),.021,.003,"claw","hand_"+side,5)
    taper("Thigh " + side,(s*.18,0,1.04),(s*.22,-.08,.60),.18,.115,"fur","thigh_"+side)
    blob("Knee " + side,(s*.22,-.08,.60),(.115,.12,.13),"fur_light","shin_"+side)
    taper("Shin " + side,(s*.22,-.08,.60),(s*.22,.12,.25),.105,.07,"fur","shin_"+side)
    taper("Hock " + side,(s*.22,.12,.25),(s*.22,-.12,.11),.075,.105,"fur_light","foot_"+side)
    blob("Foot " + side,(s*.22,-.105,.085),(.15,.215,.085),"fur","foot_"+side)
    for i in range(3):
        px=s*.22+(i-1)*.075
        taper("Toe claw",(px,-.265,.065),(px,-.34,.035),.025,.003,"claw","foot_"+side,5)
    # Separate open shirt panels, with original pointed hem instead of traced cloth.
    wedge("Torn shirt panel " + side,
          [(s*.105,-.252,1.66),(s*.32,-.20,1.61),(s*.34,-.21,1.30),
           (s*.27,-.25,1.10),(s*.20,-.26,1.23),(s*.13,-.26,1.14),
           (s*.12,-.26,1.37)], [(0,1,2),(0,2,6),(6,2,3),(6,3,4),(6,4,5)], "shirt", "spine")
    taper("Torn sleeve " + side,(s*.35,0,1.58),(s*.425,.005,1.41),.171,.145,"shirt","upper_arm_"+side)
    wedge("Sleeve tear " + side,[(s*.44,-.14,1.42),(s*.39,-.15,1.36),(s*.43,-.15,1.34)],[(0,1,2)],"shirt","upper_arm_"+side)
blob("Shirt back",(0,.19,1.41),(.34,.065,.30),"shirt","spine")
taper("Tail base",(0,.12,1.03),(0,.50,.87),.11,.15,"fur","tail_base")
taper("Tail tip",(0,.50,.87),(0,.84,.65),.15,.025,"fur_light","tail_tip")

# A single mesh, one material, named deform bones; no authored animation clips.
bpy.ops.object.select_all(action="DESELECT")
for obj in PARTS:
    obj.select_set(True)
bpy.context.view_layer.objects.active = PARTS[0]
bpy.ops.object.join()
skin = bpy.context.object
skin.name = "WerewolfSkin"
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
arm_data = bpy.data.armatures.new("WerewolfDeformRig")
arm = bpy.data.objects.new("WerewolfRig", arm_data)
bpy.context.collection.objects.link(arm)
bpy.context.view_layer.objects.active = arm
bpy.ops.object.mode_set(mode="EDIT")
for j in JOINTS:
    bone = arm_data.edit_bones.new(j["name"])
    bone.head, bone.tail = j["head"], j["tail"]
    if j["parent"]:
        bone.parent = arm_data.edit_bones[j["parent"]]
bpy.ops.object.mode_set(mode="OBJECT")
modifier = skin.modifiers.new("Procedural skeletal deformation", "ARMATURE")
modifier.object = arm
skin.parent = arm
arm.show_in_front = True
arm_data.display_type = "STICK"
# Verify actual evaluated armature deformation and restored rest pose.
def evaluated_vertices():
    bpy.context.view_layer.update()
    evaluated = skin.evaluated_get(bpy.context.evaluated_depsgraph_get())
    mesh = evaluated.to_mesh()
    values = [v.co.copy() for v in mesh.vertices]
    evaluated.to_mesh_clear()
    return values

rest_vertices = evaluated_vertices()
assert max((v.co - p).length for v, p in zip(skin.data.vertices, rest_vertices)) < 1e-5
test_bone = arm.pose.bones["thigh_left"]
test_bone.rotation_mode = "XYZ"
test_bone.rotation_euler.x = .35
posed_vertices = evaluated_vertices()
deformation_distance = max((p - q).length for p, q in zip(rest_vertices, posed_vertices))
assert deformation_distance > .05
test_bone.rotation_euler.x = 0
assert max((p - q).length for p, q in zip(rest_vertices, evaluated_vertices())) < 1e-5
# Eye closure uses only dedicated eye vertices; all other anatomy is unchanged.
eye_vertices = {v.index for v in skin.data.vertices
                if any(skin.vertex_groups[g.group].name.startswith("eye_") and g.weight > .99
                       for g in v.groups)}
assert eye_vertices
for side in ("left", "right"):
    arm.pose.bones["eye_" + side].scale.y = .05
closed_vertices = evaluated_vertices()
eye_closure_distance = max((rest_vertices[i] - closed_vertices[i]).length for i in eye_vertices)
assert eye_closure_distance > .02
assert max((p - q).length for i, (p, q) in enumerate(zip(rest_vertices, closed_vertices))
           if i not in eye_vertices) < 1e-5
for side in ("left", "right"):
    arm.pose.bones["eye_" + side].scale.y = 1
assert max((p - q).length for p, q in zip(rest_vertices, evaluated_vertices())) < 1e-5
for bone in arm_data.bones:
    item = next(j for j in JOINTS if j["name"] == bone.name)
    item["rest_matrix_blender_column_major"] = [bone.matrix_local[r][c] for c in range(4) for r in range(4)]
    local = bone.parent.matrix_local.inverted() @ bone.matrix_local if bone.parent else bone.matrix_local
    item["rest_local_matrix_blender_column_major"] = [local[r][c] for c in range(4) for r in range(4)]
    item["rest_local_translation_blender"] = list(local.translation)

bpy.ops.object.select_all(action="DESELECT")
arm.select_set(True)
skin.select_set(True)
bpy.context.view_layer.objects.active = arm
bpy.ops.export_scene.gltf(filepath=str(OUT / "werewolf.glb"), export_format="GLB", use_selection=True,
                          export_animations=False, export_skins=True, export_def_bones=True,
                          export_all_influences=False, export_yup=True)
validation = validate_glb(OUT / "werewolf.glb")
validation["blender_version"] = bpy.app.version_string
validation["armature_deformation_test_max_distance_meters"] = deformation_distance
validation["eye_closure_test_max_distance_meters"] = eye_closure_distance
validation["eye_closure_vertex_count"] = len(eye_vertices)
(OUT / "validation.json").write_text(json.dumps(validation, indent=2) + "\n")
(OUT / "provenance.json").write_text(json.dumps({
    "authoring_source":"tools/author_werewolf.py",
    "source_sha256":hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
    "license":"MIT", "original_geometry":True,"original_texture":True,
    "reference":"https://dweller.pet/pets/howl/still.png",
    "reference_use":"Broad brown fur / pale muzzle / torn red shirt direction only; no redistributed reference pixels or traced mesh",
    "blender_version":bpy.app.version_string,
    "blender_build_hash":bpy.app.build_hash.decode(),
    "fallback_distribution": {
        "source":"official mirror.blender.org Blender3.6.0 macOS arm64",
        "vendor_sha256_verified_by_root":"ab4e1793343b3d22d7481d30d7f2817b7eb19c9dbdc505db3765336545746e63",
        "startup_backend":"opengl", "review_renderer":"CPU Cycles"
    } if bpy.app.version == (3,6,0) else None}, indent=2)+"\n")
(OUT / "rig.json").write_text(json.dumps({"version":1,"blender_axes":{"up":"+Z","forward":"-Y","right":"+X"},
    "gltf_axes":{"up":"+Y","forward":"+Z","right":"+X"},"units":"meters","floor_z_blender":0,
    "joint_data_space":"Blender armature rest space; convert (x,y,z) to glTF (x,z,-y)",
    "contacts":{"left":[.22,-.105,0],"right":[-.22,-.105,0]},"joints":JOINTS}, indent=2)+"\n")

scene = bpy.context.scene
bpy.context.preferences.filepaths.save_version = 0
scene.render.engine = "CYCLES"
scene.cycles.device = "CPU"
scene.cycles.samples = 32
scene.render.resolution_x = 512
scene.render.resolution_y = 512
scene.render.resolution_percentage = 100
scene.render.film_transparent = True
scene.view_settings.view_transform = "Standard"
scene.world.color = (.18,.18,.18)
bpy.ops.object.light_add(type="AREA", location=(-3,-4,5))
key = bpy.context.object
key.name = "Review key light"
key.data.energy=500
key.data.shape="DISK"
key.data.size=4
look_at(key,(0,0,1))
bpy.ops.object.light_add(type="AREA", location=(3,-1,2.5))
fill=bpy.context.object
fill.data.energy=180
fill.data.size=3
look_at(fill,(0,0,1))
bpy.ops.object.camera_add(location=(0,-6,2))
camera=bpy.context.object
camera.data.type="ORTHO"
camera.data.ortho_scale=2.85
scene.camera=camera
bpy.ops.wm.save_as_mainfile(filepath=str(OUT / "werewolf.blend"))
for name,pos in (("front",(0,-6,1.25)),("side",(6,0,1.25)),("three-quarter",(4,-6,2.2))):
    camera.location=pos
    look_at(camera,(0,.08,1.20))
    scene.render.filepath=str(OUT / (name+".png"))
    bpy.ops.render.render(write_still=True)
# Save editable source with review camera and packed original atlas.
bpy.ops.wm.save_as_mainfile(filepath=str(OUT / "werewolf.blend"))
print("WEREWOLF_VALIDATION " + json.dumps(validation))
