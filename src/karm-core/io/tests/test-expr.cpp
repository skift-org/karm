#include <karm/test>

import Karm.Core;

using namespace Karm::Literals;
using namespace Karm::Re::Literals;

namespace Karm::Re::Tests {

test$("expr-either") {
    assert$(Re::match('a'_re | 'b'_re, ""s) == Match::NO);
    assert$(Re::match('a'_re | 'b'_re, "a"s) == Match::YES);
    assert$(Re::match('a'_re | 'b'_re, "b"s) == Match::YES);
    assert$(Re::match('a'_re | 'b'_re, "c"s) == Match::NO);
    assert$(Re::match('a'_re | 'b'_re, "ab"s) == Match::PARTIAL);

    return Ok();
}

test$("expr-chain") {
    assert$(Re::match('a'_re & 'b'_re, ""s) == Match::NO);
    assert$(Re::match('a'_re & 'b'_re, "ba"s) == Match::NO);
    assert$(Re::match('a'_re & 'b'_re, "ab"s) == Match::YES);
    assert$(Re::match('a'_re & 'b'_re, "abc"s) == Match::PARTIAL);

    return Ok();
}

test$("expr-negate") {
    assert$(Re::match(~'a'_re, ""s) == Match::NO);
    assert$(Re::match(~'a'_re, "b"s) == Match::YES);
    assert$(Re::match(~'a'_re, "a"s) == Match::NO);
    assert$(Re::match((~'a'_re) & 'a'_re, "ba"s) == Match::YES);
    assert$(Re::match((~'a'_re) & 'a'_re, "aa"s) == Match::NO);

    return Ok();
}

test$("expr-single") {

    assert$(Re::match('a'_re, ""s) == Match::NO);
    assert$(Re::match('a'_re, "a"s) == Match::YES);
    assert$(Re::match('a'_re, "b"s) == Match::NO);
    assert$(Re::match('a'_re, "aa"s) == Match::PARTIAL);

    return Ok();
}

} // namespace Karm::Re::Tests
