#include <karm/test>

import Karm.Glob;

namespace Karm::Glob::Tests {

test$("glob-match") {
    assert$(matchGlob("", ""));
    assert$(matchGlob("a", "a"));
    assert$(not matchGlob("a", ""));
    assert$(not matchGlob("", "a"));
    assert$(matchGlob("abcABC123", "abcABC123"));
    assert$(not matchGlob("ABCabc123", "abcABC123"));
    assert$(matchGlob("?", "a"));
    assert$(matchGlob("?", "b"));
    assert$(matchGlob("?", "1"));
    assert$(matchGlob("?", "8"));
    assert$(not matchGlob("?", ""));
    assert$(matchGlob("abcABC*", "abcABC123"));
    assert$(matchGlob("abc*123", "abcABC123"));
    assert$(matchGlob("abc*123", "abc123"));
    assert$(matchGlob("*ABC123", "abcABC123"));
    assert$(not matchGlob("abcABC*", "ABCabc123"));
    assert$(not matchGlob("abc*123", "ABCabc123"));
    assert$(not matchGlob("abc*123", "abcABCXYZ"));
    assert$(not matchGlob("*ABC123", "ABCabc123"));
    assert$(matchGlob("[abc]", "a"));
    assert$(matchGlob("[abc]", "b"));
    assert$(matchGlob("[abc]", "c"));
    assert$(not matchGlob("[abc]", "1"));
    assert$(not matchGlob("[abc]", "2"));
    assert$(not matchGlob("[abc]", "3"));
    assert$(matchGlob("[a-z]", "a"));
    assert$(matchGlob("[a-z]", "z"));
    assert$(not matchGlob("[a-z]", "1"));
    assert$(not matchGlob("[a-z]", "9"));
    assert$(not matchGlob("[^a-z]", "a"));
    assert$(not matchGlob("[^a-z]", "z"));
    assert$(matchGlob("[^a-z]", "1"));
    assert$(matchGlob("[^a-z]", "9"));
    assert$(matchGlob("[a-z0-9]", "a"));
    assert$(matchGlob("[a-z0-9]", "k"));
    assert$(matchGlob("[a-z0-9]", "1"));
    assert$(matchGlob("[a-z0-9]", "7"));
    assert$(matchGlob("[a-z0-9]", "a"));
    assert$(not matchGlob("[a-z0-9]", "A"));
    assert$(not matchGlob("[a-z0-9]", "K"));

    return Ok();
}

} // namespace Karm::Glob::Tests
