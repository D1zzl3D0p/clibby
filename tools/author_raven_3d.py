"""Original low-poly raven, authored/exported by Blender (no runtime dependency).

Run: blender --background --python tools/author_raven_3d.py
The character uses Z-up, -Y forward and meters; glTF exporter converts to Y-up.
"""
from pathlib import Path
import json
import hashlib
import math
import struct
import bpy
from mathutils import Vector

OUT = Path(__file__).resolve().parents[1] / "characters" / "raven-3d"
COLORS = {
    "fur": (0.035, 0.05, 0.085, 1), "fur_light": (0.08, 0.12, 0.19, 1),
    "cream": (0.78, 0.64, 0.43, 1), "shirt": (0.49, 0.035, 0.045, 1),
    "nose": (0.065, 0.046, 0.036, 1), "eye": (0.18, 0.25, 0.34, 1),
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
atlas = bpy.data.images.new("Raven original color atlas", 256, 256, alpha=True)
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

# Original bird anatomy: forward -Y, low horizontal body, folded flight feathers.
# A feather is a thin closed broad vane, not a cone or skeletal stick.
def feather(name, start, end, width, color, bone):
    a,b=Vector(start),Vector(end)
    direction=(b-a).normalized()
    normal=Vector((0,0,1)) if "tail" in name.lower() else Vector((1,0,0))
    transverse=normal.cross(direction).normalized()*width
    points=[a-transverse*.45,a+transverse*.45,a+(b-a)*.60+transverse*.52,
            b,a+(b-a)*.60-transverse*.52]
    thickness=normal*.009
    vertices=[tuple(v+thickness) for v in points]+[tuple(v-thickness) for v in points]
    faces=[(0,1,2,3,4),(9,8,7,6,5)]+[(i,(i+1)%5,(i+1)%5+5,i+5) for i in range(5)]
    return wedge(name,vertices,faces,color,bone)

def wing_cover(side,sign):
    """One continuous weighted skin tube covers shoulder/elbow/wrist seams."""
    chain=[Vector((sign*.27,-.30,.94)),Vector((sign*.43,.32,.82)),Vector((sign*.38,-.18,.72))]
    names=["wing_shoulder_"+side,"wing_elbow_"+side,"wing_wrist_"+side]
    vertices=[];weights=[]
    for segment in range(2):
        for sample in range(5):
            if segment and sample==0:continue
            fraction=sample/4
            center=chain[segment].lerp(chain[segment+1],fraction)
            for radial in range(8):
                angle=radial*math.tau/8
                vertices.append(tuple(center+Vector((sign*.10*math.cos(angle),0,.115*math.sin(angle)))))
                weights.append((segment,fraction))
    rings=len(vertices)//8
    faces=[(ring*8+n,ring*8+(n+1)%8,(ring+1)*8+(n+1)%8,(ring+1)*8+n) for ring in range(rings-1) for n in range(8)]
    faces.extend([tuple(reversed(range(8))),tuple((rings-1)*8+n for n in range(8))])
    obj=wedge("Continuous wing seam cover "+side,vertices,faces,"fur_light",names[0])
    for group in list(obj.vertex_groups):obj.vertex_groups.remove(group)
    groups=[obj.vertex_groups.new(name=name) for name in names]
    for index,(segment,fraction) in enumerate(weights):
        if fraction<1:groups[segment].add([index],1-fraction,"REPLACE")
        if fraction>0:groups[segment+1].add([index],fraction,"REPLACE")

joint("root",(0,0,0),(0,0,.15))
joint("pelvis",(0,.12,.56),(0,.10,.72),"root")
joint("spine",(0,.10,.72),(0,-.35,.90),"pelvis")
joint("head",(0,-.35,.90),(0,-.61,1.16),"spine")
joint("tail_base",(0,.52,.68),(0,.86,.56),"pelvis")
joint("tail_tip",(0,.86,.56),(0,1.39,.39),"tail_base")
blob("Long horizontal breast and back",(0,.06,.75),(.34,.65,.31),"fur","spine",14,7)
blob("Full forward breast",(0,-.38,.79),(.27,.33,.34),"fur","spine",12,6)
blob("Thick neck",(0,-.46,.98),(.24,.26,.31),"fur","head",12,6)
blob("Substantial raven head",(0,-.64,1.17),(.245,.29,.225),"fur","head",12,6)
wedge("Heavy arched bill",[(-.125,-.79,1.24),(.125,-.79,1.24),(0,-1.16,1.13),
    (-.09,-.80,1.06),(.09,-.80,1.06),(0,-1.12,1.09)],
    [(0,1,2),(0,2,5,3),(1,4,5,2),(3,5,4),(0,3,4,1)],"nose","head")
for side,sgn in (("left",1),("right",-1)):
    joint("thigh_"+side,(sgn*.15,.10,.59),(sgn*.18,-.03,.32),"pelvis")
    joint("shin_"+side,(sgn*.18,-.03,.32),(sgn*.18,.09,.13),"thigh_"+side)
    joint("foot_"+side,(sgn*.18,.09,.13),(sgn*.18,-.13,.035),"shin_"+side)
    # Folded wing: long upper wing runs aft, forearm returns forward; primaries
    # overlap rearward on the flank. Their unfolded chain is bird-sized.
    joint("wing_shoulder_"+side,(sgn*.27,-.30,.94),(sgn*.43,.32,.82),"spine")
    joint("wing_elbow_"+side,(sgn*.43,.32,.82),(sgn*.38,-.18,.72),"wing_shoulder_"+side)
    joint("wing_wrist_"+side,(sgn*.38,-.18,.72),(sgn*.42,1.04,.46),"wing_elbow_"+side)
    joint("eye_"+side,(sgn*.216,-.764,1.20),(sgn*.216,-.764,1.25),"head")
    blob("Visible slate eye rim "+side,(sgn*.231,-.770,1.20),(.052,.026,.047),"eye","eye_"+side,10,5)
    blob("Black eye "+side,(sgn*.247,-.788,1.20),(.037,.018,.035),"nose","eye_"+side,8,4)
    blob("Eye glint "+side,(sgn*.263,-.801,1.217),(.017,.012,.013),"claw","eye_"+side,6,3)
    # Thigh is buried within body plumage; only a short black tarsus is visible.
    blob("Hidden feathered thigh "+side,(sgn*.15,.04,.48),(.10,.16,.14),"fur","thigh_"+side)
    taper("Short tarsus "+side,(sgn*.18,-.03,.32),(sgn*.18,.09,.13),.027,.019,"nose","shin_"+side,6)
    taper("Foot "+side,(sgn*.18,.09,.13),(sgn*.18,-.10,.035),.020,.014,"nose","foot_"+side,6)
    for n in range(3):
        taper("Forward toe",(sgn*.18,-.08,.035),(sgn*.18+(n-1)*.10,-.27,.018),.016,.003,"nose","foot_"+side,5)
    taper("Rear toe",(sgn*.18,-.06,.035),(sgn*.18,.20,.018),.014,.003,"nose","foot_"+side,5)
    wing_cover(side,sgn)
    # Overlapping coverts define a broad solid flank, secondary vanes its edge.
    blob("Broad folded wing mantle "+side,(sgn*.29,.10,.82),(.10,.47,.22),"fur_light","wing_shoulder_"+side,12,6)
    for n in range(7):
        feather("Overlapping secondary "+side,(sgn*(.33+n*.009),-.12+n*.08,.91-n*.026),
                (sgn*(.35+n*.015),.67+n*.026,.53+n*.018),.17,"fur_light" if n%3==0 else "fur","wing_elbow_"+side)
    for n in range(9):
        feather("Layered broad primary "+side,(sgn*(.34+n*.012),-.16+n*.025,.75-n*.015),
                (sgn*(.39+n*.017),.95+n*.028,.44+n*.014),.13,"fur_light" if n%3==0 else "fur","wing_wrist_"+side)
    for n in range(5):
        feather("Shaggy throat hackle "+side,(sgn*(.025+n*.036),-.57,.95-n*.015),
                (sgn*(.035+n*.036),-.54,.72-n*.025),.065,"fur_light" if n%2 else "fur","head")
blob("Full tail covert mound",(0,.65,.61),(.27,.31,.09),"fur","tail_base",12,5)
for n in range(11):
    offset=n-5
    feather("Long wedge tail feather",(offset*.050,.43,.65),
            (offset*.074,1.46-abs(offset)*.065,.35+abs(offset)*.016),.22,
            "fur_light" if n%3==0 else "fur","tail_tip")
# A single mesh, one material, named deform bones; no authored animation clips.
bpy.ops.object.select_all(action="DESELECT")
for obj in PARTS:
    obj.select_set(True)
bpy.context.view_layer.objects.active = PARTS[0]
bpy.ops.object.join()
skin = bpy.context.object
skin.name = "RavenSkin"
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
# Every moving wing seam needs blended cover geometry, not separated rigid pieces.
wing_groups={group.index:group.name for group in skin.vertex_groups if group.name.startswith("wing_")}
for side in ("left","right"):
    for seam in (("wing_shoulder_"+side,"wing_elbow_"+side),("wing_elbow_"+side,"wing_wrist_"+side)):
        assert any(set(seam).issubset({wing_groups[g.group] for g in vertex.groups if g.group in wing_groups and g.weight>.01}) for vertex in skin.data.vertices), "Uncovered rigid wing seam: "+str(seam)
arm_data = bpy.data.armatures.new("RavenDeformRig")
arm = bpy.data.objects.new("RavenRig", arm_data)
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
bpy.ops.export_scene.gltf(filepath=str(OUT / "raven.glb"), export_format="GLB", use_selection=True,
                          export_animations=False, export_skins=True, export_def_bones=True,
                          export_all_influences=False, export_yup=True)
validation = validate_glb(OUT / "raven.glb")
validation["blender_version"] = bpy.app.version_string
validation["armature_deformation_test_max_distance_meters"] = deformation_distance
validation["eye_closure_test_max_distance_meters"] = eye_closure_distance
validation["eye_closure_vertex_count"] = len(eye_vertices)
(OUT / "validation.json").write_text(json.dumps(validation, indent=2) + "\n")
(OUT / "provenance.json").write_text(json.dumps({
    "authoring_source":"tools/author_raven_3d.py",
    "skin_bounds_source":"tools/measure_raven_bounds.py",
    "source_sha256":hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
    "license":"MIT", "original_geometry":True,"original_texture":True,
    "reference":"https://www.allaboutbirds.org/guide/Common_Raven/id",
    "reference_use":"Anatomical proportions only: heavy bill, thick neck, shaggy throat, wedge tail; original mesh and texture, no copied art",
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
    "contacts":{"left":[.18,-.10,0],"right":[-.18,-.10,0]},"joints":JOINTS}, indent=2)+"\n")

# Preserve measured anatomical dimensions for renderer calibration/review.
rig_path=OUT / "rig.json"
rig_data=json.loads(rig_path.read_text())
wing_lengths=[math.dist(j["head"],j["tail"]) for j in JOINTS if j["name"] in
              ("wing_shoulder_left","wing_elbow_left","wing_wrist_left")]
rig_data["proportions"]={"bill_to_tail_length":2.58,
    "unfolded_chain_span":2*sum(wing_lengths)+.54,
    "wing_chain_segment_lengths":wing_lengths,
    "wing_vane_rest_plane":"Blender YZ flank plane, normal X; tail XY horizontal plane",
    "note":"Stylized original raven geometry; full span assumes unfolded measured chain and overlapping feather vanes."}
rig_path.write_text(json.dumps(rig_data,indent=2)+"\n")

# Derive runtime envelopes from the exported skin, including blended seams.
import sys
sys.path.insert(0,str(Path(__file__).resolve().parent))
from measure_raven_bounds import update as update_skin_bounds
update_skin_bounds(OUT / "raven.glb",OUT / "rig.json")

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
camera.data.ortho_scale=3.2
scene.camera=camera
bpy.ops.wm.save_as_mainfile(filepath=str(OUT / "raven.blend"))
for name,pos in (("front",(0,-6,1.2)),("side",(6,0,.9)),("three-quarter",(4,-6,1.9))):
    camera.location=pos
    look_at(camera,(0,.08,.71))
    scene.render.filepath=str(OUT / (name+".png"))
    bpy.ops.render.render(write_still=True)
# Save editable source with review camera and packed original atlas.
bpy.ops.wm.save_as_mainfile(filepath=str(OUT / "raven.blend"))
print("RAVEN_VALIDATION " + json.dumps(validation))
