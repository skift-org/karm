module;

#include <karm/macros>

export module Karm.Sys:pipe;

import :file;

namespace Karm::Sys {

export struct Pipe {
    File in;
    File out;

    static Res<Pipe> create() {
        try$(ensureUnrestricted());
        auto [in, out] = try$(_Embed::createPipe());
        return Ok(Pipe{
            File{in, "pipe:"_url},
            File{out, "pipe:"_url},
        });
    }
};

} // namespace Karm::Sys

