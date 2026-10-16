#include <karm/macros>

import Karm.Core;
import Karm.Gfx.Pixels;
import Karm.Image;
import Karm.Ref;
import Karm.Gif;

namespace Karm::Gif {

static Res<Rc<Gfx::Image>> _decode(Bytes bytes) {
    auto gif = try$(Decoder::init(bytes));
    auto img = Gfx::Image::alloc({gif.width(), gif.height()});
    try$(gif.decode(*img));
    return Ok(img);
}

static Image::Codec _codec{"gif", Ref::Uti::PUBLIC_GIF, Image::Codec::MAGIC, Image::Codec::BUILTIN, Decoder::sniff, _decode};

} // namespace Karm::Gif
