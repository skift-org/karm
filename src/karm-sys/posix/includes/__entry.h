#pragma once

import Karm.Core;
import Karm.Sys;

void __panicHandler(Karm::PanicKind kind, char const* msg, Karm::usize len);
Karm::Res<Karm::Ref::Url> __pwd();

struct PosixEnv : Karm::Sys::Env {
    Karm::Sys::Argv _argv;
    Karm::Sys::Envp _envp;
    Karm::Ref::Url _cwd;

    PosixEnv(int argc, char const** argv, char** envp, Karm::Ref::Url cwd)
        : _argv(argc, argv), _envp(envp), _cwd(cwd) {}

    Karm::Sys::Args const& args() const override {
        return _argv;
    }

    Karm::Ref::Url cwd() const override {
        return _cwd;
    }

    Karm::Sys::Vars const& vars() const override {
        return _envp;
    }
};

int main(int argc, char const** argv, char** envp) {
    Karm::registerPanicHandler(__panicHandler);

    PosixEnv env{argc, argv, envp, __pwd().unwrap()};
    Karm::Async::Cancellation cancellation;
    Karm::Res<> code = Karm::Sys::run(entryPointAsync(env, cancellation.token()));
    cancellation.cancel();

    if (not code) {
        Karm::Sys::errln("{}: {}", argv[0], code);
        return 1;
    }
    return 0;
}
