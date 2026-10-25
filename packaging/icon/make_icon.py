#!/usr/bin/env python3
"""Generates the TRIPPY app icon: a rainbow Flower of Life on a deep violet
rounded square. Writes icon.png (1024px, used for the macOS .icns) and
../windows/trippy.ico. Needs Pillow. Re-run only if you change the design;
the outputs are committed."""
import colorsys, math, os
from PIL import Image, ImageDraw, ImageFilter

HERE = os.path.dirname(os.path.abspath(__file__))
S = 4096  # supersampled canvas
img = Image.new("RGBA", (S, S), (0, 0, 0, 0))
d = ImageDraw.Draw(img)
d.rounded_rectangle([S * 0.04, S * 0.04, S * 0.96, S * 0.96], radius=S * 0.2, fill=(22, 10, 44, 255))

cx = cy = S / 2
r = S * 0.13
centers = [(0.0, 0.0)]
for k in range(6):
    a = math.pi / 6 + k * math.pi / 3
    centers.append((r * math.cos(a), r * math.sin(a)))
    centers.append((2 * r * math.cos(a), 2 * r * math.sin(a)))
    b = a + math.pi / 6
    centers.append((math.sqrt(3) * r * math.cos(b), math.sqrt(3) * r * math.sin(b)))

def hue(t):
    return tuple(int(255 * v) for v in colorsys.hsv_to_rgb(t % 1.0, 0.75, 1.0)) + (255,)

glow = Image.new("RGBA", (S, S), (0, 0, 0, 0))
g = ImageDraw.Draw(glow)
w = int(S * 0.012)
for layer, width in ((g, w * 4), (d, w)):
    for (x, y) in centers:
        t = (math.atan2(y, x) / (2 * math.pi)) + math.hypot(x, y) / (S * 0.9)
        layer.ellipse([cx + x - r, cy + y - r, cx + x + r, cy + y + r], outline=hue(t), width=width)
    layer.ellipse([cx - 3 * r, cy - 3 * r, cx + 3 * r, cy + 3 * r], outline=hue(0.83), width=width)
glow = glow.filter(ImageFilter.GaussianBlur(S * 0.012))
glow.putalpha(glow.getchannel("A").point(lambda a: a * 0.55))
out = Image.alpha_composite(Image.new("RGBA", (S, S), (0, 0, 0, 0)), glow)
out = Image.alpha_composite(out, img)
# Keep the glow inside the rounded square.
mask = Image.new("L", (S, S), 0)
ImageDraw.Draw(mask).rounded_rectangle([S * 0.04, S * 0.04, S * 0.96, S * 0.96], radius=S * 0.2, fill=255)
out.putalpha(Image.composite(out.getchannel("A"), Image.new("L", (S, S), 0), mask))

png = out.resize((1024, 1024), Image.LANCZOS)
png.save(os.path.join(HERE, "icon.png"))
png.save(os.path.join(HERE, "..", "windows", "trippy.ico"),
         sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])
print("wrote icon.png and trippy.ico")
