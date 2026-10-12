export module Karm.Image:png.base;

import Karm.Core;

// Definitions shared by the PNG decoder and encoder.
// https://www.w3.org/TR/2003/REC-PNG-20031110/
namespace Karm::Image::Png {

export enum struct ColorType : u8 {
    GREYSCALE = 0,
    TRUECOLOR = 2,
    INDEXED = 3,
    GREYSCALE_ALPHA = 4,
    TRUECOLOR_ALPHA = 6,

    _LEN
};

export enum struct CompressionMethod : u8 {
    DEFLATE = 0,

    _LEN
};

export enum struct FilterMethod : u8 {
    STANDARD = 0,

    _LEN
};

export enum struct Filter : u8 {
    NONE,
    SUB,
    UP,
    AVERAGE,
    PAETH,

    _LEN
};

export enum struct InterlacingMethod : u8 {
    NULL,
    ADAM7,

    _LEN,
};

// https://www.w3.org/TR/2003/REC-PNG-20031110/#8Interlace
export struct Adam7Pass {
    usize startX;
    usize startY;
    usize stepX;
    usize stepY;
};

export constexpr Array<Adam7Pass, 7> ADAM7_PASSES = {{
    {0, 0, 8, 8},
    {4, 0, 8, 8},
    {0, 4, 4, 8},
    {2, 0, 4, 4},
    {0, 2, 2, 4},
    {1, 0, 2, 2},
    {0, 1, 1, 2},
}};

export constexpr Array<u8, 8> SIG = {
    0x89, 0x50, 0x4E, 0x47,
    0x0D, 0x0A, 0x1A, 0x0A
};

export constexpr Str IHDR = "IHDR";
export constexpr Str PLTE = "PLTE";
export constexpr Str TRNS = "tRNS";
export constexpr Str IDAT = "IDAT";
export constexpr Str IEND = "IEND";

} // namespace Karm::Image::Png
