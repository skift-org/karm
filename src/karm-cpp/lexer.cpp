export module Karm.Cpp:lexer;

import Karm.Core;

namespace Karm::Cpp {

struct Token {
    enum struct Kind {
    };

    using enum Kind;

    Kind kind;
};

} // namespace Karm::Cpp