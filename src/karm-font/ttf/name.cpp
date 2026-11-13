module;

#include <karm/macros>

export module Karm.Font.Ttf:name;

import Karm.Core;

using namespace Karm::Literals;

namespace Karm::Font::Ttf {

export using Karm::begin, Karm::end;

export struct Name : Io::BChunk {
    static constexpr Str SIG = "name";

    enum NameId : u16 {
        COPYRIGHT = 0,
        FAMILY = 1,
        SUBFAMILY = 2,
        UNIQUE = 3,
        FULL_NAME = 4,
        VERSION = 5,
        POSTSCRIPT = 6,
        TRADEMARK = 7,
        MANUFACTURER = 8,
        DESIGNER = 9,
        DESCRIPTION = 10,
        VENDOR_URL = 11,
        DESIGNER_URL = 12,
        LICENSE = 13,
        LICENSE_URL = 14,
        RESERVED = 15,
        TYPOGRAPHIC_FAMILY = 16,
        TYPOGRAPHIC_SUBFAMILY = 17,
        COMPATIBLE_FULL_NAME = 18,
        SAMPLE_TEXT = 19,
        POSTSCRIPT_CID = 20,
        WWS_FAMILY = 21,
        WWS_SUBFAMILY = 22,
        LIGHT_BACKGROUND_PALETTE = 23,
        DARK_BACKGROUND_PALETTE = 24,
        VARIATIONS_POSTSCRIPT_PREFIX = 25,

        _LEN,
    };

    // https://learn.microsoft.com/en-us/typography/opentype/spec/name#name-records
    struct [[gnu::packed]] Record {
        u16be platformId;
        u16be encodingId;
        u16be languageId;
        u16be nameId;
        u16be length;
        u16be offset;

        bool isUnicode() const {
            return (platformId == 0) or                   // unicode
                   (platformId == 3 and encodingId == 1); // windows
        }
    };

    using StorageOffset = Io::BField<u16be, 4>;

    Res<Slice<Record>> records() const {
        auto s = begin();
        try$(s.skip(2)); // version
        auto count = try$(s.nextU16be());
        try$(s.skip(2)); // storageOffset
        return s.nextSlice<Record>(count);
    }

    Res<Opt<Record>> lookupRecord(NameId nameId) const {
        for (auto const& record : try$(records())) {
            if (record.nameId == nameId and record.isUnicode())
                return Ok(Some(record));
        }

        return Ok(NONE);
    }

    // NOTE: Returns an empty string if the record is missing or malformed.
    String lookupString(NameId nameId) const {
        auto record = lookupRecord(nameId).unwrapOr(NONE);
        return record ? string(*record) : String{};
    }

    // NOTE: Returns an empty string if the record points outside the table.
    String string(Record const& r) const {
        return _string(r).unwrapOr(""s);
    }

    Res<String> _string(Record const& r) const {
        if (not r.isUnicode())
            return Ok(""s);

        auto s = try$(begin(try$(get<StorageOffset>()) + r.offset));
        try$(s.ensure(r.length));
        StringBuilder sb;
        for (usize i = 0; i < r.length / 2; i++) {
            sb.append(try$(s.nextU16be()));
        }
        return Ok(sb.take());
    }
};

} // namespace Karm::Font::Ttf
