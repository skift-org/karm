export module Karm.Drm;

import Karm.Core;
import Karm.Math;

namespace Karm::Drm {

export enum struct Format {
    ARGB8888, // A:R:G:B, 8:8:8:8
    XRGB8888, // X:R:G:B, 8:8:8:8

    ABGR8888, // A:B:G:R, 8:8:8:8
    XBGR8888, // X:B:G:R, 8:8:8:8

    ARGB2101010, // A:R:G:B, 2:10:10:10
    XRGB2101010, // X:R:G:B, 2:10:10:10

    ABGR2101010, // A:B:G:R,, 2:10:10:10
    XBGR2101010, // X:B:G:R, 2:10:10:10
};

export usize bytesPerPixels(Format) {
    return 4;
}

export struct Buffer {
    Format format;
    Math::Vec2u size;
    usize stride;

    Buffer(Format format, Math::Vec2u size, usize stride)
        : format(format), size(size), stride(stride) {}

    virtual ~Buffer() = default;

    virtual MutBytes mutBytes() { return {}; }

    virtual Bytes bytes() const { return {}; }
};

export struct ViewBuffer : Buffer {
    MutBytes _buf;

    ViewBuffer(Format format, Math::Vec2u size, usize stride, MutBytes buf)
        : Buffer(format, size, stride), _buf(buf) {}

    MutBytes mutBytes() override {
        return _buf;
    }

    Bytes bytes() const override {
        return _buf;
    }
};

export struct Sync {
    virtual ~Sync() = default;
};

} // namespace Karm::Drm
