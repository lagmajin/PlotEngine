from __future__ import annotations

import math
from pathlib import Path

from PIL import Image, ImageDraw


ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "resources" / "icons"
SIZES = (16, 24, 32, 48, 64, 128, 256)


def lerp(a: int, b: int, t: float) -> int:
    return int(a + (b - a) * t)


def gradient(size: int) -> Image.Image:
    image = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    pixels = image.load()
    for y in range(size):
        for x in range(size):
            tx = x / max(1, size - 1)
            ty = y / max(1, size - 1)
            t = (tx * 0.55) + (ty * 0.45)
            r = lerp(14, 32, t)
            g = lerp(24, 48, t)
            b = lerp(38, 64, t)
            pixels[x, y] = (r, g, b, 255)
    return image


def rounded_rectangle(draw: ImageDraw.ImageDraw, box, radius, fill, outline=None, width=1):
    draw.rounded_rectangle(box, radius=radius, fill=fill, outline=outline, width=width)


def draw_icon(size: int) -> Image.Image:
    scale = 4
    canvas = size * scale
    image = Image.new("RGBA", (canvas, canvas), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)

    bg = gradient(canvas)
    mask = Image.new("L", (canvas, canvas), 0)
    mask_draw = ImageDraw.Draw(mask)
    pad = 1.4 * scale
    mask_draw.rounded_rectangle(
        (pad, pad, canvas - pad, canvas - pad),
        radius=canvas * 0.22,
        fill=255,
    )
    image.alpha_composite(bg)
    image.putalpha(mask)

    draw = ImageDraw.Draw(image)
    stroke = max(2, int(canvas * 0.035))
    glow = max(2, int(canvas * 0.018))
    cyan = (80, 232, 229, 255)
    paper = (228, 239, 245, 255)
    paper_shadow = (127, 154, 171, 255)
    amber = (255, 190, 92, 255)

    inset = canvas * 0.11
    rounded_rectangle(
        draw,
        (inset, inset, canvas - inset, canvas - inset),
        canvas * 0.18,
        fill=None,
        outline=(92, 225, 223, 95),
        width=glow,
    )

    # Open manuscript pages.
    left = [
        (canvas * 0.20, canvas * 0.31),
        (canvas * 0.45, canvas * 0.25),
        (canvas * 0.50, canvas * 0.67),
        (canvas * 0.24, canvas * 0.74),
    ]
    right = [
        (canvas * 0.55, canvas * 0.25),
        (canvas * 0.80, canvas * 0.31),
        (canvas * 0.76, canvas * 0.74),
        (canvas * 0.50, canvas * 0.67),
    ]
    draw.polygon(left, fill=paper, outline=paper_shadow)
    draw.polygon(right, fill=paper, outline=paper_shadow)
    draw.line((canvas * 0.50, canvas * 0.30, canvas * 0.50, canvas * 0.70), fill=(58, 84, 105, 210), width=stroke)

    for i in range(4):
        y = canvas * (0.40 + i * 0.075)
        draw.line((canvas * 0.28, y, canvas * 0.43, y - canvas * 0.025), fill=(45, 65, 81, 185), width=max(1, stroke // 2))
        draw.line((canvas * 0.58, y - canvas * 0.025, canvas * 0.72, y), fill=(45, 65, 81, 185), width=max(1, stroke // 2))

    # IDE cursor/code prompt overlay.
    draw.line(
        (canvas * 0.29, canvas * 0.77, canvas * 0.72, canvas * 0.77),
        fill=(10, 15, 22, 190),
        width=max(2, stroke),
    )
    draw.line(
        (canvas * 0.33, canvas * 0.82, canvas * 0.58, canvas * 0.82),
        fill=cyan,
        width=max(2, stroke),
    )
    caret_x = canvas * (0.61 + 0.02 * math.sin(size))
    draw.line((caret_x, canvas * 0.78, caret_x, canvas * 0.86), fill=amber, width=max(2, stroke))

    # Novel sparkle marker.
    star = [
        (canvas * 0.79, canvas * 0.16),
        (canvas * 0.83, canvas * 0.25),
        (canvas * 0.92, canvas * 0.29),
        (canvas * 0.83, canvas * 0.33),
        (canvas * 0.79, canvas * 0.42),
        (canvas * 0.75, canvas * 0.33),
        (canvas * 0.66, canvas * 0.29),
        (canvas * 0.75, canvas * 0.25),
    ]
    draw.polygon(star, fill=(255, 207, 116, 255))

    return image.resize((size, size), Image.Resampling.LANCZOS)


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    images = []
    for size in SIZES:
        icon = draw_icon(size)
        icon.save(OUT / f"plotengine-{size}.png")
        images.append(icon)

    images[-1].save(
        OUT / "plotengine.ico",
        sizes=[(size, size) for size in SIZES],
        append_images=images[:-1],
    )


if __name__ == "__main__":
    main()
