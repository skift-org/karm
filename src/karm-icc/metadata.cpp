module;

#include <karm/macros>

export module Karm.Icc:metadata;

import Karm.Core;
import Karm.Logger;

namespace Karm::Icc {

export struct ColorSpace {
    enum struct Name : u8 {
        XYZ,
        LAB,
        LUV,
        YCBR,
        YXY,
        RGB,
        GRAY,
        HSV,
        HLS,
        CMYK,
        CMY,
        DEVICE_N,
    };

    Name name;
    u8 numberOfComponents;

    static Opt<ColorSpace> fromSignature(Bytes signature) {
#define COLORSPACE(name, sig, ncomp)          \
    if (signature == sig ""_bytes) {          \
        return ColorSpace{Name::name, ncomp}; \
    };
#include "defs/colorspace.inc"

#undef COLORSPACE

        return NONE;
    }

    bool operator==(ColorSpace const&) const = default;
};

export enum struct Version {
    V2,
    V4,
};

export struct Metadata {
    Version version;
    ColorSpace colorSpace;
};

export Res<Metadata> probe(Bytes bytes) {
    auto scan = Io::BScan{bytes};

    // TODO: Improve BScan to report errors and get rid of this
    if (bytes.len() < 20) {
        return Error::invalidData("icc too small");
    }

    auto mapVersion = [](u8 major) -> Opt<Version> {
        if (major == 2) {
            return Version::V2;
        } else if (major == 4) {
            return Version::V4;
        } else {
            return NONE;
        }
    };

    scan.seek(8);
    auto version = try$(mapVersion(scan.nextU8be()));

    scan.seek(16);
    auto colorSpace = try$(ColorSpace::fromSignature(scan.nextBytes(4)));

    return Ok(Metadata{
        .version = version,
        .colorSpace = colorSpace,
    });
}

} // namespace Karm::Icc
