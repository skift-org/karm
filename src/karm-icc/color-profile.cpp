module;

#include <karm/macros>

export module Karm.Icc:colorProfile;

import Karm.Core;
import Karm.Sys;
import Karm.Ref;

import :metadata;

namespace Karm::Icc {

export enum struct VersionQuery {
    PREFER_V4,
    PREFER_V2,
    ONLY_V4,
    ONLY_V2,
};

export struct ColorProfile {
    ColorSpace _colorSpace;
    bool _isDeviceDependent;
    Opt<Vec<u8>> _iccV2;
    Opt<Vec<u8>> _iccV4;

    static Res<Rc<ColorProfile>> fromIcc(Bytes v4, Bytes v2) {
        auto v4Metadata = try$(Icc::probe(v4));
        auto v2Metadata = try$(Icc::probe(v2));

        if (v2Metadata.colorSpace != v4Metadata.colorSpace)
            return Error::invalidInput("ICC metadata mismatch");

        return Ok(makeRc<ColorProfile>(v4Metadata.colorSpace, false, v2, v4));
    }

    static Res<Rc<ColorProfile>> fromIcc(Bytes bytes) {
        auto metadata = try$(Icc::probe(bytes));

        if (metadata.version == Version::V4) {
            return Ok(makeRc<ColorProfile>(metadata.colorSpace, false, NONE, Vec<u8>(bytes)));
        } else {
            return Ok(makeRc<ColorProfile>(metadata.colorSpace, false, Vec<u8>(bytes), NONE));
        }
    }

    static Res<Rc<ColorProfile>> fromIcc(Ref::Url url) {
        auto file = try$(Sys::File::open(url));
        auto mmap = try$(Sys::mmap(file));
        return fromIcc(mmap.bytes());
    }

    static Res<Rc<ColorProfile>> fromIcc(Ref::Url v4Url, Ref::Url v2Url) {
        auto v4File = try$(Sys::File::open(v4Url));
        auto v4Mmap = try$(Sys::mmap(v4File));

        auto v2File = try$(Sys::File::open(v2Url));
        auto v2Mmap = try$(Sys::mmap(v2File));

        return fromIcc(v4Mmap.bytes(), v2Mmap.bytes());
    }

    static Rc<ColorProfile> srgb() {
        static auto srgbProfile = fromIcc(
                                      "bundle://karm-icc/sRGB-v4.icc"_url,
                                      "bundle://karm-icc/sRGB-v2-magic.icc"_url
        )
                                      .unwrap();

        return srgbProfile;
    }

    static Rc<ColorProfile> sgray() {
        static auto sgrayProfile = fromIcc(
                                       "bundle://karm-icc/sGrey-v4.icc"_url,
                                       "bundle://karm-icc/sGrey-v2-magic.icc"_url
        )
                                       .unwrap();

        return sgrayProfile;
    }

    static Rc<ColorProfile> deviceRgb() {
        static auto deviceRgb = makeRc<ColorProfile>(ColorSpace{ColorSpace::Name::RGB, 3}, true, NONE, NONE);
        return deviceRgb;
    }

    static Rc<ColorProfile> deviceGray() {
        static auto deviceGray = makeRc<ColorProfile>(ColorSpace{ColorSpace::Name::GRAY, 1}, true, NONE, NONE);
        return deviceGray;
    }

    static Rc<ColorProfile> deviceCmyk() {
        static auto deviceCmyk = makeRc<ColorProfile>(ColorSpace{ColorSpace::Name::CMYK, 4}, true, NONE, NONE);
        return deviceCmyk;
    }

    ColorSpace colorSpace() const {
        return _colorSpace;
    }

    bool isDeviceDependent() const {
        return _isDeviceDependent;
    }

    Opt<Bytes> iccData(VersionQuery versionQuery = VersionQuery::PREFER_V2) {
        if (versionQuery == VersionQuery::PREFER_V4) {
            return _iccV4 ? _iccV4 : _iccV2;
        } else if (versionQuery == VersionQuery::PREFER_V2) {
            return _iccV2 ? _iccV2 : _iccV4;
        } else if (versionQuery == VersionQuery::ONLY_V4) {
            return _iccV4;
        } else if (versionQuery == VersionQuery::ONLY_V2) {
            return _iccV2;
        }

        return NONE;
    }
};

} // namespace Karm::Icc
