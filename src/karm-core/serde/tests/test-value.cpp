#include <karm/test>

import Karm.Core;

using namespace Karm::Literals;

namespace Karm::Serde::Tests {

test$("json-value-null") {
    Value value = NONE;

    assertEq$(value, NONE);
    assert$(value.isNull());
    assertEq$(value.asStr(), "null"s);
    assertEq$(value.asInt(), 0);
    assertEq$(value.asFloat(), 0.0);
    assertEq$(value.asBool(), false);
    assertEq$(value.len(), 0uz);

    return Ok();
}

test$("json-value-array") {
    Value value = Array{
        Integer{1},
        Integer{2},
        Integer{3},
    };

    assert$(not value.isNull());
    assert$(value.isArray());
    assertEq$(value.asStr(), "<array>"s);
    assertEq$(value.asInt(), 0);
    assertEq$(value.asFloat(), 0.0);
    assertEq$(value.asBool(), true);
    assertEq$(Value{Array{}}.asBool(), false);

    assertEq$(value.len(), 3uz);
    assertEq$(value.get(0).asInt(), 1);
    assertEq$(value.get(1).asInt(), 2);
    assertEq$(value.get(2).asInt(), 3);

    return Ok();
}

test$("json-value-object") {
    Value value = Object{
        {"a"s, Integer{1}},
        {"b"s, Integer{2}},
        {"c"s, Integer{3}},
    };

    assert$(not value.isNull());
    assert$(value.isObject());
    assertEq$(value.asStr(), "<object>"s);
    assertEq$(value.asInt(), 0);
    assertEq$(value.asFloat(), 0.0);
    assertEq$(value.asBool(), true);
    assertEq$(Value{Object{}}.asBool(), false);

    assertEq$(value.len(), 3uz);
    assertEq$(value.get("a").asInt(), 1);
    assertEq$(value.get("b").asInt(), 2);
    assertEq$(value.get("c").asInt(), 3);

    return Ok();
}

test$("json-value-string") {
    Value value = String{"hello"};

    assert$(value.isStr());
    assertEq$(value.asStr(), "hello"s);
    assertEq$(value.asInt(), 0);
    assertEq$(value.asFloat(), 0.0);
    assertEq$(value.asBool(), true);
    assertEq$(value.len(), 5uz);

    return Ok();
}

test$("json-value-integer") {
    Value value = Integer{42};

    assert$(value.isInt());
    assertEq$(value.asStr(), "42"s);
    assertEq$(value.asInt(), 42);
    assertEq$(value.asFloat(), 42.0);
    assertEq$(value.asBool(), true);
    assertEq$(value.len(), 0uz);

    return Ok();
}

test$("json-value-float") {
    Value value = Number{3.14};

    assert$(value.isFloat());
    assert$(Math::epsilonEq(value.asFloat(), 3.14, 0.001));
    assertEq$(value.asStr(), "3.140000"s); // FIXME: Once FloatFormatter can stop producing trailing zeros
    assertEq$(value.asInt(), 3);
    assertEq$(value.asBool(), true);
    assertEq$(value.len(), 0uz);

    return Ok();
}

test$("json-value-true") {
    Value value = true;

    assert$(value.isBool());
    assertEq$(value.asStr(), "true"s);
    assertEq$(value.asInt(), 1);
    assertEq$(value.asFloat(), 1.0);
    assertEq$(value.asBool(), true);
    assertEq$(value.len(), 0uz);

    return Ok();
}

test$("json-value-false") {
    Value value = false;

    assert$(value.isBool());
    assertEq$(value.asStr(), "false"s);
    assertEq$(value.asInt(), 0);
    assertEq$(value.asFloat(), 0.0);
    assertEq$(value.asBool(), false);
    assertEq$(value.len(), 0uz);

    return Ok();
}

} // namespace Karm::Serde::Tests
