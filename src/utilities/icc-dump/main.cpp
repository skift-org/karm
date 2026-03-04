#include <karm/entry>

import Karm.Icc;
import Karm.Core;
import Karm.Sys;

using namespace Karm;

Async::Task<> entryPointAsync(Sys::Context& ctx, Async::CancellationToken) {
    auto& args = useArgs(ctx);

    if (args.len() < 1)
        co_return Error::invalidInput("Usage: icc-dump <file>");

    auto url = Ref::parseUrlOrPath(args[0], co_try$(Sys::pwd()));
    auto file = co_try$(Sys::File::open(url));
    auto buf = co_try$(Io::readAll(file));

    Io::BChunk chunk{buf};
    Icc::Parser parser{chunk};
    Icc::ProfileHeader header = parser.header();

    Sys::println("profile size: {}", DataSize{header.profileSize});
    Sys::println("profile version number: {:#x}", header.profileVersionNumber);
    Sys::println("profile device class: {}", header.profileDeviceClass());
    Sys::println("data color space: {}", header.dataColorSpace());
    Sys::println("profile connection space: {}", header.profileConnectionSpace());
    Sys::println("creation date: {}", header.creationTime());

    Sys::println("tags:");
    for (auto tag : parser.iterTag()) {
        Sys::println(" - {}: {}", Str{tag.sig}, tag.type);
    }

    co_return Ok();
}
