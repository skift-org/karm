module;

#include <karm-core/macros.h>

export module Karm.Sys:pipe;

import :file;

namespace Karm::Sys {

export struct Pipe {
    FileWriter in;
    FileReader out;

    static Res<Pipe> create() {
        try$(ensureUnrestricted());
        auto [in, out] = try$(_Embed::createPipe());
        return Ok(Pipe{
            FileWriter{in, "pipe:"_url},
            FileReader{out, "pipe:"_url},
        });
    }
};

} // namespace Karm::Sys
