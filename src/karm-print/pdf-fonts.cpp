export module Karm.Print:pdfFonts;

import Karm.Font.Ttf;
import Karm.Pdf;
import Karm.Core;
import Karm.Gfx;
import Karm.Logger;
import Karm.Ref;

using namespace Karm::Literals;

namespace Karm::Print {

export struct TrueTypeFontAdapter {
    Rc<Font::Ttf::Fontface> _font;
    Slice<Gfx::Glyph> _glyphs;
    Slice<Vec<Rune>> _texts;
    Opt<Gfx::FontSubset> _subset;

    String registry = "Adobe"s;
    String ordering = "Identity"s;

    Pdf::Ref CIDFontRef;
    Pdf::Ref CIDSystemInfoRef;
    Opt<Pdf::Ref> CIDToGIDMapRef;
    Pdf::Ref fontFileRef;
    Pdf::Ref fontDescriptorRef;
    Pdf::Ref toUnicodeRef;
    Pdf::Ref fontRef;

    Pdf::Name CIDFontName;

    TrueTypeFontAdapter(Rc<Font::Ttf::Fontface> font, Slice<Gfx::Glyph> glyphs, Slice<Vec<Rune>> texts, Pdf::Ref& alloc)
        : _font(font),
          _glyphs(glyphs),
          _texts(texts),
          CIDFontRef(alloc.alloc()),
          CIDSystemInfoRef(alloc.alloc()),
          fontFileRef(alloc.alloc()),
          fontDescriptorRef(alloc.alloc()),
          toUnicodeRef(alloc.alloc()),
          fontRef(alloc.alloc()) {
        auto subset = font->subset(glyphs);
        if (not subset)
            logWarn("pdf: cannot subset font, embedding it whole: {}", subset.none());
        else if (subset.expect().format != Ref::Uti::PUBLIC_TTF)
            logWarn("pdf: cannot embed font subset of type {}, embedding the font whole", subset.expect().format);
        else
            _subset = Some(subset.take());

        if (not _subset)
            CIDToGIDMapRef = Some(alloc.alloc());

        auto postscriptName = font->_parser._name.lookupString(Font::Ttf::Name::POSTSCRIPT);
        if (_subset)
            CIDFontName = Pdf::Name{Io::format("{}+{}", _subsetTag(), postscriptName.str())};
        else
            CIDFontName = Pdf::Name{postscriptName.str()};
    }

    // https://www.iso.org/standard/75839.html 9.9.2 Font subsets
    String _subsetTag() const {
        u64 hash = 0xcbf29ce484222325;
        for (auto const& glyph : _glyphs) {
            hash ^= glyph.index;
            hash *= 0x100000001b3;
        }

        StringBuilder tag;
        for (usize i = 0; i < 6; i++) {
            tag.append(static_cast<Rune>('A' + hash % 26));
            hash /= 26;
        }
        return tag.take();
    }

    usize fontDescriptorFlags() {
        // 9.8.2 Font descriptor flags
        usize flag = 1 << 2; // Symbolic

        auto attrs = _font->attrs();
        if (attrs.monospace == Gfx::Monospace::YES)
            flag |= 1;

        if (attrs.style == Gfx::FontStyle::ITALIC)
            flag |= (1 << 6);

        return flag;
    }

    Pdf::Array fontBBox() {
        auto head = _font->_parser._head.bytes();
        auto at = [&](usize offset) -> f64 {
            return static_cast<i16>((head[offset] << 8) | head[offset + 1]);
        };
        f64 scale = 1000.0 / _font->_parser._head.unitPerEm();
        return Pdf::Array{at(36) * scale, at(38) * scale, at(40) * scale, at(42) * scale};
    }

    Pdf::Array widths() {
        // 9.7.4.3 Glyph metrics in CIDFonts
        Pdf::Array widths;
        for (auto const& glyph : _glyphs)
            widths.pushBack(_font->advance(glyph) * 1000);
        return Pdf::Array{usize{0}, std::move(widths)};
    }

    Pdf::Stream fontFile() {
        // 9.9 Embedded font programs
        Bytes data = _subset ? Bytes{_subset->data} : _font->_mmap.bytes();
        return Pdf::Stream::flate(
            data,
            Pdf::Dict{
                {"Length1"s, data.len()},
            }
        );
    }

    Pdf::Stream CIDToGIDMap() {
        Vec<u8> buf;
        for (auto const& glyph : _glyphs) {
            buf.pushBack(glyph.index >> 8);
            buf.pushBack(glyph.index & 0xff);
        }
        return Pdf::Stream::flate(buf);
    }

    // 9.10.3 ToUnicode CMaps
    Pdf::Stream toUnicode() {
        Io::StringWriter cmap;
        (void)Io::format(
            cmap,
            "/CIDInit /ProcSet findresource begin\n"
            "12 dict begin\n"
            "begincmap\n"
            "/CIDSystemInfo << /Registry (Adobe) /Ordering (UCS) /Supplement 0 >> def\n"
            "/CMapName /Adobe-Identity-UCS def\n"
            "/CMapType 2 def\n"
            "1 begincodespacerange\n"
            "<0000> <FFFF>\n"
            "endcodespacerange\n"
        );

        Vec<usize> codes;
        for (usize code = 1; code < _texts.len(); code++)
            if (any(_texts[code]))
                codes.pushBack(code);

        for (usize start = 0; start < codes.len(); start += 100) {
            usize end = min(start + 100, codes.len());
            (void)Io::format(cmap, "{} beginbfchar\n", end - start);
            for (usize i = start; i < end; i++) {
                (void)Io::format(cmap, "<{:04X}> <", codes[i]);
                for (auto rune : _texts[codes[i]]) {
                    if (rune < 0x10000) {
                        (void)Io::format(cmap, "{:04X}", rune);
                    } else {
                        u32 v = rune - 0x10000;
                        (void)Io::format(cmap, "{:04X}{:04X}", 0xD800 + (v >> 10), 0xDC00 + (v & 0x3FF));
                    }
                }
                (void)Io::format(cmap, ">\n");
            }
            (void)Io::format(cmap, "endbfchar\n");
        }

        (void)Io::format(
            cmap,
            "endcmap\n"
            "CMapName currentdict /CMap defineresource pop\n"
            "end\n"
            "end\n"
        );

        return Pdf::Stream::flate(cmap.bytes());
    }

    Pdf::Dict CIDSystemInfo() {
        // 9.7.3 CIDSystemInfo dictionaries
        return Pdf::Dict{
            {"Registry"s, registry},
            {"Ordering"s, ordering},
            {"Supplement"s, usize{0}},
        };
    }

    Pdf::Dict font() {
        // 9.7.6 Type 0 font dictionaries
        return Pdf::Dict{
            {"Type"s, Pdf::Name{"Font"s}},
            {"Subtype"s, Pdf::Name{"Type0"s}},
            {"BaseFont"s, CIDFontName},
            {"Encoding"s, Pdf::Name{"Identity-H"}},
            {"DescendantFonts"s, Pdf::Array{CIDFontRef}},
            {"ToUnicode"s, toUnicodeRef},
        };
    }

    Pdf::Dict fontDescriptors() {
        auto metrics = _font->metrics();

        // 9.8 Font descriptors
        // 9.8.3 Font descriptors for CIDFonts
        return Pdf::Dict{
            {"Type"s, Pdf::Name{"FontDescriptor"s}},
            {"FontName"s, CIDFontName},
            {"Flags"s, fontDescriptorFlags()},
            {"FontBBox"s, fontBBox()},
            {"FontFile2"s, fontFileRef},
            {"ItalicAngle"s, _font->_parser._post.italicAngle()},
            {"Ascent"s, metrics.ascend * 1000},
            {"Descent"s, metrics.descend * -1000},
            {"CapHeight"s, metrics.captop * 1000},
            {"StemV"s, usize{0}},
        };
    }

    Pdf::Dict CIDFont() {
        // 9.7.4 CIDFonts
        Pdf::Dict dict{
            {"Type"s, Pdf::Name{"Font"s}},
            {"Subtype"s, Pdf::Name{"CIDFontType2"s}},
            {"BaseFont"s, CIDFontName},
            {"CIDSystemInfo"s, CIDSystemInfoRef},
            {"W"s, widths()},
            {"FontDescriptor"s, fontDescriptorRef},
        };

        if (auto ref = CIDToGIDMapRef)
            dict.put("CIDToGIDMap"s, *ref);
        else
            dict.put("CIDToGIDMap"s, Pdf::Name{"Identity"s});

        return dict;
    }

    Pdf::Ref addToFile(Pdf::File& file) {
        if (auto ref = CIDToGIDMapRef)
            file.add(*ref, CIDToGIDMap());
        file.add(CIDSystemInfoRef, CIDSystemInfo());
        file.add(fontFileRef, fontFile());
        file.add(toUnicodeRef, toUnicode());
        file.add(CIDFontRef, CIDFont());
        file.add(fontDescriptorRef, fontDescriptors());

        file.add(fontRef, font());
        return fontRef;
    }
};

} // namespace Karm::Print
