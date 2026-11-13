#include <karm/entry>

import Karm.Sys;
import Karm.Cli;
import Karm.Ir.Spirv;

using namespace Karm;
using namespace Karm::Literals;
using namespace Karm::Ref::Literals;

Async::Task<> entryPointAsync(Sys::Env& env, Async::CancellationToken) {
    auto inputArg = Cli::operand<Opt<Ref::Url>>("input"s, "SPIR-V file to dump"s);

    Cli::Command cmd{
        "spriv-dis"s,
        "Dump the content of an ELF file."s,
        {
            {
                "Arguments"s,
                {
                    inputArg,
                },
            },
        },
    };

    co_try$(cmd.exec(env));
    if (not cmd)
        co_return Ok();

    auto url = co_try$(inputArg.value().okOr(Error::invalidInput("missing input")));
    auto data = co_try$(Sys::readAll(url));
    Io::BScan s{data};
    u32 magic = co_try$(s.nextU32le());
    Sys::println("magic: {:#08x}", magic);

    u32 version = co_try$(s.nextU32le());
    Sys::println("version: {:#08x}", version);

    u32 generator = co_try$(s.nextU32le());
    Sys::println("generator: {:#08x}", generator);

    u32 bound = co_try$(s.nextU32le());
    Sys::println("bound: {:#08x}", bound);

    u32 schema = co_try$(s.nextU32le());
    Sys::println("schema: {:#08x}", schema);

    while (not s.ended()) {
        auto op = co_try$(s.nextU32le());
        auto wordCount = op >> 16;
        auto instruction = static_cast<Ir::Spirv::Op>(op & 0xffff);

        Sys::print("{}", instruction);
        for (usize _ : urange::fromStartEnd(1, wordCount))
            Sys::print(" {:#08x}", co_try$(s.nextU32le()));

        Sys::println();
    }

    co_return Ok();
}
