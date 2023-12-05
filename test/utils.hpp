#pragma once

#include <immer-archive/rbts/archive.hpp>

#include <sstream>

#include <cereal/archives/json.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>

#include <fmt/format.h>

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

struct test_value
{
    std::size_t id;
    std::string value;

    auto tie() const { return std::tie(id, value); }

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(id), CEREAL_NVP(value));
    }

    friend bool operator==(const test_value& left, const test_value& right)
    {
        return left.tie() == right.tie();
    }

    friend std::ostream& operator<<(std::ostream& s, const test_value& value)
    {
        return s << fmt::format("({}, {})", value.id, value.value);
    }
};

} // namespace test
