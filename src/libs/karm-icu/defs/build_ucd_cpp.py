import sys
import requests

UNICODE_MAX = 0x10FFFF
BLOCK_SIZE = 128

UNICODE_DATABASE_URL = "https://www.unicode.org/Public/16.0.0/ucd/UnicodeData.txt"


def load_unicode_bidi_table():
    table = []
    try:
        with requests.get(UNICODE_DATABASE_URL, stream=True) as r:
            r.raise_for_status() 
            for line_bytes in r.iter_lines():
                line = line_bytes.decode('utf-8') 
                idx = int(line.split(';')[0], base=16)
                type = line.split(';')[4]
                table.append((idx, type))
    except requests.exceptions.RequestException as e:
        print(f"Error making HTTP request: {e}")
        raise e
  
    return table


def compute_block_mappings(block_size, plane_table):
    block_mappings = dict()
    for idx, type in plane_table:
        block_mappings.setdefault(idx // block_size, dict())[idx % block_size] = type

    unique_block_mappings = list(set(
        frozenset(map_.items())
        for map_ in block_mappings.values()
    ))

    unique_block_mappings_to_id = {map_: i for i, map_ in enumerate(unique_block_mappings)}

    return block_mappings, unique_block_mappings, unique_block_mappings_to_id


def compute_mapping_tables(block_size, table, space_size):
    def id_of_mapping(mapping):
        return unique_block_mappings_to_id[frozenset(mapping.items())]

    block_mappings, unique_block_mappings, unique_block_mappings_to_id = compute_block_mappings(block_size, table)
    num_of_blocks = (space_size + block_size - 1) // block_size

    mapping_ids = []
    for block_idx in range(num_of_blocks):
        mapping_ids.append(id_of_mapping(block_mappings[block_idx]) if block_idx in block_mappings else 0)

    mappings = []
    for i, frozen_block in enumerate(unique_block_mappings):
        mapping = []
        block = dict(frozen_block)
        for j in range(block_size):
            mapping.append(block.get(j, 'L'))
        mappings += mapping

    return mapping_ids, mappings

def format_file(params):
    return f"""
module;

#include <karm-base/rune.h>

export module Karm.Icu:ucd;

import :base;

namespace Karm::Icu {{
// Unicode Character Database
// https://unicode.org/reports/tr44/

namespace Bidi {{
constexpr u8 BLOCK_SIZE = {params["bidi_block_size"]};
constexpr u8 mappingIdxs[] = {{{params["bidi_mapping_idxs"]}}};
constexpr BidiType types[] = {{{params["bidi_types"]}}};

}} // namespace Bidi

BidiType getBidiType(Rune r) {{
    usize blockIdx = r / Bidi::BLOCK_SIZE;
    usize mappingIdx = Bidi::mappingIdxs[blockIdx];
    return Bidi::types[mappingIdx * Bidi::BLOCK_SIZE + (r % Bidi::BLOCK_SIZE)];
}}

}} // namespace Karm::Icu

"""

def build_ucd_cpp(ucd_cpp_path):
    table = load_unicode_bidi_table()

    with open(ucd_cpp_path, "w") as f:
        mapping_ids, mappings = compute_mapping_tables(BLOCK_SIZE, table, UNICODE_MAX + 1)
        f.write(format_file({
            "bidi_block_size": BLOCK_SIZE,
            "bidi_mapping_idxs": ', '.join(map(str, mapping_ids)),
            "bidi_types": ', '.join(f"BidiType::{c}" for c in mappings),
        }))

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python test.py <output_file>")
        sys.exit(1)

    build_ucd_cpp(sys.argv[1])



