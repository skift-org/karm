#include <karm/test>

import Karm.Http;
import Karm.Ref;

using namespace Karm::Literals;
using namespace Karm::Ref::Literals;

namespace Karm::Http::Tests {

test$("parse-unparse-http-request-no-header") {
    auto rawRequest =
        "GET / HTTP/1.1\r\n"
        "\r\n"s;

    Io::SScan s{rawRequest};
    auto request = try$(Request::parse(s));

    assertEq$(request.url.path, "/"_path);
    assertEq$(request.method, Method::GET);

    auto expectedVersion = Version{Http::Protocol::HTTP, 1u, 1u};
    assertEq$(request.version, expectedVersion);

    Io::StringWriter sw;
    try$(request.unparse(sw));

    assertEq$(rawRequest, sw.take());

    return Ok();
}

test$("parse-unparse-http-request-with-header") {
    auto rawRequest =
        "POST / HTTP/1.2\r\n"
        "Host: odoo.com\r\n"
        "User-Agent: PM\r\n"
        "Accept: */*\r\n"
        "\r\n"s;

    Io::SScan s{rawRequest};
    auto request = try$(Request::parse(s));

    assertEq$(request.url.path, "/"_path);
    assertEq$(request.method, Method::POST);

    auto expectedVersion = Version{Http::Protocol::HTTP, 1u, 2u};
    assertEq$(request.version, expectedVersion);

    Io::StringWriter sw;
    try$(request.unparse(sw));
    auto unparsedReq = sw.take();

    // the order of headers does not matter, so we cannot compare the strings
    assert$(contains(unparsedReq, "Host: odoo.com\r\n"s));
    assert$(contains(unparsedReq, "User-Agent: PM\r\n"s));
    assert$(contains(unparsedReq, "Accept: */*\r\n"s));

    auto unparsedReqLen = unparsedReq.len();
    assertEq$(unparsedReqLen, rawRequest.len());
    assertEq$(Slice(unparsedReq.buf() + unparsedReqLen - 4, unparsedReq.buf() + unparsedReqLen), "\r\n\r\n"s);

    return Ok();
}

} // namespace Karm::Http::Tests
