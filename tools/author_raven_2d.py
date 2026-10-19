"""Original editable layered raven: bird proportions and articulated feather blades."""
from pathlib import Path
import json
import math
from PIL import Image, ImageDraw

OUT = Path(__file__).resolve().parents[1] / "characters/raven"
INK, FEATHER, SHINE = "#10151e", "#1e2939", "#37465b"

def author():
    OUT.mkdir(parents=True, exist_ok=True)
    bones, attachments, layers = [], [], []
    def bone(name, parent, x, y, rotation=0):
        bones.append(dict(id=name, parent=parent, rest=dict(x=x, y=y, rotation=rotation, scaleX=1, scaleY=1)))
    def layer(name, size, pivot, order, draw, mirror=False):
        image = Image.new("RGBA", size)
        draw(ImageDraw.Draw(image))
        if mirror:
            image = image.transpose(Image.Transpose.FLIP_LEFT_RIGHT)
            pivot = [size[0]-pivot[0], pivot[1]]
        layers.append(image)
        attachments.append(dict(bone=name,source=[0,0,*size],pivot=pivot,offset=[0,0],drawOrder=order))
    def torso(d):
        d.ellipse((5,5,122,65),fill=INK)
        d.ellipse((23,7,111,46),fill=FEATHER)
        d.polygon([(96,14),(128,0),(132,34),(117,57),(93,61)],fill=INK)
        for x,y in [(107,38),(113,43),(100,47),(106,52),(94,53)]:
            d.polygon([(x-7,y-6),(x+4,y-2),(x,y+10)],fill=FEATHER)
        d.arc((28,8,112,54),190,260,fill=SHINE,width=2)
    def head(d):
        d.ellipse((3,3,53,43),fill=INK)
        d.polygon([(33,18),(52,16),(80,28),(75,32),(43,34)],fill="#252d39")
        d.polygon([(45,29),(78,29),(48,36)],fill=INK)
        d.line((50,21,70,26),fill=SHINE,width=2)
        d.arc((10,5,47,35),205,267,fill=SHINE,width=2)
        for x in (16,24,32):
            d.polygon([(x,33),(x+10,31),(x+4,51),(x-2,42)],fill=FEATHER)
    def tail(d):
        d.polygon([(64,2),(75,15),(28,48),(3,41),(12,29)],fill=INK)
        for i in range(5):
            d.line((60+i*2,12,9+i*5,39+i*2),fill=FEATHER,width=2)
    def upper(d):
        d.polygon([(6,8),(33,2),(57,16),(53,37),(28,47),(3,31)],fill=FEATHER)
        d.arc((9,6,53,44),170,330,fill=SHINE,width=2)
        for i in range(4):
            d.polygon([(15+i*8,18),(30+i*6,22),(26+i*6,42),(18+i*6,38)],fill=INK)
    def secondary(d):
        d.polygon([(4,8),(52,2),(65,23),(49,44),(7,35)],fill=INK)
        for i in range(7):
            x=8+i*7
            d.polygon([(x,11),(x+15,14),(x+12,45+(i%2)*3),(x+5,47),(x,26)],fill=FEATHER)
            d.line((x+7,20,x+8,40),fill=SHINE,width=1)
    def primary(d):
        # Long staggered primaries form the complete feather blade, rather
        # than little fingers attached to a short arm-shaped wrist.
        d.polygon([(4,10),(68,2),(139,21),(130,38),(60,48),(5,32)],fill=INK)
        for i in range(8):
            length=121-abs(i-3)*9
            y=8+i*5
            d.polygon([(8,y),(57,y-3),(length,y+2),(length+13,y+8),(length+7,y+13),(53,y+10),(8,y+7)],fill=FEATHER if i%2 else INK)
            d.line((38,y+3,length+6,y+7),fill=SHINE,width=1)
    bone("body",None,156,108)
    bone("facing","body",0,0)
    bone("head","facing",43,-30)
    bone("eyes","head",16,-7)
    bone("tail","facing",-65,20)
    layer("tail",(80,55),(70,7),0,tail)
    for side,offset in (("Left",-11),("Right",10)):
        bone("hip"+side,"body",offset,20)
        bone("knee"+side,"hip"+side,-9,18)
        bone("ankle"+side,"knee"+side,6,16)
        # Visible tibia runs back to the hock; the forward tarsus is a
        # separate driven piece. Feather cover ends above the joint.
        layer("hip"+side,(20,26),(11,3),1,lambda d:(d.line((11,3,2,21),fill="#505966",width=5),d.line((11,4,3,20),fill="#8b939b",width=1),d.polygon([(6,0),(17,0),(13,9),(8,10)],fill=INK)))
        layer("knee"+side,(18,25),(5,4),2,lambda d:(d.line((5,4,11,20),fill="#59626d",width=4),d.line((5,6,10,19),fill="#9aa0a6",width=1),d.ellipse((2,1,8,7),fill="#707984")))
        layer("ankle"+side,(35,13),(11,3),3,lambda d:(d.line((11,2,11,8,30,10),fill="#383e48",width=3),d.line((11,8,3,11),fill="#383e48",width=3),d.line((15,8,24,12),fill="#383e48",width=2),d.line((29,10,33,9),fill="#6b7078",width=1)))
        mirror=side=="Right"
        sign=-1 if mirror else 1
        bone("shoulder"+side,"facing",5 if mirror else 11,-11, -.20 if mirror else 2.94)
        bone("elbow"+side,"shoulder"+side,sign*42,0)
        bone("wrist"+side,"elbow"+side,sign*48,0)
        order=4 if mirror else 9
        layer("shoulder"+side,(61,51),(7,19),order,upper,mirror)
        layer("elbow"+side,(72,51),(5,18),order+1,secondary,mirror)
        layer("wrist"+side,(148,64),(6,20),order+2,primary,mirror)
    layer("facing",(139,73),(70,34),7,torso)
    layer("head",(83,55),(25,24),13,head)
    layer("eyes",(83,55),(41,17),14,lambda d:(d.ellipse((38,14,44,20),fill="#03060b"),d.ellipse((40,14,42,16),fill="#8796a7")))
    bindings=dict(body="body",facing="facing",head="head",eyes="eyes",tail="tail",footLeft="ankleLeft",footRight="ankleRight")
    for side in ("Left","Right"):
        for role in ("Shoulder","Elbow","Wrist"):
            bindings["wing"+role+side]=role.lower()+side
    # Shoulder rest offsets need broad validated limits for signed stroke.
    for b in bones:
        if b["id"].startswith(("shoulder", "elbow", "wrist")):
            b["minRotation"]=-6.3; b["maxRotation"]=6.3
    manifest=dict(version=1,id="raven",name="Corvid",controller="articulated",width=256,height=224,origin=[156,170],atlas="atlas.png",motionEnvelope=112,bones=bones,attachments=attachments,bindings=bindings,
        contacts=[dict(bone="ankle"+side,point=[0,8]) for side in ("Left","Right")],interaction=[[80,77],[183,48],[250,77],[228,105],[180,145],[176,169],[128,169],[103,145],[54,136]],
        locomotion=dict(legs=[dict(hip="hip"+side,knee="knee"+side,ankle="ankle"+side,contact=[0,8],bend=-1) for side in ("Left","Right")],groundY=170,speed=70,acceleration=240,reachThreshold=13,stride=18,lift=8,minStepDuration=.18,maxStepDuration=.26,secondary=dict(frequency=3,damping=.8,response=0)))
    # Scale rig, contact geometry and raster layers together; presentation
    # must retain pixel-sized details without a separate runtime scale.
    scale=.75
    manifest["width"]=round(manifest["width"]*scale)
    manifest["height"]=round(manifest["height"]*scale)
    manifest["origin"]=[v*scale for v in manifest["origin"]]
    manifest["motionEnvelope"]*=scale
    manifest["interaction"]=[[v*scale for v in point] for point in manifest["interaction"]]
    for b in bones:
        for axis in ("x","y"):b["rest"][axis]*=scale
    for contact in manifest["contacts"]:contact["point"]=[v*scale for v in contact["point"]]
    locomotion=manifest["locomotion"]
    for key in ("groundY","speed","acceleration","reachThreshold","stride","lift"):locomotion[key]*=scale
    for leg in locomotion["legs"]:leg["contact"]=[v*scale for v in leg["contact"]]
    for index,(image,a) in enumerate(zip(layers,attachments)):
        size=(round(image.width*scale),round(image.height*scale))
        layers[index]=image.resize(size,Image.Resampling.LANCZOS)
        a["source"][2:]=size
        a["pivot"]=[v*scale for v in a["pivot"]]
        a["offset"]=[v*scale for v in a["offset"]]
    # Shelf packing is deterministic, validates every rectangle, and grows
    # vertically to avoid silent clipping as feather blades become larger.
    width=512;x=y=2;row=0
    for image,a in zip(layers,attachments):
        if image.width+4>width: raise ValueError("layer exceeds atlas width")
        if x+image.width+2>width: x=2;y+=row+2;row=0
        a["source"][:2]=[x,y];x+=image.width+2;row=max(row,image.height)
    height=1
    while height<y+row+2:height*=2
    atlas=Image.new("RGBA",(width,height))
    for image,a in zip(layers,attachments):atlas.paste(image,tuple(a["source"][:2]))
    atlas.save(OUT/"atlas.png")
    (OUT/"manifest.json").write_text(json.dumps(manifest,indent=2)+"\n")
    def preview_pose(flight):
        world={};preview=Image.new("RGBA",(360,336))
        for b in bones:
            r=b["rest"];angle=r["rotation"]
            if b["id"].startswith(("shoulder","elbow","wrist")):
                side=1 if b["id"].endswith("Left") else -1
                angle+=side*((.20 if side==1 else -.20) if flight and b["id"].startswith("shoulder") else 0 if flight else math.pi if b["id"].startswith("elbow") else -math.pi if b["id"].startswith("wrist") else 0)
            px,py,pa=world.get(b["parent"],(84,84,0));c=math.cos(pa);s=math.sin(pa)
            world[b["id"]]=(px+c*r["x"]-s*r["y"],py+s*r["x"]+c*r["y"],pa+angle)
        for image,a in sorted(zip(layers,attachments),key=lambda pair:pair[1]["drawOrder"]):
            px,py,angle=world[a["bone"]];c=math.cos(angle);s=math.sin(angle);vx,vy=a["pivot"]
            transform=(c,s,vx-c*px-s*py,-s,c,vy+s*px-c*py)
            preview.alpha_composite(image.transform(preview.size,Image.Transform.AFFINE,transform,resample=Image.Resampling.BICUBIC))
        return preview
    preview_pose(False).save(OUT/"preview.png")
    preview_pose(True).save(OUT/"preview-flight.png")
    (OUT/"ATTRIBUTION.md").write_text("Original raven geometry and layered artwork authored for Clibby. Source: tools/author_raven_2d.py. MIT license. No external artwork. Anatomy reference: Cornell Lab, Common Raven identification (https://www.allaboutbirds.org/guide/Common_Raven/id); no reference artwork copied.\n")

if __name__ == "__main__":
    author()
