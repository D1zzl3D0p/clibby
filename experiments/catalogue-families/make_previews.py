"""Palette-bound, nearest-pixel previews from actual Blender motion renders."""
from pathlib import Path
import json
from PIL import Image, ImageDraw, ImageFont

from motion import PETS

HERE = Path(__file__).resolve().parent
OUTPUT = HERE.parents[1] / "results/catalogue-families-2026-10-16"
PALETTES = {
    "pebblekin": ("252d37", "454b58", "626b77", "858c95", "a3acae", "c7d0c9", "e5e7db"),
    "margin": ("1e2031", "292b3b", "414456", "656879", "979999", "c3c0aa", "e5d8b0", "f4eed5"),
    "lantern-snail": ("272b30", "4b503f", "677357", "849476", "6b4c3d", "95704b", "b68a58", "d0a76f", "dfb66a", "f1d394", "e5d8b0"),
    "velvet": ("272b30", "4b3930", "6b4c3d", "8a7156", "a89574", "beb18e", "d2c5a1", "e5d8b0", "f3edcf"),
}
TITLES = ("Pebblekin", "Margin", "Lantern Snail", "Velvet")
SUBTITLES = ("Anchored pivot / wobble", "Bounded ink morph", "Continuous-foot crawl / peek", "Six planted contacts / reveal")
DURATIONS = [70, 60, 70] * 40


def palette(colors):
    rgb = [int(c[i:i+2], 16) for c in colors for i in (0, 2, 4)]
    rgb += rgb[-3:] * (256-len(colors))
    result = Image.new("P", (1,1))
    result.putpalette(rgb)
    return result


def main():
    all_colors = tuple(dict.fromkeys(c for colors in PALETTES.values() for c in colors))
    global_palette = palette(("ff00ff", *all_colors, "dedbd2", "292d35", "1d222b", "ffffff"))
    frames, stats = {}, {}
    for pet in PETS:
        directory = OUTPUT / pet
        output = directory / "pixel-frames"
        output.mkdir(parents=True, exist_ok=True)
        images = []
        for i in range(120):
            source = Image.open(directory / "frames" / f"{i:04}.png").convert("RGBA")
            alpha = source.getchannel("A").point(lambda x: 255 if x >= 128 else 0)
            image = source.convert("RGB").quantize(palette=palette(PALETTES[pet]), dither=Image.Dither.NONE).convert("RGBA")
            image.putalpha(alpha)
            image.save(output / f"{i:04}.png")
            images.append(image)
        frames[pet] = images
        colors = set()
        for frame in images:
            colors.update(p[:3] for p in frame.getdata() if p[3])
        stats[pet] = {"frames": len(images), "opaque_colors": len(colors), "palette_limit": len(PALETTES[pet]),
                      "binary_alpha": True, "rest_bounds": images[0].getbbox(),
                      "final_identical": images[90].tobytes() == images[-1].tobytes()}
        gifs = []
        for image in images:
            indexed = image.convert("RGB").quantize(palette=global_palette, dither=Image.Dither.NONE)
            indexed.paste(0, mask=image.getchannel("A").point(lambda x: 255-x))
            gifs.append(indexed)
        gifs[0].save(directory / "motion.gif", save_all=True, append_images=gifs[1:],
                     duration=DURATIONS, loop=0, transparency=0, disposal=2, optimize=False)
    boards = []
    font = ImageFont.truetype("/System/Library/Fonts/Supplemental/Arial.ttf", 17)
    small = ImageFont.truetype("/System/Library/Fonts/Supplemental/Arial.ttf", 12)
    for frame in range(120):
        board = Image.new("RGB", (1200, 476), "#dedbd2")
        draw = ImageDraw.Draw(board)
        for i, pet in enumerate(PETS):
            x = i*300
            draw.text((x+12, 8), TITLES[i], font=font, fill="#1d222b")
            draw.text((x+12, 30), SUBTITLES[i], font=small, fill="#1d222b")
            image = frames[pet][frame]
            enlarged = image.resize((288,288), Image.Resampling.NEAREST)
            board.paste(enlarged, (x+6, 53), enlarged)
            draw.text((x+12, 347), "Native 96px / light and dark", font=small, fill="#1d222b")
            draw.rectangle((x+156, 368, x+251, 463), fill="#292d35")
            board.paste(image, (x+30, 368), image)
            board.paste(image, (x+156, 368), image)
        boards.append(board.quantize(palette=global_palette, dither=Image.Dither.NONE))
    boards[0].save(OUTPUT / "families-motion.gif", save_all=True, append_images=boards[1:],
                   duration=DURATIONS, loop=0, optimize=False, disposal=1)
    boards[0].convert("RGB").save(OUTPUT / "families-rest.png")
    contact = Image.new("RGB", (1200, 476*3))
    for row, frame in enumerate((0, 34, 76)):
        contact.paste(boards[frame].convert("RGB"), (0, row*476))
    contact.save(OUTPUT / "contact-sheet.png")
    (OUTPUT / "palette_stats.json").write_text(json.dumps(stats, indent=2)+"\n")
    print(json.dumps(stats, indent=2))


if __name__ == "__main__":
    main()
