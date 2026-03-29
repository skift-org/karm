#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

import Karm.Sys;
import Karm.Core;
import Karm.Ref;
import Karm.Sys.Posix;

Karm::Res<Karm::Ref::Url> __pwd() {
    auto buf = Karm::Buf<char>::init(256);
    while (true) {
        if (::getcwd(buf.buf(), buf.len()) != NULL)
            break;

        if (errno != ERANGE)
            return Karm::Posix::fromLastErrno();

        buf.resize(buf.len() * 2);
    }

    return Karm::Ok(Karm::Ref::parseUrlOrPath(Karm::Str::fromNullterminated(buf.buf()), "file:"_url));
}

void __panicHandler(Karm::PanicKind kind, char const* msg, Karm::usize len) {
    fprintf(stderr, "%s: %.*s\n", kind == Karm::PanicKind::PANIC ? "panic" : "debug", (int)len, msg);

    if (kind == Karm::PanicKind::PANIC) {
        abort();
        __builtin_unreachable();
    }
}
