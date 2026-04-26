module;

#include <karm/macros>

export module Karm.Icc:profile;

import Karm.Core;
import Karm.Sys;
import Karm.Ref;

import :base;

using namespace Karm::Ref::Literals;

namespace Karm::Icc {

export struct ColorProfile {
    ProfileClass _profileDeviceClass = ProfileClass::ABSTRACT;
    ColorSpace _dataColorSpace = ColorSpace::XYZ;
    ColorSpace _profileConnectionSpace = ColorSpace::XYZ;
    bool _isDeviceDependent = false;
    Opt<Vec<u8>> _data = NONE;

    static Res<Rc<ColorProfile>> from(Bytes data) {
        Parser parser{data};
        auto header = parser.header();

        if (not header.sniff())
            return Error::invalidData("not an icc profile");

        auto profile = makeRc<ColorProfile>();

        profile->_profileDeviceClass = try$(header.profileDeviceClass().okOr(Error::invalidData("unknow or invalid profile class")));
        profile->_dataColorSpace = try$(
            header.dataColorSpace()
                .okOr(Error::invalidData("unknow or invalid data color space"))
        );
        profile->_profileConnectionSpace = try$(
            header.profileConnectionSpace()
                .okOr(Error::invalidData("unknow or invalid profile connection space"))
        );

        profile->_data = data;

        return Ok(profile);
    }

    static Res<Rc<ColorProfile>> from(Io::Readable auto& in) {
        return from(try$(Io::readAll(in)));
    }

    static Res<Rc<ColorProfile>> from(Ref::Url url) {
        auto file = try$(Sys::File::open(url));
        return from(file);
    }

    static Rc<ColorProfile> srgb() {
        static auto srgbProfile =
            from("bundle://karm-icc/public/color-profiles/sRGB-v4.icc"_url).unwrap();
        return srgbProfile;
    }

    static Rc<ColorProfile> sgray() {
        static auto sgrayProfile =
            from("bundle://karm-icc/public/color-profiles/sGrey-v4.icc"_url)
                .unwrap();

        return sgrayProfile;
    }

    static Rc<ColorProfile> deviceRgb() {
        static auto profile = [] {
            auto p = makeRc<ColorProfile>();
            p->_dataColorSpace = ColorSpace::RGB;
            p->_isDeviceDependent = true;
            return p;
        }();
        return profile;
    }

    static Rc<ColorProfile> deviceGray() {
        static auto profile = [] {
            auto p = makeRc<ColorProfile>();
            p->_dataColorSpace = ColorSpace::GRAY;
            p->_isDeviceDependent = true;
            return p;
        }();
        return profile;
    }

    static Rc<ColorProfile> deviceCmyk() {
        static auto profile = [] {
            auto p = makeRc<ColorProfile>();
            p->_dataColorSpace = ColorSpace::CMYK;
            p->_isDeviceDependent = true;
            return p;
        }();
        return profile;
    }

    ColorSpace colorSpace() const {
        return _dataColorSpace;
    }

    ColorSpace profileConnectionSpace() const {
        return _profileConnectionSpace;
    }

    ProfileClass profileDeviceClass() const {
        return _profileDeviceClass;
    }

    bool isDeviceDependent() const {
        return _isDeviceDependent;
    }

    Opt<Bytes> data() const {
        return _data;
    }
};

} // namespace Karm::Icc
