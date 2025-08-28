#include <karm-gfx/prose.h>
#include <karm-gfx/canvas.h>
#include <karm-font/loader.h>
#include <karm-sys/entry.h>
#include <karm-sys/file.h>

import Karm.Core;
import Karm.Print;

using namespace Karm;

Async::Task<> entryPointAsync(Sys::Context&) {
    auto printer = co_try$(Print::PdfPrinter::create(Mime::Uti::PUBLIC_PDF));

    auto& ctx = printer->beginPage(Print::A4);
    ctx.fillStyle(Gfx::RED);
    ctx.rect({0, 0, 100, 100});
    ctx.fill(Gfx::FillRule::NONZERO);

    ctx.fillStyle(Gfx::BLUE);
    ctx.rect({0, 100, 100, 100});
    ctx.fill(Gfx::FillRule::NONZERO);

    ctx.fillStyle(Gfx::GREEN);
    ctx.rect({0, 200, 100, 100});
    ctx.fill(Gfx::FillRule::NONZERO);

    Gfx::Prose prose = Gfx::ProseStyle{
        .font = {co_try$(Font::loadFontfaceOrFallback("bundle://fonts-inter/fonts/Inter-Regular.ttf"_url)), 12},
        .color = Gfx::BLACK,
    };

    prose.append("Hello, world!"s);
    prose.layout(Au{999});
    ctx.fill(prose);

    co_try$(printer->save("file:test.pdf"_url));

    co_return Ok();
}
