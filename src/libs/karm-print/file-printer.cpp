module;

#include <karm-mime/url.h>
#include <karm-mime/uti.h>

export module Karm.Print:file_printer;

import :printer;

namespace Karm::Print {

export struct FilePrinterProps {
    /// Pixel density for raster formats (ignored for vector formats)
    f64 density = 1;
};

export struct FilePrinter : Printer {
    static Res<Rc<FilePrinter>> create(Mime::Uti uti, FilePrinterProps props = {});

    virtual Res<> write(Io::Writer& w) = 0;

    Res<> save(Mime::Url url);
};

} // namespace Karm::Print
