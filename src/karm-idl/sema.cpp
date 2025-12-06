module;

#include <karm-core/macros.h>

export module Karm.Idl:sema;

import :ast;

namespace Karm::Idl {

static Res<None, String> _checkDuplicated(Str type, auto const& list) {
    using N = decltype(list[0].name);
    Vec<N> seen;

    for (auto& item : list) {
        if (contains(seen, item.name))
            return Io::format("duplicated {} {}", type, item.name);
        seen.pushBack(item.name);
    }

    return Ok();
}

export Res<None, String> semaCheck(Module const& module) {
    try$(_checkDuplicated("interface", module.interfaces));

    for (auto& interface : module.interfaces) {
        try$(_checkDuplicated("method", interface.methods));
    }

    return Ok();
}

} // namespace Karm::Idl