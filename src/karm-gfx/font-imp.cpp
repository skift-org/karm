module Karm.Gfx;

import Karm.Math;
import Karm.Core;
import :font;
import :canvas;

namespace Karm::Gfx {

// MARK: Fallback --------------------------------------------------------------

struct TofuFontface : Fontface {
    static constexpr f64 ADVANCE = 0.4;
    static constexpr f64 ASCEND = 0.8;
    static constexpr f64 CAPTOP = 0.7;
    static constexpr f64 XHEIGHT = 0.5;
    static constexpr f64 DESCEND = 0.2;
    static constexpr f64 LINEGAP = 0.2;

    static constexpr f64 INSET = 0.04;
    static constexpr f64 STROKE = 0.06;

    static constexpr u8 GLYPH_BLANK = 0;
    static constexpr u8 GLYPH_BOX = 1;
    static constexpr u8 GLYPH_NONE = 2;

    FontMetrics metrics() const override {
        return {
            .ascend = ASCEND,
            .captop = CAPTOP,
            .descend = DESCEND,
            .linegap = LINEGAP,
            .advance = ADVANCE,
            .xHeight = XHEIGHT,
        };
    }

    FontAttrs attrs() const override {
        return {
            .family = "Tofu"_sym,
            .monospace = Monospace::YES,
        };
    }

    Run shape(Slice<Rune> runes) const override {
        Run run;
        for (auto [r, i] : iter(runes) | Index()) {
            auto g = glyph(r);
            run.items.pushBack({
                .glyph = g,
                .advance = advance(g),
                .runeIndex = i,
            });
        }
        return run;
    }

    Glyph glyph(Rune rune) const override {
        if (isAsciiBlank(rune))
            return Glyph{GLYPH_BLANK, 0};
        if (isUnicodeControl(rune))
            return Glyph{GLYPH_NONE, 0};
        return Glyph{GLYPH_BOX, 0};
    }

    f64 advance(Glyph glyph) const override {
        return glyph.index == GLYPH_NONE ? 0 : ADVANCE;
    }

    f64 kern(Glyph, Glyph) const override {
        return 0;
    }

    void glyphContour(Canvas& g, Glyph glyph) const override {
        if (glyph.index != GLYPH_BOX)
            return;

        f64 x0 = INSET;
        f64 x1 = ADVANCE - INSET;
        f64 y0 = -CAPTOP;
        f64 y1 = 0;

        g.moveTo({x0, y0});
        g.lineTo({x1, y0});
        g.lineTo({x1, y1});
        g.lineTo({x0, y1});
        g.closePath();

        g.moveTo({x0 + STROKE, y0 + STROKE});
        g.lineTo({x0 + STROKE, y1 - STROKE});
        g.lineTo({x1 - STROKE, y1 - STROKE});
        g.lineTo({x1 - STROKE, y0 + STROKE});
        g.closePath();
    }
};

Rc<Fontface> Fontface::fallback() {
    return makeRc<TofuFontface>();
}

Font Font::fallback() {
    return {
        .fontface = Fontface::fallback(),
        .fontsize = 12,
    };
}

// MARK: Fontface --------------------------------------------------------------

void Fontface::paintGlyph(Canvas& g, Glyph glyph) const {
    g.beginPath();
    glyphContour(g, glyph);
    g.fill();
}

// MARK: Font Family -----------------------------------------------------------

Flags<GlyphAttr> FontFamily::glyphAttr(Glyph glyph) const {
    return _members[glyph.font].face->glyphAttr(glyph);
}

void FontFamily::paintGlyph(Canvas& g, Glyph glyph) const {
    auto& member = _members[glyph.font];
    g.scale(_adjust.sizeAdjust * member.adjust.sizeAdjust);
    member.face->paintGlyph(g, glyph);
}

void FontFamily::glyphContour(Canvas& g, Glyph glyph) const {
    auto& member = _members[glyph.font];
    g.scale(_adjust.sizeAdjust * member.adjust.sizeAdjust);
    member.face->glyphContour(g, glyph);
}

// MARK: Font ------------------------------------------------------------------

void Font::glyphContour(Canvas& g, Glyph glyph) const {
    g.scale(fontsize);
    fontface->glyphContour(g, glyph);
}

} // namespace Karm::Gfx
