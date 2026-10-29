"""Render exact C-produced OLED framebuffer fixtures without image dependencies."""
from pathlib import Path
import re
import struct
import zlib

root = Path(__file__).resolve().parent.parent
source = (root / 'firmware/stm32/screen.c').read_text(encoding='utf-8')
font = {}
for first, last, chars in [('font_digits', 'font_letters', '0123456789'), ('font_letters', 'static uint8_t *canvas', 'ABCDEFGHIJKLMNOPQRSTUVWXYZ')]:
    section = source[source.index(first):source.index(last, source.index(first) + len(first))]
    for char, glyph in zip(chars, re.findall(r'\{([^{}]+)\}', section)):
        font[char] = [int(v.strip(), 0) for v in glyph.split(',') if v.strip()]
data = (root / '.pio/oled-ui-previews.bin').read_bytes()
frames = [(data[i:i+32].split(b'\0')[0].decode(), data[i+32:i+1056]) for i in range(0, len(data), 1056)]
cols, scale, cell_w, cell_h = 3, 3, 408, 234
w, h = cols * cell_w, ((len(frames) + cols - 1) // cols) * cell_h
pixels = bytearray((23, 28, 30)) * (w * h)


def rect(x, y, ww, hh, color):
    for yy in range(y, y+hh):
        for xx in range(x, x+ww):
            pixels[(yy*w+xx)*3:(yy*w+xx)*3+3] = bytes(color)


for i, (title, frame) in enumerate(frames):
    x, y = (i % cols)*cell_w+12, (i // cols)*cell_h+10
    for c, char in enumerate(title[:31]):
        for gx, bits in enumerate(font.get(char, [0]*5)):
            for gy in range(7):
                if bits & (1 << gy):
                    rect(x+c*12+gx*2, y+gy*2, 2, 2, (183, 198, 202))
    y += 22
    rect(x, y, 384, 192, (1, 4, 6))
    for yy in range(64):
        for xx in range(128):
            if frame[(yy//8)*128+xx] & (1 << (yy % 8)):
                rect(x+xx*scale, y+yy*scale, scale, scale, (236, 248, 252))


def chunk(kind, payload):
    return struct.pack('>I', len(payload)) + kind + payload + struct.pack('>I', zlib.crc32(kind+payload) & 0xffffffff)


raw = b''.join(b'\0' + pixels[yy*w*3:(yy+1)*w*3] for yy in range(h))
png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(raw)) + chunk(b'IEND', b'')
target = root / '.pio/oled-ui-preview.png'
target.write_bytes(png)
print(f'{len(frames)} exact framebuffer previews: {target}')
