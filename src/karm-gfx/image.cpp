module;

#include <karm-core/macros.h>

export module Karm.Gfx:image;

import Karm.Math;

import :buffer;

namespace Karm::Gfx {

export enum struct ImageEncoding {
    NONE,
    DEFLATE,
    JPEG2000,
    JPEG,
    PNG,
};

export struct ImageBlob {
    ImageEncoding _encoding;
    Buf<u8> _buf;
    Fmt _fmt;
    Math::Vec2i _size;

    static Rc<ImageBlob> create(Math::Vec2i size, Fmt fmt, Bytes bytes, ImageEncoding encoding) {
        // FIXME: Check if it calls the right buf constructor
        return makeRc<ImageBlob>(encoding, bytes, fmt, size);
    }

    // FIXME: Make sure this does a copy
    static Rc<ImageBlob> fromPixels(Pixels pixels) {
        return create(pixels.size(), pixels.fmt(), pixels.bytes(), ImageEncoding::NONE);
    }

    always_inline ImageEncoding encoding() const {
        return _encoding;
    }

    always_inline Math::Recti bound() const {
        return {0, 0, _size.x, _size.y};
    }

    always_inline Math::Vec2i size() const {
        return _size;
    }

    always_inline isize width() const {
        return _size.width;
    }

    always_inline isize height() const {
        return _size.height;
    }

    always_inline Fmt fmt() const {
        return _fmt;
    }

    always_inline Buf<u8> buf() const {
        return _buf;
    }
};

} // namespace Karm::Gfx
