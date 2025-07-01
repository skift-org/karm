module;

#include <karm-image/saver.h>
#include <karm-mime/uti.h>
#include <karm-sys/file.h>

module Karm.Print;

import :file_printer;
import :image_printer;
import :pdf_printer;

namespace Karm::Print {

Res<Rc<FilePrinter>> FilePrinter::create(Mime::Uti uti, FilePrinterProps props) {
    if (uti == Mime::Uti::PUBLIC_PDF) {
        return Ok(makeRc<PdfPrinter>());
    } else if (uti == Mime::Uti::PUBLIC_BMP or
               uti == Mime::Uti::PUBLIC_TGA or
               uti == Mime::Uti::PUBLIC_QOI) {
        return Ok(makeRc<ImagePrinter>(props.density, Image::Saver{uti}));
    }

    return Error::invalidData("cannot create printer");
}

Res<> FilePrinter::save(Mime::Url url) {
    auto outFile = try$(Sys::File::create(url));
    Io::TextEncoder<> outEncoder{outFile};
    try$(write(outEncoder));
    try$(outFile.flush());
    return Ok();
}

} // namespace Karm::Print
