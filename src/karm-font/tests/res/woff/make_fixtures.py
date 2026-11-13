#!/usr/bin/env python3

"""Builds the fonts used by the karm-font WOFF and subsetting tests.

Usage: make_fixtures.py [output directory]

Writes next to itself by default, see readme.md for what is needed to run it
and what comes out.
"""

import io
import logging
import pathlib
import shutil
import struct
import subprocess
import sys
import zlib

from fontTools.fontBuilder import FontBuilder
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.ttLib import TTFont
from fontTools.ttLib.tables import ttProgram
from fontTools.ttLib.tables._g_l_y_f import Glyph, GlyphComponent
from fontTools.ttLib.woff2 import WOFF2FlavorData

GLYPH_ORDER = [".notdef", "space", "A", "acute", "Aacute", "stairs", "wide", "AA"]

CMAP = {
    0x20: "space",
    0x41: "A",
    0x73: "stairs",  # s
    0x77: "wide",  # w
    0xB4: "acute",
    0xC1: "Aacute",
    0xC5: "AA",  # Aring, stands for a composite of composites
}


# MARK: Glyphs -----------------------------------------------------------------


def notdef(p):
    p.moveTo((50, 0)); p.lineTo((50, 700)); p.lineTo((450, 700)); p.lineTo((450, 0)); p.closePath()
    p.moveTo((100, 50)); p.lineTo((400, 50)); p.lineTo((400, 650)); p.lineTo((100, 650)); p.closePath()


def letter_a(p):
    # Curves, and deltas of every size the WOFF2 triplet encoding tells apart
    p.moveTo((0, 0)); p.lineTo((300, 700)); p.qCurveTo((330, 760), (360, 700)); p.lineTo((700, 0))
    p.lineTo((560, 0)); p.lineTo((480, 200)); p.lineTo((200, 200)); p.lineTo((130, 0)); p.closePath()
    p.moveTo((250, 300)); p.lineTo((440, 300)); p.qCurveTo((400, 420), (345, 520)); p.closePath()


def stairs(p):
    # 280 points in a row sharing their flags, more than a repeat count holds
    # so the run has to be split
    p.moveTo((0, 0))
    for i in range(1, 281):
        p.lineTo((i * 2, i * 2))
    p.lineTo((560, 0))
    p.closePath()


def wide(p):
    # Far away points, deltas that need 12 and 16 bits
    p.moveTo((-3000, -2500)); p.lineTo((1500, -2500)); p.lineTo((1500, 30000)); p.lineTo((1490, 30000))
    p.lineTo((1490, -2400)); p.lineTo((-3000, -2400)); p.closePath()


def acute(p):
    p.moveTo((0, 0)); p.lineTo((100, 150)); p.lineTo((160, 150)); p.lineTo((60, 0)); p.closePath()


def simple(draw, program=None):
    pen = TTGlyphPen(None)
    draw(pen)
    g = pen.glyph()
    if program is not None:
        g.program = ttProgram.Program()
        g.program.fromBytecode(program)
    return g


def composite(parts, program=None):
    """parts: (glyph name, x, y, None | scale | (xx, xy, yx, yy))"""
    g = Glyph()
    g.numberOfContours = -1
    g.components = []
    for name, x, y, scale in parts:
        c = GlyphComponent()
        c.glyphName = name
        c.x = x
        c.y = y
        c.flags = 0x0004  # ROUND_XY_TO_GRID
        if isinstance(scale, tuple):
            c.transform = [[scale[0], scale[1]], [scale[2], scale[3]]]
        elif scale is not None:
            c.transform = [[scale, 0], [0, scale]]
        g.components.append(c)
    if program is not None:
        g.program = ttProgram.Program()
        g.program.fromBytecode(program)
    return g


# MARK: Fonts ------------------------------------------------------------------


def build_sfnt():
    glyphs = {
        ".notdef": simple(notdef),
        "space": Glyph(),
        "A": simple(letter_a, program=b"\xb0\x00\x2f\xb0\x01\x2f"),
        "acute": simple(acute),
        "Aacute": composite(
            [("A", 0, 0, None), ("acute", 300, 800, 0.5)],
            program=b"\xb0\x01\x21",
        ),
        "stairs": simple(stairs),
        "wide": simple(wide),
        "AA": composite(
            [
                ("A", 0, 0, None),
                ("A", 720, -10, (1.0, 0.25, 0.0, 1.0)),
                ("Aacute", 1500, 0, (0.75, 0.0, 0.0, 1.25)),
            ]
        ),
    }

    fb = FontBuilder(1000, isTTF=True)
    fb.setupGlyphOrder(GLYPH_ORDER)
    fb.setupCharacterMap(CMAP)
    fb.setupGlyf(glyphs)

    glyf = fb.font["glyf"]
    metrics = {}
    for name in GLYPH_ORDER:
        g = glyf[name]
        g.recalcBounds(glyf)
        metrics[name] = (800, g.xMin if g.numberOfContours else 0)
    metrics["space"] = (250, 0)

    fb.setupHorizontalMetrics(metrics)
    fb.setupHorizontalHeader(ascent=800, descent=-200)
    fb.setupNameTable({"familyName": "Karm Woff Test", "styleName": "Regular"})
    fb.setupOS2(sTypoAscender=800, usWinAscent=800, usWinDescent=200)
    fb.setupPost()
    fb.setupMaxp()

    # Keeps the output the same from a run to the next
    fb.font["head"].created = fb.font["head"].modified = 0

    buf = io.BytesIO()
    fb.font.save(buf)
    return buf.getvalue()


def wrap_woff1(sfnt):
    """Wraps a sfnt, keeping the tables in the order they are in the file.

    fontTools can write WOFF 1.0 too but it sorts the tables by tag, decoding
    such a file gives back a different file than the one that went in.
    """
    (num,) = struct.unpack(">H", sfnt[4:6])
    tables = [struct.unpack(">4sIII", sfnt[12 + 16 * i : 28 + 16 * i]) for i in range(num)]

    offset = 44 + 20 * num
    directory = {}
    body = b""
    for tag, checksum, start, length in sorted(tables, key=lambda t: t[2]):
        raw = sfnt[start : start + length]
        comp = zlib.compress(raw, 9)
        if len(comp) >= length:
            comp = raw
        directory[tag] = struct.pack(">4sIIII", tag, offset + len(body), len(comp), length, checksum)
        body += comp + b"\0" * (-len(comp) % 4)

    total = 12 + 16 * num + sum((t[3] + 3) & ~3 for t in tables)
    header = struct.pack(
        ">4s4sIHHIHHIIIII",
        b"wOFF", sfnt[:4], offset + len(body), num, 0, total,
        1, 0,  # majorVersion, minorVersion
        0, 0, 0,  # metaOffset, metaLength, metaOrigLength
        0, 0,  # privOffset, privLength
    )  # fmt: skip
    return header + b"".join(directory[t[0]] for t in sorted(tables)) + body


def run(*cmd):
    subprocess.run([str(c) for c in cmd], check=True, capture_output=True)


def main():
    # fontTools has things to say about the timestamps being zero
    logging.getLogger("fontTools").setLevel(logging.ERROR)

    out = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else pathlib.Path(__file__).parent
    sfnt = build_sfnt()

    # tiny: the bounding box of 'acute' is made not to be the one of its
    # points, a WOFF2 encoder then has to store it instead of leaving it out.
    font = TTFont(io.BytesIO(sfnt), recalcBBoxes=False, recalcTimestamp=False)
    font["glyf"]["acute"].xMin -= 7
    font["glyf"]["acute"].yMax += 9
    font.save(out / "tiny.src.ttf")
    run("woff2_compress", out / "tiny.src.ttf")
    shutil.move(out / "tiny.src.woff2", out / "tiny.woff2")
    (out / "tiny.src.ttf").unlink()
    run("woff2_decompress", out / "tiny.woff2")  # writes tiny.ttf
    (out / "tiny.woff").write_bytes(wrap_woff1((out / "tiny.ttf").read_bytes()))

    # hmtx: woff2_compress never transforms 'hmtx', fontTools does when asked
    # and when every left side bearing is the xMin of its glyph.
    font = TTFont(io.BytesIO(sfnt), recalcBBoxes=False, recalcTimestamp=False)
    font.flavor = "woff2"
    font.flavorData = WOFF2FlavorData(transformedTables=("glyf", "loca", "hmtx"))
    font.save(out / "hmtx.woff2")
    run("woff2_decompress", out / "hmtx.woff2")  # writes hmtx.ttf

    # Second opinion, fontTools has to read the same thing out of each pair
    wrapped = TTFont(out / "tiny.woff")
    plain = TTFont(out / "tiny.ttf")
    for tag in plain.reader.keys():
        assert wrapped.reader[tag] == plain.reader[tag], tag

    wrapped = TTFont(out / "hmtx.woff2")
    plain = TTFont(out / "hmtx.ttf")
    assert wrapped["hmtx"].metrics == plain["hmtx"].metrics
    for name in wrapped.getGlyphOrder():
        assert wrapped["glyf"][name] == plain["glyf"][name], name

    for name in ["tiny.ttf", "tiny.woff", "tiny.woff2", "hmtx.ttf", "hmtx.woff2"]:
        print(f"{name:12} {(out / name).stat().st_size:5} bytes")


if __name__ == "__main__":
    main()
