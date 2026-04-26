#include <karm/test>

import Karm.Regex;

namespace Karm::Regex::Tests {

test$("regex-atom") {
    expect$("a"_regex.contains("a"));
    expectNot$("a"_regex.contains("b"));
    expect$("\\"_regex.contains("\\"));

    return Ok();
}

test$("regex-chain") {
    expect$("ab"_regex.contains("ab"));
    expect$("abc"_regex.contains("abc"));
    expectNot$("abc"_regex.contains("cba"));

    return Ok();
}

test$("regex-disjunction") {
    auto re = "a|b|c"_regex;
    expect$(re.contains("a"));
    expect$(re.contains("b"));
    expect$(re.contains("c"));
    expectNot$(re.contains("d"));

    return Ok();
}

test$("regex-group") {
    auto re = "(ab)+"_regex;
    expect$(re.wholeMatch("ab") != NONE);
    expect$(re.wholeMatch("abababababab") != NONE);
    expectNot$(re.wholeMatch("abababababa") != NONE);
    expectNot$(re.contains(""));

    return Ok();
}

} // namespace Karm::Regex::Tests
