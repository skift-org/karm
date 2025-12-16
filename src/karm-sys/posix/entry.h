#pragma once

import Karm.Core;
import Karm.Sys;

#define EXIT_FAILURE 1 /* Failing exit status.  */
#define EXIT_SUCCESS 0 /* Successful exit status.  */

void __panicHandler(Karm::PanicKind kind, char const* msg);

int main(int argc, char const** argv) {
    Karm::registerPanicHandler(__panicHandler);

    Karm::Sys::Context ctx;
    ctx.add<Karm::Sys::ArgsHook>(argc, argv);
    auto [cancellation, ct] = Karm::Async::Cancellation::create();
    Karm::Res<> code = Karm::Sys::run(entryPointAsync(ctx, ct));
    cancellation->cancel();

    if (not code) {
        Karm::Sys::errln("{}: {}", argv[0], code);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
