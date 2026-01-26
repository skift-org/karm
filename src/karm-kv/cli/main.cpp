#include <karm/entry>

import Karm.Kv;
import Karm.Ref;

using namespace Karm;

Async::Task<> entryPointAsync(Sys::Context&, Async::CancellationToken) {
    auto wal = co_try$(Kv::Wal::open("file:./db.wal"_url));
    auto store = Kv::Store::open(wal);

    co_try$(store->put(bytes("hello"s), bytes("world"s)));
    co_try$(store->del(bytes("hello"s)));

    co_return Ok();
}
