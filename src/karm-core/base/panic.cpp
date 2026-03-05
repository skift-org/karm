export module Karm.Core:base.panic;

import :base.base;
import :base.cstr;

namespace Karm {

export enum class PanicKind {
    DEBUG,
    PANIC,
};

using PanicHandler = void (*)(PanicKind kind, char const* msg, usize len);

static PanicHandler panicHandler = nullptr;

export void registerPanicHandler(PanicHandler handler) {
    panicHandler = handler;
}

void _panic(PanicKind kind, char const* msg, usize len) {
    if (panicHandler)
        panicHandler(kind, msg, len);
    else
        // Oh no! We don't have a panic handler!
        // Let's just crash the program.
        __builtin_trap();
}

export void debug(char const* msg, usize len) {
    _panic(PanicKind::DEBUG, msg, len);
}

export void debug(char const* msg) {
    debug(msg, cstrLen(msg));
}

export [[noreturn]] void panic(char const* msg, usize len) {
    _panic(PanicKind::PANIC, msg, len);
    __builtin_unreachable();
}

export [[noreturn]] void panic(char const* msg) {
    panic(msg, cstrLen(msg));
}

export [[noreturn]] void notImplemented() {
    panic("not implemented", 15);
}

export [[noreturn]] void unreachable() {
    panic("unreachable", 11);
}

export void breakpoint() {
#ifdef __ck_debug__
#    ifdef __clang__
    __builtin_debugtrap();
#    elif defined(__GNUC__)
    __builtin_trap(); // TODO: use __builtin_debugtrap() when it's available
#    elif defined(_MSC_VER)
    __debugbreak();
#    else
#        error "Unsupported compiler"
#    endif
#endif
}

} // namespace Karm
