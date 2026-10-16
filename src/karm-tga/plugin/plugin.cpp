#include <karm/macros>

import Karm.Core;
import Karm.Gfx.Pixels;
import Karm.Image;
import Karm.Ref;
import Karm.Tga;

namespace Karm::Tga {

static Res<Rc<Gfx::Image>> _decode(Bytes bytes) {
    auto tga = try$(Decoder::init(bytes));
    auto img = Gfx::Image::alloc({tga.width(), tga.height()});
    try$(tga.decode(*img));
    return Ok(img);
}

static Res<> _encode(Gfx::Pixels pixels, Io::BEmit& e, Image::Saver const&) {
    return encode(pixels, e);
}

static Image::Codec _codec{"tga", Ref::Uti::PUBLIC_TGA, Image::Codec::HEURISTIC, Image::Codec::BUILTIN, Decoder::sniff, _decode, _encode};

} // namespace Karm::Tga
