"""Original woodland pixel cat, authored/exported by Blender (no runtime dependency).

Run: blender --background --python tools/author_marmalade_3d.py
The character uses Z-up, -Y forward and meters; glTF exporter converts to Y-up.
"""
from pathlib import Path
import json
import hashlib
import math
import struct
import bpy
from mathutils import Vector

OUT = Path(__file__).resolve().parents[1] / "characters" / "marmalade-3d"
COLORS = {
    "fur": (.43,.205,.115,1), "fur_light": (.59,.31,.16,1),
    "dark": (.105,.075,.060,1), "cream": (.86,.66,.44,1),
    "eye": (.68,.40,.16,1), "ear": (.29,.13,.11,1),
    "ink": (.004,.003,.002,1), "copper": (.72,.43,.22,1), "shadow": (.23,.12,.08,1),
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
        loop.uv = ((n + 0.5) / 9, 0.5)
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
    assert triangles <= 5000
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
bpy.ops.object.select_all(action="SELECT"); bpy.ops.object.delete(use_global=False)
atlas=bpy.data.images.new("Marmalade original nine colour atlas",256,256,alpha=True)
atlas.pixels=[channel for y in range(256) for x in range(256) for channel in list(COLORS.values())[min(x*9//256,8)]]
atlas.filepath_raw=str(OUT/"atlas.png"); atlas.file_format="PNG"; atlas.save(); atlas.pack()
MATERIAL=bpy.data.materials.new("Woodland nearest atlas"); MATERIAL.use_nodes=True
shader=MATERIAL.node_tree.nodes.get("Principled BSDF"); shader.inputs["Roughness"].default_value=.94
tex=MATERIAL.node_tree.nodes.new("ShaderNodeTexImage"); tex.image=atlas; tex.interpolation="Closest"
MATERIAL.node_tree.links.new(tex.outputs["Color"],shader.inputs["Base Color"])
# Art study uses x forward, y depth, z height. Runtime uses Blender -Y forward.
def P(p): return (p[1],-p[0],p[2])
def E(name,p,s,color,bone,seg=12,rings=6):
    return blob(name,P(p),(s[1],s[0],s[2]),color,bone,seg,rings)
def W(name,points,faces,color,bone): return wedge(name,[P(p) for p in points],faces,color,bone)
def tube(name,points,radii,color,bone,weights=None,sides=8):
    vs=[]; fs=[]
    for i,point in enumerate(points):
        tangent=(Vector(points[min(i+1,len(points)-1)])-Vector(points[max(0,i-1)])).normalized()
        u=tangent.cross(Vector((0,1,0))).normalized(); v=tangent.cross(u).normalized()
        for j in range(sides):
            angle=j*math.tau/sides; vs.append(P(Vector(point)+radii[i]*(math.cos(angle)*u+math.sin(angle)*v)))
    for i in range(len(points)-1):
        for j in range(sides): fs.append((i*sides+j,i*sides+(j+1)%sides,(i+1)*sides+(j+1)%sides,(i+1)*sides+j))
    fs += [tuple(reversed(range(sides))),tuple((len(points)-1)*sides+j for j in range(sides))]
    obj=wedge(name,vs,fs,color,bone)
    if weights:
        for group in list(obj.vertex_groups):obj.vertex_groups.remove(group)
        groups={n:obj.vertex_groups.new(name=n) for n in dict.fromkeys(n for ring in weights for n in ring)}
        for ring,values in enumerate(weights):
            for n,w in values.items():
                if w:groups[n].add(list(range(ring*sides,(ring+1)*sides)),w,"REPLACE")
    return obj
joint("root",(0,0,0),(0,0,.15))
joint("pelvis",P((-.05,0,1.02)),P((.1,0,1.02)),"root")
joint("spine",P((.1,0,1.02)),P((.79,0,1.23)),"pelvis")
joint("head",P((.79,0,1.23)),P((1.01,0,1.43)),"spine")
joint("tail_base",P((-.88,0,1.10)),P((-1.5,.1,1.43)),"pelvis")
joint("tail_tip",P((-1.5,.1,1.43)),P((-1.42,.17,1.90)),"tail_base")
E("full ribcage",(-.05,0,1.02),(.89,.39,.42),"fur","spine")
E("scapula",(.56,0,1.11),(.39,.40,.44),"fur_light","spine")
E("haunch",(-.66,0,1),(.43,.42,.46),"fur","pelvis")
E("short neck",(.79,0,1.23),(.31,.31,.32),"fur_light","head")
E("compact feline head",(1.01,0,1.43),(.315,.305,.28),"fur_light","head")
E("short muzzle",(1.23,-.005,1.32),(.135,.255,.125),"cream","head")
E("small nose",(1.35,-.005,1.37),(.042,.080,.045),"ink","head",8,4)
E("chest bib",(.87,0,1),(.18,.31,.24),"cream","spine")
for side,y in (("left",-.215),("right",.215)):
    joint("ear_"+side,P((.96,y,1.51)),P((.88,y-.035,1.88)),"head")
    joint("eye_"+side,P((1.15,y*1.75,1.54)),P((1.15,y*1.75,1.60)),"head")
    W("short angular ear",[(.80,y-.145,1.51),(1.16,y-.1,1.51),(.88,y-.035,1.88),(.81,y+.13,1.51),(1.15,y+.1,1.51)],[(0,1,2),(1,4,2),(4,3,2),(3,0,2),(0,3,4,1)],"fur","ear_"+side)
    sign=1 if y>0 else -1
    W("single cheek plane",[(.94,y,1.48),(.86,y+sign*.15,1.23),(.65,y+sign*.12,1.31),(.76,y,1.57),(.88,y-sign*.1,1.37)],[(0,1,2,3),(0,4,1),(1,4,2),(2,4,3),(3,4,0)],"fur_light","head")
    E("small readable eye",(1.15,y*1.75,1.54),(.078,.055,.058),"ink","eye_"+side,8,4)
for x in [-.58,-.15,.27]:
    tube("broad dorsal band",[(x,-.04,1.43),(x+.045,-.25,1.36),(x+.07,-.38,1.19)],[.075,.065,.04],"dark","spine",sides=5)
contacts={}; chains={}
for role in ("foreLeft","foreRight","hindLeft","hindRight"):
    fore=role.startswith("fore"); depth=-.28 if role.endswith("Left") else .28
    x=.65 if fore else -.68
    hip=Vector((x,depth,1.04)); knee=Vector((x+(-.13 if fore else .24),depth,.57))
    ankle=Vector((x-.035,depth,.17)); foot=Vector((x+.065,depth,.095))
    thigh="thigh_"+role; shin="shin_"+role; paw="foot_"+role
    joint(thigh,P(hip),P(knee),"spine" if fore else "pelvis")
    joint(shin,P(knee),P(ankle),thigh); joint(paw,P(ankle),P((x+.17,depth,.095)),shin)
    contacts[role]=list(P((foot.x,depth,0)))
    chains[role]={"hip":thigh,"knee":shin,"ankle":paw,"bend":-1 if fore else 1}
    points=[hip,hip.lerp(knee,.5),knee,knee.lerp(ankle,.5),ankle,foot]
    weights=[{thigh:1},{thigh:.9,shin:.1},{thigh:.5,shin:.5},{shin:1},{shin:.5,paw:.5},{paw:1}]
    tube("continuous leg "+role,points,[.18 if fore else .23,.16 if fore else .19,.13,.1,.085,.1],"fur",thigh,weights)
    E("paw "+role,foot,(.17,.13,.095),"cream" if fore else "fur",paw)
tail=[(-.88,0,1.10),(-1.20,.04,1.18),(-1.5,.1,1.43),(-1.61,.14,1.76),(-1.42,.17,1.9)]
tw=[{"tail_base":1},{"tail_base":.9,"tail_tip":.1},{"tail_base":.5,"tail_tip":.5},{"tail_tip":1},{"tail_tip":1}]
tube("continuous curved tail",tail,[.19,.2,.18,.145,.065],"fur","tail_base",tw)
for i in [1,2]:
    a=Vector(tail[i]).lerp(Vector(tail[i+1]),.55); b=Vector(tail[i]).lerp(Vector(tail[i+1]),.73)
    tube("tail stripe",[a,b],[.205-i*.018,.201-i*.018],"dark","tail_base",[tw[i],tw[i+1]])
bpy.ops.object.select_all(action="DESELECT")
for obj in PARTS:
    obj.select_set(True)
bpy.context.view_layer.objects.active = PARTS[0]
bpy.ops.object.join()
skin = bpy.context.object
skin.name = "MarmaladeSkin"
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
arm_data = bpy.data.armatures.new("MarmaladeDeformRig")
arm = bpy.data.objects.new("MarmaladeRig", arm_data)
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
test_bone = arm.pose.bones["thigh_foreLeft"]
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
assert eye_closure_distance > .015
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
bpy.ops.export_scene.gltf(filepath=str(OUT / "marmalade.glb"), export_format="GLB", use_selection=True,
                          export_animations=False, export_skins=True, export_def_bones=True,
                          export_all_influences=False, export_yup=True)
validation = validate_glb(OUT / "marmalade.glb")
validation["blender_version"] = bpy.app.version_string
validation["armature_deformation_test_max_distance_meters"] = deformation_distance
validation["eye_closure_test_max_distance_meters"] = eye_closure_distance
validation["eye_closure_vertex_count"] = len(eye_vertices)
(OUT / "validation.json").write_text(json.dumps(validation, indent=2) + "\n")

(OUT/"provenance.json").write_text(json.dumps({"authoring_source":"tools/author_marmalade_3d.py","source_sha256":hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),"license":"MIT","original_geometry":True,"original_texture":True,"art_direction":"Approved B woodland-refined study; original geometry and palette","reference":"art-studies/marmalade/woodland-refined/author_render.py","blender_version":bpy.app.version_string},indent=2)+"\n")
rig={"version":1,"blender_axes":{"up":"+Z","forward":"-Y","right":"+X"},"gltf_axes":{"up":"+Y","forward":"+Z","right":"+X"},"units":"meters","floor_z_blender":0,"joint_data_space":"Blender armature rest space; convert (x,y,z) to glTF (x,z,-y)","gait":"quadrupedWalk","contacts":contacts,"legs":chains,"joints":JOINTS}
(OUT/"rig.json").write_text(json.dumps(rig,indent=2)+"\n")
import sys
sys.path.insert(0,str(Path(__file__).resolve().parent))
from measure_raven_bounds import update
update(OUT/"marmalade.glb",OUT/"rig.json")
(OUT/"manifest.json").write_text(json.dumps({"version":1,"kind":"builtin-3d","id":"marmalade-3d","name":"Marmalade 3D","asset":"marmalade","license":"LICENSE"})+"\n")
(OUT/"LICENSE").write_text("MIT License\n\nCopyright (c) 2026 Better Pets contributors\n\nPermission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the Software), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:\n\nThe above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.\n\nTHE SOFTWARE IS PROVIDED AS IS, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n")
scene=bpy.context.scene; scene.render.engine="CYCLES"; scene.cycles.device="CPU"; scene.cycles.samples=4
scene.render.resolution_x=96; scene.render.resolution_y=96; scene.render.resolution_percentage=100
scene.render.film_transparent=True; scene.render.filter_size=.01; scene.view_settings.view_transform="Standard"; scene.view_settings.exposure=.28
scene.world.color=(.55,.55,.55)
for name,pos,power,size in (("soft key",(-4,3,7),550,5),("fill",(4,-3,4),220,4),("face fill",(-5,-4,3),130,4)):
    bpy.ops.object.light_add(type="AREA",location=pos); lamp=bpy.context.object; lamp.name=name; lamp.data.energy=power; lamp.data.size=size; look_at(lamp,(0,0,1))
bpy.ops.object.camera_add(location=P((1.9,-10,3.8))); camera=bpy.context.object; camera.data.type="ORTHO"; camera.data.ortho_scale=4.8; scene.camera=camera
bpy.context.preferences.filepaths.save_version=0
for name,pos in (("side",P((1.9,-10,3.8))),("front",(0,-10,2.8)),("three-quarter",(-6,-8,3.8))):
    camera.location=pos; look_at(camera,(0,0,1.04)); scene.render.filepath=str(OUT/(name+".png")); bpy.ops.render.render(write_still=True)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/"marmalade.blend"))
print("MARMALADE_VALIDATION "+json.dumps(validation))
