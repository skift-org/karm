module;

#include <karm/macros>

export module Karm.Core:base.symbol;

import :base.rc;
import :base.set;
import :base.hash;
import :base.string;

namespace Karm {

/// A symbol is a unique string that is interned in a global registry.
/// It is used to represent identifiers in a way that allows for fast comparisons and lookups.
/// Symbols are immutable and can be compared by pointer equality.
/// They are typically used for identifiers in languages, such as HTML tags, attributes, and other
/// names that are used frequently and need to be compared often.
export struct Symbol {
    Str _str;

    static Symbol from(Str str);

    Str str() const {
        return _str;
    }

    void hash(Meta::Derive<Hasher> auto& h) const {
        Karm::hash(h, _str);
    }

    bool operator==(Symbol const& other) const {
        return _str.buf() == other._str.buf();
    }

    bool operator==(Str const& other) const {
        return _str == other;
    }

    auto operator<=>(Symbol const& other) const {
        return str() <=> other.str();
    }

    explicit operator bool() const {
        return _str.len() > 0;
    }
};

static Set<String>& _symboleRegistry() {
    static Set<String> _registry;
    return _registry;
}

Symbol Symbol::from(Str str) {
    return {_symboleRegistry().lookupOrAdd(str, [&] -> String {
        return str;
    })};
}

export template <>
struct Niche<Symbol> : Niche<Str> {
};

} // namespace Karm

namespace Karm::Literals {

export constexpr Karm::Symbol operator""_sym(char const* buf, Karm::usize len) {
    return Karm::Symbol::from({buf, len});
}

export constexpr Karm::Symbol operator""_sym(char const* buf) {
    return Karm::Symbol::from(buf);
}

} // namespace Karm::Literals
