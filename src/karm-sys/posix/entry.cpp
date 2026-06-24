#define UNW_LOCAL_ONLY
#include <cxxabi.h>
#include <errno.h>
#include <libunwind.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

import Karm.Sys;
import Karm.Core;
import Karm.Ref;
import Karm.Sys.Posix;

using namespace Karm::Ref::Literals;

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

void __printBacktrace() {
    unw_cursor_t cursor;
    unw_context_t context;

    if (unw_getcontext(&context) != 0 or unw_init_local(&cursor, &context) != 0) {
        fprintf(stderr, "backtrace: Failed to initialize libunwind.\n");
        return;
    }

    fprintf(stderr, "backtrace:\n");

    unw_step(&cursor);

    int i = 0;
    while (unw_step(&cursor) > 0) {
        unw_word_t ip;
        unw_word_t offset;
        char mangled[256];

        unw_get_reg(&cursor, UNW_REG_IP, &ip);

        if (unw_get_proc_name(&cursor, mangled, sizeof(mangled), &offset) == 0) {
            int status = 0;
            char* demangled = abi::__cxa_demangle(mangled, nullptr, nullptr, &status);
            fprintf(stderr, "#%d %p in %s+0x%lx\n", i, reinterpret_cast<void*>(ip), (status == 0) ? demangled : mangled, offset);
            if (demangled)
                free(demangled);
        } else {
            fprintf(stderr, "#%d %p in <unknown>\n", i, reinterpret_cast<void*>(ip));
        }

        i++;
    }
}

void __panicHandler(Karm::PanicKind kind, char const* msg, Karm::usize len) {
    fprintf(stderr, "%s: %.*s\n", kind == Karm::PanicKind::PANIC ? "panic" : "debug", (int)len, msg);

    if (kind == Karm::PanicKind::PANIC) {
        __printBacktrace();
        abort();
        __builtin_unreachable();
    }
}
