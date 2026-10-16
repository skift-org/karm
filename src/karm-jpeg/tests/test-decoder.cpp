#include <karm/test>

import Karm.Core;
import Karm.Gfx.Pixels;
import Karm.Image;
import Karm.Jpeg;
import Karm.Math;
import Karm.Ref;
import Karm.Sys;

namespace Karm::Jpeg::Tests {

static constexpr isize MAX_CHANNEL_ERROR = 8;

static isize _channelError(Gfx::Color a, Gfx::Color b) {
    return max(
        Math::abs((isize)a.red - (isize)b.red),
        Math::abs((isize)a.green - (isize)b.green),
        Math::abs((isize)a.blue - (isize)b.blue)
    );
}

static Res<> _expectMatchesReference(Test::Driver& _driver, Str name) {
    auto file = try$(Sys::File::open(Ref::Url::parse(Io::format("bundle://karm-jpeg.tests/{}.jpg", name))));
    auto map = try$(Sys::mmap(file));
    auto decoder = try$(Decoder::init(map.bytes()));
    auto jpeg = Gfx::Image::alloc({decoder.width(), decoder.height()});
    try$(decoder.decode(*jpeg));
    auto reference = try$(Image::load(Ref::Url::parse(Io::format("bundle://karm-jpeg.tests/{}.png", name))));

    assertEq$(jpeg->size(), reference->size());

    isize maxError = 0;
    for (isize y = 0; y < reference->size().y; ++y) {
        for (isize x = 0; x < reference->size().x; ++x) {
            auto error = _channelError(jpeg->pixels().load({x, y}), reference->pixels().load({x, y}));
            maxError = max(maxError, error);
        }
    }

    assertLteq$(maxError, MAX_CHANNEL_ERROR);
    return Ok();
}

test$("jpeg-decode-444") {
    return _expectMatchesReference(_driver, "cat-444");
}

test$("jpeg-decode-422") {
    return _expectMatchesReference(_driver, "cat-422");
}

test$("jpeg-decode-420") {
    return _expectMatchesReference(_driver, "cat-420");
}

test$("jpeg-decode-420-restart-interval") {
    return _expectMatchesReference(_driver, "cat-420-restart");
}

test$("jpeg-decode-420-non-interleaved") {
    return _expectMatchesReference(_driver, "cat-420-separate");
}

test$("jpeg-decode-grayscale") {
    return _expectMatchesReference(_driver, "cat-gray");
}

} // namespace Karm::Jpeg::Tests
