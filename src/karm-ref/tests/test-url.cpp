#include <karm/test>

import Karm.Ref;
import Karm.Logger;

using namespace Karm::Literals;
using namespace Karm::Ref::Literals;

namespace Karm::Ref::Tests {

test$("karm-ref-url-parse") {
    auto url = "http://example.com:1234/home"_url;

    assertEq$(url.scheme, "http"s);
    assertEq$(url.userInfo, ""s);
    assertEq$(url.host, "example.com"s);
    assertEq$(url.port, 1234uz);
    assertEq$(url.path.str(), "/home"s);
    assertEq$(url.query, ""s);
    assertEq$(url.fragment, ""s);

    auto url2 = "http://example.com:1234/home?query#fragment"_url;

    assertEq$(url2.scheme, "http"s);
    assertEq$(url2.userInfo, ""s);
    assertEq$(url2.host, "example.com"s);
    assertEq$(url2.port, 1234uz);
    assertEq$(url2.path.str(), "/home"s);
    assertEq$(url2.query, "query"s);
    assertEq$(url2.fragment, "fragment"s);

    auto url3 = "ftp://user@example.com:1234/home?query#fragment"_url;

    assertEq$(url3.scheme, "ftp"s);
    assertEq$(url3.userInfo, "user"s);
    assertEq$(url3.host, "example.com"s);
    assertEq$(url3.port, 1234uz);
    assertEq$(url3.path.str(), "/home"s);
    assertEq$(url3.query, "query"s);
    assertEq$(url3.fragment, "fragment"s);

    auto url4 = "./home"_url;

    assertEq$(url4.scheme, ""s);
    assertEq$(url4.userInfo, ""s);
    assertEq$(url4.host, ""s);
    assertEq$(url4.port, NONE);
    assertEq$(url4.path.str(), "./home"s);
    assertEq$(url4.query, ""s);
    assertEq$(url4.fragment, ""s);

    return Ok();
}

test$("karm-ref-url-unparse") {
    assertEq$("http://smnx.sh/"_url.str(), "http://smnx.sh/"s);
    assertEq$("http://smnx.sh"_url.str(), "http://smnx.sh"s);
    return Ok();
}

test$("karm-ref-url-parent-of") {
    assert$("http://example.com/"_url.parentOf("http://example.com/"_url));
    assert$("http://example.com"_url.parentOf("http://example.com/a"_url));
    assert$("http://example.com"_url.parentOf("http://example.com/a/b"_url));

    assertNot$("http://example.com/a"_url.parentOf("http://example.com"_url));
    assertNot$("http://example.com/a/b"_url.parentOf("http://example.com"_url));

    return Ok();
}

test$("karm-ref-url-resolution-reference") {
    auto base = "http://a/b/c/d;p?q"_url;

    // https://datatracker.ietf.org/doc/html/rfc3986#section-5.4.1
    assertEq$(Url::resolveReference(base, "g:h"_url).take(), "g:h"_url);
    assertEq$(Url::resolveReference(base, "g"_url).take(), "http://a/b/c/g"_url);
    assertEq$(Url::resolveReference(base, "./g"_url).take(), "http://a/b/c/g"_url);
    assertEq$(Url::resolveReference(base, "g/"_url).take(), "http://a/b/c/g/"_url);
    assertEq$(Url::resolveReference(base, "//g"_url).take(), "http://g"_url);
    assertEq$(Url::resolveReference(base, "?y"_url).take(), "http://a/b/c/d;p?y"_url);
    assertEq$(Url::resolveReference(base, "g?y"_url).take(), "http://a/b/c/g?y"_url);
    assertEq$(Url::resolveReference(base, "#s"_url).take(), "http://a/b/c/d;p?q#s"_url);
    assertEq$(Url::resolveReference(base, "g#s"_url).take(), "http://a/b/c/g#s"_url);
    assertEq$(Url::resolveReference(base, "g?y#s"_url).take(), "http://a/b/c/g?y#s"_url);
    assertEq$(Url::resolveReference(base, ";x"_url).take(), "http://a/b/c/;x"_url);
    assertEq$(Url::resolveReference(base, "g;x"_url).take(), "http://a/b/c/g;x"_url);
    assertEq$(Url::resolveReference(base, "g;x?y#s"_url).take(), "http://a/b/c/g;x?y#s"_url);
    assertEq$(Url::resolveReference(base, ""_url).take(), "http://a/b/c/d;p?q"_url);
    assertEq$(Url::resolveReference(base, "."_url).take(), "http://a/b/c/"_url);
    assertEq$(Url::resolveReference(base, "./"_url).take(), "http://a/b/c/"_url);
    assertEq$(Url::resolveReference(base, ".."_url).take(), "http://a/b/"_url);
    assertEq$(Url::resolveReference(base, "../"_url).take(), "http://a/b/"_url);
    assertEq$(Url::resolveReference(base, "../g"_url).take(), "http://a/b/g"_url);
    assertEq$(Url::resolveReference(base, "../.."_url).take(), "http://a/"_url);
    assertEq$(Url::resolveReference(base, "../../"_url).take(), "http://a/"_url);
    assertEq$(Url::resolveReference(base, "../../g"_url).take(), "http://a/g"_url);

    // https://datatracker.ietf.org/doc/html/rfc3986#section-5.4.2
    assertEq$(Url::resolveReference(base, "../../../g"_url).take(), "http://a/g"_url);
    assertEq$(Url::resolveReference(base, "../../../../g"_url).take(), "http://a/g"_url);
    assertEq$(Url::resolveReference(base, "/./g"_url).take(), "http://a/g"_url);
    assertEq$(Url::resolveReference(base, "/../g"_url).take(), "http://a/g"_url);
    assertEq$(Url::resolveReference(base, "g."_url).take(), "http://a/b/c/g."_url);
    assertEq$(Url::resolveReference(base, ".g"_url).take(), "http://a/b/c/.g"_url);
    assertEq$(Url::resolveReference(base, "g.."_url).take(), "http://a/b/c/g.."_url);
    assertEq$(Url::resolveReference(base, "..g"_url).take(), "http://a/b/c/..g"_url);
    assertEq$(Url::resolveReference(base, "./../g"_url).take(), "http://a/b/g"_url);
    assertEq$(Url::resolveReference(base, "./g/."_url).take(), "http://a/b/c/g/"_url);
    assertEq$(Url::resolveReference(base, "g/./h"_url).take(), "http://a/b/c/g/h"_url);
    assertEq$(Url::resolveReference(base, "g/../h"_url).take(), "http://a/b/c/h"_url);
    assertEq$(Url::resolveReference(base, "g;x=1/./y"_url).take(), "http://a/b/c/g;x=1/y"_url);
    assertEq$(Url::resolveReference(base, "g;x=1/../y"_url).take(), "http://a/b/c/y"_url);
    assertEq$(Url::resolveReference(base, "g?y/./x"_url).take(), "http://a/b/c/g?y/./x"_url);
    assertEq$(Url::resolveReference(base, "g?y/../x"_url).take(), "http://a/b/c/g?y/../x"_url);
    assertEq$(Url::resolveReference(base, "g#s/./x"_url).take(), "http://a/b/c/g#s/./x"_url);
    assertEq$(Url::resolveReference(base, "g#s/../x"_url).take(), "http://a/b/c/g#s/../x"_url);
    assertEq$(Url::resolveReference(base, "http:g"_url, true).take(), "http:g"_url);
    assertEq$(Url::resolveReference(base, "http:g"_url, false).take(), "http://a/b/c/g"_url);

    return Ok();
}

} // namespace Karm::Ref::Tests
