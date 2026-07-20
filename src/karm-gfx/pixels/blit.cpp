module;

#include <karm/macros>

export module Karm.Gfx.Pixels:blit;

import :pixels;

namespace Karm::Gfx {

export [[gnu::flatten]] void blitUnsafe(MutPixels dst, Pixels src) {
    if (dst.width() != src.width() or dst.height() != src.height()) [[unlikely]]
        panic("blitUnsafe() called with buffers of different sizes");

    // HACK: fast path if the stride and fmt are the same
    if (dst.stride() == src.stride() and dst.fmt().index() == src.fmt().index()) {
        usize bytesPerRow = dst.width() * dst.fmt().bpp();

        if (dst.stride() == bytesPerRow) {
            std::memcpy(dst._buf, src._buf, dst.stride() * dst.height());
        } else {
            for (isize y = 0; y < dst.height(); y++)
                std::memcpy(dst.pixelUnsafe({0, y}), src.pixelUnsafe({0, y}), bytesPerRow);
        }

        return;
    }

    dst._fmt.visit([&](auto fd) {
        src._fmt.visit([&](auto fs) {
            for (isize y = 0; y < dst.height(); y++) {
                for (isize x = 0; x < dst.width(); x++) {
                    auto c = fs.load(src.pixelUnsafe({x, y}));
                    fd.store(dst.pixelUnsafe({x, y}), c);
                }
            }
        });
    });
}

} // namespace Karm::Gfx
