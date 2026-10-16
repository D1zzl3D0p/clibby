"""Throwaway actual-volume pixel cat; Blender 3.6+/CPU Cycles. No runtime integration.
Run: Blender -b --factory-startup --python author_render.py -- [--still]
Reads sibling ../poses.json exported from the current procedural Qt controller.
"""
import bpy, math, json, sys, time
from pathlib import Path
from mathutils import Vector
OUT=Path(__file__).resolve().parent
bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete(use_global=False)
def mat(name,color):
    m=bpy.data.materials.new(name); m.diffuse_color=(*color,1); m.use_nodes=True
    p=m.node_tree.nodes.get('Principled BSDF'); p.inputs['Base Color'].default_value=(*color,1); p.inputs['Roughness'].default_value=.94
    return m
orange=mat('muted russet',(.43,.205,.115)); light=mat('russet light',(.59,.31,.16)); dark=mat('deep walnut',(.105,.075,.060)); cream=mat('old parchment',(.86,.66,.44)); eye=mat('lichen gold',(.68,.40,.16)); ear=mat('ear dark rose',(.29,.13,.11))
faceink=mat('graphic face ink',(.055,.035,.025))
nodes=faceink.node_tree.nodes; nodes.clear(); emission=nodes.new('ShaderNodeEmission'); emission.inputs['Color'].default_value=(.004,.003,.002,1); emission.inputs['Strength'].default_value=1; output=nodes.new('ShaderNodeOutputMaterial'); faceink.node_tree.links.new(emission.outputs[0],output.inputs[0])
def ell(name,p,s,m,seg=12,rings=8):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=seg,ring_count=rings,location=p); o=bpy.context.object; o.name=name; o.scale=s; o.data.materials.append(m); return o
def tube(name,points,radii,m,sides=8):
    vs=[]; fs=[]
    for i,p in enumerate(points):
        p=Vector(p); tangent=Vector(points[min(i+1,len(points)-1)])-Vector(points[max(0,i-1)])
        tangent.normalize(); ref=Vector((0,1,0)); u=tangent.cross(ref).normalized(); v=tangent.cross(u).normalized()
        for j in range(sides):
            a=2*math.pi*j/sides; q=p+radii[i]*(math.cos(a)*u+math.sin(a)*v); vs.append(q)
    for i in range(len(points)-1):
        for j in range(sides): fs.append((i*sides+j,i*sides+(j+1)%sides,(i+1)*sides+(j+1)%sides,(i+1)*sides+j))
    fs.extend([tuple(reversed(range(sides))),tuple((len(points)-1)*sides+j for j in range(sides))])
    me=bpy.data.meshes.new(name); me.from_pydata(vs,[],fs); me.update(); o=bpy.data.objects.new(name,me); bpy.context.collection.objects.link(o); o.data.materials.append(m); return o
def ear_mesh(y):
    vs=[(.80,y-.145,1.51),(1.16,y-.10,1.51),(.88,y-.035,1.88),(.81,y+.13,1.51),(1.15,y+.10,1.51)]
    me=bpy.data.meshes.new('short angular ear'); me.from_pydata(vs,[],[(0,1,2),(1,4,2),(4,3,2),(3,0,2),(0,3,4,1)]); me.update(); o=bpy.data.objects.new('ear',me); bpy.context.collection.objects.link(o); o.data.materials.append(orange); return o
def tuft(y):
    sign=1 if y>0 else -1
    vs=[(.94,y,1.48),(.86,y+sign*.15,1.23),(.65,y+sign*.12,1.31),(.76,y,1.57),(.88,y-sign*.10,1.37)]
    me=bpy.data.meshes.new('single cheek plane'); me.from_pydata(vs,[],[(0,1,2,3),(0,4,1),(1,4,2),(2,4,3),(3,4,0)]); me.update(); o=bpy.data.objects.new('cheek tuft',me); bpy.context.collection.objects.link(o); o.data.materials.append(light); return o
static=[]
static += [ell('full ribcage',(-.05,0,1.02),(.89,.39,.42),orange,12,6),ell('scapula',(.56,0,1.11),(.39,.40,.44),light,12,6),ell('haunch',(-.66,0,1.0),(.43,.42,.46),orange,12,6),ell('short neck',(.79,0,1.23),(.31,.31,.32),light,12,6),ell('compact feline head',(1.01,0,1.43),(.315,.305,.28),light,12,6),ell('short muzzle',(1.23,-.005,1.32),(.135,.255,.125),cream,12,6),ell('small nose',(1.35,-.005,1.37),(.042,.080,.045),faceink,8,4),ell('chest bib',(.87,0,1.00),(.18,.31,.24),cream,12,6)]
for y in [-.215,.215]:
    static += [ear_mesh(y),tuft(y),ell('small readable eye',(1.15,y*1.75,1.54),(.078,.055,.058),faceink,8,4)]
# Three broad markings, each a single readable shape rather than fur texture.
for x in [-.58,-.15,.27]:
    static.append(tube('broad dorsal band',[(x,-.04,1.43),(x+.045,-.25,1.36),(x+.07,-.38,1.19)], [.075,.065,.04],dark,5))
for o in static:
    o['rest_location']=list(o.location); o['rest_rotation']=list(o.rotation_euler)
animated=[]
foot_depth={}
def xyz(p): return ((p[0]-96)/40,0,(112-p[1])/40)
def animate(frame):
    global animated
    for o in animated:
        bpy.data.objects.remove(o,do_unlink=True)
    animated=[]
    b=xyz(frame['body']); yaw=math.acos(max(-1,min(1,frame['facingBlend'])))
    # A genuine rotation creates depth/occlusion during the turn. The planar foot
    # targets supply X/height, with authored near/far depths (prototype mapping).
    def rotate(p):
        x,y,z=p; return (x*math.cos(yaw)-y*math.sin(yaw)+b[0],x*math.sin(yaw)+y*math.cos(yaw),z+b[2]-1.075)
    for o in static:
        p=o['rest_location']; o.location=rotate(p); o.rotation_euler=(0,0,yaw)
    for leg in frame['legs']:
        fore=leg['role'].startswith('fore'); left=leg['role'].endswith('Left'); depth=-.28 if left else .28
        hip=Vector(rotate((.65 if fore else -.68,depth,1.04)))
        target=xyz(leg['sole']); desired_depth=depth*math.cos(yaw)+( .65 if fore else -.68)*math.sin(yaw)
        role=leg['role']
        if role not in foot_depth: foot_depth[role]=desired_depth
        if leg.get('swing',False): foot_depth[role]+=(desired_depth-foot_depth[role])*.45
        foot=Vector((target[0],foot_depth[role],max(.06,target[2]+.065)))
        # Continuous ring mesh through hip, bent knee, ankle, and paw; no detached sticks.
        knee=hip.lerp(foot,.49); knee.x += (-.13 if fore else .24)*math.cos(yaw); knee.z+=.015
        ankle=hip.lerp(foot,.87); ankle.x-=.035*math.cos(yaw)
        points=[hip,hip.lerp(knee,.5),knee,knee.lerp(ankle,.5),ankle,foot]
        animated.append(tube(leg['role'],points,[.23 if not fore else .18,.19 if not fore else .16,.13,.10,.085,.10],orange))
        animated.append(ell('paw',foot,(.17,.13,.095),cream if fore else orange))
    t=frame['time']; tail=[(-.88,0,1.10),(-1.20,.04,1.18),(-1.50,.10+math.sin(t*2)*.10,1.43),(-1.61,.14+math.sin(t*2)*.13,1.76),(-1.42,.17+math.sin(t*2)*.13,1.90)]
    animated.append(tube('continuous curved tail',[rotate(p) for p in tail],[.19,.20,.18,.145,.065],orange))
    for i in [1,2]:
        p=tail[i]; q=tail[i+1]; a=Vector(p).lerp(Vector(q),.55); c=Vector(p).lerp(Vector(q),.73)
        animated.append(tube('tail stripe',[rotate(a),rotate(c)],[.205-i*.018,.201-i*.018],dark))
scene=bpy.context.scene; scene.render.engine='CYCLES'; scene.cycles.device='CPU'; scene.cycles.samples=4
scene.render.resolution_x=96; scene.render.resolution_y=96; scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG'; scene.render.image_settings.color_mode='RGBA'; scene.render.film_transparent=True
scene.render.filter_size=.01; scene.view_settings.view_transform='Standard'; scene.view_settings.look='Medium High Contrast' if bpy.app.version<(4,0,0) else 'None'; scene.view_settings.exposure=.28; scene.view_settings.gamma=1
scene.world.color=(.55,.55,.55)
def lamp(name,p,power,size):
    data=bpy.data.lights.new(name,'AREA'); data.energy=power; data.size=size; o=bpy.data.objects.new(name,data); bpy.context.collection.objects.link(o); o.location=p; o.rotation_euler=(Vector((0,0,1))-o.location).to_track_quat('-Z','Y').to_euler()
lamp('large soft key',(-3,-4,7),550,5); lamp('warm fill',(3,4,4),220,4); lamp('face fill',(4,-5,3),130,4)
bpy.ops.object.camera_add(location=(1.9,-10,3.8)); camera=bpy.context.object; camera.rotation_euler=(Vector((0,0,1.04))-camera.location).to_track_quat('-Z','Y').to_euler(); camera.data.type='ORTHO'; camera.data.ortho_scale=4.8; scene.camera=camera
path=OUT.parent/'poses.json'
if path.exists():
    data=json.loads(path.read_text()); frames=data['frames']; scene.render.fps=data['fps']
else:
    frames=[{'time':0,'body':[96,69],'facingBlend':1,'legs':[{'role':r,'sole':[x,112]} for r,x in [('foreLeft',127),('foreRight',118),('hindLeft',64),('hindRight',72)]]}]; scene.render.fps=15
animate(frames[0]); bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'source.blend'))
start=time.monotonic(); (OUT/'frames').mkdir(exist_ok=True)
selected=frames[:1] if '--still' in sys.argv else frames
for i,f in enumerate(selected):
    animate(f); scene.render.filepath=str(OUT/'frames'/f'{i:04d}.png'); bpy.ops.render.render(write_still=True)
elapsed=time.monotonic()-start
(OUT/'render_stats.json').write_text(json.dumps({'frames':len(selected),'renderSeconds':elapsed,'framesPerSecondOffline':len(selected)/elapsed,'engine':'CPU Cycles','samples':4,'nativePixels':[96,96],'sourceFps':scene.render.fps,'blender':bpy.app.version_string},indent=2))
