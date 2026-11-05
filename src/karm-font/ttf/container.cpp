module;

#include <karm/macros>

export module Karm.Font.Ttf:container;

import Karm.Core;

import :base;

namespace Karm::Font::Ttf {

// https://learn.microsoft.com/en-us/typography/opentype/spec/otff#organization-of-an-opentype-font
export struct FontContainer {
    struct Table {
        Str tag;
        Bytes data;
    };

    u32 _version = 0x00010000;
    Vec<Table> _tables;

    // Table records must be sorted in ascending order by tag.
    void add(Str tag, Bytes data) {
        usize i = 0;
        while (i < _tables.len() and _tables[i].tag < tag)
            i++;
        _tables.insert(i, Table{tag, data});
    }

    static u32 _tableChecksum(Table const& t) {
        u32 sum = checksum(t.data);
        // https://learn.microsoft.com/en-us/typography/opentype/spec/head
        // checksumAdjustment is taken as zero when computing the head checksum.
        if (t.tag == "head" and t.data.len() >= 12)
            sum -= Io::BScan{t.data}.skip(8).nextU32be();
        return sum;
    }

    Res<> write(Io::Writer& w) const {
        u16 numTables = _tables.len();
        u16 maxPow2 = 1;
        u16 entrySelector = 0;
        while (maxPow2 * 2 <= numTables) {
            maxPow2 *= 2;
            entrySelector++;
        }

        Io::BufferWriter directory;
        Io::BEmit e{directory};
        try$(e.writeU32be(_version));
        try$(e.writeU16be(numTables));
        try$(e.writeU16be(maxPow2 * 16));
        try$(e.writeU16be(entrySelector));
        try$(e.writeU16be(numTables * 16 - maxPow2 * 16));

        usize offset = 12 + numTables * 16;
        u32 fontChecksum = 0;
        for (auto& t : _tables) {
            u32 tableChecksum = _tableChecksum(t);
            try$(e.writeStr(t.tag));
            try$(e.writeU32be(tableChecksum));
            try$(e.writeU32be(offset));
            try$(e.writeU32be(t.data.len()));
            offset += alignUp(t.data.len(), 4uz);
            fontChecksum += tableChecksum;
        }
        fontChecksum += checksum(directory.bytes());

        Io::BEmit out{w};
        try$(out.writeBytes(directory.bytes()));
        for (auto& t : _tables) {
            if (t.tag == "head" and t.data.len() >= 12) {
                try$(out.writeBytes(sub(t.data, 0, 8)));
                try$(out.writeU32be(0xB1B0AFBA - fontChecksum));
                try$(out.writeBytes(sub(t.data, 12, t.data.len())));
            } else {
                try$(out.writeBytes(t.data));
            }

            try$(out.align(4));
        }

        return Ok();
    }
};

} // namespace Karm::Font::Ttf
