"""Build the Clibby vector study and size proofs. Run with .venv/bin/python."""

from pathlib import Path
import subprocess
import xml.etree.ElementTree as ET

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parent
INK = "#292638"
LILAC = "#B9ADEB"
CREAM = "#F7F4EE"

# One continuous outline: outer crescent, rounded tips, then the face and ears.
# Ears are mirrored about x=137. Eyes are identical ellipses on one baseline.
OUTLINE = """M 211 104
C 222 109 235 97 230 85
C 213 48 173 24 128 24
C 70.562 24 24 70.562 24 128
C 24 185.438 70.562 232 128 232
C 171 232 207 211 228 181
C 233 173 225 166 219 172
C 199 192 165 213 137 213
C 92 213 57 182 57 144
C 57 122 66 102 82 91
C 73 80 80 60 94 58
C 106 56 116 63 119 74
Q 137 67 155 74
C 158 63 168 56 180 58
C 194 60 201 80 192 91
Q 198 99 211 104 Z"""


def glyph(colour):
    return (
        f'<g fill="{colour}"><path d="{OUTLINE}"/>'
        '<ellipse cx="116" cy="145" rx="10" ry="18"/>'
        '<ellipse cx="156" cy="145" rx="10" ry="18"/></g>'
    )


def svg(content, title):
    return (
        '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 256 256" '
        'width="256" height="256" role="img">'
        f'<title>{title}</title>{content}</svg>\n'
    )


def write_assets():
    assets = {
        "mark.svg": svg(glyph(INK), "Clibby mark"),
        "mark-reversed.svg": svg(glyph(CREAM), "Clibby mark, reversed"),
        "menu-template.svg": svg(glyph("#000000"), "Clibby menu bar template"),
        "app-icon.svg": svg(
            f'<rect width="256" height="256" rx="54" fill="{LILAC}"/>'
            f'<g transform="translate(4 4) scale(.96875)">{glyph(INK)}</g>',
            "Clibby lilac app icon study",
        ),
    }
    for name, source in assets.items():
        ET.fromstring(source)
        (ROOT / name).write_text(source)
    for name in assets:
        size = 1024 if name == "app-icon.svg" else 768
        render(name, size, ROOT / name.replace(".svg", ".png"))
    for size in (16, 18, 22, 32, 64, 128, 256):
        render("mark.svg", size, ROOT / f"mark-{size}.png")
        render("app-icon.svg", size, ROOT / f"app-icon-{size}.png")


def render(name, size, destination):
    subprocess.run(
        ["rtk", "proxy", "rsvg-convert", "-w", str(size), "-h", str(size),
         "-o", str(destination), str(ROOT / name)],
        check=True,
        capture_output=True,
    )


def font(size, bold=False):
    suffix = " Bold" if bold else ""
    return ImageFont.truetype(f"/System/Library/Fonts/Supplemental/Arial{suffix}.ttf", size)


def proof():
    board = Image.new("RGB", (1440, 1040), CREAM)
    draw = ImageDraw.Draw(board)
    draw.text((64, 44), "clibby", fill=INK, font=font(52, True))
    draw.text((64, 110), "Vector refinement 03", fill=INK, font=font(22))
    mark = Image.open(ROOT / "mark.png").convert("RGBA")
    board.paste(mark.resize((540, 540), Image.Resampling.LANCZOS), (70, 185),
                mark.resize((540, 540), Image.Resampling.LANCZOS))
    icon = Image.open(ROOT / "app-icon.png").convert("RGBA")
    icon = icon.resize((430, 430), Image.Resampling.LANCZOS)
    board.paste(icon, (850, 190), icon)
    draw.text((850, 654), "One outline + two identical eyes", fill=INK, font=font(22))
    draw.text((850, 691), "Ink #292638  /  Lilac #B9ADEB", fill=INK, font=font(20))
    draw.line((64, 772, 1376, 772), fill="#D8D4D0", width=2)
    draw.text((64, 804), "Actual pixel sizes", fill=INK, font=font(22, True))
    x = 64
    for size in (16, 18, 22, 32, 64, 128):
        small = Image.open(ROOT / f"mark-{size}.png").convert("RGBA")
        board.paste(small, (x, 860 + (128 - size) // 2), small)
        draw.text((x, 1000), str(size), fill=INK, font=font(16))
        x += max(size + 45, 85)
    for i, background in enumerate((CREAM, INK)):
        x, y = 1000, 820 + i * 80
        draw.rounded_rectangle((x, y, x + 325, y + 60), 12, fill=background)
        source = "mark.png" if i == 0 else "mark-reversed.png"
        small = Image.open(ROOT / source).convert("RGBA").resize((22, 22), Image.Resampling.LANCZOS)
        board.paste(small, (x + 20, y + 19), small)
        draw.text((x + 65, y + 20), "22 px / light" if i == 0 else "22 px / dark",
                  fill=INK if i == 0 else CREAM, font=font(17))
    board.save(ROOT / "proof.png")


if __name__ == "__main__":
    write_assets()
    proof()
    print(f"Built SVG masters, PNG exports and proof: {ROOT}")
