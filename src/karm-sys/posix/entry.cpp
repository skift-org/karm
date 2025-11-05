#include <stdio.h>
#include <stdlib.h>

import Karm.Sys;
import Karm.Core;

void __panicHandler(Karm::PanicKind kind, char const* msg) {
    fprintf(stderr, "%s: %s\n", kind == Karm::PanicKind::PANIC ? "panic" : "debug", msg);

    if (kind == Karm::PanicKind::PANIC) {
        abort();
        __builtin_unreachable();
    }
}
