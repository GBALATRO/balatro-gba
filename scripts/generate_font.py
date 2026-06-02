#!/usr/bin/env python3

from PIL import Image, ImageOps
import argparse

CHAR_WIDTH = 8
CHAR_HEIGHT = 8
NUM_VERT_CHARS = 6
NUM_HORZ_CHARS = 16
WORD_SIZE = 32

parser = argparse.ArgumentParser()
parser.add_argument("-i", "--input",  required=True, help="input file")
parser.add_argument("-o", "--output", required=True, help="output file")

args = parser.parse_args()

in_path  = args.input
out_path = args.output

# The font is stored in a 4-bit colormap png and is made of a grid of
# 16x6 characters:
#   |-------------16---------------|
# | [][][][][][][][][][][][][][][][]
# | [][][][][][][][][][][][][][][][]
# 6 [][][][][][][][][][][][][][][][]
# | [][][][][][][][][][][][][][][][]
# | [][][][][][][][][][][][][][][][]
# | [][][][][][][][][][][][][][][][]
# 
# Each character is a bitmap of 8x8 pixels.
#
# For example, the ! : 
#
# ___00___
# ___00___
# ___00___
# ___00___
# ___00___
# ___00___
# ________
# ___00___
# 
# The font file is organized to go character-by-character and left-to-right

img = Image.open(in_path)
img = img.convert("P")
img = img.convert('1')
img = ImageOps.invert(img)

glyphs = []
for char_y in range(NUM_VERT_CHARS):
    for char_x in range(NUM_HORZ_CHARS):
        # Get the 8x8 crop area at our current character in the image
        pix_x = char_x * CHAR_WIDTH
        pix_y = char_y * CHAR_HEIGHT
        crop_area = (pix_x, pix_y, pix_x + CHAR_WIDTH, pix_y + CHAR_HEIGHT)
        
        pixel_block_img = img.crop(crop_area)
        # Note: "getdata" is a deprecated call in recent versions of pillow in
        # favor of "get_flattened_data". However, as of May 17, 2026 the CI uses
        # an earlier version of pillow that doesn't have "get_flattened_data"
        # You can safely ignore the deprecation warning when builing on your host
        pixels = list(pixel_block_img.getdata())

        word0 = 0
        word1 = 0
        for i, pixel in enumerate(pixels):
            bit = 1 if pixel > 0 else 0
            if i < WORD_SIZE:
                word0 = word0 | (bit << i)
            else:
                word1 = word1 | (bit << (i - WORD_SIZE))
        glyphs.append((word0, word1))

NUM_WORDS_IN_ROW = 8
first = True

with open(out_path, "w") as out:
    out.write("""

@{{BLOCK(gbalatro_sys8)

    .section .rodata
    .align	2
    .global	gbalatro_sys8Font
gbalatro_sys8Font:
    .word	gbalatro_sys8Glyphs, 0, 0
    .hword	32, 96
    .byte	8, 8
    .byte	8, 8
    .hword	8
    .byte	1, 0

    .section .rodata
    .align	2
    .global gbalatro_sys8Glyphs		@ 768 bytes (192 unsigned ints)
gbalatro_sys8Glyphs:
""")
    
    for i, glyph in enumerate(glyphs):
        if first:
            out.write("    .word ")
            first = False

        out.write(f"0x{glyph[0]:08X},0x{glyph[1]:08X}")

        # This checks to see if we have reached NUM_WORDS_IN_ROW words, (divided by
        # 2 because that's the number of entries in each char[])
        if (((i + 1) % (NUM_WORDS_IN_ROW // 2)) == 0):
            out.write("\n")
            if i != (len(glyphs) - 1):
                out.write("    .word ")
        else:
            out.write(",")
            

    out.write("\n")
    out.write("\n")
    out.write('@}}BLOCK(gbalatro_sys8)\n')
