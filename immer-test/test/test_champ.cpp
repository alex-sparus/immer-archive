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

const auto gen_map = [](auto map, int count) {
    for (int i = 0; i < count; ++i) {
        map = std::move(map).set(i, fmt::format("_{}_", i));
    }
    return map;
};

const auto into_set = [](const auto& set) {
    using T     = typename std::decay_t<decltype(set)>::value_type;
    auto result = immer::set<T>{};
    for (const auto& item : set) {
        result = std::move(result).insert(item);
    }
    return result;
};

const auto into_map = [](const auto& map) {
    using K     = typename std::decay_t<decltype(map)>::key_type;
    using V     = typename std::decay_t<decltype(map)>::mapped_type;
    auto result = immer::map<K, V>{};
    for (const auto& item : map) {
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

    std::size_t operator()(int map_key) const
    {
        if (10 < map_key && map_key < 19) {
            return 123;
        }
        return std::hash<int>{}(map_key);
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
        from_json<immer_archive::champ::set_archive_load<std::string>>(ar_str);
    REQUIRE(loaded_archive.sets.size() == 1);

    auto loader = immer_archive::champ::set_loader<std::string, broken_hash>{
        loaded_archive};
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

    auto loader = immer_archive::champ::set_loader<std::string, broken_hash>{
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

TEST_CASE("Test save mutated set")
{
    auto set          = gen_set(immer::set<std::string, broken_hash>{}, 200);
    auto [ar, set_id] = immer_archive::champ::save_set(set, {});

    set                   = std::move(set).insert("435");
    auto set_id2          = immer_archive::champ::node_id{};
    std::tie(ar, set_id2) = immer_archive::champ::save_set(set, ar);

    REQUIRE(set_id != set_id2);
}

TEST_CASE("Test saving a map")
{
    const auto map = gen_map(immer::map<int, std::string, broken_hash>{}, 200);
    const auto [ar, map_id] = immer_archive::champ::save_map(map, {});
    const auto ar_str       = to_json(ar);
    // REQUIRE(ar_str == "");

    const auto loaded_archive =
        from_json<immer_archive::champ::map_archive_load<int, std::string>>(
            ar_str);
    REQUIRE(loaded_archive.maps.size() == 1);

    auto loader =
        immer_archive::champ::map_loader<int, std::string, broken_hash>{
            loaded_archive};
    const auto loaded = loader.load_map(map_id);
    REQUIRE(loaded.has_value());
    REQUIRE(into_map(map).size() == map.size());
    for (const auto& [key, value] : map) {
        // This is the only thing that actually breaks if the hash of the loaded
        // map is not the same as the hash function of the serialized map.
        REQUIRE(loaded->count(key));
        REQUIRE(loaded.value()[key] == value);
    }
}

TEST_CASE("Test map archive conversion, no json")
{
    const auto map  = gen_map(immer::map<int, std::string, broken_hash>{}, 200);
    const auto map2 = gen_map(map, 300);
    auto [ar, map_id]     = immer_archive::champ::save_map(map, {});
    auto map2_id          = immer_archive::champ::node_id{};
    std::tie(ar, map2_id) = immer_archive::champ::save_map(map2, ar);

    auto loader =
        immer_archive::champ::map_loader<int, std::string, broken_hash>{
            to_load_archive(ar)};

    const auto check_map = [&loader](auto id, const auto& expected) {
        const auto loaded = loader.load_map(id);
        REQUIRE(loaded.has_value());
        REQUIRE(into_map(*loaded) == into_map(expected));
        REQUIRE(into_map(expected).size() == expected.size());
        for (const auto& [key, value] : expected) {
            // This is the only thing that actually breaks if the hash of the
            // loaded map is not the same as the hash function of the serialized
            // map.
            REQUIRE(loaded->count(key));
            REQUIRE(loaded.value()[key] == value);
        }
        for (const auto& [key, value] : *loaded) {
            REQUIRE(expected.count(key));
            REQUIRE(expected[key] == value);
        }
    };

    check_map(map_id, map);
    check_map(map2_id, map2);
}

TEST_CASE("Test save mutated map")
{
    auto map = gen_map(immer::map<int, std::string, broken_hash>{}, 200);
    auto [ar, map_id] = immer_archive::champ::save_map(map, {});

    map                   = std::move(map).set(999, "435");
    auto map_id2          = immer_archive::champ::node_id{};
    std::tie(ar, map_id2) = immer_archive::champ::save_map(map, ar);

    REQUIRE(map_id != map_id2);
}
