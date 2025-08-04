#include <karm-core/macros.h>

#include "bundle.h"
#include "_embed.h"

namespace Karm::Sys {

Res<Bundle> Bundle::current() {
    auto id = try$(_Embed::currentBundle());
    return Ok(Bundle{id});
}

Res<Vec<Bundle>> Bundle::installed() {
    Vec<Bundle> bundles;
    auto ids = try$(_Embed::installedBundles());
    for (auto& id : ids) {
        bundles.pushBack({id});
    }
    return Ok(bundles);
}

} // namespace Karm::Sys
