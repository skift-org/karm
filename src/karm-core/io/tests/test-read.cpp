#include <karm/test>

import Karm.Core;

using namespace Karm::Literals;

namespace Karm::Io::Tests {

test$("readline-ends-with-delim-len-1") {

    BufReader bufReader{bytes("hello\nworldd\n"s)};

    BufferWriter bufWriter;

    {
        auto [read, untilDel] = try$(
            readLine(bufReader, bufWriter, bytes("\n"s))
        );
        assertEq$(bufWriter.take(), bytes("hello\n"s));
        assertEq$(read, 5u);
        assert$(untilDel);
    }

    {
        auto [read, untilDel] = try$(readLine(
            bufReader, bufWriter, bytes("\n"s)
        ));
        assertEq$(bufWriter.take(), bytes("worldd\n"s));
        assertEq$(read, 6u);
        assert$(untilDel);
    }

    return Ok();
}

test$("readline-ends-with-stream") {

    BufReader bufReader{bytes("hello\nwrld"s)};

    BufferWriter bufWriter;

    {
        auto [read, untilDel] = try$(readLine(
            bufReader, bufWriter, bytes("\n"s)
        ));
        assertEq$(bufWriter.take(), bytes("hello\n"s));

        assertEq$(read, 5u);
        assert$(untilDel);
    }

    {
        auto [read, untilDel] = try$(readLine(
            bufReader, bufWriter, bytes("\n"s)
        ));
        assertEq$(bufWriter.take(), bytes("wrld"s));
        assertEq$(read, 4u);
        assertNot$(untilDel);
    }

    return Ok();
}

test$("readline-ends-with-delim-len-5") {

    BufReader bufReader{bytes("hello12345worlds12345he12345"s)};

    BufferWriter bufWriter;

    {
        auto [read, untilDel] = try$(readLine(
            bufReader, bufWriter, bytes("12345"s)
        ));
        assertEq$(bufWriter.take(), bytes("hello12345"s));
        assertEq$(read, 5u);
        assert$(untilDel);
    }

    {
        auto [read, untilDel] = try$(readLine(
            bufReader, bufWriter, bytes("12345"s)
        ));
        assertEq$(bufWriter.take(), bytes("worlds12345"s));
        assertEq$(read, 6u);
        assert$(untilDel);
    }

    {
        auto [read, untilDel] = try$(readLine(
            bufReader, bufWriter, bytes("12345"s)
        ));
        assertEq$(bufWriter.take(), bytes("he12345"s));
        assertEq$(read, 2u);
        assert$(untilDel);
    }

    return Ok();
}

test$("read-all-text-strips-utf8-bom") {
    BufReader bufReader{"\xef\xbb\xbfhello"_bytes};

    auto text = try$(readAllText<Utf8>(bufReader));
    assertEq$(text, "hello"s);

    return Ok();
}

} // namespace Karm::Io::Tests
