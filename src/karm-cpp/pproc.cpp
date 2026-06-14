export module Karm.Cpp:pproc;

import Karm.Core;

namespace Karm::Cpp {

// https://eel.is/c++draft/#cpp
struct PpToken {
    enum struct Kind {
    };

    using enum Kind;

    Kind kind;
    Str data;
};

struct Macro {
};

} // namespace Karm::Cpp