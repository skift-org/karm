module;

#include <karm-core/macros.h>

export module Karm.Sys:bundle;

import Karm.Ref;

import :_embed;

namespace Karm::Sys {

export struct Bundle {
    String id;

    static Res<Bundle> current() {
        auto id = try$(_Embed::currentBundle());
        return Ok(Bundle{id});
    }

    static Res<Vec<Bundle>> installed() {
        Vec<Bundle> bundles;
        auto ids = try$(_Embed::installedBundles());
        for (auto& id : ids) {
            bundles.pushBack({id});
        }
        return Ok(bundles);
    }

    Ref::Url url() const {
        Ref::Url url;
        url.scheme = "bundle"_sym;
        url.host = Symbol::from(id);
        return url;
    }
};

} // namespace Karm::Sys
