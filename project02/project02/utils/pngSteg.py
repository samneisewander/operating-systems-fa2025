#!/usr/bin/env python3
"""
lsb2bitplane.py

Hide/extract plaintext into the SECOND bitplane (bit index 1) of a PNG image using LSB steganography.

Usage:
    # Embed text
    python lsb2bitplane.py hide -i input.png -o out.png -m "Secret message"

    # Embed from file
    python lsb2bitplane.py hide -i input.png -o out.png -f secret.txt

    # Extract
    python lsb2bitplane.py extract -i out.png -o recovered.txt

Notes:
- This script writes into the second-least-significant bit (bit index 1) of RGB channels.
- Alpha channel (if present) is left untouched.
- Stores a 32-bit big-endian length (bytes) before the payload.
"""

import argparse
from PIL import Image
import sys
import math

BIT_INDEX = 0  # second bitplane (0 is LSB, 1 is second LSB)

def _bytes_to_bits(data: bytes):
    """Yield bits (0/1) from bytes, MSB first per byte."""
    for b in data:
        for i in range(7, -1, -1):
            yield (b >> i) & 1

def _bits_to_bytes(bits):
    """Take an iterable of bits (0/1) and produce bytes. Bits must be multiple of 8."""
    b = 0
    out = bytearray()
    count = 0
    for bit in bits:
        b = (b << 1) | (bit & 1)
        count += 1
        if count == 8:
            out.append(b)
            b = 0
            count = 0
    return bytes(out)

def embed_message(input_path: str, output_path: str, message_bytes: bytes):
    img = Image.open(input_path)
    # work in RGBA so we can preserve alpha if present
    img = img.convert("RGBA")
    pixels = img.load()
    width, height = img.size

    # capacity: number of modifiable channels = width*height*3 (R,G,B)
    capacity_bits = width * height
    msg_len = len(message_bytes)
    # we will store a 32-bit length header followed by message bytes
    total_bits_needed = 32 + msg_len * 8

    if total_bits_needed > capacity_bits:
        raise ValueError(f"Message too large to embed. Need {total_bits_needed} bits, capacity is {capacity_bits} bits.")

    # prepare bit stream: first 32-bit length (big-endian), then message bytes
    length_bytes = msg_len.to_bytes(4, byteorder="big")
    bit_stream = _bytes_to_bits(length_bytes + message_bytes)

    mask_clear = ~(1 << BIT_INDEX) & 0xFF

    # iterate over pixels in row-major order, for each pixel modify R, G, B channels
    bit_idx = 0
    for y in range(height):
        for x in range(width):
            r, g, b, a = pixels[x, y]
            channels = [r, g, b]
            new_channels = []
            for ci in range(1):  # R, G, B
                try:
                    bit = next(bit_stream)
                    # clear the target bit, then set it to our bit
                    new_val = (channels[ci] & mask_clear) | (bit << BIT_INDEX)
                    new_channels.append(new_val)
                    bit_idx += 1
                except StopIteration:
                    # no more bits to embed; keep remaining channels unchanged
                    new_channels.append(channels[ci])
            # pixels[x, y] = (new_channels[0], new_channels[1], new_channels[2], a)
            pixels[x, y] = (new_channels[0], g, b, a)

            # early exit if we've embedded everything
            if bit_idx >= total_bits_needed:
                # copy remaining pixels as-is (they are already unchanged)
                img.save(output_path, "PNG")
                return

    # If we reach here all bits embedded; save image
    img.save(output_path, "PNG")


def extract_message(input_path: str, output_path: str = None):
    img = Image.open(input_path)
    img = img.convert("RGBA")
    pixels = img.load()
    width, height = img.size

    # read bits from the second bitplane in the same order (R,G,B per pixel)
    bits = []
    for y in range(height):
        for x in range(width):
            r, g, b, a = pixels[x, y]
            # for val in (r, g, b):
            #     bit = (val >> BIT_INDEX) & 1
            #     bits.append(bit)
            bit = (r >> BIT_INDEX) & 1
            bits.append(bit)

    # first 32 bits are length (big-endian)
    if len(bits) < 32:
        raise ValueError("Image too small or no embedded data.")

    length_bits = bits[:32]
    length_bytes = _bits_to_bytes(length_bits)
    msg_len = int.from_bytes(length_bytes, byteorder="big")

    total_bits_needed = 32 + msg_len * 8
    if len(bits) < total_bits_needed:
        raise ValueError(f"Image does not contain full message: expected {total_bits_needed} bits, found {len(bits)} bits.")

    msg_bits = bits[32:32 + msg_len * 8]
    message_bytes = _bits_to_bytes(msg_bits)

    if output_path:
        # write raw bytes to a file
        with open(output_path, "wb") as f:
            f.write(message_bytes)
    else:
        # print as UTF-8 if decodable, otherwise show repr
        try:
            text = message_bytes.decode("utf-8")
            print(text)
        except UnicodeDecodeError:
            print("Extracted raw bytes (non-UTF8):")
            print(message_bytes)

    return message_bytes


def main():
    parser = argparse.ArgumentParser(description="LSB steganography into the SECOND bitplane (bit index 1) of PNG images.")
    sub = parser.add_subparsers(dest="cmd", required=True)

    hide_p = sub.add_parser("hide", help="Embed a message into an image")
    hide_p.add_argument("-i", "--input", required=True, help="Input PNG file (cover image)")
    hide_p.add_argument("-o", "--output", required=True, help="Output PNG file (stego image)")
    group = hide_p.add_mutually_exclusive_group(required=True)
    group.add_argument("-m", "--message", help="Message text to embed (interpreted as UTF-8)")
    group.add_argument("-f", "--file", help="File to embed (embed raw bytes)")

    extract_p = sub.add_parser("extract", help="Extract a message from an image")
    extract_p.add_argument("-i", "--input", required=True, help="Input PNG file (stego image)")
    extract_p.add_argument("-o", "--output", help="Path to write recovered bytes. If omitted, prints to stdout (attempts UTF-8)")

    args = parser.parse_args()

    if args.cmd == "hide":
        if args.message is not None:
            msg_bytes = args.message.encode("utf-8")
        else:
            with open(args.file, "rb") as f:
                msg_bytes = f.read()

        try:
            embed_message(args.input, args.output, msg_bytes)
            print(f"Message embedded into {args.output} (bitplane {BIT_INDEX}).")
        except Exception as e:
            print("Error:", e, file=sys.stderr)
            sys.exit(1)

    elif args.cmd == "extract":
        try:
            message_bytes = extract_message(args.input, args.output)
            if args.output is None:
                # if printed to stdout, nothing else to do
                pass
            else:
                print(f"Recovered {len(message_bytes)} bytes -> {args.output}")
        except Exception as e:
            print("Error:", e, file=sys.stderr)
            sys.exit(1)


if __name__ == "__main__":
    main()
