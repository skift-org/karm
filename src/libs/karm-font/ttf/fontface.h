#pragma once

import Karm.Core;
import Karm.Sys;
import Karm.Gfx;

#include "../ttf/table-cmap.h"
#include "../ttf/table-glyf.h"
#include "../ttf/table-gpos.h"
#include "../ttf/table-gsub.h"
#include "../ttf/table-head.h"
#include "../ttf/table-hhea.h"
#include "../ttf/table-hmtx.h"
#include "../ttf/table-loca.h"
#include "../ttf/table-name.h"
#include "../ttf/table-os2.h"
#include "../ttf/table-post.h"

namespace Karm::Font::Ttf {

struct Table {
    Array<char, 4> tag;
    Bytes data;

    template <typename T>
    Res<T> open() {
        if (tag != T::SIG)
            return Error::invalidData("table tag mismatch");
        return Ok<T>(data);
    }
};

struct Container {
    virtual ~Container() = default;

    virtual Generator<Table> iterTables() = 0;

    Res<Table> requireTable(Str tag) {
        for (auto table : iterTables())
            if (table.tag == tag)
                return Ok(table);

        logError("ttf: '{}' table not found", tag);
        return Error::other("table not found");
    }

    template <typename T>
    Res<T> requireTable() {
        return try$(requireTable(T::SIG)).template open<T>();
    }
};

// https://tchayen.github.io/posts/ttf-file-parsing
// http://stevehanov.ca/blog/?id=143
// https://www.microsoft.com/typography/otspec/otff.htm
// https://docs.microsoft.com/en-us/typography/opentype/spec/glyf
// https://docs.microsoft.com/en-us/typography/opentype/spec/ttch01
// https://docs.microsoft.com/en-us/typography/opentype/spec/otff
// https://fontdrop.info/
struct Fontface : Gfx::Fontface {
    Rc<Container> _container;

    Head _head;
    Cmap _cmap;
    Cmap::Table _cmapTable;
    Glyf _glyf;
    Loca _loca;
    Hhea _hhea;
    Hmtx _hmtx;
    Gpos _gpos;
    Gsub _gsub;
    Name _name;
    Post _post;
    Os2 _os2;

    f64 _unitPerEm = 0;

    static Res<Rc<Fontface>> load(Rc<Container> container) {
        auto ff = makeRc<Fontface>();

        ff->_head = try$(container->requireTable<Head>());
        ff->_cmap = try$(container->requireTable<Cmap>());
        ff->_cmapTable = try$(ff->_cmap.chooseTable());
        ff->_glyf = try$(container->requireTable<Glyf>());
        ff->_loca = try$(container->requireTable<Loca>());
        ff->_hhea = try$(container->requireTable<Hhea>());
        ff->_hmtx = try$(container->requireTable<Hmtx>());
        ff->_gpos = container->requireTable<Gpos>().unwrapOr({});
        ff->_gsub = container->requireTable<Gsub>().unwrapOr({});
        ff->_name = container->requireTable<Name>().unwrapOr({});
        ff->_post = container->requireTable<Post>().unwrapOr({});
        ff->_os2 = container->requireTable<Os2>().unwrapOr({});
        ff->_unitPerEm = ff->_head.unitPerEm();

        return Ok(ff);
    }

    Fontface(Rc<Container> c)
        : _container(c) {
    }

    struct GlyphMetrics {
        f64 x;
        f64 y;
        f64 width;
        f64 height;
        f64 lsb;
        f64 advance;
    };

    GlyphMetrics glyphMetrics(Gfx::Glyph glyph) const {
        auto glyfOffset = _loca.glyfOffset(glyph.index, _head);
        auto glyf = _glyf.metrics(glyfOffset);
        auto hmtx = _hmtx.metrics(glyph.index, _hhea);

        return {
            static_cast<f64>(glyf.xMin),
            static_cast<f64>(-glyf.yMax),
            static_cast<f64>(glyf.xMax) - glyf.xMin,
            static_cast<f64>(glyf.yMax) - glyf.yMin,
            static_cast<f64>(hmtx.lsb),
            static_cast<f64>(hmtx.advanceWidth),
        };
    }

    Gfx::FontMetrics metrics() override {
        auto ascender = static_cast<f64>(_hhea.ascender());
        auto descender = static_cast<f64>(_hhea.descender());
        auto lineGap = static_cast<f64>(_hhea.lineGap());
        auto maxWidth = static_cast<f64>(_hhea.advanceWidthMax());
        auto xHeight = glyphMetrics(glyph('x')).y;

        return {
            .ascend = ascender / _unitPerEm,
            .captop = ascender / _unitPerEm,
            .descend = descender / _unitPerEm,
            .linegap = lineGap / _unitPerEm,
            .advance = maxWidth / _unitPerEm,
            .xHeight = xHeight / _unitPerEm,
        };
    }

    Gfx::FontAttrs attrs() const override {
        Gfx::FontAttrs attrs;

        if (_name.present()) {
            auto name = _name;
            attrs.family = Symbol::from(name.string(name.lookupRecord(Ttf::Name::FAMILY)));
        }

        if (_post.present()) {
            if (_post.isFixedPitch())
                attrs.monospace = Gfx::Monospace::YES;

            if (_post.italicAngle() != 0)
                attrs.style = Gfx::FontStyle::ITALIC;
        }

        if (_os2.present()) {
            attrs.weight = Gfx::FontWeight{_os2.weightClass()};
            attrs.stretch = Gfx::FontStretch{static_cast<u16>(_os2.widthClass() * 100)};
        }

        return attrs;
    }

    Map<Rune, Gfx::Glyph> _cachedEntries;

    Gfx::Glyph glyph(Rune rune) override {
        auto glyph = _cachedEntries.tryGet(rune);
        if (glyph.has())
            return glyph.unwrap();
        auto g = _cmapTable.glyphIdFor(rune);
        _cachedEntries.put(rune, g);
        return g;
    }

    Map<Gfx::Glyph, f64> _cachedAdvances;

    f64 advance(Gfx::Glyph glyph) override {
        auto advance = _cachedAdvances.tryGet(glyph);
        if (advance.has())
            return advance.unwrap();
        auto a = glyphMetrics(glyph).advance / _unitPerEm;
        _cachedAdvances.put(glyph, a);
        return a;
    }

    Map<Pair<Gfx::Glyph>, f64> _cachedKerns;

    f64 kern(Gfx::Glyph prev, Gfx::Glyph curr) override {
        auto kern = _cachedKerns.tryGet({prev, curr});
        if (kern.has())
            return kern.unwrap();

        if (not _gpos.present())
            return 0;

        auto positioning = _gpos.adjustments(prev.index, curr.index);

        if (not positioning)
            return 0;

        auto k = positioning.unwrap().v0.xAdvance / _unitPerEm;
        _cachedKerns.put({prev, curr}, k);
        return k;
    }

    void contour(Gfx::Canvas& g, Gfx::Glyph glyph) const override {
        g.scale(1.0 / _unitPerEm);

        auto glyfOffset = _loca.glyfOffset(glyph.index, _head);
        if (glyfOffset == _loca.glyfOffset(glyph.index + 1, _head))
            return;

        _glyf.contour(g, glyfOffset);
    }
};

} // namespace Karm::Font::Ttf
