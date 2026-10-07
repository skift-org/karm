#include <karm/test>

import Karm.Core;
import Karm.Icu;

namespace Karm::Icu {

test$("icu-numeric-value") {
    assertEq$(Properties::of('0').numericValue().numerator, 0);
    assertEq$(Properties::of('1').numericValue().numerator, 1);
    assertEq$(Properties::of('3').numericValue().numerator, 3);
    assertEq$(Properties::of('9').numericValue().numerator, 9);
    assertEq$(Properties::of('A').numericValue().denominator, 0);
    return Ok();
}

} // namespace Karm::Icu

