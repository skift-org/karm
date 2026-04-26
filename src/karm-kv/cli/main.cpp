#include <karm/entry>

import Karm.Kv;
import Karm.Ref;

using namespace Karm;
using namespace Karm::Literals;
using namespace Karm::Ref::Literals;

Async::Task<> entryPointAsync(Sys::Env&, Async::CancellationToken) {
    auto wal = co_try$(Kv::Wal::open("file:./db.wal"_url));
    auto store = Kv::Store::open(wal);

    co_try$(store->put(bytes("hello"s), bytes("world"s)));
    co_try$(store->del(bytes("hello"s)));

    co_return Ok();
}
