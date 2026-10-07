#include <karm/test>

import Karm.Icc;

namespace Karm::Icc::Tests {

test$("icc-srgb") {
    auto srgb = ColorProfile::srgb();
    assertEq$(srgb->colorSpace(), ColorSpace::RGB);
    assertEq$(srgb->colorSpace().components(), 3ul);
    assertEq$(srgb->isDeviceDependent(), false);
    return Ok();
}

test$("icc-sgray") {
    auto sgray = ColorProfile::sgray();
    assertEq$(sgray->colorSpace(), ColorSpace::GRAY);
    assertEq$(sgray->colorSpace().components(), 1ul);
    assertEq$(sgray->isDeviceDependent(), false);
    return Ok();
}

test$("icc-device-dependent") {
    assertEq$(ColorProfile::deviceRgb()->isDeviceDependent(), true);
    assertEq$(ColorProfile::deviceCmyk()->isDeviceDependent(), true);
    assertEq$(ColorProfile::deviceGray()->isDeviceDependent(), true);
    return Ok();
}

} // namespace Karm::Icc::Tests
