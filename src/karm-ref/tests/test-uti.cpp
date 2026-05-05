#include <karm/test>

import Karm.Ref;

using namespace Karm::Literals;
using namespace Karm::Ref::Literals;

namespace Karm::Ref::Tests {

test$("karm-ref-uti-basic-properties") {
    Uti text{Uti::PUBLIC_TEXT};

    expectEq$(text.name(), "public.text"_sym);
    expectEq$(text.description(), "Text Document"s);
    expectEq$(text.suffixes().len(), 1uz);
    expectEq$(text.suffixes()[0], "txt"s);
    expectEq$(text.mimeTypes().len(), 1uz);
    expectEq$(text.mimeTypes()[0].str(), "text/plain"s);
    expectEq$(text.declaredConformances().len(), 1uz);
    expectEq$(text.declaredConformances()[0], "public.data"_sym);
    expectEq$(text.primarySuffix(), "txt"s);
    expectEq$(text.primaryMimeType().str(), "text/plain"s);

    return Ok();
}

test$("karm-ref-uti-from-extension") {
    // Known extension
    auto htmlUti = Uti::fromSuffix("html");
    expectEq$(htmlUti.name(), "public.html"_sym);
    expectEq$(htmlUti.primaryMimeType().str(), "text/html"s);

    // Unknown extension (should generate dynamic UTI)
    auto dynamicUti = Uti::fromSuffix("mycustomext");
    expectEq$(dynamicUti.primarySuffix(), "mycustomext"s);
    expectEq$(dynamicUti.primaryMimeType().str(), "application/octet-stream"s);
    expect$(dynamicUti.conformsTo("public.data"_uti));

    return Ok();
}

test$("karm-ref-uti-from-mime") {
    // Known mime type
    auto jpegUti = Uti::fromMime("image/jpeg"_mime);
    expectEq$(jpegUti.name(), "public.jpeg"_sym);
    expectEq$(jpegUti.primarySuffix(), "jpg"s);

    // Unknown mime type (should generate dynamic UTI)
    auto dynamicUti = Uti::fromMime("application/x-custom-type"_mime);
    expectEq$(dynamicUti.primaryMimeType().str(), "application/x-custom-type"s);
    expect$(dynamicUti.conformsTo("public.data"_uti));

    return Ok();
}

test$("karm-ref-uti-from-uti-or-mime") {
    auto nameUti = Uti::fromUtiOrMime("public.png");
    expectEq$(nameUti.name(), "public.png"_sym);

    auto mimeUti = Uti::fromUtiOrMime("image/png");
    expectEq$(mimeUti.name(), "public.png"_sym);

    auto dynamicMimeUti = Uti::fromUtiOrMime("application/x-custom-type");
    expectEq$(dynamicMimeUti.primaryMimeType().str(), "application/x-custom-type"s);
    expect$(dynamicMimeUti.conformsTo("public.data"_uti));

    return Ok();
}

test$("karm-ref-uti-conformance") {
    Uti html{Uti::PUBLIC_HTML};
    Uti text{Uti::PUBLIC_TEXT};
    Uti data{Uti::PUBLIC_DATA};
    Uti item{Uti::PUBLIC_ITEM};
    Uti image{Uti::PUBLIC_IMAGE};

    // Self-conformance
    expect$(html.conformsTo(html));

    // Direct conformance
    expect$(html.conformsTo(text));

    // Transitive conformance (HTML -> TEXT -> DATA -> ITEM)
    expect$(html.conformsTo(data));
    expect$(html.conformsTo(item));

    // Non-conformance
    expect$(not html.conformsTo(image));
    expect$(not image.conformsTo(text));

    return Ok();
}

test$("karm-ref-uti-equality-and-udl") {
    auto literalUti = "public.json"_uti;
    Uti enumUti{Uti::PUBLIC_JSON};

    // Check UDL parsed properly
    expectEq$(literalUti.name(), "public.json"_sym);

    // Check equality operators
    expect$(literalUti == enumUti);
    expect$(literalUti == "public.json"_uti);
    expect$(not(literalUti == "public.xml"_uti));

    return Ok();
}

} // namespace Karm::Ref::Tests
