export module Karm.Sys:defs;

import Karm.Core;

namespace Karm::Sys {

#ifdef __ck_sys_encoding_utf8__
export using Encoding = Utf8;
#elifdef __ck_sys_encoding_utf16__
export using Encoding = Utf16;
#elifdef __ck_sys_encoding_utf32__
export using Encoding = Utf32;
#elifdef __ck_sys_encoding_ascii__
export using Encoding = Ascii;
#else
#    error "Unknown encoding"
#endif

#ifdef __ck_sys_line_ending_lf__
export constexpr char const* LINE_ENDING = "\n";
#elifdef __ck_sys_line_ending_crlf__
export constexpr char const* LINE_ENDING = "\r\n";
#else
#    error "Unknown line ending"
#endif

#ifdef __ck_sys_path_separator_slash__
export constexpr char const* PATH_SEPARATOR = "/";
#elifdef __ck_sys_path_separator_backslash__
export constexpr char const* PATH_SEPARATOR = "\\";
#else
#    error "Unknown path separator"
#endif

} // namespace Karm::Sys
