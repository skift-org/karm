#include <karm/entry>

import Karm.Sys;
import Karm.Ref;

using namespace Karm;

Async::Task<> entryPointAsync(Sys::Context& ctx, Async::CancellationToken) {
    auto& args = Sys::useArgs(ctx);

    for (usize i = 0; i < args.len(); i++) {
        auto url = Ref::parseUrlOrPath(args[i], co_try$(Sys::pwd()));
        auto file = co_try$(Sys::File::open(url));
        Sys::println("file: {}", url);
        Sys::println("mime by sniffing url: {}", Ref::Uti::fromSuffix(url.path.suffix()));
        Sys::println("mime by sniffing content: {}", file.sniff());
    }

    co_return Ok();
}
