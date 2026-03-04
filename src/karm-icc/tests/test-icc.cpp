#include <karm/test>

import Karm.Icc;

namespace Karm::Icc::Tests {

test$("karm-icc-srgb") {
    auto srgb = ColorProfile::srgb();
    expectEq$(srgb->colorSpace(), ColorSpace::RGB);
    expectEq$(srgb->colorSpace().components(), 3ul);
    expectEq$(srgb->isDeviceDependent(), false);
    return Ok();
}

test$("karm-icc-sgray") {
    auto sgray = ColorProfile::sgray();
    expectEq$(sgray->colorSpace(), ColorSpace::GRAY);
    expectEq$(sgray->colorSpace().components(), 1ul);
    expectEq$(sgray->isDeviceDependent(), false);
    return Ok();
}

test$("karm-icc-device-dependent") {
    expectEq$(ColorProfile::deviceRgb()->isDeviceDependent(), true);
    expectEq$(ColorProfile::deviceCmyk()->isDeviceDependent(), true);
    expectEq$(ColorProfile::deviceGray()->isDeviceDependent(), true);
    return Ok();
}

} // namespace Karm::Icc::Tests
