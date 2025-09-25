module;

#include <karm-gfx/buffer.h>

export module Karm.Av:video;

namespace Karm::Av {

export struct VideoFrame {
    Rc<Gfx::Surface> surface;
    Duration timestamp;
};

export struct VideoStream {
    virtual ~VideoStream() = default;

    virtual Opt<VideoFrame> next() = 0;
};

} // namespace Karm::Av