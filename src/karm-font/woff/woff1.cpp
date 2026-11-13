module;

#include <karm/macros>

export module Karm.Font.Woff:woff1;

import Karm.Archive;
import Karm.Core;
import Karm.Font.Ttf;

import :base;

// https://www.w3.org/TR/WOFF/

namespace Karm::Font::Woff {

export bool sniffWoff1(Bytes data) {
    return data.len() >= 4 and
           data[0] == 'w' and data[1] == 'O' and data[2] == 'F' and data[3] == 'F';
}

export Res<Vec<u8>> decodeWoff1(Bytes data) {
    Io::BScan s{data};

    if (try$(s.nextU32be()) != tag("wOFF"))
        return Error::invalidData("woff: bad signature");

    u32 flavor = try$(s.nextU32be());
    u32 length = try$(s.nextU32be());
    u16 numTables = try$(s.nextU16be());
    try$(s.skip(2));  // reserved
    try$(s.skip(4));  // totalSfntSize, recomputed from the directory
    try$(s.skip(4));  // majorVersion, minorVersion
    try$(s.skip(12)); // metaOffset, metaLength, metaOrigLength
    try$(s.skip(8));  // privOffset, privLength

    if (length != data.len())
        return Error::invalidData("woff: length mismatch");

    if (numTables == 0)
        return Error::invalidData("woff: no tables");

    struct Entry {
        u32 tag;
        u32 offset;
        u32 compLength;
        u32 origLength;
    };

    Vec<Entry> entries;
    usize total = Ttf::SfntBuilder::HEADER_SIZE + Ttf::SfntBuilder::ENTRY_SIZE * numTables;
    for (usize i = 0; i < numTables; i++) {
        Entry e;
        e.tag = try$(s.nextU32be());
        e.offset = try$(s.nextU32be());
        e.compLength = try$(s.nextU32be());
        e.origLength = try$(s.nextU32be());
        try$(s.skip(4)); // origChecksum, recomputed

        if (e.compLength > e.origLength)
            return Error::invalidData("woff: table larger compressed than not");

        if (e.offset > data.len() or e.compLength > data.len() - e.offset)
            return Error::invalidData("woff: table out of bounds");

        if (e.origLength > MAX_SFNT_SIZE - total)
            return Error::invalidData("woff: font too large");
        total += alignUp(e.origLength, 4);

        entries.pushBack(e);
    }

    // Tables go back in the order they are stored in, so that a font comes
    // out the way it went in.
    sort(entries, [](Entry const& a, Entry const& b) {
        return a.offset <=> b.offset;
    });

    Ttf::SfntBuilder sfnt{flavor};
    for (auto& e : entries) {
        Bytes src = sub(data, e.offset, e.offset + e.compLength);

        if (e.compLength == e.origLength) {
            try$(sfnt.add(e.tag, src));
            continue;
        }

        Io::BufferWriter w{0, e.origLength};
        try$(Archive::zlibDecompress(src, w));
        if (w.bytes().len() != e.origLength)
            return Error::invalidData("woff: table shorter than declared");
        try$(sfnt.add(e.tag, w.bytes()));
    }

    return sfnt.finish();
}

} // namespace Karm::Font::Woff
