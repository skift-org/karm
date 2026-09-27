#include <karm/test>

import Karm.Regex;

using namespace Karm::Literals;
using namespace Karm::Regex::Literals;

namespace Karm::Regex::Tests {

test$("regex-atom") {
    assert$("a"_regex.contains("a"));
    assertNot$("a"_regex.contains("b"));
    assert$("\\"_regex.contains("\\"));

    return Ok();
}

test$("regex-chain") {
    assert$("ab"_regex.contains("ab"));
    assert$("abc"_regex.contains("abc"));
    assertNot$("abc"_regex.contains("cba"));

    return Ok();
}

test$("regex-disjunction") {
    auto re = "a|b|c"_regex;
    assert$(re.contains("a"));
    assert$(re.contains("b"));
    assert$(re.contains("c"));
    assertNot$(re.contains("d"));

    return Ok();
}

test$("regex-group") {
    auto re = "(ab)+"_regex;
    assert$(re.wholeMatch("ab") != NONE);
    assert$(re.wholeMatch("abababababab") != NONE);
    assertNot$(re.wholeMatch("abababababa") != NONE);
    assertNot$(re.contains(""));

    return Ok();
}

} // namespace Karm::Regex::Tests
