#include <karm/test>

import Karm.Icc;

namespace Karm::Icc::Tests {

test$("karm-icc-srgb-v2") {
    auto srgb = ColorProfile::srgb(Version::V2);
    expectEq$(srgb.colorSpace, ColorSpace::RGB);
    expectEq$(srgb.version, Version::V2);
    return Ok();
}

test$("karm-icc-srgb-v4") {
    auto srgb = ColorProfile::srgb(Version::V4);
    expectEq$(srgb.colorSpace, ColorSpace::RGB);
    expectEq$(srgb.version, Version::V4);
    return Ok();
}

test$("karm-icc-sgray-v2") {
    auto srgb = ColorProfile::sgray(Version::V2);
    expectEq$(srgb.colorSpace, ColorSpace::GRAY);
    expectEq$(srgb.version, Version::V2);
    return Ok();
}

test$("karm-icc-sgray-v4") {
    auto srgb = ColorProfile::sgray(Version::V4);
    expectEq$(srgb.colorSpace, ColorSpace::GRAY);
    expectEq$(srgb.version, Version::V4);
    return Ok();
}

} // namespace Karm::Icc::Tests
