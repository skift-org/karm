#include <karm/test>

import Karm.Ref;

using namespace Karm::Literals;
using namespace Karm::Ref::Literals;

namespace Karm::Ref::Tests {

test$("karm-ref-mime-parse") {
    auto mime = "text/plain"_mime;
    assertEq$(mime.type(), "text"s);
    assertEq$(mime.subtype(), "plain"s);
    assertEq$(mime.str(), "text/plain"s);
    return Ok();
}

} // namespace Karm::Ref::Tests
