import Karm.Core;

#include <karm/test>

using namespace Karm::Literals;

namespace Karm::Base::Tests {

template <StaticEncoding E>
static Vec<Rune> _feed(RuneDecoder<E>& decoder, Slice<typename E::Unit> units) {
    Vec<Rune> runes;
    for (auto rune : decoder.feed(units))
        runes.pushBack(rune);
    return runes;
}

// MARK: Utf8 ------------------------------------------------------------------

test$("rune-decoder-utf8-sequences-of-every-length") {
    RuneDecoder<Utf8> decoder;
    assertEq$(_feed(decoder, "aé✓😀"s), (Vec<Rune>{U'a', U'é', U'✓', U'😀'}));
    assertNot$(decoder.pending());
    return Ok();
}

test$("rune-decoder-utf8-waits-for-the-whole-sequence") {
    RuneDecoder<Utf8> decoder;
    assertEq$(_feed(decoder, "\xf0\x9f\x98"s).len(), 0uz);
    assert$(decoder.pending());
    assertEq$(_feed(decoder, "\x80"s), (Vec<Rune>{U'😀'}));
    assertNot$(decoder.pending());
    return Ok();
}

test$("rune-decoder-utf8-sequence-cut-short") {
    RuneDecoder<Utf8> decoder;
    assertEq$(_feed(decoder, "\xe2\x9c"
                             "a"s),
              (Vec<Rune>{REPLACEMENT, U'a'}));
    assertNot$(decoder.pending());
    return Ok();
}

test$("rune-decoder-utf8-sequence-cut-short-by-another-one") {
    RuneDecoder<Utf8> decoder;
    assertEq$(_feed(decoder, "\xc3\xc3\xa9"s), (Vec<Rune>{REPLACEMENT, U'é'}));
    return Ok();
}

test$("rune-decoder-utf8-stray-units") {
    RuneDecoder<Utf8> decoder;
    assertEq$(_feed(decoder, "\x80\xff"
                             "a"s),
              (Vec<Rune>{REPLACEMENT, REPLACEMENT, U'a'}));
    return Ok();
}

test$("rune-decoder-utf8-overlong-sequence") {
    RuneDecoder<Utf8> decoder;
    assertEq$(_feed(decoder, "\xc0\xaf"
                             "a"s),
              (Vec<Rune>{REPLACEMENT, U'a'}));
    return Ok();
}

// MARK: Utf16 -----------------------------------------------------------------

test$("rune-decoder-utf16-surrogate-pair") {
    RuneDecoder<Utf16> decoder;
    Array<u16, 3> units = {u'a', 0xd83d, 0xde00};
    assertEq$(_feed(decoder, sub(units, 0, 2)), (Vec<Rune>{U'a'}));
    assert$(decoder.pending());
    assertEq$(_feed(decoder, sub(units, 2, 3)), (Vec<Rune>{U'😀'}));
    assertNot$(decoder.pending());
    return Ok();
}

test$("rune-decoder-utf16-lone-surrogates") {
    RuneDecoder<Utf16> decoder;
    Array<u16, 4> units = {0xde00, 0xd83d, u'a', u'b'};
    assertEq$(_feed(decoder, units), (Vec<Rune>{REPLACEMENT, REPLACEMENT, U'a', U'b'}));
    return Ok();
}

// MARK: Single unit encodings -------------------------------------------------

test$("rune-decoder-ibm437") {
    RuneDecoder<Ibm437> decoder;
    Array<u8, 3> units = {'c', 0x82, 0xdb};
    assertEq$(_feed(decoder, units), (Vec<Rune>{U'c', U'é', U'█'}));
    assertNot$(decoder.pending());
    return Ok();
}

test$("rune-decoder-latin1") {
    RuneDecoder<Latin1> decoder;
    Array<u8, 3> units = {'c', 0xe9, 0xff};
    assertEq$(_feed(decoder, units), (Vec<Rune>{U'c', U'é', U'ÿ'}));
    return Ok();
}

test$("rune-decoder-ascii-rejects-the-rest") {
    RuneDecoder<Ascii> decoder;
    assertEq$(_feed(decoder, "a\xe9"s), (Vec<Rune>{U'a', REPLACEMENT}));
    return Ok();
}

} // namespace Karm::Base::Tests
