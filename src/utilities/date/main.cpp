#include <karm/entry>

using namespace Karm;

Async::Task<> entryPointAsync(Sys::Env&, Async::CancellationToken) {
    Sys::println("{}", Sys::dateTime());
    co_return Ok();
}
