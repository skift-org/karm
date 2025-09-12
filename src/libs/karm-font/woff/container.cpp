module;

#include <karm-core/macros.h>
#include <karm-sys/mmap.h>

#include "../ttf/fontface.h"

export module Karm.Font:woff.container;

import Karm.Core;
import Karm.Archive;

namespace Karm::Font::Woff1 {

static constexpr Array<char, u8> SIGNATURE = {'w', 'O', 'F', 'F'};

// https://www.w3.org/TR/WOFF/#WOFFHeader
struct [[gnu::packed]] Header {
    // UInt32	signature	0x774F4646 'wOFF'
    Array<char, 4> signature;

    // UInt32	flavor	The "sfnt version" of the input font.
    u32be flavor;

    // UInt32	length	Total size of the WOFF file.
    u32be length;

    // UInt16	numTables	Number of entries in directory of font tables.
    u16be numTables;

    // UInt16	reserved	Reserved; set to zero.
    u16be reserved;

    // UInt32	totalSfntSize	Total size needed for the uncompressed font data, including the sfnt header, directory, and font tables (including padding).
    u32be totalSfntSize;

    // UInt16	majorVersion	Major version of the WOFF file.
    u16be majorVersion;

    // UInt16	minorVersion	Minor version of the WOFF file.
    u16be minorVersion;

    // UInt32	metaOffset	Offset to metadata block, from beginning of WOFF file.
    u32be metaOffset;

    // UInt32	metaLength	Length of compressed metadata block.
    u32be metaLength;

    // UInt32	metaOrigLength	Uncompressed size of metadata block.
    u32be metaOrigLength;

    // UInt32	privOffset	Offset to private data block, from beginning of WOFF file.
    u32be privOffset;

    // UInt32	privLength	Length of private data block.
    u32be privLength;
};

// https://www.w3.org/TR/WOFF/#TableDirectory
struct [[gnu::packed]] TableDirectoryEntry {
    // UInt32	tag	4-byte sfnt table identifier.
    Array<char, 4> tag;

    // UInt32	offset	Offset to the data, from beginning of WOFF file.
    u32be offset;

    // UInt32	compLength	Length of the compressed data, excluding padding.
    u32be compLength;

    // UInt32	origLength	Length of the uncompressed table, excluding padding.
    u32be origLength;

    // UInt32	origChecksum	Checksum of the uncompressed table.
    u32be origChecksum;
};

struct Table {
    Array<char, 4> tag;
    Vec<u8> data;
};

export bool sniff(Bytes b) {
    if (b.len() < sizeof(Header))
        return false;
    Io::BScan s{b};
    auto header = s.next<Header>();
    return header.signature == SIGNATURE;
}

export struct Container : Ttf::Container {
    Vec<Table> _tables;

    static Res<Rc<Container>> load(Sys::Mmap&& mmap) {
        Io::BScan s{mmap.bytes()};

        auto header = s.next<Header>();
        if (header.signature != SIGNATURE)
            return Error::invalidData("expected woff1 font");

        Vec<Table> tables;
        for (usize i : range<usize>(header.numTables)) {
            auto entry = s.next<TableDirectoryEntry>();
            auto data = sub(mmap.bytes(), entry.offset, entry.offset + entry.compLength);

            if (entry.compLength == entry.origLength) {
                tables.emplaceBack(entry.tag, data);
            } else {
                tables.emplaceBack(entry.tag, try$(Archive::zlibDecompress(data)));
            }
        }

        return Ok(makeRc<Container>(std::move(tables)));
    }

    Generator<Ttf::Table> iterTables() override {
        for (auto& table : _tables)
            co_yield Ttf::Table{table.tag, table.data};
    }
};

} // namespace Karm::Font::Woff1
