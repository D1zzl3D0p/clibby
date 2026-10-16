"""Native backend check: skip only when graphics hardware is unavailable."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

with tempfile.TemporaryDirectory(prefix="pets-3d-gpu-") as folder:
    target=Path(folder)/"frame.png"
    result=subprocess.run([sys.argv[1],"--capture",str(target),"--transparent","--target","30","--duration",".4"],
        env=dict(os.environ,QT_QPA_PLATFORM="offscreen"),capture_output=True,text=True,timeout=30)
    print(result.stdout,result.stderr)
    unavailable=("Failed to create RHI" in result.stderr or "Failed to create QRhi" in result.stderr or
                 "no Metal device" in result.stderr or "Failed to create Metal" in result.stderr or
                 "GPU unavailable:" in result.stderr or "Failed to initialize graphics backend" in result.stderr)
    if unavailable:
        print("SKIP: no functioning graphics device; build/joint tests are separate")
        sys.exit(77)
    if result.returncode:
        sys.exit(1)
    from PIL import Image
    image=Image.open(target).convert("RGBA")
    if image.getpixel((5,5))[3] != 0:
        raise AssertionError("3D transparent capture background is opaque")
    # Inspect model area only; UI labels cannot make an empty 3D pass pass.
    background=image.getpixel((5,5))
    model=sum(image.getpixel((x,y))!=background for y in range(80,480) for x in range(100,668))
    if model<200:
        raise AssertionError("Native 3D capture has no visible model pixels")
    print(f"Native framebuffer contains {model} non-background model-area pixels")
