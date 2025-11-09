export module Karm.Fs:mount;

import Karm.Core;
import :node;
import :bootfs;

namespace Karm::Fs {

export struct FsTab {
    enum struct Type {
        AUTO,
        BOOTFS,
    };

    Ref::Url device;
    Ref::Path path;
    Type type;
    Serde::Object options;
};

export [[clang::coro_wrapper]]
Async::Task<Rc<Node>> mountAsync(Ref::Url url) {
    return mountBootfsAsync(url);
}

export Async::Task<Rc<Node>> mountAsync(Vec<FsTab> tab);

} // namespace Karm::Fs