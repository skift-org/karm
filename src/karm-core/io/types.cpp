export module Karm.Core:io.types;

import :base.base;
import :base.res;
import :base.size;

using namespace Karm::Literals;

namespace Karm::Io {

export constexpr usize DEFAULT_BUFFER_SIZE = 8_KiB;

#ifdef __ck_sys_encoding_utf8__
export using Encoding = Utf8;
#elifdef __ck_sys_encoding_utf16__
export using Encoding = Utf16;
#elifdef __ck_sys_encoding_utf32__
export using Encoding = Utf32;
#elifdef __ck_sys_encoding_ascii__
export using Encoding = Ascii;
#else
#    error "Unknown encoding"
#endif

#ifdef __ck_sys_line_ending_lf__
export constexpr Str LINE_ENDING = "\n";
#elifdef __ck_sys_line_ending_crlf__
export constexpr Str LINE_ENDING = "\r\n";
#else
#    error "Unknown line ending"
#endif

#ifdef __ck_sys_path_separator_slash__
export constexpr Str PATH_SEPARATOR = "/";
#elifdef __ck_sys_path_separator_backslash__
export constexpr Str PATH_SEPARATOR = "\\";
#else
#    error "Unknown path separator"
#endif


export enum struct Whence {
    BEGIN,
    CURRENT,
    END,
};

export struct Seek {
    Whence whence;
    isize offset;

    static constexpr Seek fromBegin(isize offset) {
        return Seek{Whence::BEGIN, offset};
    }

    static constexpr Seek fromCurrent(isize offset) {
        return Seek{Whence::CURRENT, offset};
    }

    static constexpr Seek fromEnd(isize offset) {
        return Seek{Whence::END, offset};
    }

    Seek(Whence whence, isize offset = 0)
        : whence(whence), offset(offset) {}

    constexpr Res<usize> apply(usize current, usize size) const {
        switch (whence) {
        case Whence::BEGIN: {
            if (offset < 0 or static_cast<usize>(offset) > size)
                return Error::invalidInput("seek before begin");
            return Ok(static_cast<usize>(offset));
        }

        case Whence::CURRENT: {
            isize target = static_cast<isize>(current) + offset;
            if (target < 0 or static_cast<usize>(target) > size)
                return Error::invalidInput("seek out of range");
            return Ok(static_cast<usize>(target));
        }

        case Whence::END: {
            isize target = static_cast<isize>(size) - offset;
            if (target < 0 or static_cast<usize>(target) > size)
                return Error::invalidData("seek out of range");
            return Ok(static_cast<usize>(target));
        }
        }
        return Error::invalidData("unknown whence");
    }

    bool operator==(Whence const& other) const {
        return whence == other;
    }
};

} // namespace Karm::Io
