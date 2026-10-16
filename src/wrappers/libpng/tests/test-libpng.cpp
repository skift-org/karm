#include <karm/test>

import Karm.Core;
import Karm.Gfx.Pixels;
import Karm.Image;
import Karm.Math;
import Karm.Ref;

using namespace Karm::Literals;
using namespace Karm::Ref::Literals;

namespace Karm::Image::Libpng::Tests {

static isize _channelError(Gfx::Color a, Gfx::Color b) {
    return max(
        Math::abs((isize)a.red - (isize)b.red),
        Math::abs((isize)a.green - (isize)b.green),
        Math::abs((isize)a.blue - (isize)b.blue)
    );
}

test$("libpng-decode") {
    auto image = try$(Image::load("bundle://libpng.tests/image.png"_url));

    assertEq$(image->size(), (Math::Vec2i{2, 2}));
    assertLteq$(_channelError(image->pixels().load({0, 0}), Gfx::Color::fromRgb(255, 0, 0)), 0);
    assertLteq$(_channelError(image->pixels().load({1, 0}), Gfx::Color::fromRgb(0, 255, 0)), 0);
    assertLteq$(_channelError(image->pixels().load({0, 1}), Gfx::Color::fromRgb(0, 0, 255)), 0);
    auto last = image->pixels().load({1, 1});
    assertLt$(last.alpha, 32);

    return Ok();
}

test$("libpng-encode-roundtrip") {
    // NOTE: Solid 8x8 quadrants, so lossy codecs don't have to blend colors
    //       across chroma subsampling or DCT blocks.
    auto original = Gfx::Image::alloc({16, 16});
    Array<Gfx::Color, 4> colors = {
        Gfx::Color::fromRgb(255, 0, 0),
        Gfx::Color::fromRgb(0, 255, 0),
        Gfx::Color::fromRgb(0, 0, 255),
        Gfx::Color::fromRgb(255, 255, 255),
    };
    for (isize y = 0; y < 16; y++)
        for (isize x = 0; x < 16; x++)
            original->mutPixels().store({x, y}, colors[(y / 8) * 2 + x / 8]);

    auto encoded = try$(Image::save(original->pixels(), {.format = Ref::Uti::PUBLIC_PNG}));
    auto decoded = try$(Image::load(encoded));

    assertEq$(decoded->size(), original->size());
    for (isize y = 0; y < 16; y++)
        for (isize x = 0; x < 16; x++)
            assertLteq$(_channelError(decoded->pixels().load({x, y}), original->pixels().load({x, y})), 0);

    return Ok();
}

} // namespace Karm::Image::Libpng::Tests
