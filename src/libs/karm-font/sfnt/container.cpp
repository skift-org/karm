module;

#include <karm-sys/mmap.h>

#include "../ttf/fontface.h"

export module Karm.Font:sfnt.container;

import Karm.Core;

namespace Karm::Font::Sfnt {

// https://learn.microsoft.com/en-us/typography/opentype/spec/otff

static constexpr u32 TRUETYPE_SIGNATURE = 0x00010000;
static constexpr u32 OPENTYPE_SIGNATURE = 0x4F54544F;

struct [[gnu::packed]] Header {
    // uint32	sfntVersion	0x00010000 or 0x4F54544F ('OTTO') — see below.
    u32be sfntVersion;

    // uint16	numTables	Number of tables.
    u16be numTables;

    // uint16	searchRange	Maximum power of 2 less than or equal to numTables, times 16 ((2**floor(log2(numTables))) * 16, where “**” is an exponentiation operator).
    u16be searchRange;

    // uint16	entrySelector	Log2 of the maximum power of 2 less than or equal to numTables (log2(searchRange/16), which is equal to floor(log2(numTables))).
    u16be entrySelector;

    // uint16	rangeShift	numTables times 16, minus searchRange ((numTables * 16) - searchRange).
    u16be rangeShift;
};

struct [[gnu::packed]] TableRecord {
    // Tag	tableTag	Table identifier.
    Array<char, 4> tableTag;

    // uint32	checksum	Checksum for this table.
    u32be checksum;

    // Offset32	offset	Offset from beginning of font file.
    u32be offset;

    // uint32	length	Length of this table.
    u32be length;
};

export bool sniff(Bytes b) {
    if (b.len() < sizeof(Header))
        return false;
    Io::BScan s{b};
    auto header = s.next<Header>();
    return header.sfntVersion == TRUETYPE_SIGNATURE or
           header.sfntVersion == OPENTYPE_SIGNATURE;
}

export struct Container : Ttf::Container {
    Header _header;
    Sys::Mmap _mmap;

    static Res<Rc<Container>> load(Sys::Mmap&& mmap) {
        Io::BScan s{mmap.bytes()};
        auto header = s.next<Header>();
        if (header.sfntVersion != TRUETYPE_SIGNATURE and
            header.sfntVersion != OPENTYPE_SIGNATURE)
            return Error::invalidData("expected sfnt font");

        return Ok(makeRc<Container>(header, mmap));
    }

    Generator<Ttf::Table> iterTables() override {
        Io::BScan s{_mmap.bytes()};
        s.skip(sizeof(Header));

        for (usize i : range(_header.numTables.value())) {
            auto record = s.next<TableRecord>();
            co_yield Ttf::Table{
                record.tableTag,
                sub(_mmap.bytes(), record.offset, record.offset + record.length),
            };
        }
    }
};

} // namespace Karm::Font::Sfnt