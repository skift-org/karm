export module Karm.Core:io.sscan;

import :base.string;
import :base.slice;
import :base.cursor;

namespace Karm {

namespace Io {

/// Source location for tracking position in text.
export struct Loc {
    usize line = 1;   ///< 1-based line number
    usize col = 1;    ///< 1-based column number
    usize offset = 0; ///< 0-based byte offset

    bool operator==(Loc const&) const = default;
    auto operator<=>(Loc const&) const = default;
};

/// A span of source locations.
export struct LocSpan {
    Loc start;
    Loc end;

    static LocSpan single(Loc loc) {
        return LocSpan{loc, loc};
    }

    bool operator==(LocSpan const&) const = default;
};

export template <StaticEncoding E>
struct _SScan;
} // namespace Io

namespace Re {
export template <typename T>
concept Expr = requires(T expr, Io::_SScan<Utf8>& scan) {
    { expr(scan) } -> Meta::Same<bool>;
};

} // namespace Re

} // namespace Karm

namespace Karm::Io {

export template <StaticEncoding E>
struct _SScan {
    using Encoding = E;
    using Unit = typename E::Unit;

    Cursor<Unit> _cursor;
    Cursor<Unit> _begin;
    Loc _loc;

    _SScan(_Str<E> str)
        : _cursor(str), _begin(), _loc() {}

    /// Get the current source location.
    Loc loc() const {
        return _loc;
    }

    /// Check if the scanner has reached the end of the input.
    /// This is equivalent to `rem() == 0`.
    bool ended() const {
        return _cursor.ended();
    }

    /// Returns the number of runes remaining in the input.
    usize rem() {
        auto curr = _cursor;
        return transcodeLen<E>(curr);
    }

    /// Returns the remaining input as a string.
    _Str<E> remStr() {
        return {
            Karm::begin(_cursor),
            Karm::end(_cursor),
        };
    }

    /// Returns the current rune.
    Rune peek() {
        if (ended())
            return '\0';
        Rune r;
        auto curr = _cursor;
        return E::decodeUnit(r, curr) ? r : U'�';
    }

    /// Peek the next rune without advancing the cursor.
    Rune peek(usize count) {
        auto saveCursor = _cursor;
        auto saveLoc = _loc;
        next(count);
        auto r = peek();
        _cursor = saveCursor;
        _loc = saveLoc;
        return r;
    }

    /// Return the current rune and advance the cursor.
    Rune next() {
        if (ended())
            return '\0';
        Rune r;
        if (E::decodeUnit(r, _cursor)) {
            _loc.offset++;
            if (r == '\n') {
                _loc.line++;
                _loc.col = 1;
            } else {
                _loc.col++;
            }
            return r;
        }
        return U'�';
    }

    /// Advance the cursor by `count` runes.
    Rune next(usize count) {
        Rune r = '\0';
        for (usize i = 0; i < count; i++)
            r = next();
        return r;
    }

    auto& begin() {
        _begin = _cursor;
        return *this;
    }

    _Str<E> end() {
        return {_begin, _cursor};
    }

    _Str<E> slice(usize n) {
        auto begin = _cursor;
        next(n);
        return {begin, _cursor};
    }

    /// If the current rune is `c`, advance the cursor.
    bool skip(Rune c) {
        if (peek() == c) {
            next();
            return true;
        }
        return false;
    }

    /// If the current runes are `str`, advance the cursor.
    bool skip(Str str) {
        auto rollback = rollbackPoint();
        for (auto r : iterRunes(str))
            if (next() != r)
                return false;
        rollback.disarm();
        return true;
    }

    /// If the expression matches, advance the cursor.
    bool skip(Re::Expr auto expr)
        requires Meta::Callable<decltype(expr), decltype(*this)>
    {
        auto rollback = rollbackPoint();
        if (not expr(*this))
            return false;
        rollback.disarm();
        return true;
    }

    /// Keep advancing the cursor while the current rune is `c`.
    bool eat(Rune c) {
        bool result = false;
        if (skip(c)) {
            result = true;
            while (skip(c) and not ended())
                ;
        }
        return result;
    }

    /// Keep advancing the cursor while the current runes are `str`.
    bool eat(Str str) {
        bool result = false;
        if (skip(str)) {
            result = true;
            while (skip(str) and not ended())
                ;
        }
        return result;
    }

    /// Keep advancing the cursor while the expression matches.
    bool eat(Re::Expr auto expr) {
        bool result = false;
        if (skip(expr)) {
            result = true;
            while (skip(expr) and not ended())
                ;
        }
        return result;
    }

    /// Check if a rune is ahead or not.
    bool ahead(Rune c) {
        return peek() == c;
    }

    /// Check if a string is ahead or not.
    bool ahead(Str str) {
        auto rollback = rollbackPoint();
        for (auto r : iterRunes(str))
            if (next() != r)
                return false;
        return true;
    }

    /// Check if the expression is ahead or not.
    bool ahead(Re::Expr auto expr) {
        auto rollback = rollbackPoint();
        return expr(*this);
    }

    /// Check if the expression matches or not.
    /// The cursor is restored to the original position.
    Match match(Re::Expr auto expr) {
        auto rollback = rollbackPoint();
        if (expr(*this)) {
            Match result =
                ended()
                    ? Match::YES
                    : Match::PARTIAL;
            return result;
        }
        return Match::NO;
    }

    _Str<E> token(Re::Expr auto expr) {
        _begin = _cursor;
        if (not skip(expr))
            _cursor = _begin;
        return {_begin, _cursor};
    }

    /// Creates a rollback point for the scanner. If not manually disarmed,
    /// the scanner's state will be restored to its position at the time of
    /// this rollback point's creation when it goes out of scope.
    auto rollbackPoint() {
        return ArmedDefer{[&, saved = *this] {
            *this = saved;
        }};
    }
};

export using SScan = _SScan<Utf8>;

} // namespace Karm::Io
