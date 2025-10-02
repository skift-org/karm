export module Karm.Debug;

import Karm.Core;

namespace Karm::Debug {

// MARK: Debug Flag ------------------------------------------------------------

export struct Flag;

static Flag* _firstFlag = nullptr;

export struct Flag : Meta::Pinned {
    Str name;
    bool enabled = false;
    Flag* next = nullptr;

    Flag(Str name, bool enabled = false) : name(name), enabled(enabled) {
        if (_firstFlag)
            next = _firstFlag;
        _firstFlag = this;
    }

    operator bool() const {
        return enabled;
    };
};

export Res<> enable(Str flag) {
    bool ok = false;
    for (auto* f = _firstFlag; f; f = f->next) {
        if (Glob::matchGlob(flag, f->name)) {
            f->enabled = true;
            ok = true;
        }
    }
    if (not ok)
        return Error::invalidInput("no such flag");
    return Ok();
}

export Vec<Str> flags() {
    Vec<Str> res;
    for (auto* f = _firstFlag; f; f = f->next) {
        res.pushBack(f->name);
    }
    return res;
}

// MARK: Feature ---------------------------------------------------------------

export struct Feature;

static Feature* _firstFeature = nullptr;

export struct Feature : Meta::Pinned {
    Str name;
    bool enabled = false;
    Feature* next = nullptr;

    Feature(Str name, bool enabled = false) : name(name), enabled(enabled) {
        if (_firstFeature)
            next = _firstFeature;
        _firstFeature = this;
    }

    operator bool() const {
        return enabled;
    };
};

export Res<> enableFeature(Str feature, bool enabled) {
    bool ok = false;
    for (auto* f = _firstFeature; f; f = f->next) {
        if (Glob::matchGlob(feature, f->name)) {
            f->enabled = enabled;
            ok = true;
        }
    }
    if (not ok)
        return Error::invalidInput("no such feature");
    return Ok();
}

export Res<> enableAllFeatures() {
    for (auto* f = _firstFeature; f; f = f->next) {
        f->enabled = true;
    }
    return Ok();
}

export Vec<Str> features() {
    Vec<Str> res;
    for (auto* f = _firstFeature; f; f = f->next) {
        res.pushBack(f->name);
    }
    return res;
}

} // namespace Karm::Debug
