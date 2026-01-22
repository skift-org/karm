module;

#include <karm-core/macros.h>

export module Karm.Icc;

import Karm.Core;
import Karm.Sys;
import Karm.Ref;

namespace Karm::Icc {

export enum struct Version {
    V2,
    V4,
};

export enum class ColorSpace {
#define COLORSPACE(name, _sig, _ncomp) name,
#include "defs/colorspace.inc"

#undef COLORSPACE
};

static Opt<ColorSpace> colorSpaceFromSignature(Bytes signature) {
#define COLORSPACE(name, sig, _ncomp) \
    if (signature == sig ""_bytes) {  \
        return ColorSpace::name;      \
    };
#include "defs/colorspace.inc"

#undef COLORSPACE

    return NONE;
}

export struct ColorProfile {
    Version version;
    ColorSpace colorSpace;
    Vec<u8> buf;

    static Res<ColorProfile> create(Bytes bytes) {
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
        auto colorSpace = try$(colorSpaceFromSignature(scan.nextBytes(4)));

        return Ok(ColorProfile{
            .version = version,
            .colorSpace = colorSpace,
            .buf = bytes,
        });
    }

    static Res<ColorProfile> create(Ref::Url url) {
        auto reader = try$(Sys::File::open(url));
        auto data = try$(Sys::mmap(reader));
        return create(data.bytes());
    }

    static ColorProfile const& srgb(Version version) {
        if (version == Version::V4) {
            static ColorProfile srgbV4 = create("bundle://karm-icc/sRGB-v4.icc"_url).take();
            return srgbV4;
        } else {
            static ColorProfile srgbV2 = create("bundle://karm-icc/sRGB-v2-magic.icc"_url).take();
            return srgbV2;
        }
    }

    static ColorProfile const& sgray(Version maxVersion) {
        if (maxVersion == Version::V4) {
            static ColorProfile srgbV4 = create("bundle://karm-icc/sGrey-v4.icc"_url).take();
            return srgbV4;
        } else {
            static ColorProfile srgbV2 = create("bundle://karm-icc/sGrey-v2-magic.icc"_url).take();
            return srgbV2;
        }
    }
};

} // namespace Karm::Icc
