#include <karm/macros>

import Karm.Core;
import Karm.Gfx.Pixels;
import Karm.Image;
import Karm.Ref;
import Karm.Bmp;

namespace Karm::Bmp {

static Res<Rc<Gfx::Image>> _decode(Bytes bytes) {
    auto bmp = try$(Decoder::init(bytes));
    auto img = Gfx::Image::alloc({bmp.width(), bmp.height()});
    try$(bmp.decode(*img));
    return Ok(img);
}

static Res<> _encode(Gfx::Pixels pixels, Io::BEmit& e, Image::Saver const&) {
    return encode(pixels, e);
}

static Image::Codec _codec{"bmp", Ref::Uti::PUBLIC_BMP, Image::Codec::MAGIC, Image::Codec::BUILTIN, Decoder::sniff, _decode, _encode};

} // namespace Karm::Bmp
