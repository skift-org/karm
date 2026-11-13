module;

#include <karm/macros>

export module Karm.Font.Woff:woff2;

import Karm.Archive;
import Karm.Core;
import Karm.Font.Ttf;

import :base;

// https://www.w3.org/TR/WOFF2/
// https://github.com/google/woff2

namespace Karm::Font::Woff {

// MARK: Variable Length Integers ----------------------------------------------

// 4.1. Data types (UIntBase128)
static Res<u32> _nextBase128(Io::BScan& s) {
    u32 res = 0;
    for (usize i = 0; i < 5; i++) {
        u8 b = try$(s.nextU8be());
        if (i == 0 and b == 0x80)
            return Error::invalidData("woff2: UIntBase128 with leading zeros");
        if (res & 0xfe000000)
            return Error::invalidData("woff2: UIntBase128 overflow");
        res = res << 7 | (b & 0x7f);
        if (not(b & 0x80))
            return Ok(res);
    }
    return Error::invalidData("woff2: UIntBase128 too long");
}

// 4.1. Data types (255UInt16)
static Res<u16> _next255U16(Io::BScan& s) {
    static constexpr u8 WORD_CODE = 253;
    static constexpr u8 ONE_MORE_BYTE_CODE2 = 254;
    static constexpr u8 ONE_MORE_BYTE_CODE1 = 255;
    static constexpr u16 LOWEST_UCODE = 253;

    u8 code = try$(s.nextU8be());
    if (code == WORD_CODE)
        return s.nextU16be();
    if (code == ONE_MORE_BYTE_CODE1)
        return Ok<u16>(try$(s.nextU8be()) + LOWEST_UCODE);
    if (code == ONE_MORE_BYTE_CODE2)
        return Ok<u16>(try$(s.nextU8be()) + LOWEST_UCODE * 2);
    return Ok<u16>(code);
}

// MARK: Table Directory -------------------------------------------------------

// 4.2. Table directory (Known Table Tags)
static constexpr Array<u32, 63> _KNOWN_TAGS = {
    tag("cmap"),
    tag("head"),
    tag("hhea"),
    tag("hmtx"),
    tag("maxp"),
    tag("name"),
    tag("OS/2"),
    tag("post"),
    tag("cvt "),
    tag("fpgm"),
    tag("glyf"),
    tag("loca"),
    tag("prep"),
    tag("CFF "),
    tag("VORG"),
    tag("EBDT"),
    tag("EBLC"),
    tag("gasp"),
    tag("hdmx"),
    tag("kern"),
    tag("LTSH"),
    tag("PCLT"),
    tag("VDMX"),
    tag("vhea"),
    tag("vmtx"),
    tag("BASE"),
    tag("GDEF"),
    tag("GPOS"),
    tag("GSUB"),
    tag("EBSC"),
    tag("JSTF"),
    tag("MATH"),
    tag("CBDT"),
    tag("CBLC"),
    tag("COLR"),
    tag("CPAL"),
    tag("SVG "),
    tag("sbix"),
    tag("acnt"),
    tag("avar"),
    tag("bdat"),
    tag("bloc"),
    tag("bsln"),
    tag("cvar"),
    tag("fdsc"),
    tag("feat"),
    tag("fmtx"),
    tag("fvar"),
    tag("gvar"),
    tag("hsty"),
    tag("just"),
    tag("lcar"),
    tag("mort"),
    tag("morx"),
    tag("opbd"),
    tag("prop"),
    tag("trak"),
    tag("Zapf"),
    tag("Silf"),
    tag("Glat"),
    tag("Gloc"),
    tag("Feat"),
    tag("Sill"),
};

struct _Table {
    u32 tag;
    u8 version; //< Version of the transform applied to the table
    bool transformed;
    u32 origLength;
    usize srcOffset; //< Where the table is in the decompressed stream
    usize srcLength;
};

static Res<_Table> _nextTable(Io::BScan& s) {
    _Table t{};

    u8 flags = try$(s.nextU8be());
    if ((flags & 0x3f) == 0x3f)
        t.tag = try$(s.nextU32be());
    else
        t.tag = _KNOWN_TAGS[flags & 0x3f];

    // NOTE: 'glyf' and 'loca' are the odd ones out, version 0 is their
    //       transform and version 3 means they were left alone.
    t.version = flags >> 6;
    if (t.tag == tag("glyf") or t.tag == tag("loca"))
        t.transformed = t.version == 0;
    else
        t.transformed = t.version != 0;

    t.origLength = try$(_nextBase128(s));
    t.srcLength = t.origLength;
    if (t.transformed) {
        t.srcLength = try$(_nextBase128(s));
        if (t.tag == tag("loca") and t.srcLength != 0)
            return Error::invalidData("woff2: transformed 'loca' should be empty");
    }

    return Ok(t);
}

// MARK: Glyf Transform --------------------------------------------------------

struct _Glyf {
    Vec<u8> glyf;
    Vec<u8> loca;
    Vec<i16> xMins; //< By glyph, needed to rebuild 'hmtx'
};

struct _Point {
    i32 dx;
    i32 dy;
    bool onCurve;
};

// 5.2. Decoding of variable-length X and Y coordinates
static Res<_Point> _nextTriplet(u8 flag, Io::BScan& s) {
    bool onCurve = not(flag >> 7);
    flag &= 0x7f;

    auto withSign = [](usize flag, i32 val) {
        return (flag & 1) ? val : -val;
    };

    i32 dx, dy;
    if (flag < 10) {
        dx = 0;
        dy = withSign(flag, ((flag & 14) << 7) + try$(s.nextU8be()));
    } else if (flag < 20) {
        dx = withSign(flag, (((flag - 10) & 14) << 7) + try$(s.nextU8be()));
        dy = 0;
    } else if (flag < 84) {
        i32 b0 = flag - 20;
        i32 b1 = try$(s.nextU8be());
        dx = withSign(flag, 1 + (b0 & 0x30) + (b1 >> 4));
        dy = withSign(flag >> 1, 1 + ((b0 & 0x0c) << 2) + (b1 & 0x0f));
    } else if (flag < 120) {
        i32 b0 = flag - 84;
        auto b = try$(s.nextBytes(2));
        dx = withSign(flag, 1 + ((b0 / 12) << 8) + b[0]);
        dy = withSign(flag >> 1, 1 + (((b0 % 12) >> 2) << 8) + b[1]);
    } else if (flag < 124) {
        auto b = try$(s.nextBytes(3));
        dx = withSign(flag, (b[0] << 4) + (b[1] >> 4));
        dy = withSign(flag >> 1, ((b[1] & 0x0f) << 8) + b[2]);
    } else {
        auto b = try$(s.nextBytes(4));
        dx = withSign(flag, (b[0] << 8) + b[1]);
        dy = withSign(flag >> 1, (b[2] << 8) + b[3]);
    }

    return Ok(_Point{dx, dy, onCurve});
}

static void _pushPoints(BufferBuilder& out, Slice<_Point> points, bool overlap) {
    isize lastFlag = -1;
    u8 repeat = 0;

    for (usize i = 0; i < points.len(); i++) {
        auto& p = points[i];

        u8 flag = p.onCurve ? Ttf::Glyf::ON_CURVE_POINT : 0;
        if (overlap and i == 0)
            flag |= Ttf::Glyf::OVERLAY_SIMPLE;

        if (p.dx == 0)
            flag |= Ttf::Glyf::SAME_OR_POSITIVE_X;
        else if (p.dx > -256 and p.dx < 256)
            flag |= Ttf::Glyf::X_SHORT_VECTOR | (p.dx > 0 ? Ttf::Glyf::SAME_OR_POSITIVE_X : 0);

        if (p.dy == 0)
            flag |= Ttf::Glyf::SAME_OR_POSITIVE_Y;
        else if (p.dy > -256 and p.dy < 256)
            flag |= Ttf::Glyf::Y_SHORT_VECTOR | (p.dy > 0 ? Ttf::Glyf::SAME_OR_POSITIVE_Y : 0);

        if (lastFlag == flag and repeat != 255) {
            out.mutBytes()[out.len() - 1] |= Ttf::Glyf::REPEAT;
            repeat++;
        } else {
            if (repeat != 0)
                out.appendU8be(repeat);
            out.appendU8be(flag);
            repeat = 0;
        }
        lastFlag = flag;
    }

    if (repeat != 0)
        out.appendU8be(repeat);

    auto pushCoord = [&](i32 d) {
        if (d == 0)
            return;
        if (d > -256 and d < 256)
            out.appendU8be((u8)(d < 0 ? -d : d));
        else
            out.appendU16be((u16)d);
    };

    for (auto& p : points)
        pushCoord(p.dx);

    for (auto& p : points)
        pushCoord(p.dy);
}

struct _Composite {
    Bytes components;
    bool haveInstructions;
};

static Res<_Composite> _nextComposite(Io::BScan& s) {
    Bytes start = s.remBytes();
    bool haveInstructions = false;

    u16 flags = Ttf::Glyf::MORE_COMPONENTS;
    while (flags & Ttf::Glyf::MORE_COMPONENTS) {
        flags = try$(s.nextU16be());
        haveInstructions |= (flags & Ttf::Glyf::WE_HAVE_INSTRUCTIONS) != 0;

        usize size = 2; // glyphIndex
        size += (flags & Ttf::Glyf::ARG_1_AND_2_ARE_WORDS) ? 4 : 2;
        if (flags & Ttf::Glyf::WE_HAVE_A_SCALE)
            size += 2;
        else if (flags & Ttf::Glyf::WE_HAVE_AN_X_AND_Y_SCALE)
            size += 4;
        else if (flags & Ttf::Glyf::WE_HAVE_A_TWO_BY_TWO)
            size += 8;
        try$(s.skip(size));
    }

    return Ok(_Composite{sub(start, 0, start.len() - s.rem()), haveInstructions});
}

// 5.1. Transformed glyf table format
static Res<_Glyf> _rebuildGlyf(Bytes src, u32 locaOrigLength) {
    static constexpr u16 OVERLAP_SIMPLE_BITMAP = 0x0001;
    static constexpr usize BBOX = 2; //< Offset of the bounding box in a glyph

    Io::BScan s{src};
    try$(s.skip(2)); // reserved
    u16 optionFlags = try$(s.nextU16be());
    usize numGlyphs = try$(s.nextU16be());
    u16 indexFormat = try$(s.nextU16be());

    if (locaOrigLength != (indexFormat ? 4 : 2) * (numGlyphs + 1))
        return Error::invalidData("woff2: 'loca' doesn't fit the glyph count");

    Array<usize, 7> sizes;
    for (auto& size : sizes)
        size = try$(s.nextU32be());

    Io::BScan nContourStream = try$(s.nextBytes(sizes[0]));
    Io::BScan nPointsStream = try$(s.nextBytes(sizes[1]));
    Io::BScan flagStream = try$(s.nextBytes(sizes[2]));
    Io::BScan glyphStream = try$(s.nextBytes(sizes[3]));
    Io::BScan compositeStream = try$(s.nextBytes(sizes[4]));
    Io::BScan bboxStream = try$(s.nextBytes(sizes[5]));
    Io::BScan instructionStream = try$(s.nextBytes(sizes[6]));

    Bytes overlapBitmap;
    if (optionFlags & OVERLAP_SIMPLE_BITMAP)
        overlapBitmap = try$(s.nextBytes((numGlyphs + 7) >> 3));

    Bytes bboxBitmap = try$(bboxStream.nextBytes(((numGlyphs + 31) >> 5) << 2));

    auto bit = [](Bytes bitmap, usize i) -> bool {
        return bitmap.len() and (bitmap[i >> 3] & (0x80 >> (i & 7)));
    };

    _Glyf res;
    res.xMins.resize(numGlyphs);
    Vec<usize> offsets;
    Vec<_Point> points;

    BufferBuilder out;
    for (usize i = 0; i < numGlyphs; i++) {
        usize start = out.len();
        offsets.pushBack(start);

        bool haveBbox = bit(bboxBitmap, i);
        u16 nContours = try$(nContourStream.nextU16be());

        if (nContours == 0) {
            if (haveBbox)
                return Error::invalidData("woff2: empty glyph with a bounding box");
            continue;
        }

        out.appendU16be(nContours);

        if (nContours == 0xffff) {
            // NOTE: The bounding box of a composite can't be worked out
            //       without resolving its components.
            if (not haveBbox)
                return Error::invalidData("woff2: composite glyph without a bounding box");
            out.append(try$(bboxStream.nextBytes(8)));

            auto composite = try$(_nextComposite(compositeStream));
            out.append(composite.components);

            if (composite.haveInstructions) {
                u16 instructionLength = try$(_next255U16(glyphStream));
                out.appendU16be(instructionLength);
                out.append(try$(instructionStream.nextBytes(instructionLength)));
            }
        } else {
            out.appendFill(8); // xMin, yMin, xMax, yMax

            usize nPoints = 0;
            for (usize j = 0; j < nContours; j++) {
                nPoints += try$(_next255U16(nPointsStream));
                if (nPoints == 0 or nPoints > 0x10000)
                    return Error::invalidData("woff2: bad point count");
                out.appendU16be((u16)(nPoints - 1)); // endPtsOfContours
            }

            // NOTE: Points are kept as deltas, which is how they are stored
            //       on both sides. Only the bounding box wants positions.
            points.clear();
            i64 x = 0, y = 0;
            i64 xMin = 0, yMin = 0, xMax = 0, yMax = 0;
            auto flags = try$(flagStream.nextBytes(nPoints));
            for (usize j = 0; j < nPoints; j++) {
                auto p = try$(_nextTriplet(flags[j], glyphStream));
                points.pushBack(p);

                x += p.dx;
                y += p.dy;
                xMin = j ? min(xMin, x) : x;
                yMin = j ? min(yMin, y) : y;
                xMax = j ? max(xMax, x) : x;
                yMax = j ? max(yMax, y) : y;
            }

            if (haveBbox) {
                out.patch(start + BBOX, try$(bboxStream.nextBytes(8)));
            } else {
                out.patchU16be(start + BBOX, (u16)xMin);
                out.patchU16be(start + BBOX + 2, (u16)yMin);
                out.patchU16be(start + BBOX + 4, (u16)xMax);
                out.patchU16be(start + BBOX + 6, (u16)yMax);
            }

            u16 instructionLength = try$(_next255U16(glyphStream));
            out.appendU16be(instructionLength);
            out.append(try$(instructionStream.nextBytes(instructionLength)));

            _pushPoints(out, points, bit(overlapBitmap, i));
        }

        auto bbox = sub(out.bytes(), start + BBOX, start + BBOX + 8);
        res.xMins[i] = (i16)(bbox[0] << 8 | bbox[1]);

        out.align(4);
        if (out.len() > MAX_SFNT_SIZE)
            return Error::invalidData("woff2: 'glyf' table too large");
    }
    offsets.pushBack(out.len());

    BufferBuilder loca;
    for (auto offset : offsets) {
        if (indexFormat) {
            loca.appendU32be(offset);
        } else {
            if (offset > 0x1fffe)
                return Error::invalidData("woff2: 'glyf' table too large for a short 'loca'");
            loca.appendU16be(offset >> 1);
        }
    }

    res.glyf = out.take();
    res.loca = loca.take();

    return Ok(std::move(res));
}

// MARK: Hmtx Transform --------------------------------------------------------

// 5.4. Transformed hmtx table format
static Res<Vec<u8>> _rebuildHmtx(Bytes src, usize numHMetrics, Slice<i16> xMins) {
    static constexpr u8 NO_PROPORTIONAL_LSBS = 0x01;
    static constexpr u8 NO_MONOSPACE_LSBS = 0x02;

    usize numGlyphs = xMins.len();

    Io::BScan s{src};
    u8 flags = try$(s.nextU8be());

    if (flags & 0xfc)
        return Error::invalidData("woff2: reserved 'hmtx' flags set");

    if (not(flags & (NO_PROPORTIONAL_LSBS | NO_MONOSPACE_LSBS)))
        return Error::invalidData("woff2: transformed 'hmtx' leaves nothing out");

    if (numHMetrics < 1 or numHMetrics > numGlyphs)
        return Error::invalidData("woff2: bad 'hmtx' metrics count");

    Io::BScan advances = try$(s.nextBytes(numHMetrics * 2));

    // NOTE: A left side bearing that was left out is the xMin of its glyph.
    BufferBuilder out;
    for (usize i = 0; i < numGlyphs; i++) {
        bool proportional = i < numHMetrics;
        if (proportional)
            out.appendU16be(try$(advances.nextU16be()));

        if (flags & (proportional ? NO_PROPORTIONAL_LSBS : NO_MONOSPACE_LSBS))
            out.appendU16be((u16)xMins[i]);
        else
            out.appendU16be(try$(s.nextU16be()));
    }

    return Ok(out.take());
}

// MARK: Container -------------------------------------------------------------

export bool sniffWoff2(Bytes data) {
    return data.len() >= 4 and
           data[0] == 'w' and data[1] == 'O' and data[2] == 'F' and data[3] == '2';
}

/// The tables that went through a transform are rebuilt the same way the
/// reference implementation does it, so the result is the same file
/// `woff2_decompress` would write.
export Res<Vec<u8>> decodeWoff2(Bytes data) {
    static constexpr usize NUM_HMETRICS = 34; //< Offset in the 'hhea' table

    Io::BScan s{data};

    if (try$(s.nextU32be()) != tag("wOF2"))
        return Error::invalidData("woff2: bad signature");

    u32 flavor = try$(s.nextU32be());
    u32 length = try$(s.nextU32be());
    u16 numTables = try$(s.nextU16be());
    try$(s.skip(2)); // reserved
    try$(s.skip(4)); // totalSfntSize, recomputed from the directory
    u32 totalCompressedSize = try$(s.nextU32be());
    try$(s.skip(4));  // majorVersion, minorVersion
    try$(s.skip(12)); // metaOffset, metaLength, metaOrigLength
    try$(s.skip(8));  // privOffset, privLength

    if (length != data.len())
        return Error::invalidData("woff2: length mismatch");

    if (numTables == 0)
        return Error::invalidData("woff2: no tables");

    if (flavor == tag("ttcf"))
        return Error::notImplemented("woff2: font collections not supported");

    Vec<_Table> tables;
    usize total = 0;
    for (usize i = 0; i < numTables; i++) {
        auto t = try$(_nextTable(s));
        if (t.srcLength > MAX_SFNT_SIZE - total)
            return Error::invalidData("woff2: font too large");
        t.srcOffset = total;
        total += t.srcLength;
        tables.pushBack(t);
    }

    Io::BufferWriter stream{0, total};
    try$(Archive::brotliDecompress(try$(s.nextBytes(totalCompressedSize)), stream));
    if (stream.bytes().len() != total)
        return Error::invalidData("woff2: compressed stream shorter than declared");

    auto find = [&](u32 wanted) -> Opt<_Table> {
        for (auto& t : tables)
            if (t.tag == wanted)
                return Some(t);
        return NONE;
    };

    auto source = [&](_Table const& t) {
        return sub(stream.bytes(), t.srcOffset, t.srcOffset + t.srcLength);
    };

    // Transformed tables depend on each others, so they are rebuilt upfront
    // no matter where they sit in the directory.
    Opt<_Glyf> glyf = NONE;
    Opt<Vec<u8>> hmtx = NONE;

    auto glyfTable = find(tag("glyf"));
    auto locaTable = find(tag("loca"));
    if (glyfTable.has() != locaTable.has())
        return Error::invalidData("woff2: 'glyf' and 'loca' go together");

    if (glyfTable) {
        if (glyfTable->transformed != locaTable->transformed)
            return Error::invalidData("woff2: 'glyf' and 'loca' transforms go together");

        if (glyfTable->transformed)
            glyf = Some(try$(_rebuildGlyf(source(*glyfTable), locaTable->origLength)));
    }

    if (auto hmtxTable = find(tag("hmtx")); hmtxTable and hmtxTable->transformed) {
        if (hmtxTable->version != 1)
            return Error::invalidData("woff2: unknown 'hmtx' transform");

        auto hheaTable = find(tag("hhea"));
        if (not glyf or not hheaTable or hheaTable->transformed)
            return Error::invalidData("woff2: transformed 'hmtx' needs 'glyf' and 'hhea'");

        Io::BScan hhea = source(*hheaTable);
        try$(hhea.skip(NUM_HMETRICS));
        u16 numHMetrics = try$(hhea.nextU16be());

        hmtx = Some(try$(_rebuildHmtx(source(*hmtxTable), numHMetrics, glyf->xMins)));
    }

    // NOTE: Nothing stops a directory from listing a table twice, hence
    //       checking again that what was rebuilt is there.
    Ttf::SfntBuilder sfnt{flavor};
    for (auto& t : tables) {
        if (not t.transformed) {
            try$(sfnt.add(t.tag, source(t)));
        } else if (t.tag == tag("glyf") and glyf) {
            // The rebuilt 'loca' goes right after 'glyf' wherever the
            // directory has it, like the reference implementation does.
            try$(sfnt.add(t.tag, glyf->glyf));
            try$(sfnt.add(tag("loca"), glyf->loca));
        } else if (t.tag == tag("loca") and glyf) {
            continue;
        } else if (t.tag == tag("hmtx") and hmtx) {
            try$(sfnt.add(t.tag, *hmtx));
        } else {
            return Error::invalidData("woff2: unknown table transform");
        }
    }

    if (sfnt.len() != numTables)
        return Error::invalidData("woff2: missing tables");

    return sfnt.finish();
}

} // namespace Karm::Font::Woff
