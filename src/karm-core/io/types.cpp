export module Karm.Core:io.types;

import :base.base;
import :base.res;

namespace Karm::Io {

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
