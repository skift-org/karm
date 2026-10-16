#include <karm/macros>

import Karm.Core;
import Karm.Gfx.Pixels;
import Karm.Image;
import Karm.Ref;
import Karm.Png;

namespace Karm::Png {

static Res<Rc<Gfx::Image>> _decode(Bytes bytes) {
    auto png = try$(Decoder::init(bytes));
    auto img = Gfx::Image::alloc({png.width(), png.height()});
    try$(png.decode(*img));
    return Ok(img);
}

static Res<> _encode(Gfx::Pixels pixels, Io::BEmit& e, Image::Saver const&) {
    return encode(pixels, e);
}

static Image::Codec _codec{"png", Ref::Uti::PUBLIC_PNG, Image::Codec::MAGIC, Image::Codec::BUILTIN, Decoder::sniff, _decode, _encode};

} // namespace Karm::Png
