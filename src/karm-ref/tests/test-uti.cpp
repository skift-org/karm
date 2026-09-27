#include <karm/test>

import Karm.Ref;

using namespace Karm::Literals;
using namespace Karm::Ref::Literals;

namespace Karm::Ref::Tests {

test$("karm-ref-uti-basic-properties") {
    Uti text{Uti::PUBLIC_TEXT};

    assertEq$(text.name(), "public.text"_sym);
    assertEq$(text.description(), "Text Document"s);
    assertEq$(text.suffixes().len(), 1uz);
    assertEq$(text.suffixes()[0], "txt"s);
    assertEq$(text.mimeTypes().len(), 1uz);
    assertEq$(text.mimeTypes()[0].str(), "text/plain"s);
    assertEq$(text.declaredConformances().len(), 1uz);
    assertEq$(text.declaredConformances()[0], "public.data"_sym);
    assertEq$(text.primarySuffix(), "txt"s);
    assertEq$(text.primaryMimeType().str(), "text/plain"s);

    return Ok();
}

test$("karm-ref-uti-from-extension") {
    // Known extension
    auto htmlUti = Uti::fromSuffix("html");
    assertEq$(htmlUti.name(), "public.html"_sym);
    assertEq$(htmlUti.primaryMimeType().str(), "text/html"s);

    // Unknown extension (should generate dynamic UTI)
    auto dynamicUti = Uti::fromSuffix("mycustomext");
    assertEq$(dynamicUti.primarySuffix(), "mycustomext"s);
    assertEq$(dynamicUti.primaryMimeType().str(), "application/octet-stream"s);
    assert$(dynamicUti.conformsTo("public.data"_uti));

    return Ok();
}

test$("karm-ref-uti-from-mime") {
    // Known mime type
    auto jpegUti = Uti::fromMime("image/jpeg"_mime);
    assertEq$(jpegUti.name(), "public.jpeg"_sym);
    assertEq$(jpegUti.primarySuffix(), "jpg"s);

    // Unknown mime type (should generate dynamic UTI)
    auto dynamicUti = Uti::fromMime("application/x-custom-type"_mime);
    assertEq$(dynamicUti.primaryMimeType().str(), "application/x-custom-type"s);
    assert$(dynamicUti.conformsTo("public.data"_uti));

    return Ok();
}

test$("karm-ref-uti-from-uti-or-mime") {
    auto nameUti = Uti::fromUtiOrMime("public.png");
    assertEq$(nameUti.name(), "public.png"_sym);

    auto mimeUti = Uti::fromUtiOrMime("image/png");
    assertEq$(mimeUti.name(), "public.png"_sym);

    auto dynamicMimeUti = Uti::fromUtiOrMime("application/x-custom-type");
    assertEq$(dynamicMimeUti.primaryMimeType().str(), "application/x-custom-type"s);
    assert$(dynamicMimeUti.conformsTo("public.data"_uti));

    return Ok();
}

test$("karm-ref-uti-conformance") {
    Uti html{Uti::PUBLIC_HTML};
    Uti text{Uti::PUBLIC_TEXT};
    Uti data{Uti::PUBLIC_DATA};
    Uti item{Uti::PUBLIC_ITEM};
    Uti image{Uti::PUBLIC_IMAGE};

    // Self-conformance
    assert$(html.conformsTo(html));

    // Direct conformance
    assert$(html.conformsTo(text));

    // Transitive conformance (HTML -> TEXT -> DATA -> ITEM)
    assert$(html.conformsTo(data));
    assert$(html.conformsTo(item));

    // Non-conformance
    assert$(not html.conformsTo(image));
    assert$(not image.conformsTo(text));

    return Ok();
}

test$("karm-ref-uti-equality-and-udl") {
    auto literalUti = "public.json"_uti;
    Uti enumUti{Uti::PUBLIC_JSON};

    // Check UDL parsed properly
    assertEq$(literalUti.name(), "public.json"_sym);

    // Check equality operators
    assert$(literalUti == enumUti);
    assert$(literalUti == "public.json"_uti);
    assert$(not(literalUti == "public.xml"_uti));

    return Ok();
}

} // namespace Karm::Ref::Tests
