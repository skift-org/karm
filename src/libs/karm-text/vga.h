#pragma once

import Karm.Core;

#include <karm-gfx/canvas.h>

#include "font.h"

namespace Karm::Text {

struct VgaFontface : Fontface {
    static constexpr isize WIDTH = 8;
    static constexpr isize HEIGHT = 8;
    static constexpr f64 UNIT_PER_EM = 8;

    static constexpr Array<u8, 1024> const DATA = {
#include "vga.inc"
    };

    FontMetrics metrics() override {
        return {
            .ascend = 12 / UNIT_PER_EM,
            .captop = 10 / UNIT_PER_EM,
            .descend = 4 / UNIT_PER_EM,
            .linegap = 4 / UNIT_PER_EM,
            .advance = 8 / UNIT_PER_EM,
            .xHeight = 6 / UNIT_PER_EM,
        };
    }

    FontAttrs attrs() const override {
        return {
            .family = "IBM VGA8"_sym,
            .monospace = Monospace::YES,
        };
    }

    Glyph glyph(Rune rune) override {
        One<Ibm437> one;
        encodeOne<Ibm437>(rune, one);
        return Glyph(one);
    }

    f64 advance(Glyph) override {
        return WIDTH / UNIT_PER_EM;
    }

    f64 kern(Glyph, Glyph) override {
        return 0;
    }

    void contour(Gfx::Canvas& g, Glyph glyph) const override {
        g.scale(1 / UNIT_PER_EM);
        for (isize y = 0; y < HEIGHT; y++) {
            for (isize x = 0; x < WIDTH; x++) {
                u8 byte = DATA[glyph.index * HEIGHT + y];
                if (byte & (0x80 >> x)) {
                    g.rect(Math::Recti{x, y - 8, 1, 1}.cast<f64>());
                }
            }
        }
    }
};

} // namespace Karm::Text
