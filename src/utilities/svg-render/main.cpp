#include <karm/entry>

import Karm.Svg;
import Karm.Image;

using namespace Karm;
using namespace Karm::Literals;

Async::Task<> entryPointAsync(Sys::Env& env, Async::CancellationToken) {
    auto str = co_try$(Sys::readAllText(Ref::parseUrlOrPath("cat.svg"s, env.cwd())));
    auto snapshot = co_try$(Svg::render(str));
    auto image = co_try$(snapshot.rasterize());
    co_try$(Image::save(image->pixels(), Ref::parseUrlOrPath("cat.png"s, env.cwd())));

    co_return Ok();
}
