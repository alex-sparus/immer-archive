//
//  main.cpp
//  immer-test
//
//  Created by Alex Shabalin on 11/10/2023.
//

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <immer-archive/rbts/load.hpp>
#include <immer-archive/rbts/save.hpp>

#include <test/utils.hpp>

#include <boost/hana.hpp>
#include <boost/hana/ext/std/tuple.hpp>
#include <spdlog/spdlog.h>

#include <nlohmann/json.hpp>

namespace {

auto load_vec(const auto& json, auto vec_id)
{
    const auto archive =
        test::from_json<immer_archive::rbts::archive_load<int>>(json);
    auto loader =
        immer_archive::rbts::make_loader_for(test::example_vector{}, archive);
    return loader.load(vec_id);
}

auto load_flex_vec(const auto& json, auto vec_id)
{
    const auto archive =
        test::from_json<immer_archive::rbts::archive_load<int>>(json);
    auto loader = immer_archive::rbts::make_loader_for(
        test::example_flex_vector{}, archive);
    return loader.load(vec_id);
}

} // namespace

using namespace test;
using immer_archive::rbts::save_to_archive;
using json_t   = nlohmann::json;
namespace hana = boost::hana;

TEST_CASE("Save and load multiple times into the same archive")
{
    // spdlog::set_level(spdlog::level::trace);

    auto test_vectors = std::vector<example_vector>{
        // gen(example_vector{}, 4)
        example_vector{},
    };
    auto counter = std::size_t{};
    auto ar      = immer_archive::rbts::make_save_archive_for(example_vector{});
    const auto save_and_load = [&]() {
        const auto vec = test_vectors.back().push_back(++counter);
        test_vectors.push_back(vec);

        auto vector_id          = immer_archive::rbts::node_id{};
        std::tie(ar, vector_id) = immer_archive::rbts::save_to_archive(vec, ar);

        SPDLOG_DEBUG("start test size {}", vec.size());
        {
            auto loader =
                std::make_optional(example_loader{fix_leaf_nodes(ar)});
            auto loaded_vec = loader->load_vector(vector_id);
            REQUIRE(loaded_vec == vec);
        }
        SPDLOG_DEBUG("end test size {}", vec.size());
    };
    // Memory leak investigation: starts only with 4 iterations.
    // for (int i = 0; i < 4; ++i) {
    //     save_and_load();
    // }
    save_and_load();
    save_and_load();
    save_and_load();
    save_and_load();
}

TEST_CASE("Save and load vectors with shared nodes")
{
    // Create a bunch of vectors with shared nodes
    const auto generate_vectors = [] {
        const auto v1 = gen(example_vector{}, 69);
        const auto v2 = v1.push_back(900);
        const auto v3 = v2.push_back(901);
        return std::vector<example_vector>{v1, v2, v3};
    };

    // Save them
    const auto save_vectors = [](const auto& vectors)
        -> std::pair<example_archive_save,
                     std::vector<immer_archive::rbts::node_id>> {
        auto ar  = example_archive_save{};
        auto ids = std::vector<immer_archive::rbts::node_id>{};
        for (const auto& v : vectors) {
            auto [ar2, id] = save_to_archive(v, ar);
            ar             = ar2;
            ids.push_back(id);
        }
        REQUIRE(ids.size() == vectors.size());
        return {std::move(ar), std::move(ids)};
    };

    const auto vectors   = generate_vectors();
    const auto [ar, ids] = save_vectors(vectors);

    {
        // Check that if we generate the same but independent vectors and save
        // them again, the archives look the same
        const auto [ar2, ids2] = save_vectors(generate_vectors());
        REQUIRE(to_json(ar) == to_json(ar2));
    }

    // Load them and verify they're equal to the original vectors
    auto loader = example_loader{fix_leaf_nodes(ar)};
    std::vector<example_vector> loaded;
    auto index = std::size_t{};
    for (const auto& id : ids) {
        auto v = loader.load_vector(id);
        REQUIRE(v == vectors[index]);
        loaded.push_back(v);
        ++index;
    }

    SPDLOG_DEBUG("loaded == vectors {}", loaded == vectors);
    REQUIRE(loaded == vectors);

    loaded = {};
    // Now the loader should deallocate all the nodes it has.
}

TEST_CASE("Save and load vectors and flex vectors with shared nodes")
{
    // Create a bunch of vectors with shared nodes
    const auto generate_vectors = [] {
        const auto v1 = gen(example_vector{}, 69);
        const auto v2 = v1.push_back(900);
        const auto v3 = v2.push_back(901);
        return std::vector<example_vector>{
            v1,
            v2,
            v3,
        };
    };

    const auto generate_flex_vectors = [] {
        const auto v1 = gen(example_vector{}, 69);
        const auto v2 = example_flex_vector{v1}.push_back(900);
        const auto v3 = v2.push_back(901);
        return std::vector<example_flex_vector>{
            v1,
            v2,
            v3,
            v1 + v3,
            v3 + v2,
            v2 + v3 + v1,
        };
    };

    // Save them
    const auto save_vectors = [](example_archive_save ar, const auto& vectors)
        -> std::pair<example_archive_save,
                     std::vector<immer_archive::rbts::node_id>> {
        auto ids = std::vector<immer_archive::rbts::node_id>{};
        for (const auto& v : vectors) {
            auto [ar2, id] = save_to_archive(v, ar);
            ar             = ar2;
            ids.push_back(id);
        }
        REQUIRE(ids.size() == vectors.size());
        return {std::move(ar), std::move(ids)};
    };

    auto ar                  = example_archive_save{};
    const auto vectors       = generate_vectors();
    const auto flex_vectors  = generate_flex_vectors();
    auto vector_ids          = std::vector<immer_archive::rbts::node_id>{};
    auto flex_vectors_ids    = std::vector<immer_archive::rbts::node_id>{};
    std::tie(ar, vector_ids) = save_vectors(ar, vectors);
    std::tie(ar, flex_vectors_ids) = save_vectors(ar, flex_vectors);
    REQUIRE(!vector_ids.empty());
    REQUIRE(!flex_vectors_ids.empty());

    {
        // Check that if we generate the same but independent vectors and save
        // them again, the archives look the same
        const auto ar2 =
            save_vectors(save_vectors({}, generate_vectors()).first,
                         generate_flex_vectors())
                .first;
        REQUIRE(to_json(ar) == to_json(ar2));
    }

    // Load them and verify they're equal to the original vectors
    auto loader = example_loader{fix_leaf_nodes(ar)};
    auto loaded = [&] {
        auto result = std::vector<example_vector>{};
        auto index  = std::size_t{};
        for (const auto& id : vector_ids) {
            auto v = loader.load_vector(id);
            REQUIRE(v == vectors[index]);
            result.push_back(v);
            ++index;
        }
        return result;
    }();
    REQUIRE(loaded == vectors);

    auto loaded_flex = [&] {
        auto result = std::vector<example_flex_vector>{};
        auto index  = std::size_t{};
        for (const auto& id : flex_vectors_ids) {
            auto v = loader.load_flex_vector(id);
            REQUIRE(v == flex_vectors[index]);
            result.push_back(v);
            ++index;
        }
        return result;
    }();
    REQUIRE(loaded_flex == flex_vectors);

    loaded = {};
    // Now the loader should deallocated all the nodes it has.
}

TEST_CASE("Archive in-place mutated vector")
{
    auto vec          = example_vector{1, 2, 3};
    auto ar           = example_archive_save{};
    auto id1          = immer_archive::rbts::node_id{};
    std::tie(ar, id1) = save_to_archive(vec, ar);

    vec               = std::move(vec).push_back(90);
    auto id2          = immer_archive::rbts::node_id{};
    std::tie(ar, id2) = save_to_archive(vec, ar);

    REQUIRE(id1 != id2);

    auto loader        = example_loader{fix_leaf_nodes(ar)};
    const auto loaded1 = loader.load_vector(id1);
    const auto loaded2 = loader.load_vector(id2);
    REQUIRE(loaded2 == loaded1.push_back(90));
}

TEST_CASE("Archive in-place mutated flex_vector")
{
    auto vec          = example_flex_vector{1, 2, 3};
    auto ar           = example_archive_save{};
    auto id1          = immer_archive::rbts::node_id{};
    std::tie(ar, id1) = save_to_archive(vec, ar);

    vec               = std::move(vec).push_back(90);
    auto id2          = immer_archive::rbts::node_id{};
    std::tie(ar, id2) = save_to_archive(vec, ar);

    REQUIRE(id1 != id2);

    auto loader        = example_loader{fix_leaf_nodes(ar)};
    const auto loaded1 = loader.load_flex_vector(id1);
    const auto loaded2 = loader.load_flex_vector(id2);
    REQUIRE(loaded2 == loaded1.push_back(90));
}

TEST_CASE("Test nodes reuse")
{
    const auto small_vec = gen(test::flex_vector_one<int>{}, 67);
    const auto big_vec   = small_vec + small_vec;

    auto ar           = example_archive_save{};
    auto id1          = immer_archive::rbts::node_id{};
    std::tie(ar, id1) = save_to_archive(big_vec, ar);

    {
        // Loads correctly
        auto loader        = example_loader{fix_leaf_nodes(ar)};
        const auto loaded1 = loader.load_flex_vector(id1);
        REQUIRE(loaded1 == big_vec);
    }

    // REQUIRE(to_json(ar) == "");
}

TEST_CASE("Test saving and loading vectors of different lengths", "[slow]")
{
    constexpr auto for_each_generated_length =
        [](auto init, int count, auto&& process) {
            process(init);
            for (int i = 0; i < count; ++i) {
                init = std::move(init).push_back(i);
                process(init);
            }
        };

    SECTION("archive each vector by itself")
    {
        for_each_generated_length(
            test::vector_one<int>{}, 350, [&](const auto& vec) {
                auto ar           = example_archive_save{};
                auto id1          = immer_archive::rbts::node_id{};
                std::tie(ar, id1) = save_to_archive(vec, ar);

                {
                    // Loads correctly
                    auto loader        = example_loader{fix_leaf_nodes(ar)};
                    const auto loaded1 = loader.load_vector(id1);
                    REQUIRE(loaded1 == vec);
                }
            });
    }

    SECTION("keep archiving into the same archive")
    {
        auto ar = example_archive_save{};
        for_each_generated_length(
            test::vector_one<int>{}, 350, [&](const auto& vec) {
                auto id1          = immer_archive::rbts::node_id{};
                std::tie(ar, id1) = save_to_archive(vec, ar);

                {
                    // Loads correctly
                    auto loader        = example_loader{fix_leaf_nodes(ar)};
                    const auto loaded1 = loader.load_vector(id1);
                    REQUIRE(loaded1 == vec);
                }
            });
    }
}

TEST_CASE("Test saving and loading flex vectors of different lengths", "[slow]")
{
    constexpr auto for_each_generated_length_flex =
        [](auto init, int count, auto&& process) {
            process(init);
            process(init + init);
            for (int i = 0; i < count; ++i) {
                auto prev = init;
                init      = std::move(init).push_back(i);

                process(init);
                process(prev + init);
                process(init + prev);
                process(init + init);
            }
        };

    SECTION("one vector per archive")
    {
        for_each_generated_length_flex(
            test::flex_vector_one<int>{}, 350, [&](const auto& vec) {
                auto ar           = example_archive_save{};
                auto id1          = immer_archive::rbts::node_id{};
                std::tie(ar, id1) = save_to_archive(vec, ar);

                {
                    // Loads correctly
                    auto loader        = example_loader{fix_leaf_nodes(ar)};
                    const auto loaded1 = loader.load_flex_vector(id1);
                    REQUIRE(loaded1 == vec);
                }
            });
    }

    SECTION("one archive for all")
    {
        auto ar = example_archive_save{};
        for_each_generated_length_flex(
            test::flex_vector_one<int>{}, 350, [&](const auto& vec) {
                auto id1          = immer_archive::rbts::node_id{};
                std::tie(ar, id1) = save_to_archive(vec, ar);

                {
                    // Loads correctly
                    auto loader        = example_loader{fix_leaf_nodes(ar)};
                    const auto loaded1 = loader.load_flex_vector(id1);
                    REQUIRE(loaded1 == vec);
                }
            });
    }
}

TEST_CASE("A loop with 2 nodes")
{
    const auto json = std::string{R"({
    "value0": {
        "leaves": [
            {
                "key": 32,
                "value": [
                    58,
                    59
                ]
            },
            {
                "key": 34,
                "value": [
                    62,
                    63
                ]
            },
            {
                "key": 3,
                "value": [
                    0,
                    1
                ]
            },
            {
                "key": 5,
                "value": [
                    4,
                    5
                ]
            },
            {
                "key": 6,
                "value": [
                    6,
                    7
                ]
            },
            {
                "key": 7,
                "value": [
                    8,
                    9
                ]
            },
            {
                "key": 8,
                "value": [
                    10,
                    11
                ]
            },
            {
                "key": 9,
                "value": [
                    12,
                    13
                ]
            },
            {
                "key": 10,
                "value": [
                    14,
                    15
                ]
            },
            {
                "key": 11,
                "value": [
                    16,
                    17
                ]
            },
            {
                "key": 12,
                "value": [
                    18,
                    19
                ]
            },
            {
                "key": 13,
                "value": [
                    20,
                    21
                ]
            },
            {
                "key": 14,
                "value": [
                    22,
                    23
                ]
            },
            {
                "key": 15,
                "value": [
                    24,
                    25
                ]
            },
            {
                "key": 16,
                "value": [
                    26,
                    27
                ]
            },
            {
                "key": 17,
                "value": [
                    28,
                    29
                ]
            },
            {
                "key": 18,
                "value": [
                    30,
                    31
                ]
            },
            {
                "key": 19,
                "value": [
                    32,
                    33
                ]
            },
            {
                "key": 20,
                "value": [
                    34,
                    35
                ]
            },
            {
                "key": 21,
                "value": [
                    36,
                    37
                ]
            },
            {
                "key": 22,
                "value": [
                    38,
                    39
                ]
            },
            {
                "key": 23,
                "value": [
                    40,
                    41
                ]
            },
            {
                "key": 24,
                "value": [
                    42,
                    43
                ]
            },
            {
                "key": 25,
                "value": [
                    44,
                    45
                ]
            },
            {
                "key": 26,
                "value": [
                    46,
                    47
                ]
            },
            {
                "key": 27,
                "value": [
                    48,
                    49
                ]
            },
            {
                "key": 28,
                "value": [
                    50,
                    51
                ]
            },
            {
                "key": 29,
                "value": [
                    52,
                    53
                ]
            },
            {
                "key": 30,
                "value": [
                    54,
                    55
                ]
            },
            {
                "key": 31,
                "value": [
                    56,
                    57
                ]
            },
            {
                "key": 1,
                "value": [
                    66
                ]
            },
            {
                "key": 33,
                "value": [
                    60,
                    61
                ]
            },
            {
                "key": 4,
                "value": [
                    2,
                    3
                ]
            },
            {
                "key": 36,
                "value": [
                    64,
                    65
                ]
            }
        ],
        "inners": [
            {
                "key": 0,
                "value": {
                    "children": [
                        2,
                        35
                    ],
                    "relaxed": false
                }
            },
            {
                "key": 35,
                "value": {
                    "children": [
                        36, 0
                    ],
                    "relaxed": false
                }
            },
            {
                "key": 2,
                "value": {
                    "children": [ 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34 ],
                    "relaxed": false
                }
            }
        ],
        "vectors": [],
        "flex_vectors": [
            {
                "key": 0,
                "value": {
                    "root": 0,
                    "tail": 1
                }
            }
        ]
    }
}
)"};
    REQUIRE_THROWS_AS(load_flex_vec(json, 0),
                      immer_archive::archive_has_cycles);
}

namespace {
struct big_object
{
    double a, b, c, d, e;

    big_object(int val)
        : a{static_cast<double>(val)}
        , b{a * 10}
        , c{a * 100}
        , d{a * 1000}
        , e{a * 10000}
    {
    }

    auto tie() const { return std::tie(a, b, c, d, e); }

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(a),
           CEREAL_NVP(b),
           CEREAL_NVP(c),
           CEREAL_NVP(d),
           CEREAL_NVP(e));
    }

    friend bool operator==(const big_object& left, const big_object& right)
    {
        return left.tie() == right.tie();
    }

    friend std::ostream& operator<<(std::ostream& s, const big_object& value)
    {
        return s << fmt::format("({}, {}, {}, {}, {})",
                                value.a,
                                value.b,
                                value.c,
                                value.d,
                                value.e);
    }
};

template <class T>
class show_type;

template <class T>
using node_for = typename decltype([] {
    using rbtree_t = std::decay_t<decltype(test::vector_one<T>{}.impl())>;
    using node_t   = typename rbtree_t::node_t;
    return boost::hana::type_c<node_t>;
}())::type;
} // namespace

TEST_CASE("Test vector with very big objects")
{
    // There are always two branches, two objects in the leaf.
    static_assert(immer::detail::rbts::branches<1> == 2);

    // Even when the object is big, immer::vector puts up to 2 of them in a
    // leaf.
    using big_object_node_t = node_for<big_object>;
    static_assert(big_object_node_t::bits_leaf == 1);
    static_assert(sizeof(big_object) == 40);
    // 96 probably because 40 is aligned to 48, times two.
    static_assert(big_object_node_t::max_sizeof_leaf == 96);

    using int_node_t = node_for<int>;
    static_assert(int_node_t::bits_leaf == 1);
    static_assert(sizeof(int) == 4);
    // show_type<boost::hana::size_t<int_node_t::max_sizeof_leaf>> show;
    static_assert(int_node_t::max_sizeof_leaf == 24);

    const auto small_vec = gen(test::vector_one<big_object>{}, 67);

    auto ar = immer_archive::rbts::make_save_archive_for(
        test::vector_one<big_object>{});
    auto id1          = immer_archive::rbts::node_id{};
    std::tie(ar, id1) = save_to_archive(small_vec, ar);

    {
        // Loads correctly
        auto loader = immer_archive::rbts::make_loader_for(
            test::vector_one<big_object>{}, fix_leaf_nodes(ar));
        const auto loaded1 = loader.load(id1);
        REQUIRE(loaded1 == small_vec);
    }

    // REQUIRE(to_json(ar) == "");
}

TEST_CASE("Test modifying vector nodes")
{
    json_t data;
    data["value0"]["leaves"] = {
        {
            {"key", 1},
            {"value", {6}},
        },
        {
            {"key", 2},
            {"value", {0, 1}},
        },
        {
            {"key", 3},
            {"value", {2, 3}},
        },
        {
            {"key", 4},
            {"value", {4, 5}},
        },
    };
    data["value0"]["inners"] = {
        {
            {"key", 0},
            {
                "value",
                {
                    {"children", {2, 3, 4}},
                    {"relaxed", false},
                },
            },
        },
    };
    data["value0"]["vectors"] = {
        {
            {"key", 0},
            {"value",
             {
                 {"root", 0},
                 {"tail", 1},
             }},
        },
    };
    data["value0"]["flex_vectors"] = json_t::array();

    SECTION("Loads correctly")
    {
        const auto vec = test::vector_one<int>{0, 1, 2, 3, 4, 5, 6};
        REQUIRE(load_vec(data.dump(), 0) == vec);
    }

    SECTION("Invalid root id")
    {
        data["value0"]["vectors"][0]["value"]["root"] = 1;
        REQUIRE_THROWS_AS(load_vec(data.dump(), 0),
                          immer_archive::invalid_node_id);
    }

    SECTION("Invalid tail id")
    {
        data["value0"]["vectors"][0]["value"]["tail"] = 999;
        REQUIRE_THROWS_AS(load_vec(data.dump(), 0),
                          immer_archive::invalid_node_id);
    }

    SECTION("A leaf with too few elements")
    {
        auto& item = data["value0"]["leaves"][2];
        REQUIRE(item["key"] == 3);
        // Leaf #3 should have two elements, but it has only one.
        item["value"] = {2};
        REQUIRE_THROWS_AS(load_vec(data.dump(), 0),
                          immer_archive::rbts::vector_corrupted_exception);
    }

    SECTION("Mess with the tail")
    {
        auto& item = data["value0"]["leaves"][0];
        REQUIRE(item["key"] == 1);
        SECTION("Add to the tail")
        {
            item["value"]  = {6, 7};
            const auto vec = test::vector_one<int>{0, 1, 2, 3, 4, 5, 6, 7};
            REQUIRE(load_vec(data.dump(), 0) == vec);
        }
        SECTION("Remove from the tail")
        {
            item["value"] = json_t::array();
            REQUIRE_THROWS_AS(load_vec(data.dump(), 0),
                              immer_archive::rbts::vector_corrupted_exception);
        }
        SECTION("Add too many elements")
        {
            // Three elements can't be in a leaf
            item["value"] = {6, 7, 8};
            REQUIRE_THROWS_AS(load_vec(data.dump(), 0),
                              immer_archive::invalid_children_count);
        }
    }

    SECTION("Modify children")
    {
        auto& item = data["value0"]["inners"][0];
        REQUIRE(item["key"] == 0);
        SECTION("Add too many")
        {
            item["value"]["children"] = std::vector<int>(33, 2);
            REQUIRE_THROWS_AS(load_vec(data.dump(), 0),
                              immer_archive::invalid_children_count);
        }
        SECTION("Remove one")
        {
            item["value"]["children"] = {2, 4};
            const auto vec            = test::vector_one<int>{0, 1, 4, 5, 6};
            REQUIRE(load_vec(data.dump(), 0) == vec);
        }
        SECTION("Unknown child")
        {
            item["value"]["children"] = {2, 4, 9};
            REQUIRE_THROWS_AS(load_vec(data.dump(), 0),
                              immer_archive::invalid_node_id);
        }
        SECTION("Node has itself as a child")
        {
            item["value"]["children"] = {2, 0, 4};
            REQUIRE_THROWS_AS(load_vec(data.dump(), 0),
                              immer_archive::archive_has_cycles);
        }
        SECTION("Strict vector can not have relaxed nodes")
        {
            item["value"]["relaxed"] = true;
            REQUIRE_THROWS_AS(
                load_vec(data.dump(), 0),
                immer_archive::rbts::relaxed_node_not_allowed_exception);
        }
    }
}

TEST_CASE("Test modifying flex vector nodes")
{
    json_t data;
    data["value0"]["leaves"] = {
        {
            {"key", 1},
            {"value", {6, 99}},
        },
        {
            {"key", 2},
            {"value", {0, 1}},
        },
        {
            {"key", 3},
            {"value", {2, 3}},
        },
        {
            {"key", 4},
            {"value", {4, 5}},
        },
        {
            {"key", 5},
            {"value", {6}},
        },
    };
    data["value0"]["inners"] = {
        {
            {"key", 0},
            {
                "value",
                {
                    {"children", {2, 3, 4, 5, 2, 3, 4}},
                    {"relaxed", true},
                },
            },
        },
    };
    data["value0"]["flex_vectors"] = {
        {
            {"key", 0},
            {"value",
             {
                 {"root", 0},
                 {"tail", 1},
             }},
        },
    };
    data["value0"]["vectors"] = json_t::array();

    SECTION("Loads correctly")
    {
        const auto small_vec = gen(test::flex_vector_one<int>{}, 7);
        const auto vec = small_vec + small_vec + test::flex_vector_one<int>{99};
        REQUIRE(load_flex_vec(data.dump(), 0) == vec);
    }
    SECTION("Non-relaxed vector can not have relaxed nodes")
    {
        data["value0"]["vectors"]      = data["value0"]["flex_vectors"];
        data["value0"]["flex_vectors"] = json_t::array();
        REQUIRE_THROWS_AS(
            load_vec(data.dump(), 0),
            immer_archive::rbts::relaxed_node_not_allowed_exception);
    }
    SECTION("Modify starting leaf")
    {
        auto& item = data["value0"]["leaves"][1];
        REQUIRE(item["key"] == 2);
        SECTION("One element")
        {
            item["value"]      = {0};
            const auto vec_234 = test::flex_vector_one<int>{0, 2, 3, 4, 5};
            const auto leaf_1  = test::flex_vector_one<int>{6, 99};
            const auto leaf_5  = test::flex_vector_one<int>{6};
            const auto vec     = vec_234 + leaf_5 + vec_234 + leaf_1;
            REQUIRE(load_flex_vec(data.dump(), 0) == vec);
        }

        // Empty starting leaf triggers a separate branch in the code, actually.
        SECTION("Empty leaf")
        {
            item["value"]      = json_t::array();
            const auto vec_234 = test::flex_vector_one<int>{2, 3, 4, 5};
            const auto leaf_1  = test::flex_vector_one<int>{6, 99};
            const auto leaf_5  = test::flex_vector_one<int>{6};
            const auto vec     = vec_234 + leaf_5 + vec_234 + leaf_1;
            REQUIRE(load_flex_vec(data.dump(), 0) == vec);
        }
    }
    SECTION("Modify second leaf")
    {
        auto& item = data["value0"]["leaves"][2];
        REQUIRE(item["key"] == 3);
        SECTION("One element")
        {
            item["value"]      = {2};
            const auto vec_234 = test::flex_vector_one<int>{0, 1, 2, 4, 5};
            const auto leaf_1  = test::flex_vector_one<int>{6, 99};
            const auto leaf_5  = test::flex_vector_one<int>{6};
            const auto vec     = vec_234 + leaf_5 + vec_234 + leaf_1;
            REQUIRE(load_flex_vec(data.dump(), 0) == vec);
        }
        SECTION("Empty leaf")
        {
            item["value"]      = json_t::array();
            const auto vec_234 = test::flex_vector_one<int>{0, 1, 4, 5};
            const auto leaf_1  = test::flex_vector_one<int>{6, 99};
            const auto leaf_5  = test::flex_vector_one<int>{6};
            const auto vec     = vec_234 + leaf_5 + vec_234 + leaf_1;
            REQUIRE(load_flex_vec(data.dump(), 0) == vec);
        }
    }
    SECTION("Modify inner node")
    {
        auto& item = data["value0"]["inners"][0];
        REQUIRE(item["key"] == 0);
        SECTION("No children")
        {
            item["value"]["children"] = json_t::array();
            // There was a problem when an empty relaxed node triggered an
            // assert, while non-relaxed worked fine.
            const auto is_relaxed    = GENERATE(false, true);
            item["value"]["relaxed"] = is_relaxed;
            const auto leaf_1        = test::flex_vector_one<int>{6, 99};
            REQUIRE(load_flex_vec(data.dump(), 0) == leaf_1);
        }
        SECTION("Too many children")
        {
            item["value"]["children"] = std::vector<int>(40, 3);
            const auto is_relaxed     = GENERATE(false, true);
            item["value"]["relaxed"]  = is_relaxed;
            REQUIRE_THROWS_AS(load_flex_vec(data.dump(), 0),
                              immer_archive::invalid_children_count);
        }
        SECTION("Remove a child")
        {
            SECTION("All leaves are full, non-relaxed works")
            {
                item["value"]["children"] = {2, 3, 4, 2, 3, 4};
                const auto is_relaxed     = GENERATE(false, true);
                item["value"]["relaxed"]  = is_relaxed;
                const auto leaf_1         = test::flex_vector_one<int>{6, 99};
                const auto vec_234 =
                    test::flex_vector_one<int>{0, 1, 2, 3, 4, 5};
                const auto vec = vec_234 + vec_234 + leaf_1;
                REQUIRE(load_flex_vec(data.dump(), 0) == vec);
            }
            SECTION("Relaxed leaf")
            {
                item["value"]["children"] = {2, 3, 5, 3};
                SECTION("Non-relaxed breaks")
                {
                    item["value"]["relaxed"] = false;
                    REQUIRE_THROWS_AS(
                        load_flex_vec(data.dump(), 0),
                        immer_archive::rbts::vector_corrupted_exception);
                }
                SECTION("Relaxed works")
                {
                    const auto leaf_1 = test::flex_vector_one<int>{6, 99};
                    const auto leaf_2 = test::flex_vector_one<int>{0, 1};
                    const auto leaf_3 = test::flex_vector_one<int>{2, 3};
                    const auto leaf_5 = test::flex_vector_one<int>{6};

                    const auto vec = leaf_2 + leaf_3 + leaf_5 + leaf_3 + leaf_1;
                    REQUIRE(load_flex_vec(data.dump(), 0) == vec);
                }
            }
        }
    }
}

TEST_CASE("Print shift calculation", "[.print_shift]")
{
    // It starts with BL, which is 1.
    // Then grows by B, which is 5 by default.
    // size [0, 66], shift 1
    // size [67, 2050], shift 6
    // size [2051, 65538], shift 11
    auto vec        = example_vector{};
    auto last_shift = vec.impl().shift;
    for (auto index = 0; index < 90000; ++index) {
        vec              = vec.push_back(index);
        const auto shift = vec.impl().shift;
        if (shift != last_shift) {
            SPDLOG_INFO("size {}, shift {}", vec.size(), shift);
            last_shift = shift;
        }
    }
}

namespace {
template <class B_BL>
struct verify_shift_calculation
{
    static bool run()
    {
        using namespace hana::literals;
        constexpr auto B  = hana::value<decltype(std::declval<B_BL>()[0_c])>();
        constexpr auto BL = hana::value<decltype(std::declval<B_BL>()[1_c])>();
        using Vector = immer::vector<int, immer::default_memory_policy, B, BL>;
        auto vec     = Vector{};
        constexpr auto max_vector_length = 90000;
        for (auto index = 0; index < max_vector_length; ++index) {
            INFO("B = " << B);
            INFO("BL = " << BL);
            INFO("size = " << vec.size());
            REQUIRE(vec.impl().shift ==
                    immer_archive::rbts::get_shift_for_size<B, BL>(vec.size()));
        }
        return true;
    }
};
} // namespace

// Generate a tuple of pairs (B, BL) for each B [3, 7] and BL [1, 5].
using Bs_BLs = decltype([] {
    constexpr auto bs   = hana::range_c<immer::detail::rbts::bits_t, 3, 8>;
    constexpr auto bls  = hana::range_c<immer::detail::rbts::bits_t, 1, 6>;
    constexpr auto prod = hana::cartesian_product(hana::make_tuple(bs, bls));
    return hana::to<hana::ext::std::tuple_tag>(prod);
}());

TEMPLATE_LIST_TEST_CASE_METHOD(verify_shift_calculation,
                               "Verify shift calculation",
                               "[shift][slow]",
                               Bs_BLs)
{
    REQUIRE(verify_shift_calculation<TestType>::run());
}

TEST_CASE("Test more inner nodes")
{
    json_t data;
    data["value0"]["leaves"] = {
        {{"key", 32}, {"value", {58, 59}}}, {{"key", 34}, {"value", {62, 63}}},
        {{"key", 3}, {"value", {0, 1}}},    {{"key", 5}, {"value", {4, 5}}},
        {{"key", 6}, {"value", {6, 7}}},    {{"key", 7}, {"value", {8, 9}}},
        {{"key", 8}, {"value", {10, 11}}},  {{"key", 9}, {"value", {12, 13}}},
        {{"key", 10}, {"value", {14, 15}}}, {{"key", 11}, {"value", {16, 17}}},
        {{"key", 12}, {"value", {18, 19}}}, {{"key", 13}, {"value", {20, 21}}},
        {{"key", 14}, {"value", {22, 23}}}, {{"key", 15}, {"value", {24, 25}}},
        {{"key", 16}, {"value", {26, 27}}}, {{"key", 17}, {"value", {28, 29}}},
        {{"key", 18}, {"value", {30, 31}}}, {{"key", 19}, {"value", {32, 33}}},
        {{"key", 20}, {"value", {34, 35}}}, {{"key", 21}, {"value", {36, 37}}},
        {{"key", 22}, {"value", {38, 39}}}, {{"key", 23}, {"value", {40, 41}}},
        {{"key", 24}, {"value", {42, 43}}}, {{"key", 25}, {"value", {44, 45}}},
        {{"key", 26}, {"value", {46, 47}}}, {{"key", 27}, {"value", {48, 49}}},
        {{"key", 28}, {"value", {50, 51}}}, {{"key", 29}, {"value", {52, 53}}},
        {{"key", 30}, {"value", {54, 55}}}, {{"key", 31}, {"value", {56, 57}}},
        {{"key", 1}, {"value", {66}}},      {{"key", 33}, {"value", {60, 61}}},
        {{"key", 4}, {"value", {2, 3}}},    {{"key", 36}, {"value", {64, 65}}},
    };
    data["value0"]["inners"] = {
        {{"key", 0}, {"value", {{"children", {2, 35}}, {"relaxed", false}}}},
        {{"key", 2},
         {"value",
          {{"children",
            {3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18,
             19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34}},
           {"relaxed", false}}}},
        {{"key", 35}, {"value", {{"children", {36}}, {"relaxed", false}}}},
    };
    data["value0"]["vectors"]      = {{
        {"key", 0},
        {
            "value",
            {
                {"root", 0},
                {"tail", 1},
            },
        },
    }};
    data["value0"]["flex_vectors"] = json_t::array();

    SECTION("Loads correctly")
    {
        REQUIRE(load_vec(data.dump(), 0) == gen(example_vector{}, 67));
    }
    SECTION("Throw in some empty nodes")
    {
        // Empty leaf #205
        data["value0"]["leaves"].push_back(
            {{"key", 205}, {"value", json_t::array()}});
        // Inner node #560 referencing the empty leaf
        data["value0"]["inners"].push_back(
            {{"key", 560},
             {"value", {{"children", {205}}, {"relaxed", false}}}});
        auto& item = data["value0"]["inners"][0];
        SECTION("Three nodes")
        {
            item["value"]["children"] = {2, 560, 35};
            REQUIRE(load_vec(data.dump(), 0) == gen(example_vector{}, 67));
        }

        /**
         * NOTE: These two tests do not work as long as there is a bug in "Test
         * flex vector with a weird shape strict".
         */
        // SECTION("Empty first")
        // {
        //     FAIL("Fix this test");
        //     item["value"]["children"] = {560, 35};
        //     REQUIRE_NOTHROW(load_vec(data.dump(), 0));
        //     for (auto i : load_vec(data.dump(), 0)) {
        //         SPDLOG_INFO(i);
        //     }
        //     REQUIRE(1 == 2);
        //     // REQUIRE(load_vec(data.dump(), 0) == example_vector{64, 65,
        //     66});
        // }
        // SECTION("Empty last")
        // {
        //     FAIL("Fix this test");
        //     item["value"]["children"] = {35, 560};
        //     REQUIRE(load_vec(data.dump(), 0) == example_vector{64, 65, 66});
        // }
    }
}

TEST_CASE("Test flex vector with a weird shape relaxed")
{
    json_t data;
    data["value0"]["leaves"] = {
        {{"key", 1}, {"value", {66}}},
        {{"key", 36}, {"value", {64, 65}}},
    };
    data["value0"]["inners"] = {
        {{"key", 0}, {"value", {{"children", {35}}, {"relaxed", true}}}},
        {{"key", 35}, {"value", {{"children", {36}}, {"relaxed", true}}}},
    };
    data["value0"]["flex_vectors"] = {
        {{"key", 0}, {"value", {{"root", 0}, {"tail", 1}}}}};
    data["value0"]["vectors"] = json_t::array();

    const auto loaded = load_flex_vec(data.dump(), 0);
    // {
    //     auto ar        = immer_archive::rbts::make_save_archive_for(loaded);
    //     auto vector_id = immer_archive::rbts::node_id{};
    //     std::tie(ar, vector_id) =
    //         immer_archive::rbts::save_to_archive(loaded, ar);
    //     SPDLOG_INFO("{}", test::to_json(ar));
    // }

    const auto expected = example_vector{64, 65, 66};

    // Test iteration over the loaded vector
    {
        const auto rebuilt = example_vector{loaded.begin(), loaded.end()};
        REQUIRE(rebuilt == expected);
    }

    REQUIRE(loaded == expected);
}

TEST_CASE("Test flex vector with a weird shape strict", "[valgrind]")
{
    json_t data;
    data["value0"]["leaves"] = {
        {{"key", 1}, {"value", {66}}},
        {{"key", 36}, {"value", {64, 65}}},
    };
    data["value0"]["inners"] = {
        {{"key", 0}, {"value", {{"children", {35}}, {"relaxed", false}}}},
        {{"key", 35}, {"value", {{"children", {36}}, {"relaxed", false}}}},
    };
    data["value0"]["flex_vectors"] = {
        {{"key", 0}, {"value", {{"root", 0}, {"tail", 1}}}}};
    data["value0"]["vectors"] = json_t::array();

    const auto loaded = load_flex_vec(data.dump(), 0);
    // {
    //     auto ar        = immer_archive::rbts::make_save_archive_for(loaded);
    //     auto vector_id = immer_archive::rbts::node_id{};
    //     std::tie(ar, vector_id) =
    //         immer_archive::rbts::save_to_archive(loaded, ar);
    //     SPDLOG_INFO("{}", test::to_json(ar));
    // }

    const auto expected = example_vector{64, 65, 66};

    // Test iteration over the loaded vector
    {
        const auto rebuilt = example_vector{loaded.begin(), loaded.end()};
        REQUIRE(rebuilt == expected);
    }

    REQUIRE(loaded == expected);
}

// TEST_CASE("Test flex vector with uneven depth")
// {
//     const auto small = gen(example_flex_vector{}, 5);
//     const auto vec2  = small + small + small;
//     const auto vec   = example_flex_vector{0, 1} + vec2;
//     {
//         auto ar        = immer_archive::rbts::make_save_archive_for(vec);
//         auto vector_id = immer_archive::rbts::node_id{};
//         std::tie(ar, vector_id) = immer_archive::rbts::save_to_archive(vec,
//         ar);
//         // SPDLOG_INFO("{}", test::to_json(ar));
//         REQUIRE(test::to_json(ar) == "");
//     }
// }
