#include <karm/entry>

import Karm.Ml;
import Karm.Ref;

using namespace Karm;

Async::Task<> entryPointAsync(Sys::Context&, Async::CancellationToken) {
    co_try$(Ml::Gguf::loadGguf("bundle://karm-ml/smollm2-360m-instruct-q8_0.gguf"_url));
    co_return Ok();
}
