#include <karm/test>

import Karm.Gfx;
import Karm.Math;

namespace Karm::Gfx::Tests {

static Rc<Surface> _renderText(Str text, Math::Vec2f baseline, auto prepare) {
    auto font = Font::fallback();
    auto surface = Surface::alloc({64, 32}, Gfx::RGBA8888);
    surface->mutPixels().clear(Gfx::BLACK);

    CpuCanvas g;
    g.begin(surface->mutPixels());
    g.fillStyle(Gfx::WHITE);
    prepare(g);
    for (auto rune : iterRunes(text)) {
        auto glyph = font.glyph(rune);
        g.fill(font, glyph, baseline);
        baseline.x += font.advance(glyph);
    }
    g.end();

    return surface;
}

static isize _maxChannelDiff(Pixels a, Pixels b) {
    isize maxDiff = 0;
    for (isize y = 0; y < a.height(); y++) {
        for (isize x = 0; x < a.width(); x++) {
            auto ca = a.loadUnsafe({x, y});
            auto cb = b.loadUnsafe({x, y});
            maxDiff = max(
                maxDiff,
                (isize)Math::abs(ca.red - cb.red),
                (isize)Math::abs(ca.green - cb.green),
                (isize)Math::abs(ca.blue - cb.blue)
            );
        }
    }
    return maxDiff;
}

static bool _anyInk(Pixels p) {
    for (isize y = 0; y < p.height(); y++)
        for (isize x = 0; x < p.width(); x++)
            if (p.loadUnsafe({x, y}) != Gfx::BLACK)
                return true;
    return false;
}

test$("karm-gfx-glyph-cache-matches-direct") {
    auto cached = _renderText("skift", {4, 20}, [](CpuCanvas&) {
    });

    auto direct = _renderText("skift", {4, 20}, [](CpuCanvas& g) {
        g.fillStyle(
            Gfx::Gradient::linear()
                .withColors(Gfx::WHITE, Gfx::WHITE)
                .bake()
        );
    });

    expect$(_anyInk(cached->pixels()));
    expectEq$(_maxChannelDiff(cached->pixels(), direct->pixels()), 0);

    return Ok();
}

test$("karm-gfx-glyph-cache-matches-direct-scaled") {
    auto cached = _renderText("skift", {4, 20}, [](CpuCanvas& g) {
        g.scale(1.5);
    });

    auto direct = _renderText("skift", {4, 20}, [](CpuCanvas& g) {
        g.scale(1.5);
        g.fillStyle(
            Gfx::Gradient::linear()
                .withColors(Gfx::WHITE, Gfx::WHITE)
                .bake()
        );
    });

    expect$(_anyInk(cached->pixels()));
    expectEq$(_maxChannelDiff(cached->pixels(), direct->pixels()), 0);

    return Ok();
}

test$("karm-gfx-glyph-cache-replay-deterministic") {
    auto first = _renderText("hello", {4.25, 20}, [](CpuCanvas&) {
    });
    auto second = _renderText("hello", {4.25, 20}, [](CpuCanvas&) {
    });

    expect$(_anyInk(first->pixels()));
    expectEq$(_maxChannelDiff(first->pixels(), second->pixels()), 0);

    return Ok();
}

test$("karm-gfx-glyph-cache-respects-clip") {
    auto unclipped = _renderText("mm", {4, 20}, [](CpuCanvas&) {
    });

    auto clipped = _renderText("mm", {4, 20}, [](CpuCanvas& g) {
        g.clip(Math::Recti{0, 0, 10, 32});
    });

    expect$(_anyInk(clipped->pixels()));

    for (isize y = 0; y < 32; y++) {
        for (isize x = 0; x < 64; x++) {
            auto c = clipped->pixels().loadUnsafe({x, y});
            if (x < 10)
                expectEq$(c, unclipped->pixels().loadUnsafe({x, y}));
            else
                expectEq$(c, Gfx::BLACK);
        }
    }

    return Ok();
}

} // namespace Karm::Gfx::Tests
