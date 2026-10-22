export module Karm.Icu:base;

import Karm.Core;

namespace Karm::Icu {

export constexpr Rune NULL = '\0';
export constexpr Rune ZWJ = 0x200D; // zero-width joiner
export constexpr Rune REPLACEMENT = U'�';

export template <StaticEncoding E>
struct RuneIter {
    using Encoding = E;
    using Unit = E::Unit;

    Cursor<Unit> _cursor;
    Cursor<Unit> _begin = _cursor;

    bool startOfText() const {
        return _cursor == _begin;
    }

    bool endOfText() const {
        return _cursor.ended();
    }

    usize pos() const {
        return _begin.rem() - _cursor.rem();
    }

    Cursor<Unit> cursor() {
        return _cursor;
    }

    Rune peek() {
        if (endOfText())
            return NULL;
        Rune r;
        auto curr = _cursor;
        return E::decodeUnit(r, curr) ? r : REPLACEMENT;
    }

    Rune peek(usize count) {
        auto saved = _cursor;
        next(count);
        auto r = peek();
        _cursor = saved;
        return r;
    }

    Rune next() {
        if (endOfText())
            return NULL;
        Rune r;
        if (E::decodeUnit(r, _cursor))
            return r;
        return REPLACEMENT;
    }

    Rune next(usize count) {
        Rune r = '\0';
        for (usize i = 0; i < count; i++)
            r = next();
        return r;
    }
};

} // namespace Karm::Icu
