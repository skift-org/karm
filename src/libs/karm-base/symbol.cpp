#include "symbol.h"
#include "set.h"

namespace Karm {

static Set<Rc<_SymbolBuf>>& _symboleRegistry() {
    static Set<Rc<_SymbolBuf>> _registry;
    return _registry;
}

Symbol Symbol::from(Str str) {
    auto& registry = _symboleRegistry();
    registry.ensureForInsert();
    auto* slot = registry.lookup(str);
    if (slot and slot->state == Set<Rc<_SymbolBuf>>::State::USED) {
        return {slot->unwrap()};
    }

    auto buf = _SymbolBuf::from(str);
    registry.put(buf);
    return {buf};
}

} // namespace Karm
