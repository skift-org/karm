module;

#include <sys/random.h>

module Karm.Crypto;

import Karm.Sys.Posix;

namespace Karm::Crypto::_Embed {
    Res<> entropy(MutBytes out) {
        if (getentropy(out.buf(), out.len()) == -1) {
            return Posix::fromLastErrno();
        }

        return Ok();
    }
} // namespace Karm::Crypto::_Embed