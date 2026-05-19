#pragma once

import Karm.Core;
import Karm.Sys;

void __panicHandler(Karm::PanicKind kind, char const* msg, Karm::usize len);
Karm::Res<Karm::Ref::Url> __pwd();

int main(int argc, char const** argv, char** envp) {
    Karm::registerPanicHandler(__panicHandler);

    Karm::Sys::Env env{(Karm::usize)argc, argv, envp, __pwd().unwrap()};
    Karm::Async::Cancellation cancellation;
    Karm::Res<> code = Karm::Sys::run(entryPointAsync(env, cancellation.token()));
    cancellation.cancel();

    if (not code) {
        Karm::Sys::errln("{}: {}", argv[0], code);
        return 1;
    }
    return 0;
}
