#include <karm/test>

import Karm.Core;

using namespace Karm::Literals;

namespace Karm::Io::Tests {

test$("sscan-ended") {
    SScan s{""s};
    assert$(s.ended());

    s = SScan{"a"s};
    assert$(not s.ended());

    return Ok();
}

test$("sscan-rem") {
    SScan s{"abc"};
    assert$(s.rem() == 3);

    s = SScan{"abc"};
    s.next();
    assert$(s.rem() == 2);

    s = SScan{"abc"};
    s.next(3);
    assert$(s.rem() == 0);

    return Ok();
}

test$("sscan-rem-str") {
    SScan s{"abc"};
    assert$(s.remStr() == "abc");

    s = SScan{"abc"};
    s.next();
    assert$(s.remStr() == "bc");

    s = SScan{"abc"};
    s.next(3);
    assert$(s.remStr() == "");

    return Ok();
}

test$("sscan-curr") {
    SScan s{"abc"};
    assert$(s.peek() == 'a');

    s = SScan{"abc"};
    s.next();
    assert$(s.peek() == 'b');

    s = SScan{"abc"};
    s.next(3);
    assert$(s.peek() == '\0');

    return Ok();
}

test$("sscan-peek") {
    SScan s{"abc"};

    assert$(s.peek() == 'a');
    assert$(s.peek(1) == 'b');
    assert$(s.peek(2) == 'c');
    assert$(s.peek(3) == '\0');
    assert$(s.peek(4) == '\0');

    return Ok();
}

test$("sscan-next") {
    SScan s{"abc"};

    assert$(s.next() == 'a');
    assert$(s.next() == 'b');
    assert$(s.next() == 'c');
    assert$(s.next() == '\0');
    assert$(s.next() == '\0');

    return Ok();
}

test$("sscan-skip") {
    SScan s{"abc"};

    assert$(s.skip('a'));
    assert$(s.rem() == 2);
    assert$(s.skip('b'));
    assert$(s.rem() == 1);
    assert$(s.skip('c'));
    assert$(s.rem() == 0);

    assert$(not s.skip('d'));
    assert$(s.rem() == 0);

    s = SScan{"abc"};
    assert$(s.skip("ab"));
    assert$(s.rem() == 1);
    assert$(s.skip("c"));
    assert$(s.rem() == 0);
    assert$(not s.skip("d"));
    assert$(s.rem() == 0);

    return Ok();
}

test$("sscan-eat") {
    SScan s{"abc"};

    assert$(s.eat('a'));
    assert$(s.eat('b'));
    assert$(s.eat('c'));
    assert$(not s.eat('d'));

    s = SScan{"abc"};
    assert$(s.eat("ab"));
    assert$(s.eat("c"));
    assert$(not s.eat("d"));

    s = SScan{"aaaaaa"};
    assert$(s.eat('a'));
    assert$(s.ended());

    return Ok();
}

test$("sscan-ahead") {
    SScan s{"abc"};

    assert$(s.ahead('a'));
    assertNot$(s.ahead('b'));
    assert$(s.rem() == 3);

    assert$(s.ahead("ab"));
    assertNot$(s.ahead("bc"));
    assert$(s.rem() == 3);

    return Ok();
}

} // namespace Karm::Io::Tests
