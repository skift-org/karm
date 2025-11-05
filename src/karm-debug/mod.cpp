export module Karm.Debug;

import Karm.Core;

namespace Karm::Debug {

// MARK: Debug Flag ------------------------------------------------------------

export struct Flag;

static Flag* _firstFlag = nullptr;

export enum struct FlagType {
    DEBUG,
    FEATURE,
    ALL,
};

export using enum FlagType;

export struct Flag : Meta::Pinned {
    Str name;
    Str description;
    bool enabled = false;
    FlagType type = FlagType::DEBUG;

    Flag* next = nullptr;

    static Flag debug(Str name, Str description, bool enabled = false) {
        return {name, description, enabled, FlagType::DEBUG};
    }

    static Flag feature(Str name, Str description, bool enabled = false) {
        return {name, description, enabled, FlagType::FEATURE};
    }

    Flag(Str name, Str description, bool enabled = false, FlagType type = FlagType::DEBUG)
        : name(name), description(description), enabled(enabled), type(type) {
        if (_firstFlag)
            next = _firstFlag;
        _firstFlag = this;
    }

    operator bool() const {
        return enabled;
    };
};

export Res<> toggleFlag(FlagType type, Str flag, bool enabled) {
    bool ok = false;
    for (auto* f = _firstFlag; f; f = f->next) {
        if (f->type == type and Glob::matchGlob(flag, f->name)) {
            f->enabled = enabled;
            ok = true;
        }
    }
    if (not ok)
        return Error::invalidInput("no such flag");
    return Ok();
}

export Flag const* flags() {
    return _firstFlag;
}

} // namespace Karm::Debug
