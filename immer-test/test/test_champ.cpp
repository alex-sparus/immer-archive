#include <catch2/catch_test_macros.hpp>

#include <immer_champ_load.hpp>
#include <immer_champ_save.hpp>

#include "utils.hpp"

using namespace test;

namespace {

const auto gen_set = [](auto set, int count) {
    for (int i = 0; i < count; ++i) {
        set = std::move(set).insert(fmt::format("{}", i));
    }
    return set;
};

const auto into_set = [](const auto& set) {
    using T     = typename std::decay_t<decltype(set)>::value_type;
    auto result = immer::set<T>{};
    for (const auto& item : set) {
        result = std::move(result).insert(item);
    }
    return result;
};

struct broken_hash
{
    std::size_t operator()(const std::string& str) const
    {
        if ("10" < str && str < "19") {
            return 123;
        }
        return std::hash<std::string>{}(str);
    }
};

} // namespace

TEST_CASE("Test saving a set")
{
    const auto set = gen_set(immer::set<std::string, broken_hash>{}, 200);
    const auto [ar, set_id] = immer_archive::champ::save_set(set, {});
    const auto ar_str       = to_json(ar);
    // REQUIRE(ar_str == "");

    const auto loaded_archive =
        from_json<immer_archive::champ::archive_load<std::string>>(ar_str);
    REQUIRE(loaded_archive.sets.size() == 1);

    auto loader =
        immer_archive::champ::loader<std::string, broken_hash>{loaded_archive};
    const auto loaded = loader.load_set(set_id);
    REQUIRE(loaded.has_value());
    REQUIRE(into_set(set).size() == set.size());
    for (const auto& item : set) {
        // This is the only thing that actually breaks if the hash of the loaded
        // set is not the same as the hash function of the serialized set.
        REQUIRE(loaded->count(item));
    }
}

TEST_CASE("Test archive conversion, no json")
{
    const auto set    = gen_set(immer::set<std::string, broken_hash>{}, 200);
    const auto set2   = gen_set(set, 300);
    auto [ar, set_id] = immer_archive::champ::save_set(set, {});
    auto set2_id      = immer_archive::champ::node_id{};
    std::tie(ar, set2_id) = immer_archive::champ::save_set(set2, ar);

    auto loader = immer_archive::champ::loader<std::string, broken_hash>{
        to_load_archive(ar)};

    const auto check_set = [&loader](auto id, const auto& expected) {
        const auto loaded = loader.load_set(id);
        REQUIRE(loaded.has_value());
        REQUIRE(into_set(*loaded) == into_set(expected));
        REQUIRE(into_set(expected).size() == expected.size());
        for (const auto& item : expected) {
            // This is the only thing that actually breaks if the hash of the
            // loaded set is not the same as the hash function of the serialized
            // set.
            REQUIRE(loaded->count(item));
        }
        for (const auto& item : *loaded) {
            REQUIRE(expected.count(item));
        }
    };

    check_set(set_id, set);
    check_set(set2_id, set2);
}
