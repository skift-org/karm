module;

#include <errno.h>
#include <sys/random.h>
#include <unistd.h>

module Karm.Crypto;

import Karm.Sys.Posix;

namespace Karm::Crypto::_Embed {

Res<> entropy(MutBytes out) {
    usize total = 0;

    while (total < out.len()) {
        usize n = getrandom(out.buf() + total, out.len() - total, 0);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            return Posix::fromLastErrno();
        }
        total += n;
    }

    return Ok();
}

} // namespace Karm::Crypto::_Embed
