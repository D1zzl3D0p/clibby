"""Non-destructive native-grid palette experiment; no resize, dither or filtering."""
from pathlib import Path
import json
import time
from PIL import Image

ROOT=Path(__file__).resolve().parent
PALETTE=['#302923','#574332','#78543b','#986743','#b58255','#cea371','#d3b489','#ead5b3','#a29462']
COLORS=[tuple(int(c[i:i+2],16) for i in (1,3,5)) for c in PALETTE]
CACHE={}
OUT=ROOT/'pixel-frames'
OUT.mkdir(exist_ok=True)
started=time.perf_counter()
sources=sorted((ROOT/'frames').glob('*.png'))
for source in sources:
    image=Image.open(source).convert('RGBA')
    assert image.size==(96,96)
    result=[]
    for r,g,b,a in image.getdata():
        if a<128:
            result.append((0,0,0,0)); continue
        key=(r,g,b)
        if key not in CACHE:
            CACHE[key]=min(COLORS,key=lambda c:(r-c[0])**2+(g-c[1])**2+(b-c[2])**2)
        result.append((*CACHE[key],255))
    output=Image.new('RGBA',image.size); output.putdata(result); output.save(OUT/source.name)
elapsed=time.perf_counter()-started
for index in [0,53]:
    im=Image.open(OUT/f'{index:04d}.png')
    preview=Image.new('RGBA',im.size,'#252c35'); preview.alpha_composite(im)
    preview.resize((384,384),Image.Resampling.NEAREST).save(ROOT/f'palette-preview-{index:04d}.png')
(ROOT/'palette_stats.json').write_text(json.dumps({'frames':len(sources),'seconds':elapsed,'palette':PALETTE,'distance':'squared Euclidean in rendered sRGB bytes','alphaCutoffInclusive':128,'outputAlpha':[0,255],'dither':False,'filter':None,'nativePixels':[96,96],'additionalBridgingTones':[]},indent=2))
print(f'Palette-mapped {len(sources)} native96 frames in {elapsed:.3f}s')
