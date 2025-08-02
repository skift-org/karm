#include <karm-sys/entry.h>
#include <karm-sys/file.h>
#include <karm-sys/proc.h>

using namespace Karm;

Res<Mime::Mime> sniffFile(Mime::Url url) {
    auto file = try$(Sys::File::open(url));
    return file.sniff(true);
}

Async::Task<> entryPointAsync(Sys::Context& c) {
    auto& args = Sys::useArgs(c);

    for (usize i = 0; i < args.len(); i++) {
        auto url = Mime::parseUrlOrPath(args[i], co_try$(Sys::pwd()));
        Sys::println("file: {}", url);
        Sys::println("mime by sniffing url: {}", Mime::sniffSuffix(url.path.suffix()));
        Sys::println("mime by sniffing content: {}", sniffFile(url));
    }

    co_return Ok();
}
