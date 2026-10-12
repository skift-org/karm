#include <karm/test>

import Karm.Core;
import Karm.Gfx.Pixels;
import Karm.Image;
import Karm.Ref;

namespace Karm::Image::Png::Tests {

// An odd sized image where every pixel is different and alpha varies.
static Rc<Gfx::Image> _testPattern() {
    auto image = Gfx::Image::alloc({37, 23});
    for (isize y = 0; y < 23; y++) {
        for (isize x = 0; x < 37; x++) {
            image->mutPixels().store(
                {x, y},
                Gfx::Color::fromRgba(x * 7, y * 11, (x ^ y) * 5, 255 - x * 3)
            );
        }
    }
    return image;
}

test$("png-encode-roundtrip") {
    auto original = _testPattern();

    auto encoded = try$(Image::save(original->pixels(), {.format = Ref::Uti::PUBLIC_PNG}));
    auto decoded = try$(Image::load(encoded));

    assertEq$(decoded->size(), original->size());

    // NOTE: PNG is lossless, every pixel must survive the roundtrip.
    for (isize y = 0; y < original->size().y; y++) {
        for (isize x = 0; x < original->size().x; x++) {
            assertEq$(decoded->pixels().load({x, y}), original->pixels().load({x, y}));
        }
    }

    return Ok();
}

} // namespace Karm::Image::Png::Tests
