export module Karm.Gfx:shadow;

import Karm.Math;
import :canvas;

namespace Karm::Gfx {

export struct BoxShadow {
    Gfx::Color fill;
    f64 blur;
    f64 spread;
    Math::Vec2i offset;
    bool skipOccluded = false; //< Skip painting parts of the shadow that are occluded by the element itself

    static BoxShadow elevated(f64 v, Gfx::Color fill = Gfx::BLACK) {
        return {
            fill.withOpacity(0.25),
            v * 2,
            -v,
            {0, (int)v},
        };
    }

    auto& withFill(Gfx::Color p) {
        fill = p;
        return *this;
    }

    auto& withBlur(f64 r) {
        blur = r;
        return *this;
    }

    auto& withSpread(f64 r) {
        spread = r;
        return *this;
    }

    auto& withOffset(Math::Vec2i o) {
        offset = o;
        return *this;
    }

    auto& withSkipOccluded(bool value) {
        skipOccluded = value;
        return *this;
    }

    void paint(Gfx::Canvas& g, Math::Recti bound) const {

        /// 1 / sqrt(2)
        static constexpr f64 IS2 = 0.7071067811865475;

        // 1 - (1 / sqrt(2))
        static constexpr f64 IS2M = 1 - IS2;

        auto shadowBound = bound;
        shadowBound = shadowBound.grow(spread);
        shadowBound = shadowBound.offset(offset);

        auto grad = Gfx::Gradient::linear()
                        .withColors(fill, fill.withOpacity(0))
                        .bake();

        auto topStart = Math::Recti::fromTwoPoint(
            shadowBound.topStart(),
            shadowBound.topStart() - Math::Vec2i{(int)blur, (int)blur}
        );

        auto topEnd = Math::Recti::fromTwoPoint(
            shadowBound.topEnd(),
            shadowBound.topEnd() + Math::Vec2i{(int)blur, -(int)blur}
        );

        auto bottomStart = Math::Recti::fromTwoPoint(
            shadowBound.bottomStart(),
            shadowBound.bottomStart() + Math::Vec2i{-(int)blur, (int)blur}
        );

        auto bottomEnd = Math::Recti::fromTwoPoint(
            shadowBound.bottomEnd(),
            shadowBound.bottomEnd() + Math::Vec2i{(int)blur, (int)blur}
        );

        auto top = Math::Recti::fromTwoPoint(
            shadowBound.topStart() - Math::Vec2i{0, (int)blur},
            shadowBound.topEnd()
        );

        auto bottom = Math::Recti::fromTwoPoint(
            shadowBound.bottomStart(),
            shadowBound.bottomEnd() + Math::Vec2i{0, (int)blur}
        );

        auto start = Math::Recti::fromTwoPoint(
            shadowBound.topStart() - Math::Vec2i{(int)blur, 0},
            shadowBound.bottomStart()
        );

        auto end = Math::Recti::fromTwoPoint(
            shadowBound.topEnd(),
            shadowBound.bottomEnd() + Math::Vec2i{(int)blur, 0}
        );

        grad.withType(Gradient::RADIAL);
        if (not skipOccluded or not bound.contains(topStart)) {
            g.fillStyle(grad.withPoints({1, 1}, {IS2M, IS2M}));
            g.fill(topStart);
        }

        if (not skipOccluded or not bound.contains(topEnd)) {
            g.fillStyle(grad.withPoints({0, 1}, {IS2, IS2M}));
            g.fill(topEnd);
        }

        if (not skipOccluded or not bound.contains(bottomStart)) {
            g.fillStyle(grad.withPoints({1, 0}, {IS2M, IS2}));
            g.fill(bottomStart);
        }

        if (not skipOccluded or not bound.contains(bottomEnd)) {
            g.fillStyle(grad.withPoints({0, 0}, {IS2, IS2}));
            g.fill(bottomEnd);
        }

        grad.withType(Gradient::LINEAR);
        if (not skipOccluded or not bound.contains(top)) {
            g.fillStyle(grad.withPoints({0, 1}, {0, 0}));
            g.fill(top);
        }

        if (not skipOccluded or not bound.contains(bottom)) {
            g.fillStyle(grad.withPoints({0, 0}, {0, 1}));
            g.fill(bottom);
        }

        if (not skipOccluded or not bound.contains(start)) {
            g.fillStyle(grad.withPoints({1, 0}, {0, 0}));
            g.fill(start);
        }

        if (not skipOccluded or not bound.contains(end)) {
            g.fillStyle(grad.withPoints({0, 0}, {1, 0}));
            g.fill(end);
        }

        if (not skipOccluded or not bound.contains(shadowBound.shrink(blur))) {
            g.fillStyle(fill);
            g.fill(shadowBound.shrink(blur));
        }
    }
};

Gfx::BoxShadow boxShadow(auto... args) {
    return Gfx::BoxShadow(args...);
}

} // namespace Karm::Gfx
