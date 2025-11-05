module;

#include <fcntl.h>
#include <unistd.h>

#include <karm-sys/posix/utils.h>

module Karm.Crypto;

namespace Karm::Crypto::_Embed {

Res<> entropy(MutBytes out) {
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0)
        return Posix::fromLastErrno();

    ssize_t n = read(fd, out.buf(), out.len());
    close(fd);

    if (n != static_cast<ssize_t>(out.len()))
        return Posix::fromLastErrno();

    return Ok();
}

} // namespace Karm::Crypto::_Embed
