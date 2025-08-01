#pragma once

import Karm.Core;

#include "book.h"
#include "font.h"

namespace Karm::Text {

struct FontAdjust {
    f64 ascent = 1;
    f64 descent = 1;
    f64 linegap = 1;
    f64 sizeAdjust = 1;

    FontAdjust combine(FontAdjust other) const;
};

struct FontFamily : Fontface {
    struct Member {
        FontAdjust adjust;
        Rc<Fontface> face;
        Opt<Ranges<Range<Rune>>> ranges;
    };

    FontAdjust _adjust;
    Vec<Member> _members;

    FontFamily(Vec<Member> members) : _members(std::move(members)) {}

    struct Builder : Meta::NoCopy {
        FontBook& book;
        Vec<Member> members = {};
        FontAdjust adjust = {};

        Builder(FontBook& book) : book(book) {}

        Builder& add(FontQuery query);

        Builder& withAdjust(FontAdjust adjust);

        Builder& adjustAll(FontAdjust adjust);

        Builder& withRange(Range<Rune> range);

        Rc<FontFamily> bake();
    };

    static Builder make(FontBook& book);

    FontMetrics metrics() override;

    FontAttrs attrs() const override;

    Glyph glyph(Rune rune) override;

    f64 advance(Glyph glyph) override;

    f64 kern(Glyph prev, Glyph curr) override;

    void contour(Gfx::Canvas& g, Glyph glyph) const override;
};

} // namespace Karm::Text
