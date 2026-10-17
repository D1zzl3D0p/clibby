"""Small original, palette-textured Blender assets for the procedural catalogue.

Offline authoring only. Native Blender coordinates: X right, Y back, Z up.
"""
from pathlib import Path
import hashlib
import json
import math
import struct
import bpy
from mathutils import Vector


class Asset:
    def __init__(self, asset_id, display_name, palette, skin_name=None):
        bpy.ops.object.select_all(action='SELECT')
        bpy.ops.object.delete(use_global=False)
        self.id, self.name = asset_id, display_name
        self.out = Path(__file__).resolve().parents[1] / 'characters' / (asset_id + '-3d')
        self.out.mkdir(parents=True, exist_ok=True)
        self.skin_name = skin_name or ''.join(s.title() for s in asset_id.split('-')) + 'Skin'
        self.joints, self.parts, self.weighted_tubes = [], [], []
        self.palette = palette
        assert 2 <= len(palette) <= 12
        atlas = bpy.data.images.new(display_name + ' original palette', 256, 256, alpha=True)
        colors = list(palette.values())
        atlas.pixels = [c for y in range(256) for x in range(256)
                        for c in colors[min(x * len(colors) // 256, len(colors) - 1)]]
        atlas.filepath_raw = str(self.out / 'atlas.png')
        atlas.file_format = 'PNG'
        atlas.save()
        atlas.pack()
        self.material = bpy.data.materials.new(display_name + ' nearest palette')
        self.material.use_nodes = True
        shader = self.material.node_tree.nodes.get('Principled BSDF')
        shader.inputs['Roughness'].default_value = .94
        tex = self.material.node_tree.nodes.new('ShaderNodeTexImage')
        tex.image, tex.interpolation = atlas, 'Closest'
        self.material.node_tree.links.new(tex.outputs['Color'], shader.inputs['Base Color'])

    def joint(self, name, head, tail, parent=None):
        assert name not in [j['name'] for j in self.joints]
        assert (Vector(tail) - Vector(head)).length > 1e-5
        self.joints.append(dict(name=name, head=list(head), tail=list(tail), parent=parent))

    def shade(self, obj, color, bone):
        obj.data.materials.append(self.material)
        uv = obj.data.uv_layers.active or obj.data.uv_layers.new(name='Atlas')
        uv.name, uv.active_render = 'Atlas', True
        n = list(self.palette).index(color)
        for loop in uv.data:
            loop.uv = ((n + .5) / len(self.palette), .5)
        group = obj.vertex_groups.new(name=bone)
        group.add(list(range(len(obj.data.vertices))), 1, 'REPLACE')
        self.parts.append(obj)
        return obj

    def blob(self, name, pos, scale, color, bone, segments=10, rings=5):
        bpy.ops.mesh.primitive_uv_sphere_add(segments=segments, ring_count=rings, location=pos)
        obj = bpy.context.object
        obj.name, obj.scale = name, scale
        bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
        return self.shade(obj, color, bone)

    def box(self, name, pos, scale, color, bone):
        bpy.ops.mesh.primitive_cube_add(size=2, location=pos)
        obj = bpy.context.object
        obj.name, obj.scale = name, scale
        bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
        return self.shade(obj, color, bone)

    def taper(self, name, start, end, r1, r2, color, bone, vertices=8):
        direction = Vector(end) - Vector(start)
        bpy.ops.mesh.primitive_cone_add(vertices=vertices, radius1=r1, radius2=r2,
                                      depth=direction.length, location=(Vector(start)+Vector(end))/2)
        obj = bpy.context.object
        obj.name = name
        obj.rotation_euler = direction.to_track_quat('Z', 'Y').to_euler()
        bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
        return self.shade(obj, color, bone)

    def wedge(self, name, vertices, faces, color, bone):
        mesh = bpy.data.meshes.new(name)
        mesh.from_pydata(vertices, [], faces)
        mesh.update()
        obj = bpy.data.objects.new(name, mesh)
        bpy.context.collection.objects.link(obj)
        return self.shade(obj, color, bone)

    def tube(self, name, points, radii, color, bone, weights=None, sides=8):
        vertices, faces = [], []
        for i, p in enumerate(points):
            tangent = (Vector(points[min(i+1,len(points)-1)]) - Vector(points[max(0,i-1)])).normalized()
            axis = Vector((0,1,0)) if abs(tangent.y) < .9 else Vector((1,0,0))
            u = tangent.cross(axis).normalized()
            v = tangent.cross(u).normalized()
            for j in range(sides):
                angle = j * math.tau / sides
                vertices.append(Vector(p) + radii[i] * (math.cos(angle)*u + math.sin(angle)*v))
        for i in range(len(points)-1):
            for j in range(sides):
                faces.append((i*sides+j, i*sides+(j+1)%sides,
                              (i+1)*sides+(j+1)%sides, (i+1)*sides+j))
        faces += [tuple(reversed(range(sides))), tuple((len(points)-1)*sides+j for j in range(sides))]
        obj = self.wedge(name, vertices, faces, color, bone)
        if weights:
            assert len(weights) == len(points)
            for group in list(obj.vertex_groups):
                obj.vertex_groups.remove(group)
            groups = {n:obj.vertex_groups.new(name=n) for n in dict.fromkeys(n for ring in weights for n in ring)}
            for ring, values in enumerate(weights):
                assert abs(sum(values.values()) - 1) < 1e-6 and len(values) <= 2
                for n, w in values.items():
                    if w:
                        groups[n].add(list(range(ring*sides,(ring+1)*sides)), w, 'REPLACE')
            self.weighted_tubes.append(name)
        return obj

    def biped(self, hip_z=.65, width=.25, knee_y=-.08, radius=.11,
              foot_scale=(.16,.24,.09), color='body', feet_color='dark'):
        contacts = {}
        for side, x in [('left',width),('right',-width)]:
            hip = Vector((x,0,hip_z))
            knee = Vector((x,knee_y,(hip_z+.13)/2))
            ankle = Vector((x,0,.13))
            foot = Vector((x,-.10,foot_scale[2]))
            thigh, shin, paw = ['%s_%s' % (p,side) for p in ('thigh','shin','foot')]
            self.joint(thigh, hip, knee, 'pelvis')
            self.joint(shin, knee, ankle, thigh)
            self.joint(paw, ankle, (x,-.28,.09), shin)
            self.tube('continuous leg '+side, [hip,hip.lerp(knee,.5),knee,knee.lerp(ankle,.5),ankle,foot],
                      [radius*1.2,radius,radius*.85,radius*.75,radius*.7,radius*.8], color, thigh,
                      [{thigh:1},{thigh:.9,shin:.1},{thigh:.5,shin:.5},{shin:1},{shin:.5,paw:.5},{paw:1}])
            self.blob('planted foot '+side, foot, foot_scale, feet_color, paw)
            contacts[side] = [x,-.10,0]
        return contacts

    def eyes(self, z, y, spacing=.17, size=.045, color='ink'):
        for side, x in [('left',spacing),('right',-spacing)]:
            bone = 'eye_'+side
            self.joint(bone,(x,y,z),(x,y,z+.07),'head')
            self.blob('eye '+side,(x,y,z),(size,.025,size*.82),color,bone,8,4)

    def finish(self, source_path, contacts, legs=None, gait=None, arms=False, presentation=None):
        bpy.ops.object.select_all(action='DESELECT')
        for obj in self.parts:
            obj.select_set(True)
        bpy.context.view_layer.objects.active = self.parts[0]
        bpy.ops.object.join()
        skin = bpy.context.object
        skin.name = self.skin_name
        bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
        data = bpy.data.armatures.new(self.skin_name+'RigData')
        arm = bpy.data.objects.new(self.skin_name+'Rig',data)
        bpy.context.collection.objects.link(arm)
        bpy.context.view_layer.objects.active = arm
        bpy.ops.object.mode_set(mode='EDIT')
        for j in self.joints:
            bone = data.edit_bones.new(j['name'])
            bone.head, bone.tail = j['head'], j['tail']
            if j['parent']:
                bone.parent = data.edit_bones[j['parent']]
        bpy.ops.object.mode_set(mode='OBJECT')
        modifier = skin.modifiers.new('Procedural deformation','ARMATURE')
        modifier.object, skin.parent = arm, arm
        def evaluated():
            bpy.context.view_layer.update()
            obj = skin.evaluated_get(bpy.context.evaluated_depsgraph_get())
            mesh = obj.to_mesh()
            points = [v.co.copy() for v in mesh.vertices]
            obj.to_mesh_clear()
            return points
        rest = evaluated()
        assert max((v.co-p).length for v,p in zip(skin.data.vertices,rest)) < 1e-5
        test = next(b for b in arm.pose.bones if b.name.startswith('thigh_'))
        test.rotation_mode = 'XYZ'
        test.rotation_euler.x = .35
        distance = max((p-q).length for p,q in zip(rest,evaluated()))
        assert distance > .01
        test.rotation_euler.x = 0
        assert max((p-q).length for p,q in zip(rest,evaluated())) < 1e-5
        eye_vertices = {v.index for v in skin.data.vertices
                        if any(skin.vertex_groups[g.group].name.startswith('eye_') and g.weight > .99
                               for g in v.groups)}
        eye_distance = 0
        if eye_vertices:
            for bone in arm.pose.bones:
                if bone.name.startswith('eye_'):
                    bone.scale.y = .05
            closed = evaluated()
            eye_distance = max((rest[i]-closed[i]).length for i in eye_vertices)
            assert eye_distance > .005
            assert max((p-q).length for i,(p,q) in enumerate(zip(rest,closed)) if i not in eye_vertices) < 1e-5
            for bone in arm.pose.bones:
                if bone.name.startswith('eye_'):
                    bone.scale.y = 1
            assert max((p-q).length for p,q in zip(rest,evaluated())) < 1e-5
        for bone in data.bones:
            item = next(j for j in self.joints if j['name'] == bone.name)
            local = bone.parent.matrix_local.inverted() @ bone.matrix_local if bone.parent else bone.matrix_local
            item['rest_matrix_blender_column_major'] = [bone.matrix_local[r][c] for c in range(4) for r in range(4)]
            item['rest_local_matrix_blender_column_major'] = [local[r][c] for c in range(4) for r in range(4)]
            item['rest_local_translation_blender'] = list(local.translation)
        bpy.ops.object.select_all(action='DESELECT')
        arm.select_set(True)
        skin.select_set(True)
        bpy.context.view_layer.objects.active = arm
        path = self.out / (self.id+'.glb')
        bpy.ops.export_scene.gltf(filepath=str(path),export_format='GLB',use_selection=True,
                                  export_animations=False,export_skins=True,export_def_bones=True,
                                  export_all_influences=False,export_yup=True)
        validation = validate_glb(path, {j['name'] for j in self.joints})
        validation.update(blender_version=bpy.app.version_string,
                          armature_deformation_test_max_distance_meters=distance,
                          eye_closure_test_max_distance_meters=eye_distance,
                          eye_closure_vertex_count=len(eye_vertices),
                          continuous_weighted_legs=sum(n.startswith('continuous leg') for n in self.weighted_tubes))
        (self.out/'validation.json').write_text(json.dumps(validation,indent=2)+'\n')
        rig = dict(version=1,blender_axes=dict(up='+Z',forward='-Y',right='+X'),
                   gltf_axes=dict(up='+Y',forward='+Z',right='+X'),units='meters',floor_z_blender=0,
                   joint_data_space='Blender armature rest space; convert (x,y,z) to glTF (x,z,-y)',
                   contacts=contacts,joints=self.joints,skin=self.skin_name,arms=arms)
        if legs is not None: rig['legs'] = legs
        if gait is not None: rig['gait'] = gait
        if presentation is not None: rig['presentation'] = presentation
        (self.out/'rig.json').write_text(json.dumps(rig,indent=2)+'\n')
        from measure_raven_bounds import update
        update(path,self.out/'rig.json')
        source = Path(source_path).resolve()
        (self.out/'provenance.json').write_text(json.dumps(dict(authoring_source=str(source.relative_to(self.out.parents[1])),
             source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
             helper_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),license='MIT',
             original_geometry=True,original_texture=True,art_direction='Refined pixel-rendered low-poly catalogue; original authored forms',
             blender_version=bpy.app.version_string),indent=2)+'\n')
        (self.out/'manifest.json').write_text(json.dumps(dict(version=1,kind='builtin-3d',id=self.id+'-3d',
             name=self.name+' 3D',asset=self.id,license='LICENSE'))+'\n')
        (self.out/'LICENSE').write_text((self.out.parent/'marmalade-3d'/'LICENSE').read_text())
        scene = bpy.context.scene
        scene.render.engine, scene.cycles.device, scene.cycles.samples = 'CYCLES','CPU',4
        scene.render.resolution_x = scene.render.resolution_y = 96
        scene.render.resolution_percentage = 100
        scene.render.film_transparent, scene.render.filter_size = True,.01
        scene.view_settings.view_transform,scene.view_settings.exposure = 'Standard',.28
        scene.world.color = (.55,.55,.55)
        for pos,power,size in [((-4,-3,7),550,5),((4,3,4),220,4),((0,-5,3),130,4)]:
            bpy.ops.object.light_add(type='AREA',location=pos)
            lamp = bpy.context.object
            lamp.data.energy,lamp.data.size = power,size
            look_at(lamp,(0,0,1))
        bpy.ops.object.camera_add(location=(-6,-9,3.8))
        camera = bpy.context.object
        camera.data.type,camera.data.ortho_scale = 'ORTHO',3.5
        scene.camera = camera
        for name,pos in [('preview',(-6,-9,3.8)),('front',(0,-10,2.8)),('side',(-10,-1.9,3.8))]:
            camera.location = pos
            look_at(camera,(0,0,1.1))
            scene.render.filepath = str(self.out/(name+'.png'))
            bpy.ops.render.render(write_still=True)
        bpy.context.preferences.filepaths.save_version=0
        bpy.ops.wm.save_as_mainfile(filepath=str(self.out/(self.id+'.blend')))
        print('CATALOGUE_ASSET '+self.id+' '+json.dumps(validation))


def look_at(obj,target):
    obj.rotation_euler = (Vector(target)-obj.location).to_track_quat('-Z','Y').to_euler()


def validate_glb(path,joint_names):
    raw = path.read_bytes()
    magic,version,length = struct.unpack_from('<III',raw)
    assert magic == 0x46546c67 and version == 2 and length == len(raw)
    count,kind = struct.unpack_from('<II',raw,12)
    assert kind == 0x4e4f534a
    data = json.loads(raw[20:20+count])
    start = 20+count+8
    primitives = [p for m in data['meshes'] for p in m['primitives']]
    triangles = sum(data['accessors'][p['indices']]['count']//3 for p in primitives)
    vertices = sum(data['accessors'][p['attributes']['POSITION']]['count'] for p in primitives)
    assert triangles <= 5000 and len(data.get('materials',[])) <= 4
    assert len(data['skins']) == 1 and len(data['skins'][0]['joints']) <= 32
    assert {data['nodes'][i]['name'] for i in data['skins'][0]['joints']} == joint_names
    assert not data.get('animations')
    max_influences = 0
    for p in primitives:
        assert 'JOINTS_0' in p['attributes'] and 'JOINTS_1' not in p['attributes']
        accessor = data['accessors'][p['attributes']['WEIGHTS_0']]
        view = data['bufferViews'][accessor['bufferView']]
        offset = start+view.get('byteOffset',0)+accessor.get('byteOffset',0)
        assert accessor['componentType'] == 5126 and accessor['type'] == 'VEC4'
        for i in range(accessor['count']):
            weights = struct.unpack_from('<4f',raw,offset+i*view.get('byteStride',16))
            assert all(math.isfinite(w) and 0 <= w <= 1 for w in weights)
            assert abs(sum(weights)-1) < 1e-5
            max_influences = max(max_influences,sum(w>0 for w in weights))
    assert max_influences <= 2
    assert len(data['images']) == 1
    for sampler in data.get('samplers',[]):
        assert sampler.get('magFilter') == 9728
    return dict(triangles=triangles,exported_vertices=vertices,materials=len(data['materials']),
                deform_joints=len(joint_names),max_influences=max_influences,
                atlas=[256,256],animation_clips=0,embedded_images=1,
                joint_order=[data['nodes'][i]['name'] for i in data['skins'][0]['joints']])
