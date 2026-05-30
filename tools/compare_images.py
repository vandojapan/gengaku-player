#!/usr/bin/env python3
"""Compare two PNG images pixel-for-pixel.

The script prefers Pillow when available. If Pillow is not installed, it falls
back to a small PNG reader that supports common non-interlaced 8-bit PNG files
with grayscale, RGB, grayscale+alpha, or RGBA color types.
"""

from __future__ import annotations

import argparse
import struct
import sys
import zlib
from pathlib import Path
from typing import Iterable, List, Sequence, Tuple

Pixel = Tuple[int, int, int, int]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Compare two PNG images pixel-for-pixel.")
    parser.add_argument("expected", type=Path, help="Reference image, usually RPG_RT/Maniacs output")
    parser.add_argument("actual", type=Path, help="Image to compare, usually EasyRPG output")
    parser.add_argument("--diff", type=Path, help="Optional PNG diff output path")
    parser.add_argument(
        "--max-differences",
        type=int,
        default=20,
        help="Maximum differing pixel samples to print (default: 20)",
    )
    return parser.parse_args()


def load_with_pillow(path: Path):
    try:
        from PIL import Image
    except ImportError:
        return None

    image = Image.open(path).convert("RGBA")
    return image.size[0], image.size[1], list(image.getdata())


def paeth(a: int, b: int, c: int) -> int:
    p = a + b - c
    pa = abs(p - a)
    pb = abs(p - b)
    pc = abs(p - c)
    if pa <= pb and pa <= pc:
        return a
    if pb <= pc:
        return b
    return c


def unfilter_scanline(filter_type: int, raw: bytearray, prev: bytearray, bpp: int) -> bytearray:
    out = bytearray(raw)
    for i in range(len(out)):
        left = out[i - bpp] if i >= bpp else 0
        up = prev[i] if prev else 0
        up_left = prev[i - bpp] if prev and i >= bpp else 0

        if filter_type == 0:
            value = out[i]
        elif filter_type == 1:
            value = out[i] + left
        elif filter_type == 2:
            value = out[i] + up
        elif filter_type == 3:
            value = out[i] + ((left + up) // 2)
        elif filter_type == 4:
            value = out[i] + paeth(left, up, up_left)
        else:
            raise ValueError(f"Unsupported PNG filter type {filter_type}")

        out[i] = value & 0xFF
    return out


def channels_for_color_type(color_type: int) -> int:
    if color_type == 0:
        return 1
    if color_type == 2:
        return 3
    if color_type == 4:
        return 2
    if color_type == 6:
        return 4
    raise ValueError(f"Unsupported PNG color type {color_type}; install Pillow for broader support")


def rgba_from_scanline(scanline: Sequence[int], color_type: int) -> Iterable[Pixel]:
    if color_type == 0:
        for g in scanline:
            yield (g, g, g, 255)
    elif color_type == 2:
        for i in range(0, len(scanline), 3):
            yield (scanline[i], scanline[i + 1], scanline[i + 2], 255)
    elif color_type == 4:
        for i in range(0, len(scanline), 2):
            g = scanline[i]
            yield (g, g, g, scanline[i + 1])
    elif color_type == 6:
        for i in range(0, len(scanline), 4):
            yield (scanline[i], scanline[i + 1], scanline[i + 2], scanline[i + 3])


def load_png_fallback(path: Path):
    data = path.read_bytes()
    if not data.startswith(b"\x89PNG\r\n\x1a\n"):
        raise ValueError(f"{path} is not a PNG file")

    offset = 8
    width = height = bit_depth = color_type = interlace = None
    idat = bytearray()

    while offset < len(data):
        length = struct.unpack(">I", data[offset : offset + 4])[0]
        chunk_type = data[offset + 4 : offset + 8]
        chunk_data = data[offset + 8 : offset + 8 + length]
        offset += 12 + length

        if chunk_type == b"IHDR":
            width, height, bit_depth, color_type, _compression, _filter, interlace = struct.unpack(
                ">IIBBBBB", chunk_data
            )
        elif chunk_type == b"IDAT":
            idat.extend(chunk_data)
        elif chunk_type == b"IEND":
            break

    if width is None or height is None or bit_depth is None or color_type is None:
        raise ValueError(f"{path} has no IHDR")
    if bit_depth != 8:
        raise ValueError(f"Unsupported PNG bit depth {bit_depth}; install Pillow for broader support")
    if interlace != 0:
        raise ValueError("Unsupported interlaced PNG; install Pillow for broader support")

    channels = channels_for_color_type(color_type)
    bpp = channels
    stride = width * channels
    decompressed = zlib.decompress(bytes(idat))
    expected_len = (stride + 1) * height
    if len(decompressed) != expected_len:
        raise ValueError(f"Unexpected decompressed size in {path}: {len(decompressed)} != {expected_len}")

    pixels: List[Pixel] = []
    prev = bytearray(stride)
    pos = 0
    for _y in range(height):
        filter_type = decompressed[pos]
        pos += 1
        raw = bytearray(decompressed[pos : pos + stride])
        pos += stride
        scanline = unfilter_scanline(filter_type, raw, prev, bpp)
        pixels.extend(rgba_from_scanline(scanline, color_type))
        prev = scanline

    return width, height, pixels


def load_image(path: Path):
    loaded = load_with_pillow(path)
    if loaded is not None:
        return loaded
    return load_png_fallback(path)


def write_diff(path: Path, width: int, height: int, expected: Sequence[Pixel], actual: Sequence[Pixel]) -> None:
    try:
        from PIL import Image
    except ImportError:
        write_png_fallback(path, width, height, diff_pixels(expected, actual))
        return

    image = Image.new("RGBA", (width, height))
    image.putdata(diff_pixels(expected, actual))
    image.save(path)


def diff_pixels(expected: Sequence[Pixel], actual: Sequence[Pixel]) -> List[Pixel]:
    pixels: List[Pixel] = []
    for exp, act in zip(expected, actual):
        if exp == act:
            pixels.append((0, 0, 0, 0))
        else:
            pixels.append((255, abs(exp[1] - act[1]), abs(exp[2] - act[2]), 255))
    return pixels


def png_chunk(chunk_type: bytes, chunk_data: bytes) -> bytes:
    crc = zlib.crc32(chunk_type)
    crc = zlib.crc32(chunk_data, crc) & 0xFFFFFFFF
    return struct.pack(">I", len(chunk_data)) + chunk_type + chunk_data + struct.pack(">I", crc)


def write_png_fallback(path: Path, width: int, height: int, pixels: Sequence[Pixel]) -> None:
    raw = bytearray()
    for y in range(height):
        raw.append(0)
        for r, g, b, a in pixels[y * width : (y + 1) * width]:
            raw.extend((r, g, b, a))

    ihdr = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    data = b"\x89PNG\r\n\x1a\n"
    data += png_chunk(b"IHDR", ihdr)
    data += png_chunk(b"IDAT", zlib.compress(bytes(raw)))
    data += png_chunk(b"IEND", b"")
    path.write_bytes(data)


def main() -> int:
    args = parse_args()
    exp_w, exp_h, expected = load_image(args.expected)
    act_w, act_h, actual = load_image(args.actual)

    if (exp_w, exp_h) != (act_w, act_h):
        print(f"SIZE_MISMATCH expected={exp_w}x{exp_h} actual={act_w}x{act_h}")
        return 2

    differences = []
    max_delta = 0
    for idx, (exp, act) in enumerate(zip(expected, actual)):
        if exp == act:
            continue
        x = idx % exp_w
        y = idx // exp_w
        delta = max(abs(exp[i] - act[i]) for i in range(4))
        max_delta = max(max_delta, delta)
        if len(differences) < args.max_differences:
            differences.append((x, y, exp, act, delta))

    total = exp_w * exp_h
    diff_count = sum(1 for exp, act in zip(expected, actual) if exp != act)

    if args.diff:
        write_diff(args.diff, exp_w, exp_h, expected, actual)

    if diff_count == 0:
        print(f"MATCH pixels={total} size={exp_w}x{exp_h}")
        return 0

    print(
        f"DIFFER pixels={diff_count}/{total} "
        f"ratio={diff_count / total:.8f} max_channel_delta={max_delta} size={exp_w}x{exp_h}"
    )
    for x, y, exp, act, delta in differences:
        print(f"  x={x} y={y} expected={exp} actual={act} max_delta={delta}")
    if args.diff:
        print(f"diff={args.diff}")
    return 1


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"ERROR {exc}", file=sys.stderr)
        raise SystemExit(3)
