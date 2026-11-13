#include <karm/test>

import Karm.Core;

using namespace Karm::Literals;

namespace Karm::Io::Tests {

static constexpr Array<u8, 6> BSCAN_DATA = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};

test$("bscan-read-ints") {
    BScan s{BSCAN_DATA};
    assertEq$(try$(s.nextU16be()), 0x0102u);
    assertEq$(try$(s.nextU16le()), 0x0403u);
    assertEq$(try$(s.peekU8be()), 0x05u);
    assertEq$(s.tell(), 4uz);
    assertEq$(try$(s.nextU8be()), 0x05u);
    assertEq$(s.rem(), 1uz);
    return Ok();
}

test$("bscan-short-read-fails-without-advancing") {
    BScan s{BSCAN_DATA};
    try$(s.skip(4));

    // Only 2 bytes left, a u32 must not be readable.
    auto r = s.nextU32le();
    assert$(not r);
    assertEq$(r.none().code(), Error::UNEXPECTED_EOF);
    assertEq$(s.tell(), 4uz);

    assert$(not s.peekU32be());
    assert$(not s.next<u64>());
    assertEq$(s.tell(), 4uz);

    // Remaining bytes are still readable.
    assertEq$(try$(s.nextU16be()), 0x0506u);
    assert$(s.ended());
    assert$(not s.nextU8le());
    return Ok();
}

test$("bscan-skip-seek-bounds") {
    BScan s{BSCAN_DATA};
    assert$(not s.skip(7));
    assertEq$(s.tell(), 0uz);
    try$(s.skip(6));
    assert$(s.ended());

    assert$(not s.seek(7));
    assertEq$(s.tell(), 6uz);
    try$(s.seek(2));
    assertEq$(s.tell(), 2uz);

    assert$(not s.peek(5));
    auto p = try$(s.peek(4));
    assertEq$(p.tell(), 6uz);
    assertEq$(s.tell(), 2uz);

    try$(s.ensure(4));
    assert$(not s.ensure(5));
    return Ok();
}

test$("bscan-read-to-array") {
    BScan s{BSCAN_DATA};
    u16 buf[3] = {};
    try$(s.readTo(buf, 3));
    assert$(s.ended());

    s.rewind();
    u16 big[4] = {};
    assert$(not s.readTo(big, 4));
    assertEq$(s.tell(), 0uz);
    return Ok();
}

test$("bscan-strings-and-bytes") {
    BScan s{Bytes{(u8 const*)"ab\0cdef", 7}};
    assertEq$(try$(s.nextCStr()), "ab"s);
    assertEq$(s.tell(), 3uz);

    // Fixed-width field without terminator.
    assertEq$(try$(s.nextCStr(2)), "cd"s);

    // Unterminated string at end of buffer.
    assert$(not s.nextCStr());
    assertEq$(s.tell(), 5uz);

    assert$(not s.nextStr(3));
    assert$(not s.nextBytes(3));
    assertEq$(try$(s.nextStr(2)), "ef"s);
    assert$(s.ended());
    return Ok();
}

test$("bscan-bits") {
    BScan s{Bytes{BSCAN_DATA.buf(), 1}};
    assertEq$(try$(s.nextBitsbe(7)), 0u);
    assertEq$(try$(s.nextBitbe()), 1u);
    assert$(not s.nextBitbe());

    s = BScan{Bytes{BSCAN_DATA.buf(), 1}};
    assertEq$(try$(s.nextBitle()), 1u);
    assertEq$(try$(s.nextBitsle(7)), 0u);
    assert$(not s.nextBitle());
    return Ok();
}

test$("bscan-slice") {
    struct [[gnu::packed]] Record {
        u8 tag;
        u16be value;
    };

    BScan s{BSCAN_DATA};
    assert$(not s.nextSlice<Record>(3));
    assertEq$(s.tell(), 0uz);

    auto peeked = try$(s.peekSlice<Record>(2));
    assertEq$(peeked.len(), 2uz);
    assertEq$(s.tell(), 0uz);

    auto records = try$(s.nextSlice<Record>(2));
    assertEq$(s.tell(), 6uz);
    assertEq$(records[0].tag, 0x01u);
    assertEq$(records[0].value, 0x0203u);
    assertEq$(records[1].tag, 0x04u);
    assertEq$(records[1].value, 0x0506u);

    auto empty = try$(s.nextSlice<Record>(0));
    assertEq$(empty.len(), 0uz);
    return Ok();
}

test$("bchunk-get") {
    BChunk c{BSCAN_DATA};
    using F = BField<u16be, 4>;
    using G = BField<u32be, 4>;
    assertEq$(try$(c.get<F>()), 0x0506u);
    assert$(not c.get<G>());
    assert$(not c.begin(7));
    return Ok();
}

} // namespace Karm::Io::Tests
