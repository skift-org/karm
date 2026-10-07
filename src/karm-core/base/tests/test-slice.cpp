#include <karm/test>

using namespace Karm::Literals;

namespace Karm::Base {

auto needle(isize value) {
    return [value](auto x) {
        return x <=> value;
    };
}

test$("slice-search") {
    Array arr{1, 5, 10};

    assertEq$(search(arr, needle(0)), NONE);

    assertEq$(search(arr, needle(1)), 0uz);
    assertEq$(search(arr, needle(2)), NONE);
    assertEq$(search(arr, needle(3)), NONE);
    assertEq$(search(arr, needle(4)), NONE);
    assertEq$(search(arr, needle(5)), 1uz);
    assertEq$(search(arr, needle(6)), NONE);
    assertEq$(search(arr, needle(7)), NONE);
    assertEq$(search(arr, needle(8)), NONE);
    assertEq$(search(arr, needle(9)), NONE);
    assertEq$(search(arr, needle(10)), 2uz);

    assertEq$(search(arr, needle(11)), NONE);

    return Ok();
}

test$("slice-search-lower-bound") {
    Array arr{1, 5, 10};

    assertEq$(searchLowerBound(arr, needle(0)), NONE);

    assertEq$(searchLowerBound(arr, needle(1)), 0uz);
    assertEq$(searchLowerBound(arr, needle(2)), 0uz);
    assertEq$(searchLowerBound(arr, needle(3)), 0uz);
    assertEq$(searchLowerBound(arr, needle(4)), 0uz);
    assertEq$(searchLowerBound(arr, needle(5)), 1uz);
    assertEq$(searchLowerBound(arr, needle(6)), 1uz);
    assertEq$(searchLowerBound(arr, needle(7)), 1uz);
    assertEq$(searchLowerBound(arr, needle(8)), 1uz);
    assertEq$(searchLowerBound(arr, needle(9)), 1uz);
    assertEq$(searchLowerBound(arr, needle(10)), 2uz);

    assertEq$(searchLowerBound(arr, needle(11)), 2uz);

    return Ok();
}

test$("slice-search-upper-bound") {
    Array arr{1, 5, 10};

    assertEq$(searchUpperBound(arr, needle(0)), 0uz);

    assertEq$(searchUpperBound(arr, needle(1)), 0uz);
    assertEq$(searchUpperBound(arr, needle(2)), 1uz);
    assertEq$(searchUpperBound(arr, needle(3)), 1uz);
    assertEq$(searchUpperBound(arr, needle(4)), 1uz);
    assertEq$(searchUpperBound(arr, needle(5)), 1uz);
    assertEq$(searchUpperBound(arr, needle(6)), 2uz);
    assertEq$(searchUpperBound(arr, needle(7)), 2uz);
    assertEq$(searchUpperBound(arr, needle(8)), 2uz);
    assertEq$(searchUpperBound(arr, needle(9)), 2uz);
    assertEq$(searchUpperBound(arr, needle(10)), 2uz);

    assertEq$(searchUpperBound(arr, needle(11)), NONE);

    return Ok();
}

test$("slice-contains") {
    assert$(contains("Hello, world!"s, "world"s));
    assert$(contains("Hello, world!"s, "world!"s));
    assert$(contains("Hello, world!"s, "Hello"s));
    assert$(contains("Hello, world!"s, "Hello, world!"s));
    assertNot$(contains("Hello, world!"s, "Hello, world! "s));
    assertNot$(contains("Hello, world!"s, "bruh"s));

    auto customCmp = [](Rune a, Rune b) {
        return toAsciiLower(a) == toAsciiLower(b);
    };

    assert$(contains("Ab"s, "ab"s, customCmp));
    assert$(contains("ab"s, "Ab"s, customCmp));
    assertNot$(contains("Ab"s, "ab"s));
    assertNot$(contains("ab"s, "Ab"s));

    return Ok();
}

test$("slice-index-of") {
    assertEq$(indexOf("Hello, world!"s, "world"s), 7uz);
    assertEq$(indexOf("Hello, world!"s, "world!"s), 7uz);
    assertEq$(indexOf("Hello, world!"s, "Hello"s), 0uz);
    assertEq$(indexOf("Hello, world!"s, "Hello, world!"s), 0uz);
    assertEq$(indexOf("Hello, world!"s, "Hello, world! "s), NONE);
    assertEq$(indexOf("Hello, world!"s, "bruh"s), NONE);

    auto customCmp = [](Rune a, Rune b) {
        return toAsciiLower(a) == toAsciiLower(b);
    };

    assertEq$(indexOf("Ab"s, "ab"s, customCmp), 0uz);
    assertEq$(indexOf("ab"s, "Ab"s, customCmp), 0uz);
    assertEq$(indexOf("Ab"s, "ab"s), NONE);
    assertEq$(indexOf("ab"s, "Ab"s), NONE);

    return Ok();
}

test$("slice-split-simple") {

    Str text = "hello my friends"s;

    auto pieces = split(text, ' ');

    assertEq$(pieces.next(), "hello"s);
    assertEq$(pieces.next(), "my"s);
    assertEq$(pieces.next(), "friends"s);
    assertEq$(pieces.next(), NONE);

    return Ok();
}

test$("slice-split-consecutive-delim") {
    {
        Str text = "hello  my  friends"s;

        auto pieces = split(text, ' ');

        assertEq$(pieces.next(), "hello"s);
        assertEq$(pieces.next(), ""s);
        assertEq$(pieces.next(), "my"s);
        assertEq$(pieces.next(), ""s);
        assertEq$(pieces.next(), "friends"s);
        assertEq$(pieces.next(), NONE);

        return Ok();
    }
    {
        Str text = " my "s;
        auto pieces = split(text, ' ');

        assertEq$(pieces.next(), ""s);
        assertEq$(pieces.next(), "my"s);
        assertEq$(pieces.next(), ""s);
        assertEq$(pieces.next(), NONE);

        return Ok();
    }
}

test$("slice-split-no-delim") {

    Str text = "hellomyfriends"s;
    auto pieces = split(text, ' ');

    assertEq$(pieces.next(), "hellomyfriends"s);
    assertEq$(pieces.next(), NONE);

    return Ok();
}

test$("slice-split-empty") {
    {
        Str text = ""s;
        auto pieces = split(text, ' ');

        assertEq$(pieces.next(), ""s);
        assertEq$(pieces.next(), NONE);
    }

    {
        Str text = " "s;
        auto pieces = split(text, ' ');

        assertEq$(pieces.next(), ""s);
        assertEq$(pieces.next(), ""s);
        assertEq$(pieces.next(), NONE);
    }

    return Ok();
}

test$("slice-niche") {
    Opt<Slice<char>> test;

    auto comp = Slice<char>("test", 5);

    assertEq$(sizeof(test), sizeof(Slice<char>));
    assertEq$(test.has(), false);
    assertEq$(test, NONE);
    test = Some(Slice<char>("test", 5));
    assertEq$(test.expect(), comp);
    assertEq$(test.take(), comp);
    assertEq$(test, NONE);
    test = Some(Slice<char>("", 1));
    assertEq$(test.has(), true);

    return Ok();
}

test$("mutslice-niche") {
    Opt<MutSlice<char>> test;

    auto comp = Slice<char>("test", 5);

    assertEq$(sizeof(test), sizeof(MutSlice<char>));
    assertEq$(test.has(), false);
    assertEq$(test, NONE);
    test = Some(MutSlice<char>(new char[5], 5));
    copy(comp, test.expect());
    assertEq$(test.expect(), comp);
    delete[] test.take().buf();
    assertEq$(test, NONE);

    test = Some(MutSlice<char>(new char[5], 5));
    assertEq$(test.has(), true);
    delete[] test->buf();

    return Ok();
}

} // namespace Karm::Base
