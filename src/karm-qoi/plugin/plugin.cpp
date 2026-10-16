#include <karm/macros>

import Karm.Core;
import Karm.Gfx.Pixels;
import Karm.Image;
import Karm.Ref;
import Karm.Qoi;

namespace Karm::Qoi {

static Res<Rc<Gfx::Image>> _decode(Bytes bytes) {
    auto qoi = try$(Decoder::init(bytes));
    auto img = Gfx::Image::alloc({qoi.width(), qoi.height()});
    try$(qoi.decode(*img));
    return Ok(img);
}

static Res<> _encode(Gfx::Pixels pixels, Io::BEmit& e, Image::Saver const&) {
    return encode(pixels, e);
}

static Image::Codec _codec{"qoi", Ref::Uti::PUBLIC_QOI, Image::Codec::MAGIC, Image::Codec::BUILTIN, Decoder::sniff, _decode, _encode};

} // namespace Karm::Qoi
