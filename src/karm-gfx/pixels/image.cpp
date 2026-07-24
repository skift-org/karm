module;

#include <karm/macros>

export module Karm.Gfx.Pixels:image;

import :pixels;

namespace Karm::Gfx {

export struct Image {
    Union<Vec<u8>, MutBytes> _buf;
    Math::Vec2i _size;
    usize _stride;
    Format _format;

    static Rc<Image> alloc(Math::Vec2i size, Format fmt = RGBA8888) {
        return makeRc<Image>(
            Vec<u8>{Buf<u8>::init(size.x * size.y * fmt.bpp())},
            size,
            size.x * fmt.bpp(),
            fmt
        );
    }

    static Rc<Image> wrap(MutBytes buf, Math::Vec2i size, usize stride, Format fmt) {
        return makeRc<Image>(std::move(buf), size, stride, fmt);
    }

    static Rc<Image> fallback() {
        auto img = alloc({2, 2}, RGBA8888);
        img->mutPixels().clear(Color::fromHex(0xFF00FF));
        return img;
    }

    always_inline operator Pixels() const {
        return pixels();
    }

    always_inline operator MutPixels() {
        return mutPixels();
    }

    u8* buf() {
        return _buf.visit([](auto& b) {
            return b.buf();
        });
    }

    u8 const* buf() const {
        return _buf.visit([](auto& b) {
            return b.buf();
        });
    }

    always_inline Pixels pixels() const {
        return {buf(), _size, _stride, _format};
    }

    always_inline MutPixels mutPixels() {
        return {buf(), _size, _stride, _format};
    }

    always_inline isize width() const {
        return _size.x;
    }

    always_inline isize height() const {
        return _size.y;
    }

    always_inline Math::Vec2i size() const {
        return _size;
    }

    always_inline Math::Recti bound() const {
        return {0, 0, width(), height()};
    }

    void repr(Io::Emit& e) const {
        e("(image {}x{} stride={} fmt={})", width(), height(), _stride, _format.index());
    }
};

} // namespace Karm::Gfx
