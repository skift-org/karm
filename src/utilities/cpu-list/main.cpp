#include <karm/entry>

using namespace Karm;

Async::Task<> entryPointAsync(Sys::Env&, Async::CancellationToken) {
    for (auto& c : co_try$(Sys::Cpu::list()))
        Sys::println("name:{} brand:{} vendor:{} user-time:{} system-time:{} idle-time:{}", c.name, c.brand, c.vendor, c.userTime.value(), c.systemTime.value(), c.idleTime.value());
    co_return Ok();
}
