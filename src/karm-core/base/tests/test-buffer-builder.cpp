import Karm.Core;

#include <karm/test>

namespace Karm::Base::Tests {

test$("buffer-builder-append") {
    BufferBuilder b;
    assertEq$(b.len(), 0uz);

    b.appendU8be(0x01);
    b.appendU16be(0x0203);
    b.appendU32be(0x04050607);
    b.appendU16le(0x0908);
    b.appendI16be(-2);
    b.append(Array<u8, 2>{0xaa, 0xbb});
    b.appendStr("hi");
    b.appendCStr("!");

    Array<u8, 17> expected = {
        0x01,
        0x02,
        0x03,
        0x04,
        0x05,
        0x06,
        0x07,
        0x08,
        0x09,
        0xff,
        0xfe,
        0xaa,
        0xbb,
        'h',
        'i',
        '!',
        0x00,
    };
    assert$(b.bytes() == Bytes{expected});

    return Ok();
}

test$("buffer-builder-append-large") {
    Vec<u8> chunk;
    for (usize i = 0; i < 1000; i++)
        chunk.pushBack(i);

    BufferBuilder b{0};
    for (usize i = 0; i < 10; i++)
        b.append(chunk);

    assertEq$(b.len(), 10000uz);
    for (usize i = 0; i < b.len(); i++)
        assertEq$(b.bytes()[i], (u8)(i % 1000));

    return Ok();
}

test$("buffer-builder-align") {
    BufferBuilder b;
    b.align(4);
    assertEq$(b.len(), 0uz);

    b.appendU8be(1);
    b.align(4);
    assertEq$(b.len(), 4uz);

    b.align(4);
    assertEq$(b.len(), 4uz);

    b.appendFill(3, 0xff);
    b.align(8, 0x11);
    Array<u8, 8> expected1 = {1, 0, 0, 0, 0xff, 0xff, 0xff, 0x11};
    assert$(b.bytes() == Bytes{expected1});

    return Ok();
}

test$("buffer-builder-patch") {
    BufferBuilder b;
    b.appendFill(6);
    b.patchU16be(0, 0x0102);
    b.patchU32le(2, 0x06050403);
    Array<u8, 6> expected2 = {1, 2, 3, 4, 5, 6};
    assert$(b.bytes() == Bytes{expected2});

    b.patch(4, Array<u8, 2>{0xaa, 0xbb});
    b.patch(6, Bytes{});
    Array<u8, 6> expected3 = {1, 2, 3, 4, 0xaa, 0xbb};
    assert$(b.bytes() == Bytes{expected3});

    b.mutBytes()[0] |= 0x80;
    assertEq$(b.bytes()[0], 0x81);

    return Ok();
}

test$("buffer-builder-take") {
    BufferBuilder b;
    b.appendU16be(0x0102);

    Vec<u8> taken = b.take();
    assertEq$(taken.len(), 2uz);
    assertEq$(b.len(), 0uz);

    b.appendU8be(3);
    assertEq$(b.len(), 1uz);

    b.clear();
    assertEq$(b.len(), 0uz);

    return Ok();
}

test$("buffer-writer-limit") {
    Io::BufferWriter w{0, 4};
    assertEq$(try$(w.write(Array<u8, 3>{1, 2, 3})), 3uz);
    assert$(not w.write(Array<u8, 2>{4, 5}));
    assertEq$(w.len(), 3uz);
    assertEq$(try$(w.write(Array<u8, 1>{4})), 1uz);
    assert$(not w.write(Array<u8, 1>{5}));
    Array<u8, 4> expected4 = {1, 2, 3, 4};
    assert$(w.bytes() == Bytes{expected4});

    Io::BufferWriter unlimited;
    Io::BEmit e{unlimited};
    try$(e.writeU32be(0xdeadbeef));
    assertEq$(unlimited.len(), 4uz);

    return Ok();
}

} // namespace Karm::Base::Tests
