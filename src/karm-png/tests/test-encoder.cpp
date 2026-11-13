#include <karm/test>

import Karm.Core;
import Karm.Gfx.Pixels;
import Karm.Png;
import Karm.Ref;

namespace Karm::Png::Tests {

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

    Io::BufferWriter bw;
    Io::BEmit e{bw};
    try$(encode(original->pixels(), e));
    auto encoded = bw.take();

    auto decoder = try$(Decoder::init(encoded));
    auto decoded = Gfx::Image::alloc({decoder.width(), decoder.height()});
    try$(decoder.decode(*decoded));

    assertEq$(decoded->size(), original->size());

    // NOTE: PNG is lossless, every pixel must survive the roundtrip.
    for (isize y = 0; y < original->size().y; y++) {
        for (isize x = 0; x < original->size().x; x++) {
            assertEq$(decoded->pixels().load({x, y}), original->pixels().load({x, y}));
        }
    }

    return Ok();
}

static Res<> _decode(Bytes bytes) {
    auto decoder = try$(Decoder::init(bytes));
    auto decoded = Gfx::Image::alloc({decoder.width(), decoder.height()});
    return decoder.decode(*decoded);
}

test$("png-decode-truncated") {
    Io::BufferWriter bw;
    Io::BEmit e{bw};
    try$(encode(_testPattern()->pixels(), e));
    auto encoded = bw.take();

    // NOTE: IEND is the last chunk, so every truncation must fail cleanly.
    for (usize len = 0; len < encoded.len(); len++)
        assert$(not _decode(sub(encoded, 0, len)));

    try$(_decode(encoded));
    return Ok();
}

test$("png-decode-ignores-data-after-iend") {
    Io::BufferWriter bw;
    Io::BEmit e{bw};
    try$(encode(_testPattern()->pixels(), e));
    try$(e.writeStr("trailing garbage"));

    try$(_decode(bw.take()));
    return Ok();
}

} // namespace Karm::Png::Tests
