#include <karm/test>

import Karm.Core;

using namespace Karm::Literals;

namespace Karm::Io::Tests {

test$("aton-atoi") {
    assertEq$(Io::atoi("0"s), 0);
    assertEq$(Io::atoi("1"s), 1);
    assertEq$(Io::atoi("2"s), 2);
    assertEq$(Io::atoi("3      "s), 3);
    assertEq$(Io::atoi("10"s), 10);
    assertEq$(Io::atoi("100"s), 100);
    assertEq$(Io::atoi("+100"s), 100);

    assertEq$(Io::atoi("-1"s), -1);
    assertEq$(Io::atoi("-10"s), -10);
    assertEq$(Io::atoi("-100"s), -100);

    assertEq$(Io::atoi("+"s), NONE);
    assertEq$(Io::atoi("-"s), NONE);
    assertEq$(Io::atoi("+-100"s), NONE);
    assertEq$(Io::atoi("-+100"s), NONE);

    return Ok();
}

test$("aton-atof") {
    assert$(Math::epsilonEq(try$(Io::atof("0.0"s)), 0.0));
    assert$(Math::epsilonEq(try$(Io::atof("0.1"s)), 0.1));
    assert$(Math::epsilonEq(try$(Io::atof("0.5"s)), 0.5));
    assert$(Math::epsilonEq(try$(Io::atof("+0.5"s)), 0.5));

    assert$(Math::epsilonEq(try$(Io::atof(".0"s)), 0.0));
    assert$(Math::epsilonEq(try$(Io::atof(".1"s)), 0.1));
    assert$(Math::epsilonEq(try$(Io::atof(".5"s)), 0.5));

    assert$(Math::epsilonEq(try$(Io::atof("0"s)), 0.));
    assert$(Math::epsilonEq(try$(Io::atof("1"s)), 1.));
    assert$(Math::epsilonEq(try$(Io::atof("5"s)), 5.));

    assert$(Math::epsilonEq(try$(Io::atof("0."s)), 0.));
    assert$(Math::epsilonEq(try$(Io::atof("1."s)), 1.));
    assert$(Math::epsilonEq(try$(Io::atof("5."s)), 5.));

    assert$(Math::epsilonEq(try$(Io::atof("-0"s)), -0.));
    assert$(Math::epsilonEq(try$(Io::atof("-1"s)), -1.));
    assert$(Math::epsilonEq(try$(Io::atof("-5"s)), -5.));

    assert$(Math::epsilonEq(try$(Io::atof("-0."s)), -0.));
    assert$(Math::epsilonEq(try$(Io::atof("-1."s)), -1.));
    assert$(Math::epsilonEq(try$(Io::atof("-5."s)), -5.));

    assert$(Math::epsilonEq(try$(Io::atof("-.0"s)), -.0));
    assert$(Math::epsilonEq(try$(Io::atof("-.1"s)), -.1));
    assert$(Math::epsilonEq(try$(Io::atof("-.5"s)), -.5));

    assert$(Math::epsilonEq(try$(Io::atof("-0.0"s)), -0.0));
    assert$(Math::epsilonEq(try$(Io::atof("-0.1"s)), -0.1));
    assert$(Math::epsilonEq(try$(Io::atof("-0.5"s)), -0.5));

    assert$(Math::epsilonEq(try$(Io::atof("2.5e3"s)), 2500.0));
    assert$(Math::epsilonEq(try$(Io::atof("-1.0e6"s)), -1000000.0));
    assert$(Math::epsilonEq(try$(Io::atof("3e0"s)), 3.0));
    assert$(Math::epsilonEq(try$(Io::atof("-7.89e2"s)), -789.0));
    assert$(Math::epsilonEq(try$(Io::atof("5.00e-3"s)), 0.005));

    assertEq$(Io::atof("+"s), NONE);
    assertEq$(Io::atof("."s), NONE);
    assertEq$(Io::atof("+."s), NONE);
    assertEq$(Io::atof("-"s), NONE);
    assertEq$(Io::atof("-+100"s), NONE);
    assertEq$(Io::atof("+-100"s), NONE);

    assert$(Math::epsilonEq(try$(Io::atof("1px"s)), 1.0));

    return Ok();
}

} // namespace Karm::Io::Tests
