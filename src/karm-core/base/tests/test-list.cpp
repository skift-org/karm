import Karm.Core;

#include <karm/test>

namespace Karm::Base::Tests {

test$("list-push-and-pop") {
    List<int> list;

    list.pushBack(1);
    list.pushBack(2);
    list.pushBack(3);

    assertEq$(list.len(), 3uz);

    assertEq$(list[0], 1);
    assertEq$(list[1], 2);
    assertEq$(list[2], 3);

    assertEq$(list.popBack(), 3);
    assertEq$(list.popBack(), 2);
    assertEq$(list.popBack(), 1);

    return Ok();
}

test$("list-requeue") {
    List<int> list;

    list.pushBack(1);
    list.pushBack(2);
    list.pushBack(3);

    assertEq$(list.len(), 3uz);

    assertEq$(list[0], 1);
    assertEq$(list[1], 2);
    assertEq$(list[2], 3);

    list.requeue();

    assertEq$(list[0], 2);
    assertEq$(list[1], 3);
    assertEq$(list[2], 1);

    return Ok();
}

test$("list-trunc") {
    List<int> list;

    list.pushBack(1);
    list.pushBack(2);
    list.pushBack(3);

    assertEq$(list.len(), 3uz);

    assertEq$(list[0], 1);
    assertEq$(list[1], 2);
    assertEq$(list[2], 3);

    list.trunc(1);

    assertEq$(list.len(), 1uz);
    assertEq$(list[0], 1);

    return Ok();
}

test$("list-clear") {
    List<int> list;

    list.pushBack(1);
    list.pushBack(2);
    list.pushBack(3);

    assertEq$(list.len(), 3uz);

    assertEq$(list[0], 1);
    assertEq$(list[1], 2);
    assertEq$(list[2], 3);

    list.clear();

    assertEq$(list.len(), 0uz);

    return Ok();
}

test$("list-insert") {
    List<int> list;

    list.pushBack(1);
    list.pushBack(2);
    list.pushBack(3);

    assertEq$(list.len(), 3uz);

    assertEq$(list[0], 1);
    assertEq$(list[1], 2);
    assertEq$(list[2], 3);

    list.insert(1, 4);

    assertEq$(list.len(), 4uz);

    assertEq$(list[0], 1);
    assertEq$(list[1], 4);
    assertEq$(list[2], 2);
    assertEq$(list[3], 3);

    return Ok();
}

test$("list-remove") {
    List<int> list;

    list.pushBack(1);
    list.pushBack(2);
    list.pushBack(3);

    assertEq$(list.len(), 3uz);

    assertEq$(list[0], 1);
    assertEq$(list[1], 2);
    assertEq$(list[2], 3);

    list.remove(1);

    assertEq$(list.len(), 2uz);

    assertEq$(list[0], 2);
    assertEq$(list[1], 3);

    return Ok();
}

test$("list-remove-at") {
    List<int> list;

    list.pushBack(1);
    list.pushBack(2);
    list.pushBack(3);

    assertEq$(list.len(), 3uz);

    assertEq$(list[0], 1);
    assertEq$(list[1], 2);
    assertEq$(list[2], 3);

    list.removeAt(1);

    assertEq$(list.len(), 2uz);

    assertEq$(list[0], 1);
    assertEq$(list[1], 3);

    return Ok();
}

test$("list-iter") {
    List<int> list;

    list.pushBack(1);
    list.pushBack(2);
    list.pushBack(3);

    assertEq$(list.len(), 3uz);

    assertEq$(list[0], 1);
    assertEq$(list[1], 2);
    assertEq$(list[2], 3);

    int i = 0;
    for (auto& el : list.iter()) {
        assertEq$(el, i + 1);
        i++;
    }

    return Ok();
}

test$("list-iter-rev") {
    List<int> list;

    list.pushBack(1);
    list.pushBack(2);
    list.pushBack(3);

    assertEq$(list.len(), 3uz);

    assertEq$(list[0], 1);
    assertEq$(list[1], 2);
    assertEq$(list[2], 3);

    int i = 3;
    for (auto& el : list.iterRev()) {
        assertEq$(el, i);
        i--;
    }

    return Ok();
}

} // namespace Karm::Base::Tests
