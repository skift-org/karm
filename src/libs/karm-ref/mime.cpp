export module Karm.Ref:mime;

import Karm.Core;

namespace Karm::Ref {

export struct Mime {
    String _buf;

    Mime(Str buf)
        : _buf(buf) {}

    Str type() const {
        Io::SScan s(_buf);

        s.begin();
        s.skip(Re::until('/'_re));
        return s.end();
    }

    Str subtype() const {
        Io::SScan s(_buf);

        s.skip(type());
        s.skip('/');

        s.begin();
        s.skip(Re::until(';'_re));
        return s.end();
    }

    bool is(Mime const& other) const {
        return type() == other.type() and
               subtype() == other.subtype();
    }

    Str str() const {
        return _buf;
    }

    void repr(Io::Emit& e) const {
        e("{}", str());
    }

    bool operator==(Mime const&) const = default;
};

} // namespace Karm::Ref

export auto operator""_mime(char const* buf, Karm::usize len) {
    return Karm::Ref::Mime(Karm::Str{buf, len});
}
