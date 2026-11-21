"""
Procedural texture generators (Pillow + NumPy) for Douala roads,
building facades (with night-lit window masks), and Douala cultural billboards.
"""

from __future__ import annotations

import numpy as np
from PIL import Image, ImageDraw, ImageFont


def _load_font(size: int) -> ImageFont.ImageFont:
    candidates = [
        "/System/Library/Fonts/Supplemental/Arial Bold.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        "/Library/Fonts/Arial Bold.ttf",
    ]
    for path in candidates:
        try:
            return ImageFont.truetype(path, size)
        except Exception:
            continue
    return ImageFont.load_default()


def generate_road_texture(size: int = 512) -> bytes:
    """Generate RGBA Douala 3-lane boulevard asphalt texture with lane dividers."""
    rng = np.random.default_rng(237)
    base = np.zeros((size, size, 4), dtype=np.uint8)

    # Rich weathered tarmac grain
    grain = rng.integers(-11, 12, size=(size, size))
    coarse = rng.integers(-6, 7, size=(size // 4, size // 4))
    coarse = np.repeat(np.repeat(coarse, 4, axis=0), 4, axis=1)

    r = np.clip(56 + grain + coarse, 26, 92)
    g = np.clip(58 + grain + coarse, 28, 94)
    b = np.clip(64 + grain + coarse, 32, 102)

    # Subtle darker tire-wear tracks along the 3 lane centers!
    xs = np.linspace(0.0, 1.0, size)
    # Road half-width is 5.2, lanes at -2.8, 0.0, +2.8 -> normalized U: 0.23, 0.50, 0.77
    for lane_u in (0.23, 0.50, 0.77):
        for offset in (-0.045, 0.045):
            dist = np.abs(xs - (lane_u + offset))
            wear = np.exp(-((dist / 0.022) ** 2)) * 12.0
            r = np.clip(r - wear, 20, 255)
            g = np.clip(g - wear, 20, 255)
            b = np.clip(b - wear, 24, 255)

    base[:, :, 0] = r.astype(np.uint8)
    base[:, :, 1] = g.astype(np.uint8)
    base[:, :, 2] = b.astype(np.uint8)
    base[:, :, 3] = 255

    img = Image.fromarray(base, mode="RGBA")
    draw = ImageDraw.Draw(img)

    # Continuous Douala yellow double edge lines near curbs
    edge_col = (242, 196, 36, 255)
    w_edge = max(4, size // 96)
    draw.rectangle([int(size * 0.055), 0, int(size * 0.055) + w_edge, size], fill=edge_col)
    draw.rectangle([int(size * 0.945) - w_edge, 0, int(size * 0.945), size], fill=edge_col)

    # Crisp dashed white lane dividers separating the 3 lanes!
    # Lane boundaries are at x = -1.4 and +1.4 -> U = (5.2 - 1.4)/10.4 = 0.3654 and 0.6346
    dash_col = (242, 242, 238, 255)
    w_dash = max(5, size // 78)
    for div_u in (0.3654, 0.6346):
        cx = int(size * div_u)
        for seg in range(0, size, size // 2):
            y0 = seg + size // 12
            y1 = seg + int(size * 0.34)
            draw.rectangle([cx - w_dash // 2, y0, cx + w_dash // 2, y1], fill=dash_col)

    # Subtle weathered road tar seam
    draw.line([(0, size // 2), (size, size // 2)], fill=(38, 40, 46, 255), width=2)
    return img.tobytes()


def generate_facade_texture(size: int = 512) -> bytes:
    """
    Generate RGBA Douala building facade texture.
    RGB = plaster wall + shaded window frames;
    Alpha = 255 on wall, 60..200 on glass panes (used by shader to light windows warmly at night!).
    """
    rng = np.random.default_rng(2371)
    arr = np.zeros((size, size, 4), dtype=np.uint8)
    noise = rng.integers(-8, 9, size=(size, size))

    arr[:, :, 0] = np.clip(226 + noise, 190, 250).astype(np.uint8)
    arr[:, :, 1] = np.clip(222 + noise, 185, 248).astype(np.uint8)
    arr[:, :, 2] = np.clip(216 + noise, 180, 245).astype(np.uint8)
    arr[:, :, 3] = 255  # 255 = solid plaster wall

    img = Image.fromarray(arr, mode="RGBA")
    draw = ImageDraw.Draw(img)

    cols, rows = 4, 4
    cell_w = size // cols
    cell_h = size // rows

    for r_i in range(rows):
        # Horizontal architectural cornice band
        y_band = r_i * cell_h
        draw.rectangle([0, y_band, size, y_band + 8], fill=(176, 172, 166, 255))
        for c_i in range(cols):
            x0 = c_i * cell_w + 18
            y0 = r_i * cell_h + 22
            x1 = (c_i + 1) * cell_w - 18
            y1 = (r_i + 1) * cell_h - 18

            # Window outer frame & tropical sunshade lintel
            draw.rectangle([x0 - 5, y0 - 7, x1 + 5, y0], fill=(155, 152, 148, 255))
            draw.rectangle([x0 - 3, y0, x1 + 3, y1 + 3], fill=(92, 98, 108, 255))

            # Glass pane: encode window-lit probability in Alpha!
            # Alpha < 120 means "lit warm window at night", Alpha 180 means "dark window"
            is_lit = ((r_i * 7 + c_i * 5) % 3 != 0)
            alpha_tag = 45 if is_lit else 185
            glass_col = (44, 68, 96, alpha_tag)
            draw.rectangle([x0, y0, x1, y1], fill=glass_col)
            # Subtle sky reflection diagonal streak
            draw.line([(x0 + 6, y1 - 8), (x1 - 8, y0 + 8)], fill=(86, 118, 148, alpha_tag), width=3)
            # Window mullion crossbars
            mx = (x0 + x1) // 2
            my = (y0 + y1) // 2
            draw.line([(mx, y0), (mx, y1)], fill=(112, 118, 126, 255), width=3)
            draw.line([(x0, my), (x1, my)], fill=(112, 118, 126, 255), width=3)

            # Small Douala exterior AC condenser unit beneath alternating windows
            if (r_i + c_i) % 2 == 0:
                draw.rectangle([x1 - 26, y1 + 4, x1 + 2, y1 + 18], fill=(208, 206, 200, 255))
                draw.ellipse([x1 - 22, y1 + 6, x1 - 12, y1 + 16], fill=(95, 98, 102, 255))

    return img.tobytes()


def generate_billboard_texture(width: int = 512, height: int = 256) -> bytes:
    """Generate a vibrant Douala cultural & welcome boulevard billboard graphic."""
    img = Image.new("RGBA", (width, height), (14, 28, 48, 255))
    draw = ImageDraw.Draw(img)

    # Tropical sunset gradient stripes
    for y in range(height):
        t = y / max(1, height - 1)
        r = int(16 + 22 * t)
        g = int(42 + 46 * (1.0 - t))
        b = int(84 + 34 * (1.0 - t))
        draw.line([(0, y), (width, y)], fill=(r, g, b, 255))

    # Cameroon vibrant tricolor accent ribbon along top & bottom!
    bar_h = 18
    third = width // 3
    draw.rectangle([0, 0, third, bar_h], fill=(12, 152, 74, 255))
    draw.rectangle([third, 0, third * 2, bar_h], fill=(216, 36, 42, 255))
    draw.rectangle([third * 2, 0, width, bar_h], fill=(250, 208, 24, 255))

    draw.rectangle([0, height - bar_h, third, height], fill=(12, 152, 74, 255))
    draw.rectangle([third, height - bar_h, third * 2, height], fill=(216, 36, 42, 255))
    draw.rectangle([third * 2, height - bar_h, width, height], fill=(250, 208, 24, 255))

    # Radiant sun emblem behind lettering
    draw.ellipse([width - 146, 42, width - 38, 150], fill=(250, 186, 30, 255))
    draw.ellipse([width - 130, 58, width - 54, 134], fill=(255, 224, 96, 255))

    # Bold lowercase no-accent signage text
    f_big = _load_font(56)
    f_med = _load_font(28)
    f_sm = _load_font(20)
    draw.text((28, 34), "douala run 237", font=f_big, fill=(255, 214, 42, 255))
    draw.text((30, 104), "maximus express", font=f_med, fill=(248, 250, 255, 255))
    draw.text((30, 152), "akwa - mboppi - deido - wouri", font=f_sm, fill=(138, 238, 184, 255))
    draw.text((30, 192), "benskin - taxi jaune - port de douala", font=f_sm, fill=(255, 198, 128, 255))

    # Outer frame border
    draw.rectangle([4, 4, width - 5, height - 5], outline=(250, 208, 24, 255), width=4)
    return img.tobytes()
