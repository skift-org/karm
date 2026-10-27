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

    void repr(Io::Emit& e) const {
        e("(object url:{} type:{})", url, type);
    }
};

export struct Intent {
    Ref::Uti action = Ref::Uti::PUBLIC_OPEN;
    Vec<Object> objects = {};
    Opt<Ref::Url> handler = NONE;
    Opt<Ref::Url> callback = NONE;

    void repr(Io::Emit& e) const {
        e("(intent action:{} objects:{} handler:{} callback:{})", action, objects, handler, callback);
    }
};

export Res<> launch(Intent intent) {
    return _Embed::launch(intent);
}

export Async::Task<> launchAsync(Intent intent) {
    co_return co_await _Embed::launchAsync(intent);
}

} // namespace Karm::Sys
