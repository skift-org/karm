#include <stdio.h>
#include <stdlib.h>

import Karm.Sys;
import Karm.Core;

void __panicHandler(Karm::PanicKind kind, char const* msg, Karm::usize len) {
    fprintf(stderr, "%s: %.*s\n", kind == Karm::PanicKind::PANIC ? "panic" : "debug", (int)len, msg);

    if (kind == Karm::PanicKind::PANIC) {
        abort();
        __builtin_unreachable();
    }
}
