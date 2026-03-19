module;

#include <karm/macros>

module Karm.Print;

import Karm.Core;
import Karm.Image;
import Karm.Ref;
import Karm.Sys;

import :filePrinter;
import :imagePrinter;
import :pdfPrinter;

namespace Karm::Print {

Res<Rc<FilePrinter>> FilePrinter::create(Ref::Uti uti, FilePrinterProps props) {
    if (uti == Ref::Uti::PUBLIC_PDF) {
        return Ok(makeRc<PdfPrinter>());
    } else if (uti.conformsTo(Ref::Uti::PUBLIC_IMAGE)) {
        return Ok(makeRc<ImagePrinter>(props.density, Image::Saver{uti}));
    } else {
        return Error::invalidData(Io::format("cannot create printer producing '{}'", uti.mimeTypes()));
    }
}

Res<> FilePrinter::save(Ref::Url url) {
    auto outFile = try$(Sys::File::create(url));
    try$(write(outFile));
    try$(outFile.flush());
    return Ok();
}

} // namespace Karm::Print
