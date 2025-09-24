export module Karm.Sys:time;

import Karm.Core;

import :_embed;

namespace Karm::Sys {

export SystemTime now() {
    return _Embed::now();
}

export Instant instant() {
    return _Embed::instant();
}

export Duration uptime() {
    return _Embed::uptime();
}

export DateTime dateTime() {
    return DateTime::fromInstant(now());
}

export Date date() {
    return dateTime().date;
}

export Time time() {
    return dateTime().time;
}

} // namespace Karm::Sys
