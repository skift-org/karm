export module Karm.Core:base.match;

namespace Karm {

export template <typename T, typename... Args>
constexpr bool oneOf(T const& val, Args const&... args) {
    return ((val == args) or ...);
}

export template <typename T, typename... Args>
constexpr bool allOf(T const& val, Args const&... args) {
    return ((val == args) and ...);
}

} // namespace Karm
