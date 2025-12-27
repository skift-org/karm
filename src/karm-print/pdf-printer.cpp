module;

#include <karm-font/ttf/fontface.h>

export module Karm.Print:pdfPrinter;

import Karm.Archive;
import Karm.Pdf;

import :filePrinter;
import :pdfFonts;

namespace Karm::Print {

struct PdfPage {
    PaperStock paper;
    Io::StringWriter data;
};

export struct PdfPrinter : FilePrinter {
    Vec<PdfPage> _pages;
    Opt<Pdf::Canvas> _canvas;
    Pdf::FontManager fontManager;
    Pdf::ImageManager imageManager;
    Vec<Pdf::GraphicalStateDict> graphicalStates;

    Gfx::Canvas& beginPage(PaperStock paper) override {
        auto& page = _pages.emplaceBack(paper);
        _canvas = Pdf::Canvas{page.data, paper.size(), &fontManager, &imageManager, graphicalStates};

        // Convert fron the karm-pdf internal units to PDF units (1/72 inch)
        _canvas->scale(72.0 / DPI);

        // NOTE: PDF has the coordinate system origin at the bottom left corner.
        //       But we want to have it at the top left corner.
        _canvas->transform(
            {1, 0, 0, -1, 0, paper.height}
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
        for (auto& [_, value] : fontManager.mapping._els) {
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

        // Images
        Pdf::Dict xObjectDict;
        for (auto& [id, surface]: imageManager.mapping.iterUnordered()) {
            auto getColorSpaceName = [](Gfx::Fmt fmt) -> Opt<Pdf::Name> {
                return fmt.visit(Visitor{
                    [](Gfx::Rgb888) -> Opt<Pdf::Name> {
                        return "DeviceRGB"s;
                    },
                    [](Gfx::Greyscale8) -> Opt<Pdf::Name> {
                        return "DeviceGray"s;
                    },
                    [](auto) -> Opt<Pdf::Name> {
                        return NONE;
                    }
                });
            };

            auto deflateBytes = [](Bytes bytes) -> Buf<u8> {
                auto alphaReader = Io::BufReader(bytes);
                auto alphaWriter = Io::BufferWriter(bytes.len());
                Archive::deflate(alphaReader, alphaWriter).unwrap();
                return alphaWriter.take();
            };

            auto imageStreamParams =
                Pdf::Dict{
                                    {"Type"s, Pdf::Name{"XObject"s}},
                                    {"Subtype"s, Pdf::Name{"Image"s}},
                                    {"Width"s, surface->width()},
                                    {"Height"s, surface->height()},
                                    {"BitsPerComponent"s, surface->fmt().bpc()},
                };

            struct PreparedImage {
                Pdf::Name filterName;
                Pdf::Name colorSpaceName;
                Buf<u8> rawImage;
                Opt<Buf<u8>> alphaMask;
            };

            auto prepareEmbeddableImage = [&]() -> Opt<PreparedImage> {
                if (surface->mimeData()) {
                    auto& mimeData = *surface->mimeData();

                    auto colorSpaceName = try$(getColorSpaceName(surface->fmt()));

                    if (mimeData.uti == Ref::Uti::PUBLIC_JPEG) {
                        return PreparedImage{
                            .filterName = "DCTDecode"s,
                            .colorSpaceName = colorSpaceName,
                            .rawImage = mimeData.buf,
                            .alphaMask = NONE,
                        };
                    }
                }

                return NONE;
            };

            auto deflateFallback = [&]() -> PreparedImage {
                auto imageSurface = surface;
                Opt<Buf<u8>> alphaMask = NONE;

                auto colorSpaceName = getColorSpaceName(surface->fmt());
                if (!colorSpaceName) {
                    colorSpaceName = "DeviceRGB"s,
                    imageSurface = surface->convert(Gfx::RGB888);

                    if (auto alpha = surface->extractAlpha()) {
                        alphaMask = deflateBytes(*alpha);
                    }
                }

                return PreparedImage{
                    .filterName = "FlateDecode"s,
                    .colorSpaceName = colorSpaceName.take(),
                    .rawImage = deflateBytes(imageSurface->pixels().bytes()),
                    .alphaMask = alphaMask,
                };
            };

            Opt<Pdf::Name> filterName = NONE;
            Opt<Buf<u8>> image = NONE;
            Opt<Buf<u8>> alphaMask = NONE;
            Opt<Pdf::Name> colorSpaceName = NONE;

            if (surface->mimeData()) {
                auto& mimeData = *surface->mimeData();
                imageStreamParams.put("Length"s, static_cast<isize>(mimeData.buf.size()));

                if (mimeData.uti == Ref::Uti::PUBLIC_JPEG) {
                    filterName = "DCTDecode"s;
                    image = mimeData.buf; // FIXME: Is it ok to move and leave the surface in an invalid state ?
                    colorSpaceName = getColorSpaceName(surface->fmt());
                }
            }

            if (not image or not colorSpaceName) {
                auto imageSurface = surface;
                // To here (decode fallback)
                if (not surface->fmt().is<Gfx::Rgb888>() and not surface->fmt().is<Gfx::Greyscale8>()) {
                    imageSurface = surface->convert(Gfx::RGB888);

                    if (auto alpha = surface->extractAlpha()) {
                        alphaMask = deflateBytes(*alpha);
                    }
                }

                image = deflateBytes(imageSurface->pixels().bytes());
                colorSpaceName = getColorSpaceName(imageSurface->fmt());
            }

            if (not image or not colorSpaceName) {
                logError("skipping invalid image");
                continue;
            }

            imageStreamParams.put("Length"s, image->len());
            imageStreamParams.put("ColorSpace"s, colorSpaceName.take());

            if (filterName) {
                imageStreamParams.put("Filter"s, filterName.take());
            }

            if (alphaMask) {
                auto smaskRef = alloc.alloc();

                imageStreamParams.put("SMask"s, smaskRef);

                auto deflatedAlphaMask = deflateBytes(*alphaMask);
                auto len = deflatedAlphaMask.len();

                file.add(smaskRef, Pdf::Stream{
                    .dict = Pdf::Dict{
                        {"Type"s, Pdf::Name{"XObject"s}},
                        {"Subtype"s, Pdf::Name{"Image"s}},
                        {"Width"s, surface->width()},
                        {"Height"s, surface->height()},
                        {"ColorSpace"s, Pdf::Name{"DeviceGray"s}},
                        {"BitsPerComponent"s, Gfx::GREYSCALE8.bpc()},
                        {"Filter"s, Pdf::Name{"FlateDecode"s}},
                        {"Length"s, len}
                    },
                    .data = std::move(deflatedAlphaMask)
                });
            }

            auto xObjectRef = alloc.alloc();

            file.add(
                xObjectRef,
                Pdf::Stream{
                    .dict = imageStreamParams,
                    .data = image.take(),
                }
            );

            xObjectDict.put(Pdf::Name{Io::format("Im{}", id)}, xObjectRef);
        }

        // Page
        for (auto& p : _pages) {
            Pdf::Ref pageRef = alloc.alloc();
            Pdf::Ref contentsRef = alloc.alloc();

            // FIXME: adding all fonts for now on each page; later, we will need to filter by page
            Pdf::Dict pageFontsDict;
            for (auto& [managerId, objRef] : fontManagerId2FontObjRef._els) {
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
                         p.paper.width * (72.0 / DPI),
                         p.paper.height * (72.0 / DPI),
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
                             graphicalStatesDict},
                            {"XObject"s,
                             xObjectDict},
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

        // Sorting object by their refs, so they are printed in order
        sort(file.body._els, [](auto& a, auto& b) {
            return a.v0.num <=> b.v0.num;
        });

        return file;
    }

    Res<> write(Io::Writer& w) override {
        return pdf().write(w);
    }
};

} // namespace Karm::Print
