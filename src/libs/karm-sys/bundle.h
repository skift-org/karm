#pragma once

#include <karm-mime/url.h>

namespace Karm::Sys {

struct Bundle {
    String id;

    static Res<Bundle> current();
    
    static Res<Vec<Bundle>> installed();

    Mime::Url url() {
        Mime::Url url;
        url.scheme = "bundle"s;
        url.host = id;
        return url;
    }
};


} // namespace Karm::Pkg
