module;

#include <karm/macros>

export module Karm.Font.Ttf:sfnt;

import Karm.Core;

import :base;

// https://learn.microsoft.com/en-us/typography/opentype/spec/otff

namespace Karm::Font::Ttf {

/// Table data is laid out in the order it was added, the directory is
/// sorted by tag as the spec requires, and the checksums are recomputed.
export struct SfntBuilder {
    static constexpr usize HEADER_SIZE = 12;
    static constexpr usize ENTRY_SIZE = 16;
    static constexpr usize MAX_TABLES = 0xffff;

    /// Offsets are 32 bits wide and the directory comes before the tables.
    static constexpr usize MAX_DATA_SIZE = alignDown(0xffffffffuz - HEADER_SIZE - ENTRY_SIZE * MAX_TABLES, 4uz);

    static constexpr u32 HEAD = 0x68656164;         //< 'head'
    static constexpr usize CHECKSUM_ADJUSTMENT = 8; //< Offset in the 'head' table
    static constexpr u32 CHECKSUM_MAGIC = 0xB1B0AFBA;

    struct Table {
        u32 tag;
        u32 checksum;
        u32 offset; //< From the start of the table data
        u32 length;
    };

    u32 _flavor;
    BufferBuilder _data;
    Vec<Table> _tables;

    SfntBuilder(u32 flavor = 0x00010000)
        : _flavor(flavor) {}

    usize len() const {
        return _tables.len();
    }

    Res<> add(u32 tag, Bytes data) {
        if (_tables.len() == MAX_TABLES)
            return Error::invalidData("sfnt: too many tables");

        if (data.len() > MAX_DATA_SIZE or _data.len() > MAX_DATA_SIZE - alignUp(data.len(), 4uz))
            return Error::invalidData("sfnt: font too large");

        usize offset = _data.len();
        _data.append(data);
        _data.align(4);

        if (tag == HEAD) {
            if (data.len() < CHECKSUM_ADJUSTMENT + 4)
                return Error::invalidData("sfnt: 'head' table too small");
            // The table is summed with its adjustment cleared
            _data.patchU32be(offset + CHECKSUM_ADJUSTMENT, 0);
        }

        _tables.pushBack({
            .tag = tag,
            .checksum = checksum(sub(_data.bytes(), offset, _data.len())),
            .offset = (u32)offset,
            .length = (u32)data.len(),
        });
        return Ok();
    }

    Res<> add(Str tag, Bytes data) {
        if (tag.len() != 4)
            return Error::invalidInput("sfnt: table tags are four characters long");
        return add((u32)(u8)tag[0] << 24 | (u32)(u8)tag[1] << 16 | (u32)(u8)tag[2] << 8 | (u32)(u8)tag[3], data);
    }

    Res<Vec<u8>> finish() {
        if (_tables.len() == 0)
            return Error::invalidData("sfnt: no tables");

        sort(_tables, [](Table const& a, Table const& b) {
            return a.tag <=> b.tag;
        });

        for (usize i = 1; i < _tables.len(); i++) {
            if (_tables[i - 1].tag == _tables[i].tag)
                return Error::invalidData("sfnt: duplicate table");
        }

        u16 numTables = _tables.len();
        u16 entrySelector = 0;
        while ((2u << entrySelector) <= numTables)
            entrySelector++;
        u16 searchRange = (1u << entrySelector) * ENTRY_SIZE;
        usize dataStart = HEADER_SIZE + ENTRY_SIZE * numTables;

        BufferBuilder res{dataStart + _data.len()};
        res.appendU32be(_flavor);
        res.appendU16be(numTables);
        res.appendU16be(searchRange);
        res.appendU16be(entrySelector);
        res.appendU16be(numTables * ENTRY_SIZE - searchRange);
        for (auto& t : _tables) {
            res.appendU32be(t.tag);
            res.appendU32be(t.checksum);
            res.appendU32be(dataStart + t.offset);
            res.appendU32be(t.length);
        }

        // Everything is 4-byte aligned, so the checksum of the file is the
        // sum of the checksums of its parts.
        u32 sum = checksum(res.bytes());
        for (auto& t : _tables)
            sum += t.checksum;

        res.append(_data.bytes());

        for (auto& t : _tables) {
            if (t.tag == HEAD)
                res.patchU32be(dataStart + t.offset + CHECKSUM_ADJUSTMENT, CHECKSUM_MAGIC - sum);
        }

        return Ok(res.take());
    }
};

} // namespace Karm::Font::Ttf
