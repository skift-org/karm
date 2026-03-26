export module Karm.Print:pdfPrinter;

import Karm.Font.Ttf;
import Karm.Pdf;

import :filePrinter;
import :pdfFonts;

namespace Karm::Print {

struct PdfPage {
    Math::Vec2f size;
    Io::StringWriter data;
};

export struct PdfPrinter : FilePrinter {
    Vec<PdfPage> _pages;
    Opt<Pdf::Canvas> _canvas;
    Pdf::FontManager fontManager;
    Vec<Pdf::GraphicalStateDict> graphicalStates;

    Gfx::Canvas& beginPage(Math::Vec2f size) override {
        auto& page = _pages.emplaceBack(size);
        _canvas = Pdf::Canvas{page.data, size, &fontManager, graphicalStates};

        // Convert fron the karm-pdf internal units to PDF units (1/72 inch)
        _canvas->scale(72.0 / DPI.cast<f64>());

        // NOTE: PDF has the coordinate system origin at the bottom left corner.
        //       But we want to have it at the top left corner.
        _canvas->transform(
            {1, 0, 0, -1, 0, size.height}
        );

        return *_canvas;
    }

    Pdf::File pdf() {
        Pdf::Ref alloc;

        Pdf::File file;
        file.header = "PDF-2.0"s;

        Pdf::Array pagesKids;
        Pdf::Ref pagesRef = alloc.alloc();

        // Fonts
        Map<usize, Pdf::Ref> fontManagerId2FontObjRef;
        for (auto const& [_, value] : fontManager.mapping.iterItems()) {
            auto& [id, fontFace] = value;

            if (not fontFace.is<Font::Ttf::Fontface>()) {
                panic("no support for printing fonts other than TrueType");
            }

            TrueTypeFontAdapter ttfAdapter{
                fontFace.cast<Font::Ttf::Fontface>().unwrap(),
                alloc
            };

            auto fontRef = ttfAdapter.addToFile(file);
            fontManagerId2FontObjRef.put(id, fontRef);
        }

        // Graphical States
        Pdf::Dict graphicalStatesDict;
        for (usize i = 0; i < graphicalStates.len(); ++i) {
            auto stateRef = alloc.alloc();
            file.add(
                stateRef,
                Pdf::Dict{
                    {"Type"s, Pdf::Name{"ExtGState"s}},
                    {"ca"s, graphicalStates[i].opacity},
                }
            );

            graphicalStatesDict.put(
                Pdf::Name{Io::format("GS{}", i)},
                stateRef
            );
        }

        // Page
        for (auto& p : _pages) {
            Pdf::Ref pageRef = alloc.alloc();
            Pdf::Ref contentsRef = alloc.alloc();

            // FIXME: adding all fonts for now on each page; later, we will need to filter by page
            Pdf::Dict pageFontsDict;
            for (auto const& [managerId, objRef] : fontManagerId2FontObjRef.iterItems()) {
                auto formattedName = Io::format("F{}", managerId);
                pageFontsDict.put(formattedName.str(), objRef);
            }

            file.add(
                pageRef,
                Pdf::Dict{
                    {"Type"s, Pdf::Name{"Page"s}},
                    {"Parent"s, pagesRef},
                    {"MediaBox"s,
                     Pdf::Array{
                         usize{0},
                         usize{0},
                         // Convert fron the karm-pdf internal units to PDF units (1/72 inch)
                         p.size.width * (72.0 / DPI.cast<f64>()),
                         p.size.height * (72.0 / DPI.cast<f64>()),
                     }},
                    {
                        "Contents"s,
                        contentsRef,
                    },
                    {
                        "Resources"s,
                        Pdf::Dict{
                            {
                                "Font"s,
                                pageFontsDict,
                            },
                            {"ExtGState"s,
                             graphicalStatesDict}
                        },
                    }
                }
            );

            file.add(
                contentsRef,
                Pdf::Stream{
                    .dict = Pdf::Dict{
                        {"Length"s, p.data.bytes().len()},
                    },
                    .data = p.data.bytes(),
                }
            );

            pagesKids.pushBack(pageRef);
        }

        // Pages
        file.add(
            pagesRef,
            Pdf::Dict{
                {"Type"s, Pdf::Name{"Pages"s}},
                {"Count"s, _pages.len()},
                {"Kids"s, std::move(pagesKids)},
            }
        );

        // Catalog
        auto catalogRef = file.add(
            alloc.alloc(),
            Pdf::Dict{
                {"Type"s, Pdf::Name{"Catalog"s}},
                {"Pages"s, pagesRef},
            }
        );

        // Trailer
        file.trailer = Pdf::Dict{
            {"Size"s, file.body.len() + 1},
            {"Root"s, catalogRef},
        };

        return file;
    }

    Res<> write(Io::Writer& w) override {
        return pdf().write(w);
    }
};

} // namespace Karm::Print
