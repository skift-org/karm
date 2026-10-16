#include <karm/test>

import Karm.Core;
import Karm.Gfx.Pixels;
import Karm.Image;
import Karm.Math;
import Karm.Ref;

using namespace Karm::Literals;
using namespace Karm::Ref::Literals;

namespace Karm::Image::Libtiff::Tests {

static isize _channelError(Gfx::Color a, Gfx::Color b) {
    return max(
        Math::abs((isize)a.red - (isize)b.red),
        Math::abs((isize)a.green - (isize)b.green),
        Math::abs((isize)a.blue - (isize)b.blue)
    );
}

test$("libtiff-decode") {
    auto image = try$(Image::load("bundle://libtiff.tests/image.tiff"_url));

    assertEq$(image->size(), (Math::Vec2i{2, 2}));
    assertLteq$(_channelError(image->pixels().load({0, 0}), Gfx::Color::fromRgb(255, 0, 0)), 0);
    assertLteq$(_channelError(image->pixels().load({1, 0}), Gfx::Color::fromRgb(0, 255, 0)), 0);
    assertLteq$(_channelError(image->pixels().load({0, 1}), Gfx::Color::fromRgb(0, 0, 255)), 0);
    auto last = image->pixels().load({1, 1});
    assertLt$(last.alpha, 32);

    return Ok();
}

} // namespace Karm::Image::Libtiff::Tests
