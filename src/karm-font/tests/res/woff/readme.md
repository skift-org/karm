# WOFF test fonts

Small fonts for the WOFF decoders (`test-woff.cpp`) and the TrueType
subsetter (`test-subset.cpp`). They are all the same made-up font, family
name "Karm Woff Test", going through different encoders.

| File         | What it is                                                         |
| ------------ | ------------------------------------------------------------------ |
| `tiny.woff2` | Written by `woff2_compress`, the reference encoder (google/woff2). |
| `tiny.ttf`   | What `woff2_decompress` makes of `tiny.woff2`.                     |
| `tiny.woff`  | `tiny.ttf` wrapped table by table, in the order of the file.       |
| `hmtx.woff2` | Written by fontTools, with the `hmtx` transform on.                |
| `hmtx.ttf`   | What `woff2_decompress` makes of `hmtx.woff2`.                     |

The `.ttf` files are what the decoders are expected to give back, byte for
byte. They come from the reference decoder rather than from the font the
encoders were given, as going through WOFF2 doesn't give the same file back:
tables get reordered, checksums recomputed and glyphs rewritten.

## What is in the font

Each glyph is there to take the WOFF2 `glyf` transform down a different path.

| Id | Glyph     | Char | Why                                                                  |
| -- | --------- | ---- | -------------------------------------------------------------------- |
| 0  | `.notdef` |      | Two contours.                                                        |
| 1  | `space`   | ` `  | No outline, and a different advance than the others.                 |
| 2  | `A`       | `A`  | Curves, instructions, coordinate deltas of one and two bytes.        |
| 3  | `acute`   | `´`  | In `tiny`, a bounding box that isn't the one of its points.          |
| 4  | `Aacute`  | `Á`  | Composite of `A` and a scaled `acute`, with instructions.            |
| 5  | `stairs`  | `s`  | 280 points in a row with the same flags, more than one repeat holds. |
| 6  | `wide`    | `w`  | Far away points, deltas of three and four bytes, negative bearing.   |
| 7  | `AA`      | `Å`  | Composite of `A`, a sheared `A` and `Aacute`, itself a composite.    |

`tiny` and `hmtx` differ on two points:

- `tiny` has the bounding box of `acute` tampered with, so that the encoder
  has to store it instead of leaving it to the decoder to work out.
- `hmtx` leaves its left side bearings out, which only works when each of
  them is the `xMin` of its glyph. `woff2_compress` never does it, fontTools
  does when asked to.

## Regenerating

```sh
python3 make_fixtures.py
```

It needs:

- Python 3 with [fontTools](https://github.com/fonttools/fonttools) and the
  `brotli` module, which fontTools wants to write WOFF2.
- `woff2_compress` and `woff2_decompress` from
  [google/woff2](https://github.com/google/woff2), in the `PATH`.

The files here were made with fontTools 4.68.0, brotli 1.2.1 and woff2 1.0.2,
with which the script gives the very same files back. Other versions might
compress differently: the `.woff` and `.woff2` files can then come out
different while still being valid. The tests only rely on each of them
decoding to its `.ttf`, and on what the font holds: the family name, the
characters above and what the composites are made of.

`tiny.woff` is not written by fontTools as it sorts the tables by tag,
`make_fixtures.py` wraps them itself to keep them in the order of the file.
