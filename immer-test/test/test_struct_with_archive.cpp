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

struct archives_save
{
    immer_archive::archive_save<int> ints;
    immer_archive::archive_save<std::string> strings;

    template <class Archive>
    void save(Archive& ar) const
    {
        ar(CEREAL_NVP(ints), CEREAL_NVP(strings));
    }
};

template <class T>
immer_archive::archive_save<T>& get_save_archive(archives_save& ars);

template <>
immer_archive::archive_save<int>& get_save_archive(archives_save& ars)
{
    return ars.ints;
}

template <>
immer_archive::archive_save<std::string>& get_save_archive(archives_save& ars)
{
    return ars.strings;
}

struct archives_load
{
    immer_archive::archive_load<int> ints;
    immer_archive::archive_load<std::string> strings;

    std::optional<immer_archive::loader<int>> int_loader;
    std::optional<immer_archive::loader<std::string>> strings_loader;

    template <class Archive>
    void load(Archive& ar)
    {
        ar(CEREAL_NVP(ints), CEREAL_NVP(strings));
    }
};

template <class T>
immer_archive::loader<T>& get_loader(archives_load& ars);

template <>
immer_archive::loader<int>& get_loader(archives_load& ars)
{
    if (!ars.int_loader) {
        ars.int_loader.emplace(ars.ints);
    }
    return *ars.int_loader;
}

template <>
immer_archive::loader<std::string>& get_loader(archives_load& ars)
{
    if (!ars.strings_loader) {
        ars.strings_loader.emplace(ars.strings);
    }
    return *ars.strings_loader;
}

template <typename T>
std::pair<std::string, archives_save>
to_json_with_archive(const T& serializable)
{
    auto archives = archives_save{};
    auto os       = std::ostringstream{};
    {
        auto ar = immer_archive::json_immer_output_archive<archives_save>{os};
        ar(serializable);
        archives = ar.get_archives();
    }
    return {os.str(), archives};
}

template <typename T>
T from_json_with_archive(const std::string& input)
{
    const auto archives = [&input] {
        auto is       = std::istringstream{input};
        auto ar       = cereal::JSONInputArchive{is};
        auto archives = archives_load{};
        ar(CEREAL_NVP(archives));
        return archives;
    }();

    auto is = std::istringstream{input};
    auto ar =
        immer_archive::json_immer_input_archive<archives_load>{archives, is};
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

    const auto [json_str, archives] = to_json_with_archive(test1);
    SECTION("Try to save and load the archive")
    {
        const auto archives_json = [&archives = archives] {
            auto os = std::ostringstream{};
            {
                auto ar = cereal::JSONOutputArchive{os};
                ar(123);
                ar(CEREAL_NVP(archives));
            }
            return os.str();
        }();
        // REQUIRE(archives_json == "");
        const auto archives_loaded = [&archives_json] {
            auto is       = std::istringstream{archives_json};
            auto ar       = cereal::JSONInputArchive{is};
            auto archives = archives_load{};
            ar(CEREAL_NVP(archives));
            return archives;
        }();
        REQUIRE(archives_loaded.ints.leaves.size() == 2);
    }

    // REQUIRE(json_str == "");

    {
        auto full_load = from_json_with_archive<test_data>(json_str);
        REQUIRE(full_load == test1);
    }
}
