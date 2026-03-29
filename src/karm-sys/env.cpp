module;

export module Karm.Sys:env;

import Karm.Core;
import Karm.Ref;

namespace Karm::Sys {

export struct Vars {
    virtual ~Vars() = default;
    virtual Opt<Str> get(Str key) const = 0;
    virtual bool has(Str arg) const = 0;
    virtual Yield<Pair<Str>> iter() const = 0;
};

export struct Envp : Vars {
    char const* const* _envp;

    Envp(char const* const* envp)
        : _envp(envp) {}

    Opt<Str> get(Str name) const override {
        for (auto [k, v] : iter())
            if (k == name)
                return v;
        return NONE;
    }

    bool has(Str name) const override {
        return get(name) != NONE;
    }

    Yield<Pair<Str>> iter() const override {
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
};

export struct Args {
    virtual ~Args() = default;

    virtual Str self() const = 0;
    virtual usize len() const = 0;
    virtual Str operator[](usize i) const = 0;
    virtual bool has(Str arg) const = 0;
};

export struct Argv : Args {
    usize _argc;
    char const** _argv;

    Argv(usize argc, char const** argv)
        : _argc(argc), _argv(argv) {}

    Str self() const override {
        return _argv[0];
    }

    usize len() const override {
        return _argc - 1;
    }

    Str operator[](usize i) const override {
        if (i >= len())
            panic("index out of bounds");
        return _argv[i + 1];
    }

    bool has(Str arg) const override {
        for (usize i = 0; i < len(); ++i)
            if (operator[](i) == arg)
                return true;
        return false;
    }
};

export struct Env;

static Env* _globalEnv = nullptr;

export struct Env {
    Env() {
        if (not _globalEnv)
            _globalEnv = this;
    }

    virtual ~Env() = default;

    virtual Vars const& vars() const = 0;
    virtual Args const& args() const = 0;
    virtual Ref::Url cwd() const = 0;
};

export Env const& globalEnv() {
    return *_globalEnv;
}

} // namespace Karm::Sys