module;

#include <karm/macros>

export module Karm.Ref:sniff;

import :uti;

namespace Karm::Ref {

// MARK: Sniff Bytes -----------------------------------------------------------

struct MimePattern {
    Io::BPattern pattern;
    Vec<u8> leading;
    Vec<u8> terminating;
    Uti::Common uti;

    bool match(Bytes bytes) const {
        Io::BScan s{bytes};

        while (not s.ended() and contains(leading, s.peekU8le())) {
            s.nextU8le();
        }

        if (pattern.match(s.remBytes()).v0 != Match::YES) {
            return false;
        }

        s.skip(pattern.len());

        if (terminating.len()) {
            if (s.ended() or not contains(terminating, s.peekU8le())) {
                return false;
            }
        }
        return true;
    }
};

bool _isBinaryData(Bytes const bytes) {
    for (u8 byte : bytes) {
        if ((byte <= 0x08) or
            (byte == 0x0B) or
            (byte >= 0x0E and byte <= 0x1A) or
            (byte >= 0x1C and byte <= 0x1F)) {
            return true;
        }
    }
    return false;
}

// https://mimesniff.spec.whatwg.org/#identifying-a-resource-with-an-unknown-mime-type
export Uti sniffBytes(Bytes bytes) {
    static Vec<MimePattern> patterns = {
        {Io::BPattern::from("3C 21 44 4F 43 54 59 50 45 20 48 54 4D 4C", "FF FF DF DF DF DF DF DF DF FF DF DF DF DF"), {' ', '\t', '\n', '\r'}, {' ', '>'}, Uti::PUBLIC_HTML},
        {Io::BPattern::from("3C 48 54 4D 4C", "FF DF DF DF DF"), {' ', '\t', '\n', '\r'}, {' ', '>'}, Uti::PUBLIC_HTML},
        {Io::BPattern::from("3C 48 45 41 44", "FF DF DF DF DF"), {' ', '\t', '\n', '\r'}, {' ', '>'}, Uti::PUBLIC_HTML},
        {Io::BPattern::from("3C 53 43 52 49 50 54", "FF DF DF DF DF DF DF"), {' ', '\t', '\n', '\r'}, {' ', '>'}, Uti::PUBLIC_HTML},
        {Io::BPattern::from("3C 49 46 52 41 4D 45", "FF DF DF DF DF DF DF"), {' ', '\t', '\n', '\r'}, {' ', '>'}, Uti::PUBLIC_HTML},
        {Io::BPattern::from("3C 48 31", "FF DF FF"), {' ', '\t', '\n', '\r'}, {' ', '>'}, Uti::PUBLIC_HTML},
        {Io::BPattern::from("3C 44 49 56", "FF DF DF DF"), {' ', '\t', '\n', '\r'}, {' ', '>'}, Uti::PUBLIC_HTML},
        {Io::BPattern::from("3C 46 4F 4E 54", "FF DF DF DF DF"), {' ', '\t', '\n', '\r'}, {' ', '>'}, Uti::PUBLIC_HTML},
        {Io::BPattern::from("3C 54 41 42 4C 45", "FF DF DF DF DF DF"), {' ', '\t', '\n', '\r'}, {' ', '>'}, Uti::PUBLIC_HTML},
        {Io::BPattern::from("3C 41", "FF DF"), {' ', '\t', '\n', '\r'}, {' ', '>'}, Uti::PUBLIC_HTML},
        {Io::BPattern::from("3C 53 54 59 4C 45", "FF DF DF DF DF DF"), {' ', '\t', '\n', '\r'}, {' ', '>'}, Uti::PUBLIC_HTML},
        {Io::BPattern::from("3C 54 49 54 4C 45", "FF DF DF DF DF DF"), {' ', '\t', '\n', '\r'}, {' ', '>'}, Uti::PUBLIC_HTML},
        {Io::BPattern::from("3C 42", "FF DF"), {' ', '\t', '\n', '\r'}, {' ', '>'}, Uti::PUBLIC_HTML},
        {Io::BPattern::from("3C 42 4F 44 59", "FF DF DF DF DF"), {' ', '\t', '\n', '\r'}, {' ', '>'}, Uti::PUBLIC_HTML},
        {Io::BPattern::from("3C 42 52", "FF DF DF"), {' ', '\t', '\n', '\r'}, {' ', '>'}, Uti::PUBLIC_HTML},
        {Io::BPattern::from("3C 50", "FF DF"), {' ', '\t', '\n', '\r'}, {' ', '>'}, Uti::PUBLIC_HTML},
        {Io::BPattern::from("3C 21 2D 2D", "FF FF FF FF"), {' ', '\t', '\n', '\r'}, {' ', '>'}, Uti::PUBLIC_HTML},
        {Io::BPattern::from("3C 3F 78 6D 6C", "FF FF FF FF FF"), {' ', '\t', '\n', '\r'}, {' ', '>'}, Uti::PUBLIC_XML},
        {Io::BPattern::from("25 50 44 46 2D", "FF FF FF FF FF"), {}, {}, Uti::PUBLIC_PDF},

        {Io::BPattern::from("25 21 50 53 2D 41 64 6F 62 65 2D", "FF FF FF FF FF FF FF FF FF FF FF"), {}, {}, Uti::PUBLIC_POSTSCRIPT},
        {Io::BPattern::from("FE FF 00 00", "FF FF 00 00"), {}, {}, Uti::PUBLIC_TEXT}, // UTF-16BE BOM
        {Io::BPattern::from("FF FE 00 00", "FF FF 00 00"), {}, {}, Uti::PUBLIC_TEXT}, // UTF-16LE BOM
        {Io::BPattern::from("EF BB BF 00", "FF FF FF 00"), {}, {}, Uti::PUBLIC_TEXT}, // UTF-8 BOM

        // https://mimesniff.spec.whatwg.org/#image-type-pattern-matching-algorithm
        {Io::BPattern::from("00 00 01 00", "FF FF FF FF"), {}, {}, Uti::PUBLIC_ICO},                                                              // ICO signature
        {Io::BPattern::from("00 00 02 00", "FF FF FF FF"), {}, {}, Uti::PUBLIC_ICO},                                                              // CUR signature
        {Io::BPattern::from("42 4D", "FF FF"), {}, {}, Uti::PUBLIC_BMP},                                                                          // BMP signature ("BM")
        {Io::BPattern::from("47 49 46 38 37 61", "FF FF FF FF FF FF"), {}, {}, Uti::PUBLIC_GIF},                                                  // GIF87a
        {Io::BPattern::from("47 49 46 38 39 61", "FF FF FF FF FF FF"), {}, {}, Uti::PUBLIC_GIF},                                                  // GIF89a
        {Io::BPattern::from("52 49 46 46 00 00 00 00 57 45 42 50 56 50", "FF FF FF FF 00 00 00 00 FF FF FF FF FF FF"), {}, {}, Uti::PUBLIC_WEBP}, // WEBP ("RIFF....WEBPVP")
        {Io::BPattern::from("89 50 4E 47 0D 0A 1A 0A", "FF FF FF FF FF FF FF FF"), {}, {}, Uti::PUBLIC_PNG},                                      // PNG
        {Io::BPattern::from("FF D8 FF", "FF FF FF"), {}, {}, Uti::PUBLIC_JPEG},

        // https://mimesniff.spec.whatwg.org/#matching-an-archive-type-pattern
        {Io::BPattern::from("1F 8B 08", "FF FF FF"), {}, {}, Uti::PUBLIC_GZ},                          // GZIP signature
        {Io::BPattern::from("50 4B 03 04", "FF FF FF FF"), {}, {}, Uti::PUBLIC_ZIP},                   // ZIP signature "PK\x03\x04"
        {Io::BPattern::from("52 61 72 20 1A 07 00", "FF FF FF FF FF FF FF"), {}, {}, Uti::PUBLIC_RAR}, // RAR signature "Rar \x1A\x07\x00"

        // https://mimesniff.spec.whatwg.org/#matching-a-font-type-pattern
        {
            Io::BPattern::from(
                "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 4C 50",
                "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 FF FF"
            ),
            {},
            {},
            Uti::PUBLIC_EOT,
        },                                                                             // EOT signature
        {Io::BPattern::from("00 01 00 00", "FF FF FF FF"), {}, {}, Uti::PUBLIC_TTF},   // TrueType signature
        {Io::BPattern::from("4F 54 54 4F", "FF FF FF FF"), {}, {}, Uti::PUBLIC_OTF},   // OpenType "OTTO"
        {Io::BPattern::from("74 74 63 66", "FF FF FF FF"), {}, {}, Uti::PUBLIC_TTC},   // TrueType collection "ttcf"
        {Io::BPattern::from("77 4F 46 46", "FF FF FF FF"), {}, {}, Uti::PUBLIC_WOFF},  // WOFF 1.0 "wOFF"
        {Io::BPattern::from("77 4F 46 32", "FF FF FF FF"), {}, {}, Uti::PUBLIC_WOFF2}, // WOFF 2.0 "wOF2"
    };

    for (auto const& p : patterns) {
        if (p.match(bytes))
            return p.uti;
    }

    // 9. If resource’s resource header contains no binary data bytes, return "text/plain".
    if (not _isBinaryData(bytes)) {
        return Uti::PUBLIC_TEXT;
    }

    // 10. Return "application/octet-stream".
    return Uti::PUBLIC_DATA;
}

export Res<Uti> sniffReader(Io::Reader& reader) {
    Array<u8, 1445> header;
    auto len = try$(reader.read(header));
    return Ok(sniffBytes(sub(header, 0, len)));
}

} // namespace Karm::Ref
