#include <catch2/catch_test_macros.hpp>

#include <immer-archive/champ/champ.hpp>
#include <immer-archive/xxhash/xxhash.hpp>

#include "utils.hpp"

#include <nlohmann/json.hpp>

using namespace test;
using json_t = nlohmann::json;

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

auto gen_table(auto table, std::size_t from, std::size_t to)
{
    for (auto i = std::min(from, to); i < std::max(from, to); ++i) {
        table = std::move(table).insert(test_value{i, fmt::format("_{}_", i)});
    }
    return table;
}

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
    using Container = immer::set<std::string, broken_hash>;

    const auto set          = gen_set(Container{}, 200);
    const auto [ar, set_id] = immer_archive::champ::save_to_archive(set, {});
    const auto ar_str       = to_json(ar);
    // REQUIRE(ar_str == "");

    SECTION("Load with the correct hash")
    {
        const auto loaded_archive =
            from_json<immer_archive::champ::container_archive_load<Container>>(
                ar_str);
        REQUIRE(loaded_archive.containers.size() == 1);

        auto loader = immer_archive::champ::container_loader{loaded_archive};
        const auto loaded = loader.load(set_id);
        REQUIRE(into_set(set).size() == set.size());
        for (const auto& item : set) {
            // This is the only thing that actually breaks if the hash of the
            // loaded set is not the same as the hash function of the serialized
            // set.
            REQUIRE(loaded.count(item));
        }
    }

    SECTION("Load with a different hash")
    {
        using WrongContainer      = immer::set<std::string>;
        const auto loaded_archive = from_json<
            immer_archive::champ::container_archive_load<WrongContainer>>(
            ar_str);
        REQUIRE(loaded_archive.containers.size() == 1);

        auto loader = immer_archive::champ::container_loader{loaded_archive};
        REQUIRE_THROWS_AS(
            loader.load(set_id),
            immer_archive::champ::hash_validation_failed_exception);
    }
}

TEST_CASE("Test set with xxHash")
{
    using Container =
        immer::set<std::string, immer_archive::xx_hash<std::string>>;

    const auto set          = gen_set(Container{}, 200);
    const auto [ar, set_id] = immer_archive::champ::save_to_archive(set, {});
    const auto ar_str       = to_json(ar);
    // REQUIRE(ar_str == "");

    SECTION("Load with the correct hash")
    {
        const auto loaded_archive =
            from_json<immer_archive::champ::container_archive_load<Container>>(
                ar_str);
        REQUIRE(loaded_archive.containers.size() == 1);

        auto loader = immer_archive::champ::container_loader{loaded_archive};
        const auto loaded = loader.load(set_id);
        REQUIRE(into_set(set).size() == set.size());
        for (const auto& item : set) {
            // This is the only thing that actually breaks if the hash of the
            // loaded set is not the same as the hash function of the serialized
            // set.
            REQUIRE(loaded.count(item));
        }
    }
}

TEST_CASE("Test archive conversion, no json")
{
    using Container = immer::set<std::string, broken_hash>;

    const auto set        = gen_set(Container{}, 200);
    const auto set2       = gen_set(set, 300);
    auto [ar, set_id]     = immer_archive::champ::save_to_archive(set, {});
    auto set2_id          = immer_archive::champ::node_id{};
    std::tie(ar, set2_id) = immer_archive::champ::save_to_archive(set2, ar);

    auto loader = immer_archive::champ::container_loader{to_load_archive(ar)};

    const auto check_set = [&loader](auto id, const auto& expected) {
        const auto loaded = loader.load(id);
        REQUIRE(into_set(loaded) == into_set(expected));
        REQUIRE(into_set(expected).size() == expected.size());
        for (const auto& item : expected) {
            // This is the only thing that actually breaks if the hash of the
            // loaded set is not the same as the hash function of the serialized
            // set.
            REQUIRE(loaded.count(item));
        }
        for (const auto& item : loaded) {
            REQUIRE(expected.count(item));
        }
    };

    check_set(set_id, set);
    check_set(set2_id, set2);
}

TEST_CASE("Test save mutated set")
{
    using Container = immer::set<std::string, broken_hash>;

    auto set          = gen_set(Container{}, 200);
    auto [ar, set_id] = immer_archive::champ::save_to_archive(set, {});

    set                   = std::move(set).insert("435");
    auto set_id2          = immer_archive::champ::node_id{};
    std::tie(ar, set_id2) = immer_archive::champ::save_to_archive(set, ar);

    REQUIRE(set_id != set_id2);
}

TEST_CASE("Test saving a map")
{
    using Container = immer::map<int, std::string, broken_hash>;

    const auto map          = gen_map(Container{}, 200);
    const auto [ar, map_id] = immer_archive::champ::save_to_archive(map, {});
    const auto ar_str       = to_json(ar);
    // REQUIRE(ar_str == "");

    SECTION("Load with the correct hash")
    {
        const auto loaded_archive =
            from_json<immer_archive::champ::container_archive_load<Container>>(
                ar_str);
        REQUIRE(loaded_archive.containers.size() == 1);

        auto loader = immer_archive::champ::container_loader{loaded_archive};
        const auto loaded = loader.load(map_id);
        REQUIRE(into_map(map).size() == map.size());
        for (const auto& [key, value] : map) {
            // This is the only thing that actually breaks if the hash of the
            // loaded map is not the same as the hash function of the serialized
            // map.
            REQUIRE(loaded.count(key));
            REQUIRE(loaded[key] == value);
        }
    }

    SECTION("Load with a different hash")
    {
        using WrongContainer      = immer::map<int, std::string>;
        const auto loaded_archive = from_json<
            immer_archive::champ::container_archive_load<WrongContainer>>(
            ar_str);
        REQUIRE(loaded_archive.containers.size() == 1);

        auto loader = immer_archive::champ::container_loader{loaded_archive};
        REQUIRE_THROWS_AS(
            loader.load(map_id),
            immer_archive::champ::hash_validation_failed_exception);
    }
}

TEST_CASE("Test map archive conversion, no json")
{
    using Container = immer::map<int, std::string, broken_hash>;

    const auto map        = gen_map(Container{}, 200);
    const auto map2       = gen_map(map, 300);
    auto [ar, map_id]     = immer_archive::champ::save_to_archive(map, {});
    auto map2_id          = immer_archive::champ::node_id{};
    std::tie(ar, map2_id) = immer_archive::champ::save_to_archive(map2, ar);

    auto loader = immer_archive::champ::container_loader{to_load_archive(ar)};

    const auto check_map = [&loader](auto id, const auto& expected) {
        const auto loaded = loader.load(id);
        REQUIRE(into_map(loaded) == into_map(expected));
        REQUIRE(into_map(expected).size() == expected.size());
        for (const auto& [key, value] : expected) {
            // This is the only thing that actually breaks if the hash of the
            // loaded map is not the same as the hash function of the serialized
            // map.
            REQUIRE(loaded.count(key));
            REQUIRE(loaded[key] == value);
        }
        for (const auto& [key, value] : loaded) {
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
    auto [ar, map_id] = immer_archive::champ::save_to_archive(map, {});

    map                   = std::move(map).set(999, "435");
    auto map_id2          = immer_archive::champ::node_id{};
    std::tie(ar, map_id2) = immer_archive::champ::save_to_archive(map, ar);

    REQUIRE(map_id != map_id2);
}

namespace {
template <class T1, class T2, class Verify>
void test_table_types(Verify&& verify)
{
    const auto t1 = gen_table(T1{}, 0, 100);
    const auto t2 = gen_table(t1, 200, 210);

    auto [ar, t1_id] = immer_archive::champ::save_to_archive(t1, {});

    auto t2_id          = immer_archive::champ::node_id{};
    std::tie(ar, t2_id) = immer_archive::champ::save_to_archive(t2, ar);

    const auto ar_str = to_json(ar);
    // REQUIRE(ar_str == "");

    const auto loaded_archive =
        from_json<immer_archive::champ::container_archive_load<T2>>(ar_str);

    auto loader = immer_archive::champ::container_loader{loaded_archive};

    const auto check = [&loader, &verify](auto id, const auto& expected) {
        const auto loaded = loader.load(id);
        verify(loaded, expected);
    };

    check(t1_id, t1);
    check(t2_id, t2);
}
} // namespace

TEST_CASE("Test saving a table")
{
    using different_table_t =
        immer::table<test_value, immer::table_key_fn, broken_hash>;
    using table_t = immer::table<test_value>;

    const auto verify_is_equal = [](const auto& loaded, const auto& expected) {
        for (const auto& item : expected) {
            INFO(item);
            REQUIRE(loaded.count(item.id));
            REQUIRE(loaded[item.id] == item);
        }
        for (const auto& item : loaded) {
            REQUIRE(expected[item.id] == item);
        }
    };

    // Verify that saving and loading with customized hash works.
    test_table_types<table_t, table_t>(verify_is_equal);
    test_table_types<different_table_t, different_table_t>(verify_is_equal);

    const auto test1 = [&] {
        test_table_types<different_table_t, table_t>(verify_is_equal);
    };
    const auto test2 = [&] {
        test_table_types<table_t, different_table_t>(verify_is_equal);
    };

    REQUIRE_THROWS_AS(test1(),
                      immer_archive::champ::hash_validation_failed_exception);
    REQUIRE_THROWS_AS(test2(),
                      immer_archive::champ::hash_validation_failed_exception);
}

TEST_CASE("Test saving a table, no json")
{
    using table_t = immer::table<test_value>;
    const auto t1 = gen_table(table_t{}, 0, 100);
    const auto t2 = gen_table(t1, 200, 210);

    auto [ar, t1_id] = immer_archive::champ::save_to_archive(t1, {});

    auto t2_id          = immer_archive::champ::node_id{};
    std::tie(ar, t2_id) = immer_archive::champ::save_to_archive(t2, ar);

    const auto ar_str = to_json(ar);
    // REQUIRE(ar_str == "");

    auto loader = immer_archive::champ::container_loader{to_load_archive(ar)};

    const auto check = [&loader](auto id, const auto& expected) {
        const auto loaded = loader.load(id);

        for (const auto& item : expected) {
            REQUIRE(loaded[item.id] == item);
        }
        for (const auto& item : loaded) {
            REQUIRE(expected[item.id] == item);
        }
    };

    check(t1_id, t1);
    check(t2_id, t2);
}

TEST_CASE("Test modifying set nodes")
{
    using Container =
        immer::set<std::string, immer_archive::xx_hash<std::string>>;

    // const auto set = gen_set(Container{}, 30);
    // const auto [ar, set_id] = immer_archive::champ::save_to_archive(set, {});
    // const auto ar_str       = to_json(ar);
    // REQUIRE(ar_str == "");

    auto data = json_t::parse(R"({
  "value0": {
    "nodes": {
      "inners": [
        {
          "key": 0,
          "value": {
            "values": ["15", "27", "6", "0", "1", "20", "4"],
            "children": [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11],
            "nodemap": 1343560972,
            "datamap": 19407009
          }
        },
        {
          "key": 1,
          "value": {
            "values": ["25", "24"],
            "children": [],
            "nodemap": 0,
            "datamap": 536875008
          }
        },
        {
          "key": 2,
          "value": {
            "values": ["14", "16"],
            "children": [],
            "nodemap": 0,
            "datamap": 16777224
          }
        },
        {
          "key": 3,
          "value": {
            "values": ["23", "26", "9"],
            "children": [],
            "nodemap": 0,
            "datamap": 2147747840
          }
        },
        {
          "key": 4,
          "value": {
            "values": ["13", "8"],
            "children": [],
            "nodemap": 0,
            "datamap": 16512
          }
        },
        {
          "key": 5,
          "value": {
            "values": ["28", "22"],
            "children": [],
            "nodemap": 0,
            "datamap": 8388640
          }
        },
        {
          "key": 6,
          "value": {
            "values": ["3", "19"],
            "children": [],
            "nodemap": 0,
            "datamap": 2147485696
          }
        },
        {
          "key": 7,
          "value": {
            "values": ["10", "5"],
            "children": [],
            "nodemap": 0,
            "datamap": 33556480
          }
        },
        {
          "key": 8,
          "value": {
            "values": ["29", "18"],
            "children": [],
            "nodemap": 0,
            "datamap": 16778240
          }
        },
        {
          "key": 9,
          "value": {
            "values": ["2", "12"],
            "children": [],
            "nodemap": 0,
            "datamap": 9
          }
        },
        {
          "key": 10,
          "value": {
            "values": ["7", "11"],
            "children": [],
            "nodemap": 0,
            "datamap": 8388616
          }
        },
        {
          "key": 11,
          "value": {
            "values": ["21", "17"],
            "children": [],
            "nodemap": 0,
            "datamap": 268451840
          }
        }
      ],
      "collisions": []
    },
    "containers": [{"key": 0, "value": 0}]
  }
})");

    const auto load_set = [&] {
        const auto loaded_archive =
            from_json<immer_archive::champ::container_archive_load<Container>>(
                data.dump());
        auto loader = immer_archive::champ::container_loader{loaded_archive};
        return loader.load(0);
    };

    SECTION("Loads correctly")
    {
        const auto set = gen_set(Container{}, 30);
        REQUIRE(load_set() == set);
    }
    SECTION("Modify nodemap of node 0")
    {
        auto& nodemap =
            data["value0"]["nodes"]["inners"][0]["value"]["nodemap"];
        REQUIRE(nodemap == 1343560972);
        nodemap = 1343560971;
        REQUIRE_THROWS_AS(
            load_set(),
            immer_archive::champ::children_count_corrupted_exception);
    }
    SECTION("Modify datamap of node 0")
    {
        auto& datamap =
            data["value0"]["nodes"]["inners"][0]["value"]["datamap"];
        REQUIRE(datamap == 19407009);
        datamap = 19407008;
        REQUIRE_THROWS_AS(load_set(),
                          immer_archive::champ::data_count_corrupted_exception);
    }
    SECTION("Modify nodemap of node 1")
    {
        auto& nodemap =
            data["value0"]["nodes"]["inners"][1]["value"]["nodemap"];
        REQUIRE(nodemap == 0);
        nodemap = 1;
        REQUIRE_THROWS_AS(
            load_set(),
            immer_archive::champ::children_count_corrupted_exception);
    }
    SECTION("Modify datamap of node 1")
    {
        auto& datamap =
            data["value0"]["nodes"]["inners"][1]["value"]["datamap"];
        REQUIRE(datamap == 536875008);
        datamap = 536875007;
        REQUIRE_THROWS_AS(load_set(),
                          immer_archive::champ::data_count_corrupted_exception);
    }
}
