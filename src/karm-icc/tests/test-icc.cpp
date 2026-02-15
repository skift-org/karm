#include <karm/test>

import Karm.Icc;

namespace Karm::Icc::Tests {

test$("karm-icc-srgb") {
    auto srgb = ColorProfile::srgb();
    expectEq$(srgb->colorSpace().name, ColorSpace::Name::RGB);
    expectEq$(srgb->colorSpace().numberOfComponents, 3ul);
    expectEq$(srgb->isDeviceDependent(), false);
    expect$(srgb->iccData(VersionQuery::ONLY_V4).has());
    expect$(srgb->iccData(VersionQuery::ONLY_V2).has());
    return Ok();
}

test$("karm-icc-sgray") {
    auto sgray = ColorProfile::sgray();
    expectEq$(sgray->colorSpace().name, ColorSpace::Name::GRAY);
    expectEq$(sgray->colorSpace().numberOfComponents, 1ul);
    expectEq$(sgray->isDeviceDependent(), false);
    expect$(sgray->iccData(VersionQuery::ONLY_V4).has());
    expect$(sgray->iccData(VersionQuery::ONLY_V2).has());
    return Ok();
}

test$("karm-icc-device-dependent") {
    expectEq$(ColorProfile::deviceRgb()->isDeviceDependent(), true);
    expectEq$(ColorProfile::deviceCmyk()->isDeviceDependent(), true);
    expectEq$(ColorProfile::deviceGray()->isDeviceDependent(), true);
    return Ok();
}

} // namespace Karm::Icc::Tests
