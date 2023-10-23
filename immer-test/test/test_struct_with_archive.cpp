#include <catch2/catch_test_macros.hpp>

#include "test/test_data.hpp"
#include <test/utils.hpp>
#include <with_archive_adapter.hpp>

using namespace test;

TEST_CASE("Test saving and loading a structure")
{
    const auto ints1 = gen(example_vector{}, 3);
    const auto test1 = test::info{
        .ints    = ints1,
        .strings = {"one", "two"},
    };

    // XXX We need a type-level function make_with_archive<T>, generating
    // test::with_archive for a list of types to archive.

    auto full = test::with_archive{
        .data = test1,
    };
    full.inject_save();
    const auto json_str = to_json(full);
    // REQUIRE(json_str == "");

    {
        auto full_load = from_json<test::with_archive>(json_str);
        full_load.load_from_archive();
        // INFO(to_json(full_load.data));
        // INFO(to_json(test1));
        REQUIRE(full_load.data == test1);
    }
}

namespace {

struct test_data
{
    immer_archive::vector_one_archivable<int> ints;
    immer_archive::vector_one_archivable<std::string> strings;

    auto tie() const { return std::tie(ints, strings); }

    friend bool operator==(const test_data& left, const test_data& right)
    {
        return left.tie() == right.tie();
    }

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(ints), CEREAL_NVP(strings));
    }
};

template <typename T>
std::string to_json_with_archive(const T& serializable)
{
    auto os = std::ostringstream{};
    {
        auto ar = immer_archive::with_archives_adapter_save<
            immer_archive::archives_save,
            cereal::JSONOutputArchive>{os};
        ar(serializable);
    }
    return os.str();
}

template <typename T>
T from_json_with_archive(std::string input)
{
    auto is = std::istringstream{input};
    auto ar =
        immer_archive::with_archives_adapter_load<immer_archive::archives_load,
                                                  cereal::JSONInputArchive>{is};
    auto r = T{};
    ar(r);
    return r;
}

} // namespace

TEST_CASE("Save with a special archive")
{
    const auto ints1 = gen(example_vector{}, 3);
    const auto test1 = test_data{
        .ints    = ints1,
        .strings = {"one", "two"},
    };

    const auto json_str = to_json_with_archive(test1);
//    REQUIRE(json_str == "");

    {
        auto full_load = from_json_with_archive<test_data>(json_str);
        REQUIRE(full_load == test1);
    }
}
