module;

#include <karm/macros>

export module Karm.Font.Ttf:glyf;

import Karm.Core;
import Karm.Gfx;
import Karm.Math;
import Karm.Logger;

import :loca;

namespace Karm::Font::Ttf {

static constexpr bool DEBUG_GLYF = false;

export struct Glyf : Io::BChunk {
    static constexpr Str SIG = "glyf";

    static constexpr u8 ON_CURVE_POINT = 0x01;
    static constexpr u8 X_SHORT_VECTOR = 0x02;
    static constexpr u8 Y_SHORT_VECTOR = 0x04;
    static constexpr u8 REPEAT = 0x08;
    static constexpr u8 SAME_OR_POSITIVE_X = 0x10;
    static constexpr u8 SAME_OR_POSITIVE_Y = 0x20;
    static constexpr u8 OVERLAY_SIMPLE = 0x40;

    struct Metrics {
        i16 numContours;
        i16 xMin;
        i16 yMin;
        i16 xMax;
        i16 yMax;
    };

    always_inline Metrics metrics(Io::BScan& s, usize glyfOffset) const {
        s.skip(glyfOffset);
        auto numContours = s.nextI16be();
        if (numContours == 0) {
            return {};
        }
        auto xMin = s.nextI16be();
        auto yMin = s.nextI16be();
        auto xMax = s.nextI16be();
        auto yMax = s.nextI16be();
        return {numContours, xMin, yMin, xMax, yMax};
    }

    always_inline Metrics metrics(usize glyfOffset) const {
        auto s = begin();
        return metrics(s, glyfOffset);
    }

    static constexpr usize MAX_COMPOSITE_DEPTH = 16;

    void contour(Gfx::Canvas& g, usize glyfOffset, Loca const& loca, Head const& head, Math::Trans2f trans = Math::Trans2f::identity(), usize depth = 0) const {
        auto s = begin();
        auto m = metrics(s, glyfOffset);

        if (m.numContours > 0) {
            contourSimple(g, m, s, trans);
        } else if (m.numContours < 0) {
            if (depth >= MAX_COMPOSITE_DEPTH) {
                logWarn("glyf: composite glyph nesting too deep");
                return;
            }
            contourComposite(g, s, loca, head, trans, depth);
        }
    }

    // MARK: Simple Contour ----------------------------------------------------

    struct SimpleContour {
        enum Flag {
            ON_CURVE_POINT = 0x01,
            X_SHORT_VECTOR = 0x02,
            Y_SHORT_VECTOR = 0x04,
            REPEAT = 0x08,
            X_SAME_OR_POSITIVE_X_SHORT_VECTOR = 0x10,
            Y_SAME_OR_POSITIVE_Y_SHORT_VECTOR = 0x20,
            OVERLAP_SIMPLE = 0x40,
        };

        u8 flags;
        i16 x;
        i16 y;
    };

    void contourSimple(Gfx::Canvas& g, Metrics m, Io::BScan& s, Math::Trans2f trans) const {
        auto endPtsOfContours = s;
        auto nPoints = s.peek(2 * (m.numContours - 1)).nextU16be() + 1u;
        u16 instructionLength = s.skip(m.numContours * 2).nextU16be();

        auto flagsScan = s.skip(instructionLength);

        usize nXCoords = 0;
        u8 flags = 0;
        u8 flagsRepeat = 0;

        for (usize i = 0; i < nPoints; i++) {
            if (not flagsRepeat) {
                flags = s.nextU8be();
                if (flags & REPEAT) {
                    flagsRepeat = s.nextU8be();
                }
            } else {
                flagsRepeat--;
            }

            nXCoords += flags & SimpleContour::X_SHORT_VECTOR
                            ? 1
                            : (flags & SAME_OR_POSITIVE_X ? 0 : 2);
        }

        auto xCoordsScan = s;
        auto yCoordsScan = s.skip(nXCoords);

        usize start = 0;
        Math::Vec2f curr{};
        flags = 0;
        flagsRepeat = 0;
        for (isize c = 0; c < m.numContours; c++) {
            usize end = endPtsOfContours.nextU16be();

            Math::Vec2f cp{};
            Math::Vec2f startP{};
            bool wasCp = false;

            for (usize i = start; i <= end; i++) {
                if (not flagsRepeat) {
                    flags = flagsScan.nextU8be();
                    if (flags & REPEAT) {
                        flagsRepeat = flagsScan.nextU8be();
                    }
                } else {
                    flagsRepeat--;
                }

                isize x = (flags & X_SHORT_VECTOR)
                              ? ((flags & SAME_OR_POSITIVE_X) ? xCoordsScan.nextU8be() : -xCoordsScan.nextU8be())
                              : ((flags & SAME_OR_POSITIVE_X) ? 0 : xCoordsScan.nextI16be());

                isize y = (flags & Y_SHORT_VECTOR)
                              ? ((flags & SAME_OR_POSITIVE_Y) ? yCoordsScan.nextU8be() : -yCoordsScan.nextU8be())
                              : ((flags & SAME_OR_POSITIVE_Y) ? 0 : yCoordsScan.nextI16be());

                curr = curr + Math::Vec2f{(f64)x, (f64)-y};

                if (i == start) {
                    g.moveTo(trans.apply(curr));
                    startP = curr;
                } else {
                    if (flags & ON_CURVE_POINT) {
                        if (wasCp) {
                            g.quadTo(trans.apply(cp), trans.apply(curr));
                        } else {
                            g.lineTo(trans.apply(curr));
                        }
                        wasCp = false;
                    } else {
                        if (wasCp) {
                            auto p1 = (cp + curr) / 2;
                            g.quadTo(trans.apply(cp), trans.apply(p1));
                            cp = curr;
                            wasCp = true;
                        } else {
                            cp = curr;
                            wasCp = true;
                        }
                    }
                }
            }

            if (wasCp) {
                g.quadTo(trans.apply(cp), trans.apply(startP));
            }

            g.closePath();
            start = end + 1;
        }
    }

    // MARK: Composite Contour -------------------------------------------------

    static constexpr u16 ARG_1_AND_2_ARE_WORDS = 0x0001;
    static constexpr u16 ARGS_ARE_XY_VALUES = 0x0002;
    static constexpr u16 ROUND_XY_TO_GRID = 0x0004;
    static constexpr u16 WE_HAVE_A_SCALE = 0x0008;
    static constexpr u16 MORE_COMPONENTS = 0x0020;
    static constexpr u16 WE_HAVE_AN_X_AND_Y_SCALE = 0x0040;
    static constexpr u16 WE_HAVE_A_TWO_BY_TWO = 0x0080;
    static constexpr u16 WE_HAVE_INSTRUCTIONS = 0x0100;
    static constexpr u16 USE_MY_METRICS = 0x0200;
    static constexpr u16 OVERLAP_COMPOUND = 0x0400; // ignore for rasterless outline

    void contourComposite(Gfx::Canvas& g, Io::BScan& s, Loca const& loca, Head const& head, Math::Trans2f parentTrans, usize depth) const {
        while (true) {
            u16 flags = s.nextU16be();
            u16 glyphIndex = s.nextU16be();

            // args
            i16 arg1 = 0, arg2 = 0;
            if (flags & ARG_1_AND_2_ARE_WORDS) {
                arg1 = s.nextI16be();
                arg2 = s.nextI16be();
            } else {
                arg1 = (i8)s.nextU8be();
                arg2 = (i8)s.nextU8be();
            }

            f64 tx = 0, ty = 0;
            i16 compPoint = 0, basePoint = 0;

            if (flags & ARGS_ARE_XY_VALUES) {
                tx = arg1;
                ty = -(f64)arg2;
                if (flags & ROUND_XY_TO_GRID) {
                    tx = Math::round(tx);
                    ty = Math::round(ty);
                }
            } else {
                compPoint = arg1;
                basePoint = arg2;
                logDebugIf(DEBUG_GLYF, "glyf: anchor-point composite (base={}, comp={}) not aligned", basePoint, compPoint);
            }

            f64 a = 1, b = 0, c = 0, d = 1;
            if (flags & WE_HAVE_A_SCALE) {
                f64 s14 = (f64)s.nextI16be() / 16384.0;
                a = d = s14;
            } else if (flags & WE_HAVE_AN_X_AND_Y_SCALE) {
                a = (f64)s.nextI16be() / 16384.0;
                d = (f64)s.nextI16be() / 16384.0;
            } else if (flags & WE_HAVE_A_TWO_BY_TWO) {
                a = (f64)s.nextI16be() / 16384.0;
                b = (f64)s.nextI16be() / 16384.0;
                c = (f64)s.nextI16be() / 16384.0;
                d = (f64)s.nextI16be() / 16384.0;
            }

            // NOTE: TrueType maps x' = a*x + c*y and y' = b*x + d*y, in a y-up space.
            //       Glyph outlines are emitted y-down, so the off-diagonal terms flip sign.
            Math::Trans2f t{a, -b, -c, d, tx, ty};

            auto subOff = loca.glyfOffset(glyphIndex, head);
            if (subOff != loca.glyfOffset(glyphIndex + 1, head))
                contour(g, subOff, loca, head, t.multiply(parentTrans), depth + 1);

            // last component?
            if (!(flags & MORE_COMPONENTS)) {
                if (flags & WE_HAVE_INSTRUCTIONS) {
                    u16 ilen = s.nextU16be();
                    s.skip(ilen);
                }
                break;
            }
        }
    }
};

} // namespace Karm::Font::Ttf
