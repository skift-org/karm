import Karm.Core;

#include <karm/test>

namespace Karm::Base::Tests {

struct CowPayload {
    int x = 0;
};

test$("cow-mutates-in-place-when-unique") {
    Cow<CowPayload> cow = CowPayload{.x = 1};
    auto* before = &cow._inner.unwrap();

    expectEq$(cow._inner.strong(), 1uz);

    cow.cow().x = 2;

    expectEq$(cow->x, 2);
    expectEq$(&cow._inner.unwrap(), before);

    return Ok();
}

test$("cow-deep-copies-when-shared") {
    Cow<CowPayload> cow = CowPayload{.x = 1};
    Rc<CowPayload> alias = cow._inner;

    expectEq$(cow._inner.strong(), 2uz);

    cow.cow().x = 2;

    expectEq$(cow._inner.strong(), 1uz);
    expectEq$(cow->x, 2);
    expectEq$(alias->x, 1);

    return Ok();
}

} // namespace Karm::Base::Tests
