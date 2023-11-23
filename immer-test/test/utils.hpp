#pragma once

#include <immer-archive/json/json_immer.hpp>
#include <immer-archive/rbts/archive.hpp>

#include <sstream>

#include <cereal/archives/json.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>

namespace test {

using example_vector      = immer_archive::vector_one<int>;
using example_flex_vector = immer_archive::flex_vector_one<int>;

const auto gen = [](auto init, int count) {
    for (int i = 0; i < count; ++i) {
        init = std::move(init).push_back(i);
    }
    return init;
};

template <typename T>
std::string to_json(const T& serializable)
{
    auto os = std::ostringstream{};
    {
        auto ar = cereal::JSONOutputArchive{os};
        ar(serializable);
    }
    return os.str();
}

template <typename T>
T from_json(std::string input)
{
    auto is = std::istringstream{input};
    auto ar = cereal::JSONInputArchive{is};
    auto r  = T{};
    ar(r);
    return r;
}

} // namespace test
