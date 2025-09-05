#pragma once

import Karm.Ref;

namespace Karm::Sys {

struct Bundle {
    String id;

    static Res<Bundle> current();

    static Res<Vec<Bundle>> installed();

    Ref::Url url() {
        Ref::Url url;
        url.scheme = "bundle"s;
        url.host = id;
        return url;
    }
};

} // namespace Karm::Sys
