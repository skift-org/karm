module;

#include <karm/macros>

export module Karm.Sys:launch;

import Karm.Core;
import Karm.Ref;

import :async;

namespace Karm::Sys {

export struct Object {
    Ref::Url url;
    Opt<Ref::Uti> type = NONE;

    Object(Ref::Url url) : url(url) {}

    Object(Ref::Url url, Ref::Uti type) : url(url), type(type) {}
};

export struct Intent {
    Ref::Uti action;
    Vec<Object> objects = {};
    Opt<Ref::Url> handler = NONE;
    Opt<Ref::Url> callback = NONE;
};

export Res<> launch(Intent intent) {
    try$(ensureUnrestricted());
    return _Embed::launch(intent);
}

export Async::Task<> launchAsync(Intent intent) {
    co_try$(ensureUnrestricted());
    co_return co_await _Embed::launchAsync(intent);
}

} // namespace Karm::Sys
