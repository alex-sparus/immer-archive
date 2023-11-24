#pragma once

#include <fmt/format.h>
#include <fmt/ranges.h>

namespace immer_archive {

inline std::string to_string(int val) { return fmt::format("{}", val); }

inline std::string to_string(const std::string& str) { return str; }

template <typename T,
          typename MemoryPolicy         = immer::default_memory_policy,
          immer::detail::rbts::bits_t B = immer::default_bits>
std::string to_string(const immer::vector<T, MemoryPolicy, B, 1>& value)
{
    std::vector<std::string> str;
    for (const auto& item : value) {
        str.push_back(to_string(item));
    }
    return fmt::format("[{}]", fmt::join(str, ", "));
}

template <typename T>
std::string to_string(const immer::array<T>& value)
{
    std::vector<std::string> str;
    for (const auto& item : value) {
        str.push_back(to_string(item));
    }
    return fmt::format(
        "array id {} [{}]", value.identity(), fmt::join(str, ", "));
}

template <typename T,
          typename MemoryPolicy         = immer::default_memory_policy,
          immer::detail::rbts::bits_t B = immer::default_bits>
std::string to_string(const immer::flex_vector<T, MemoryPolicy, B, 1>& value)
{
    std::vector<std::string> str;
    for (const auto& item : value) {
        str.push_back(to_string(item));
    }
    return fmt::format("[{}]", fmt::join(str, ", "));
}

template <typename K,
          typename T,
          typename Hash,
          typename Equal,
          typename MemoryPolicy,
          immer::detail::hamts::bits_t B>
std::string
to_string(const immer::map<K, T, Hash, Equal, MemoryPolicy, B>& value)
{
    std::vector<std::string> str;
    for (const auto& item : value) {
        str.push_back(to_string(item.first) + ": " + to_string(item.second));
    }
    return fmt::format("[{}]", fmt::join(str, ", "));
}

} // namespace immer_archive
