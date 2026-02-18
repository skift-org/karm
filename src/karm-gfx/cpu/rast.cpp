export module Karm.Gfx:cpu.rast;

import Karm.Core;
import Karm.Math;

import :fill;

namespace Karm::Gfx {

export struct CpuRast {
    struct Frag {
        Math::Vec2i xy;
        Math::Vec2f uv;
        f64 a;
    };

    Vec<f64> _scanline{};

    void _accumulate(isize xOffset, isize width, Math::Vec2f p0, Math::Vec2f p1) {
        f64 dy = p1.y - p0.y;

        if (Math::epsilonEq(dy, 0.0))
            return;

        f64 xMid = (p0.x + p1.x) / 2.0;
        f64 xRel = xMid - Math::floori(xMid);
        f64 area = dy * (1.0 - xRel);

        isize idx = Math::floori(xMid) - xOffset;

        if (idx < 0) {
            _scanline[0] += dy;
            return;
        }

        if (idx >= width)
            return;

        _scanline[idx] += area;
        _scanline[idx + 1] += (dy - area);
    }
    void fill(Math::Polyf& poly, Math::Recti clip, FillRule fillRule, auto cb) {
        auto polyBound = poly.bound();

        auto clipBound = polyBound
                             .ceil()
                             .cast<isize>()
                             .clipTo(clip);

        if (clipBound.width <= 0 or clipBound.height <= 0)
            return;

        _scanline.resize(clipBound.width + 1);

        for (isize y = clipBound.top(); y < clipBound.bottom(); y++) {
            zeroFill<f64>(mutSub(_scanline, 0, clipBound.width + 1));

            f64 yTop = y;
            f64 yBot = y + 1.0;

            for (auto& edge : poly) {
                if (edge.bound().bottom() <= yTop or edge.bound().top() >= yBot)
                    continue;

                auto d = edge.delta();

                if (Math::epsilonEq(d.y, 0.0))
                    continue;

                f64 t0 = clamp01((yTop - edge.start.y) / d.y);
                f64 t1 = clamp01((yBot - edge.start.y) / d.y);

                if (t0 > t1)
                    std::swap(t0, t1);

                Math::Vec2f p0 = edge.start + d * t0;
                Math::Vec2f p1 = edge.start + d * t1;

                f64 xDir = (p1.x > p0.x) ? 1.0 : -1.0;
                Math::Vec2f cursor = p0;

                isize ixStart = Math::floori(p0.x);
                isize ixEnd = Math::floori(p1.x);

                if (ixStart == ixEnd) {
                    _accumulate(clipBound.x, clipBound.width, cursor, p1);
                } else {
                    isize steps = Math::abs(ixEnd - ixStart);
                    for (isize i = 0; i < steps; i++) {
                        f64 nextX = (xDir > 0) ? Math::floor(cursor.x) + 1.0 : Math::ceil(cursor.x) - 1.0;

                        f64 t = (nextX - cursor.x) / (p1.x - cursor.x);

                        Math::Vec2f nextP = cursor + (p1 - cursor) * t;
                        nextP.x = nextX;

                        _accumulate(clipBound.x, clipBound.width, cursor, nextP);
                        cursor = nextP;
                    }
                    _accumulate(clipBound.x, clipBound.width, cursor, p1);
                }
            }

            f64 accumulator = 0;
            for (isize x = 0; x < clipBound.width; x++) {
                accumulator += _scanline[x];
                f64 coverage = accumulator;
                if (fillRule == FillRule::NONZERO) {
                    coverage = clamp01(Math::abs(coverage));
                } else {
                    f64 val = Math::abs(coverage);
                    val = val - 2.0 * Math::floor(val / 2.0);
                    if (val > 1.0)
                        val = 2.0 - val;
                    coverage = val;
                }

                if (coverage > Limits<f64>::EPSILON) {
                    Math::Vec2i pos = {clipBound.x + x, y};
                    Math::Vec2f uv = (pos.cast<f64>() - polyBound.topStart()) / polyBound.size();

                    cb(Frag{pos, uv, coverage});
                }
            }
        }
    }
};

} // namespace Karm::Gfx
