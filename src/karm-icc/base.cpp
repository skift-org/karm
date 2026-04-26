module;

#include <karm/macros>

export module Karm.Icc:base;

import Karm.Core;

using namespace Karm::Literals;

namespace Karm::Icc {

export using Karm::begin, Karm::end;

using s15Fixed16be = u32be;

// 7.2.8 MARK: Date/Time -------------------------------------------------------

export struct DateTimeNumber {
    u16be year;
    u16be month;
    u16be day;
    u16be hours;
    u16be minutes;
    u16be seconds;
};

// 7.2.16 MARK: XYZ Illuminant -------------------------------------------------

export struct XYZNumber {
    s15Fixed16be X;
    s15Fixed16be Y;
    s15Fixed16be Z;
};

// 7.2.6 MARK: Data colour space -----------------------------------------------

export struct ColorSpace {
    enum struct _Name : u8 {
        XYZ,
        LAB,
        LUV,
        YCBR,
        YXY,
        RGB,
        GRAY,
        HSV,
        HLS,
        CMYK,
        CMY,
        _DEVICE_N,

        _LEN,
    };

    using enum _Name;

    _Name _name = XYZ;
    u8 _ncomp = 3;

    static Opt<ColorSpace> fromSignature(Bytes signature) {
#define COLORSPACE(NAME, SIG, NCOMP)           \
    if (signature == SIG ""_bytes) {           \
        return ColorSpace{_Name::NAME, NCOMP}; \
    };
#include "defs/colorspace.inc"
#include "defs/device-colorspace.inc"

#undef COLORSPACE
        return NONE;
    }

    static Opt<ColorSpace> fromSignature(u32be sig) {
        auto bytes = unionCast<Array<u8, 4>>(sig);
        return fromSignature(bytes);
    }

    ColorSpace(_Name name) : _name(name) {
        switch (name) {
#define COLORSPACE(NAME, SIG, NCOMP) \
    case NAME:                       \
        _ncomp = NCOMP;              \
        break;
#include "defs/colorspace.inc"

        default:
            break;

#undef COLORSPACE
        }
    }

    ColorSpace(_Name name, u8 ncomp) : _name(name), _ncomp(ncomp) {}

    bool operator==(ColorSpace const&) const = default;

    bool operator==(_Name const& other) const {
        return _name == other;
    }

    usize components() {
        return _ncomp;
    }

    void repr(Io::Emit& e) const {
        if (_name == _DEVICE_N)
            e("DEVICE{}", _ncomp);
        else
            e("{}", _name);
    }
};

// 7.2.4 MARK: Profile Version -------------------------------------------------

u8 fromBcd(u8 value) {
    return (value / 16) * 10 + (value & 0xf);
}

export struct ProfileVersion {
    u8 majorBcd;
    u8 minorBcd;
    u8 reserved0;
    u8 reserved1;

    u8 major() const {
        return fromBcd(majorBcd);
    }

    u8 minor() const {
        return fromBcd(majorBcd & 0xf);
    }

    u8 bugfix() const {
        return fromBcd(majorBcd >> 4);
    }

    void repr(Io::Emit& e) const {
        e("v{}.{}.{}.0", major(), minor(), bugfix());
    }
};

// 7.2.5 MARK: Profile Class ---------------------------------------------------

export struct ProfileClass {
    enum struct _Name : u8 {
        INPUT_DEVICE,
        DISPLAY_DEVICE,
        OUTPUT_DEVICE,
        DEVICE_LINK,
        COLOR_SPACE,
        ABSTRACT,
        NAMED_COLOR,

        _LEN,
    };
    using enum _Name;

    static constexpr Array ALL = {
        INPUT_DEVICE,
        DISPLAY_DEVICE,
        OUTPUT_DEVICE,
        DEVICE_LINK,
        COLOR_SPACE,
        ABSTRACT,
        NAMED_COLOR,
    };

    _Name _name;

    static Opt<ProfileClass> fromSignature(Bytes signature) {
#define PROFILE_CLASS(NAME, SIG)          \
    if (signature == SIG ""_bytes) {      \
        return ProfileClass{_Name::NAME}; \
    };
#include "defs/profile-class.inc"

#undef PROFILE_CLASS
        return NONE;
    }

    static Opt<ProfileClass> fromSignature(u32be sig) {
        auto bytes = unionCast<Array<u8, 4>>(sig);
        return fromSignature(bytes);
    }

    ProfileClass(_Name name) : _name(name) {}

    bool operator==(_Name name) const {
        return _name == name;
    }

    void repr(Io::Emit& e) const {
        e("{}", _name);
    }
};

// 7.2 MARK: Profile Header ----------------------------------------------------

export struct ProfileHeader {
    u32be profileSize;                   // Bytes 0-3
    u32be preferredCmmType;              // Bytes 4-7
    ProfileVersion profileVersionNumber; // Bytes 8-11
    u32be _profileDeviceClass;           // Bytes 12-15

    u32be _dataColourSpace; // Bytes 16-19
    u32be _pcs;             // Bytes 20-23 (Profile Connection Space)

    DateTimeNumber _creationDate; // Bytes 24-35 (12 bytes)

    u32be profileFileSignature;     // Bytes 36-39 ('acsp')
    u32be primaryPlatformSignature; // Bytes 40-43
    u32be profileFlags;             // Bytes 44-47
    u32be deviceManufacturer;       // Bytes 48-51
    u32be deviceModel;              // Bytes 52-55

    u64be deviceAttributes; // Bytes 56-63 (8 bytes)
    u32be renderingIntent;  // Bytes 64-67
    XYZNumber illuminant;   // Bytes 68-79 (12 bytes - PCS Illuminant)

    u32be profileCreatorSignature; // Bytes 80-83

    Array<u8, 16> profileID; // Bytes 84-99 (MD5 Fingerprint)
    Array<u8, 28> reserved;  // Bytes 100-127 (Must be zero)

    Opt<ProfileClass> profileDeviceClass() const {
        return ProfileClass::fromSignature(_profileDeviceClass);
    }

    Opt<ColorSpace> dataColorSpace() const {
        return ColorSpace::fromSignature(_dataColourSpace);
    }

    Opt<ColorSpace> profileConnectionSpace() const {
        auto space = try$(ColorSpace::fromSignature(_pcs));
        // FIXME: Handle special case for DeviceLink profile
        if (space != ColorSpace::XYZ and space != ColorSpace::LAB)
            return NONE;
        return space;
    }

    DateTime creationTime() const {
        return {
            .date = {
                .day = Day{_creationDate.day},
                .month = Month{_creationDate.month},
                .year = Year{_creationDate.year},
            },
            .time = {
                .second = Duration::fromSecs(_creationDate.seconds),
                .minute = static_cast<u8>(_creationDate.minutes),
                .hour = static_cast<u8>(_creationDate.hours),
            }
        };
    }

    /// Does this looks like a icc profile?
    bool sniff() const {
        return profileSize > 128 and
               profileFileSignature == 0x61637370 and
               dataColorSpace() and
               profileConnectionSpace() and
               creationTime().date.year > Year{1990};
    }
};

// 7.3 MARK: Tag Table ---------------------------------------------------------

export struct TagTableEntry {
    Array<char, 4> sig;
    u32be off;
    u32be len;
};

// 10: Mark: Tag Type ----------------------------------------------------------

// 10.15 MARK: MultiLocalizedUnicodeType ---------------------------------------

struct MultiLocalizedUnicodeType : Io::BChunk {
    static constexpr Str SIG = "mluc"s;

    struct Header {
        Array<char, 4> sig;
        u32be _; // reserved
        u32be numberOfRecord;
        u32be recordSize;
    };

    struct Record {
        Array<char, 2> languageCode;
        Array<char, 2> countryCode;
        u32be stringLen;
        u32be stringOff;
    };

    auto iterRecords() const {
        struct Iter {
            Io::BScan scan;
            usize numberOfRecord;
            usize recordSize;

            Opt<Record> next() {
                if (not numberOfRecord or scan.ended())
                    return NONE;

                auto record = scan.peek<Record>();
                scan.skip(recordSize);
                numberOfRecord--;
                return record;
            }
        };

        auto scan = begin();
        auto header = scan.next<Header>();
        return Iter{
            begin().skip(sizeof(Header)),
            header.numberOfRecord,
            header.recordSize
        };
    }

    String lookupStringPreferringEnglish() const {
        _Str<Utf16be> best = {};
        for (auto record : iterRecords()) {
            _Str<Utf16be> str =
                begin()
                    .skip(record.stringOff)
                    .nextBytes(record.stringLen)
                    .cast<Utf16be::Unit>();

            if (not best)
                best = str;
        }
        return transcode(best);
    }

    void repr(Io::Emit& e) const {
        e("multi localised string {:#}", lookupStringPreferringEnglish());
    }
};

// 10.24 MARK: TextType --------------------------------------------------------

struct TextType : Io::BChunk {
    static constexpr Str SIG = "text"s;

    Str text() const {
        auto scan = begin().skip(8);
        return scan.nextCStr(scan.rem());
    }

    void repr(Io::Emit& e) const {
        e("text {:#}", text());
    }
};

struct UnknowType : Io::BChunk {
    void repr(Io::Emit& e) const {
        e("unknow tag {:#}", begin().nextStr(4));
    }
};

using TagType = Union<
    MultiLocalizedUnicodeType,
    TextType,
    UnknowType>;

bool tagTypeMatch(Str query, Str sig) {
    // NOTE: "mluc" can be referred as "desc" in earlier specs.
    if (sig == MultiLocalizedUnicodeType::SIG and query == "desc"s)
        return true;
    return query == sig;
}

TagType tagTypeFrom(Bytes bytes) {
    auto query = Io::BScan{bytes}.nextStr(4);
    TagType result = UnknowType{bytes};
    TagType::any([&]<typename T>() {
        if constexpr (requires { T::SIG; })
            if (tagTypeMatch(query, T::SIG))
                result = T{bytes};
    });
    return result;
}

// MARK: Parser ----------------------------------------------------------------

struct Tag {
    Array<char, 4> sig;
    TagType type;
};

export struct Parser : Io::BChunk {
    Opt<ProfileHeader> _header = NONE;

    ProfileHeader header() {
        if (not _header)
            _header = begin().next<ProfileHeader>();
        return *_header;
    }

    auto iterTag() {
        auto s = begin().skip(sizeof(ProfileHeader));
        auto n = s.next<u32be>();

        struct Iter {
            Bytes data;
            Io::BScan s;
            u32 n;

            Opt<Tag> next() {
                if (n == 0 or s.ended())
                    return NONE;
                auto entry = s.next<TagTableEntry>();
                auto type =
                    Io::BScan{data}
                        .skip(entry.off)
                        .nextBytes(entry.len);
                n--;
                return Tag{
                    entry.sig,
                    tagTypeFrom(type)
                };
            }
        };

        return Iter{bytes(), s, n};
    }
};

} // namespace Karm::Icc
