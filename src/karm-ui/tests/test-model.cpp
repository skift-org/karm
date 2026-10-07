#include <karm/test>

import Karm.Ui;

namespace Karm::Ui::Tests {

test$("model-moves") {
    TextModel mdl{"foo bar baz"};

    mdl.moveStart();
    assertEq$(mdl._cur.head, 0uz);

    mdl.moveNext();
    assertEq$(mdl._cur.head, 1uz);

    mdl.movePrev();
    assertEq$(mdl._cur.head, 0uz);

    mdl.moveEnd();
    assertEq$(mdl._cur.head, 11uz);

    mdl.moveStart();
    assertEq$(mdl._cur.head, 0uz);

    return Ok();
}

} // namespace Karm::Ui::Tests
