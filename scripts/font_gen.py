#!/usr/bin/env python2
"""
Helper script to generate new 8x8 bitmap fonts for the editor
to replace the default "Microknight" font.

Usage:
1. Call this script without arguments and redirect the output into a text file.
2. Open the text file in an editor or word processor. Ensure it's opened using
   the Windows-1252 character set (default on Windows).
3. Use an 8x8 pixel font and ensure pixel-perfect reproduction. This requires
   turning off ClearType in Windows.
4. Make a screenshot and crop out the 112x112 area of the character map.
   Save it.
5. Run this script with the saved filename. It will replace the font.
"""
import sys, os
try:
    from PIL import Image, ImageOps
except ImportError:
    print >>sys.stderr, "PIL or Pillow is required"
    sys.exit(1)


def xy2char(x, y):
    c = 32 + y * 14 + x
    if c >= 0x80: c += 0x20
    if c >  0xFF: c  = 0x20
    return chr(c)


def char2xy(c):
    if c >= 0x80:
        if c < 0xA0:
            return (0, 0)
        c -= 0x20
    c -= 0x20
    return (c % 14, c / 14)


if __name__ == "__main__":
    if len(sys.argv) < 2:
        for y in xrange(14):
            print ''.join(xy2char(x, y) for x in xrange(14))
        sys.exit(0)

    src = Image.open(sys.argv[1]).convert('L')
    assert src.size == (112, 112)
    if src.getpixel((4, 4)) >= 128:
        src = ImageOps.invert(src)

    dest = Image.new('L', (128, 128), 0)
    for y in xrange(14):
        for x in xrange(14):
            dest.paste(src.crop((x*8, y*8, (x+1)*8, (y+1)*8)), (x*9+1, y*9+1))
    dest = dest.convert('1', dither=Image.NONE)

    f = open(os.path.join(os.path.dirname(sys.argv[0]), "../src/MicroknightFont.c"), "w")

    print >>f, '// automatically generated by font_gen.py'
    print >>f, '#include "MicroknightFont.h"'
    print >>f, 'unsigned char s_microkinghtFontData[] = {'

    data = map(ord, dest.tobytes())
    while data:
        print>>f, ''.join("0x%02X," % x for x in data[:16])
        del data[:16]
    print >>f, '};'
    print >>f, 'struct MicroknightLayout s_microknightLayout[] = {'
    for c in xrange(32, 256):
        x, y = char2xy(c)
        print >>f, '  { %3d, %3d },' % (x*9+1, y*9+1)
    print >>f, '};'

    f.close()
    dest.show()