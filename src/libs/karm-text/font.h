#pragma once

#include <karm-gfx/canvas.h>
#include <karm-math/rect.h>

#include "base.h"

namespace Karm::Text {

/**
   _____       _             _                          <- ascend
  / ____|     | |           | |                         <- captop/cap height
 | |  __ _   _| |_ ___ _ __ | |__   ___ _ __ __ _
 | | |_ | | | | __/ _ \ '_ \| '_ \ / _ \ '__/ _` |
 | |__| | |_| | ||  __/ | | | |_) |  __/ | | (_| |
  \_____|\__,_|\__\___|_| |_|_.__/ \___|_|  \__, |      <- baseline (origin)
                                             __/ |
                                            |___/       <- descend
 | ---- |                                               ...advance

                                                        <- line gap
*/
struct FontMetrics {
    f64 ascend;
    f64 captop;
    f64 descend;
    f64 linegap;
    f64 advance;
    f64 xHeight;

    f64 lineheight() {
        return ascend + descend + linegap;
    }

    // MARK: Baselines ---------------------------------------------------------
    // Theses are relative to the origin (0, 0) of the font metrics.

    f64 xMiddleBaseline() {
        return (ascend + descend) / 2;
    }

    f64 alphabeticBaseline() {
        return ascend;
    }

    // MARK: Transformations ---------------------------------------------------

    FontMetrics combine(FontMetrics other) {
        return {
            .ascend = Karm::max(ascend, other.ascend),
            .captop = Karm::max(captop, other.captop),
            .descend = Karm::max(descend, other.descend),
            .linegap = Karm::max(linegap, other.linegap),
            .advance = Karm::max(advance, other.advance),
            .xHeight = Karm::max(xHeight, other.xHeight),
        };
    }

    FontMetrics scale(f64 factor) {
        return {
            .ascend = ascend * factor,
            .captop = captop * factor,
            .descend = descend * factor,
            .linegap = linegap * factor,
            .advance = advance * factor,
            .xHeight = xHeight * factor,
        };
    }
};

struct FontMeasure {
    Math::Rectf capbound;
    Math::Rectf linebound;
    Math::Vec2f baseline;
};

struct Fontface {
    static Rc<Fontface> fallback();

    virtual ~Fontface() = default;

    virtual FontMetrics metrics() = 0;

    virtual FontAttrs attrs() const = 0;

    virtual Glyph glyph(Rune rune) = 0;

    virtual f64 advance(Glyph glyph) = 0;

    virtual f64 kern(Glyph prev, Glyph curr) = 0;

    virtual void contour(Gfx::Canvas& g, Glyph glyph) const = 0;
};

struct Font {
    Rc<Fontface> fontface;
    f64 fontsize;
    f64 lineheight = 1.2;

    static Font fallback();

    FontMetrics metrics();

    Glyph glyph(Rune rune);

    f64 advance(Glyph glyph);

    f64 kern(Glyph prev, Glyph curr);

    FontMeasure measure(Glyph glyph);

    void contour(Gfx::Canvas& g, Glyph glyph) {
        g.scale(fontsize);
        fontface->contour(g, glyph);
    }

    f64 fontSize();

    f64 xHeight();

    f64 capHeight();

    f64 zeroAdvance();

    f64 lineHeight();
};

} // namespace Karm::Text
