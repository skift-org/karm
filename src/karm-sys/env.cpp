module;

#include <karm/macros>

export module Karm.Sys:env;

import Karm.Core;
import Karm.Ref;

using namespace Karm::Literals;

namespace Karm::Sys {

export struct Env;

static Env* _globalEnv = nullptr;

export struct Env {
    usize _argc;
    char const** _argv;
    char const* const* _envp;
    Ref::Url _cwd;

    Env(usize argc, char const** argv, char const* const* envp, Ref::Url cwd)
        : _argc(argc), _argv(argv), _envp(envp), _cwd(cwd) {
        if (not _globalEnv)
            _globalEnv = this;
    }

    Str self() const {
        return _argv[0];
    }

    usize argsLen() const {
        return _argc - 1;
    }

    Str operator[](usize i) const {
        if (i >= argsLen())
            panic("index out of bounds");
        return _argv[i + 1];
    }

    bool argsHas(Str arg) const {
        for (usize i = 0; i < argsLen(); ++i)
            if (operator[](i) == arg)
                return true;
        return false;
    }

    Opt<Str> getVar(Str name) const {
        for (auto [k, v] : iterVars())
            if (k == name)
                return v;
        return NONE;
    }

    bool hasVar(Str name) const {
        return getVar(name) != NONE;
    }

    Yield<Pair<Str>> iterVars() const {
        for (char const* const* e = _envp; *e != nullptr; e++) {
            auto env = Str::fromNullterminated(*e);
            auto index = indexOf(env, '=');

            if (not index.has()) {
                co_yield Pair<Str>{env, ""s};
                continue;
            }

            co_yield Pair<Str>{
                sub(env, 0, index.unwrap()),
                sub(env, index.unwrap() + 1, env.len())
            };
        }
    }

    Ref::Url cwd() const {
        return _cwd;
    }
};

export Env const& globalEnv() {
    return *_globalEnv;
}

} // namespace Karm::Sys
