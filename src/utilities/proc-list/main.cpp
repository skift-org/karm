#include <karm/entry>

using namespace Karm;
using namespace Karm::Re::Literals;
using namespace Karm::Ref::Literals;
using namespace Karm::Literals;

Async::Task<> entryPointAsync(Sys::Env&, Async::CancellationToken) {
    for (auto& p : co_try$(Sys::Process::list()))
        if (auto const& [stat] = p->stat(Sys::ProcessStat::ALL).ok())
            Sys::println(
                "id:{} parent:{} status:{} name:{:#} rss:{} shared:{} utime:{} stime:{}", stat->id, stat->parent, stat->status, stat->name, DataSize{stat->memory}, DataSize{stat->virtualMemory}, stat->userTime.value(), stat->systemTime.value()
            );
    co_return Ok();
}
