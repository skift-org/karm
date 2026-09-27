#include <karm/test>

import Karm.Core;

using namespace Karm::Literals;
using namespace Karm::Json::Literals;

namespace Karm::Json::Tests {

test$("json-parse-null") {
    auto val = "null"_json;
    assert$(val.isNull());
    return Ok();
}

test$("json-parse-array") {
    auto val = "[1, 2, 3]"_json;

    assert$(val.isArray());
    assertEq$(val.len(), 3uz);
    assertEq$(val.get(0).asInt(), 1);
    assertEq$(val.get(1).asInt(), 2);
    assertEq$(val.get(2).asInt(), 3);

    return Ok();
}

test$("json-parse-object") {
    auto val = R"({"a": 1, "b": 2, "c": 3})"_json;

    assert$(val.isObject());
    assertEq$(val.len(), 3uz);
    assertEq$(val.get("a").asInt(), 1);
    assertEq$(val.get("b").asInt(), 2);
    assertEq$(val.get("c").asInt(), 3);

    return Ok();
}

test$("json-parse-string") {
    auto val = R"("hello")"_json;
    assert$(val.isStr());
    assertEq$(val.asStr(), "hello"s);
    return Ok();
}

test$("json-parse-integer") {
    auto val = "42"_json;
    assert$(val.isInt());
    assertEq$(val.asInt(), 42);
    assertEq$(val.asBool(), true);
    return Ok();
}

test$("json-parse-float") {
    auto val = "3.14"_json;
    assert$(val.isFloat());
    assert$(Math::epsilonEq(val.asFloat(), 3.14, 0.001));
    assertEq$(val.asBool(), true);
    return Ok();
}

test$("json-parse-bool") {
    auto val = "true"_json;
    assert$(val.isBool());
    assertEq$(val.asBool(), true);

    val = "false"_json;
    assert$(val.isBool());
    assertEq$(val.asBool(), false);

    return Ok();
}

test$("json-parse-escaped-unicode") {
    auto val = "\"\\u0041\""_json;
    assert$(val.isStr());
    assertEq$(val.asStr(), "A"s);

    val = "\"\\uD83E\\uDD21\""_json;
    assert$(val.isStr());
    assertEq$(val.asStr(), "🤡"s);

    return Ok();
}

} // namespace Karm::Json::Tests
