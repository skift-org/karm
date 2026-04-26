#include <karm/entry>

import Karm.Fs;
import Karm.Ref;

using namespace Karm;
using namespace Karm::Literals;
using namespace Karm::Ref::Literals;


Async::Task<> entryPointAsync(Sys::Env&, Async::CancellationToken) {
    auto file = co_trya$(Fs::Image::openOrCreateAsync("file:disk.raw"_url));
    co_trya$(file->truncateAsync(DataSize::fromMiB(512)));
    auto mbr = co_trya$(Fs::Mbr::formatAsync(file));

    auto root = co_trya$(Fs::createAsync<Fs::VDir>());
    auto volumes = co_trya$(root->createAsync("volumes"s, Sys::Type::DIR));
    co_trya$(volumes->linkAsync("sda", mbr));

    co_return Ok();
}
