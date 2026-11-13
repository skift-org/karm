#include <karm/entry>

import Karm.Cli;
import Karm.Core;
import Karm.Font.Woff;
import Karm.Ref;
import Karm.Sys;

using namespace Karm;
using namespace Karm::Literals;

Async::Task<> entryPointAsync(Sys::Env& env, Async::CancellationToken) {
    auto inputArg = Cli::operand<Opt<Ref::Url>>("input"s, "WOFF or WOFF2 file to unwrap"s);
    auto outputArg = Cli::operand<Opt<Ref::Url>>("output"s, "Where to write the font, next to the input by default"s);

    Cli::Command cmd{
        "woff-decompress"s,
        "Unwrap a WOFF or WOFF2 file into the font it holds."s,
        {
            {
                "Arguments"s,
                {
                    inputArg,
                    outputArg,
                },
            },
        },
    };

    co_try$(cmd.exec(env));
    if (not cmd)
        co_return Ok();

    auto input = co_try$(inputArg.value().okOr(Error::invalidInput("missing input")));
    auto file = co_try$(Sys::File::open(input));
    auto map = co_try$(Sys::mmap(file));

    Vec<u8> sfnt;
    if (Font::Woff::sniffWoff1(map.bytes()))
        sfnt = co_try$(Font::Woff::decodeWoff1(map.bytes()));
    else if (Font::Woff::sniffWoff2(map.bytes()))
        sfnt = co_try$(Font::Woff::decodeWoff2(map.bytes()));
    else
        co_return Error::invalidInput("not a woff or woff2 file");

    // NOTE: An OpenType font with CFF outlines starts with 'OTTO'
    Str suffix = startWith(sfnt, bytes("OTTO"s)) != Match::NO ? "otf" : "ttf";
    auto output = outputArg.value().unwrapOrElse([&] {
        return input.parent(1) / Io::format("{}.{}", input.path.stem(), suffix);
    });

    co_try$(Sys::writeAll(output, sfnt));
    co_return Ok();
}
