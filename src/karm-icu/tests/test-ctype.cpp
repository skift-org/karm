#include <karm/test>

import Karm.Core;
import Karm.Icu;

namespace Karm::Icu {

test$("ctype-alpha") {
    assert$(isAlpha('a'));
    assert$(isAlpha(U'é'));
    assert$(isAlpha(U'日'));
    assertNot$(isAlpha('5'));
    assertNot$(isAlpha('!'));
    return Ok();
}

test$("ctype-lower-upper") {
    assert$(isLower('a'));
    assert$(isLower(U'ß'));
    assertNot$(isLower('A'));
    assert$(isUpper('A'));
    assert$(isUpper(U'Σ'));
    assertNot$(isUpper('a'));
    return Ok();
}

test$("ctype-punct") {
    assert$(isPunct('!'));
    assert$(isPunct('_'));
    assert$(isPunct(U'¿'));
    assertNot$(isPunct('+'));
    assertNot$(isPunct('a'));
    return Ok();
}

test$("ctype-digit") {
    assert$(isDigit('5'));
    assert$(isDigit(U'٣'));
    assertNot$(isDigit(U'Ⅻ'));
    assertNot$(isDigit('f'));

    assert$(isXdigit('f'));
    assert$(isXdigit(U'٣'));
    assert$(isXdigit(U'Ｆ'));
    assertNot$(isXdigit('g'));

    assert$(isAlnum('a'));
    assert$(isAlnum('5'));
    assert$(isAlnum(U'日'));
    assertNot$(isAlnum('!'));
    return Ok();
}

test$("ctype-space") {
    assert$(isSpace(' '));
    assert$(isSpace('\n'));
    assert$(isSpace(U'　'));
    assertNot$(isSpace('a'));

    assert$(isBlank(' '));
    assert$(isBlank('\t'));
    assert$(isBlank(U'　'));
    assertNot$(isBlank('\n'));
    return Ok();
}

test$("ctype-cntrl-graph-print") {
    assert$(isCntrl('\n'));
    assert$(isCntrl(0x7F));
    assertNot$(isCntrl('a'));

    assert$(isGraph('a'));
    assert$(isGraph('!'));
    assertNot$(isGraph(' '));
    assertNot$(isGraph('\n'));
    assertNot$(isGraph(0x0378));

    assert$(isPrint('a'));
    assert$(isPrint(' '));
    assertNot$(isPrint('\n'));
    return Ok();
}

test$("ctype-word") {
    assert$(isWord('a'));
    assert$(isWord('5'));
    assert$(isWord('_'));
    assert$(isWord(0x0301));
    assert$(isWord(0x200D));
    assertNot$(isWord(' '));
    assertNot$(isWord('-'));
    return Ok();
}

test$("ctype-planes") {
    assert$(isBmp('a'));
    assert$(isBmp(0xFFFF));
    assertNot$(isBmp(0x10000));

    assert$(isSupplementary(0x10000));
    assert$(isSupplementary(0x10FFFF));
    assertNot$(isSupplementary(0xFFFF));
    assertNot$(isSupplementary(0x110000));
    return Ok();
}

} // namespace Karm::Icu
