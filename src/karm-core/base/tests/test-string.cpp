import Karm.Core;

#include <karm/test>

using namespace Karm::Literals;

namespace Karm::Base::Tests {

test$("string-default-constructed-inline") {
    InlineString<16> str;

    assertEq$(str.len(), 0uz);
    assertEq$(str, ""s);

    return Ok();
}

test$("string-value-constructed-inline") {
    InlineString<16> str("Hello, World!");

    assertEq$(str.len(), 13uz);
    assertEq$(str, "Hello, World!"s);

    return Ok();
}

test$("string-default-constructed") {
    String str;

    assertEq$(str.len(), 0uz);
    assertEq$(str, ""s);
    // We have to use _buf here because in the case of a default
    // constructed String, buf() will lie to us and return ""
    // but internally it is nullptr and no buffer has been allocated.
    assertEq$(str._buf, nullptr);

    return Ok();
}

test$("string-value-constructed") {
    String str("Hello, World!");

    assertEq$(str.len(), 13uz);
    assertEq$(str, "Hello, World!"s);

    return Ok();
}

test$("string-niche") {
    Opt<String> test;

    auto comp = String("test");

    assertEq$(sizeof(test), sizeof(String));
    assertEq$(test.has(), false);
    assertEq$(test, NONE);
    test = Some("test"s);
    assertEq$(test.expect(), comp);
    assertEq$(test.take(), comp);
    assertEq$(test, NONE);
    test = Some(""s);
    assertEq$(test.has(), true);

    return Ok();
}

test$("str-niche") {
    Opt<Str> test;

    auto comp = Str("test");

    assertEq$(sizeof(test), sizeof(Str));
    assertEq$(test.has(), false);
    assertEq$(test, NONE);
    test = Some("test"s);
    assertEq$(test.expect(), comp);
    assertEq$(test.take(), comp);
    assertEq$(test, NONE);
    test = Some(""s);
    assertEq$(test.has(), true);

    return Ok();
}

} // namespace Karm::Base::Tests
