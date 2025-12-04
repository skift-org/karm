#include <karm-sys/entry.h>

import Karm.Idl;

using namespace Karm;

Async::Task<> entryPointAsync(Sys::Context&) {
    auto data = Sys::readAllUtf8("bundle://karm-idl/public/idl/echo.idl"_url).unwrap("could not read idl file");
    auto maybeModule = Idl::parseModule(data);
    if (not maybeModule)
        Sys::errln("could not parse module: {}", maybeModule);
    auto module = co_try$(maybeModule);
    auto semaResult = Idl::semaCheck(module);

    if (not semaResult) {
        Sys::errln("sema check failed: {}", semaResult);
        co_return Error::invalidData("sema check failed");
    }

    Sys::println("{}", module);
    co_return Ok();
}