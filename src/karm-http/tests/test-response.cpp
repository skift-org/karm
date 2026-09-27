#include <karm/test>

import Karm.Http;

using namespace Karm::Literals;

namespace Karm::Http::Tests {

test$("read-http-response-good-body") {
    auto rawResponse =
        "HTTP/1.1 200 OK\r\n"
        "Server: Apache\r\n"
        "Content-Length: 3\r\n"
        "\r\n"s;

    Io::BufReader br{bytes(rawResponse)};

    auto response = try$(Response::read(br));

    assertEq$(response.code, Code{200});

    auto expectedVersion = Version{Http::Protocol::HTTP, 1u, 1u};
    assertEq$(response.version, expectedVersion);

    assertEq$(response.header.len(), 2u);
    assertEq$(response.header.lookup(Header::SERVER), "Apache"s);
    assertEq$(response.header.lookup(Header::CONTENT_LENGTH), "3"s);

    return Ok();
}

test$("read-http-response-body-content-length-mismatch") {
    auto rawResponse =
        "HTTP/1.2 500 Internal Server Error\r\n"
        "Content-Length: 100\r\n"
        "\r\n"s;

    Io::BufReader br{bytes(rawResponse)};

    auto response = try$(Response::read(br));

    assertEq$(response.code, Code{500});

    auto expectedVersion = Version{Http::Protocol::HTTP, 1u, 2u};
    assertEq$(response.version, expectedVersion);

    assertEq$(response.header.len(), 1u);
    assertEq$(response.header.lookup(Header::CONTENT_LENGTH), "100"s);

    return Ok();
}

test$("read-http-response-body-empty-body") {
    auto rawResponse =
        "HTTP/1.1 404 Not Found\r\n"
        "\r\n"s;

    Io::BufReader br{bytes(rawResponse)};

    auto response = try$(Response::read(br));

    assertEq$(response.code, Http::Code::NOT_FOUND);
    assertEq$(response.header.len(), 0u);

    return Ok();
}

} // namespace Karm::Http::Tests
