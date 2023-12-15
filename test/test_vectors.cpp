//
//  main.cpp
//  immer-test
//
//  Created by Alex Shabalin on 11/10/2023.
//

#include <catch2/catch_test_macros.hpp>

#include <immer-archive/rbts/load.hpp>
#include <immer-archive/rbts/save.hpp>

#include <test/utils.hpp>

#include <boost/hana.hpp>
#include <spdlog/spdlog.h>

namespace {

auto load_vec(const auto& json, auto vec_id)
{
    const auto archive =
        test::from_json<immer_archive::rbts::archive_load<int>>(json);
    auto loader = immer_archive::rbts::loader<int>{archive};
    return loader.load_vector(vec_id);
}

auto load_flex_vec(const auto& json, auto vec_id)
{
    const auto archive =
        test::from_json<immer_archive::rbts::archive_load<int>>(json);
    auto loader = immer_archive::rbts::loader<int>{archive};
    return loader.load_flex_vector(vec_id);
}

} // namespace

using namespace test;
using immer_archive::rbts::save_to_archive;

TEST_CASE("Save and load multiple times into the same archive")
{
    // spdlog::set_level(spdlog::level::trace);

    auto test_vectors = std::vector<example_vector>{
        // gen(example_vector{}, 4)
        example_vector{},
    };
    auto counter             = std::size_t{};
    auto ar                  = immer_archive::rbts::archive_save<int>{};
    const auto save_and_load = [&]() {
        const auto vec = test_vectors.back().push_back(++counter);
        test_vectors.push_back(vec);

        auto vector_id          = immer_archive::rbts::node_id{};
        std::tie(ar, vector_id) = immer_archive::rbts::save_to_archive(vec, ar);

        SPDLOG_DEBUG("start test size {}", vec.size());
        {
            auto loader = std::make_optional(
                immer_archive::rbts::loader<int>{fix_leaf_nodes(ar)});
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
        -> std::pair<immer_archive::rbts::archive_save<int>,
                     std::vector<immer_archive::rbts::node_id>> {
        auto ar  = immer_archive::rbts::archive_save<int>{};
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
    auto loader = immer_archive::rbts::loader<int>{fix_leaf_nodes(ar)};
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
    const auto save_vectors = [](immer_archive::rbts::archive_save<int> ar,
                                 const auto& vectors)
        -> std::pair<immer_archive::rbts::archive_save<int>,
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

    auto ar                  = immer_archive::rbts::archive_save<int>{};
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
    auto loader = immer_archive::rbts::loader<int>{fix_leaf_nodes(ar)};
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
    auto ar           = immer_archive::rbts::archive_save<int>{};
    auto id1          = immer_archive::rbts::node_id{};
    std::tie(ar, id1) = save_to_archive(vec, ar);

    vec               = std::move(vec).push_back(90);
    auto id2          = immer_archive::rbts::node_id{};
    std::tie(ar, id2) = save_to_archive(vec, ar);

    REQUIRE(id1 != id2);

    auto loader        = immer_archive::rbts::loader<int>{fix_leaf_nodes(ar)};
    const auto loaded1 = loader.load_vector(id1);
    const auto loaded2 = loader.load_vector(id2);
    REQUIRE(loaded2 == loaded1.push_back(90));
}

TEST_CASE("Archive in-place mutated flex_vector")
{
    auto vec          = example_flex_vector{1, 2, 3};
    auto ar           = immer_archive::rbts::archive_save<int>{};
    auto id1          = immer_archive::rbts::node_id{};
    std::tie(ar, id1) = save_to_archive(vec, ar);

    vec               = std::move(vec).push_back(90);
    auto id2          = immer_archive::rbts::node_id{};
    std::tie(ar, id2) = save_to_archive(vec, ar);

    REQUIRE(id1 != id2);

    auto loader        = immer_archive::rbts::loader<int>{fix_leaf_nodes(ar)};
    const auto loaded1 = loader.load_flex_vector(id1);
    const auto loaded2 = loader.load_flex_vector(id2);
    REQUIRE(loaded2 == loaded1.push_back(90));
}

TEST_CASE("Test nodes reuse")
{
    const auto small_vec = gen(immer_archive::flex_vector_one<int>{}, 67);
    const auto big_vec   = small_vec + small_vec;

    auto ar           = immer_archive::rbts::archive_save<int>{};
    auto id1          = immer_archive::rbts::node_id{};
    std::tie(ar, id1) = save_to_archive(big_vec, ar);

    {
        // Loads correctly
        auto loader = immer_archive::rbts::loader<int>{fix_leaf_nodes(ar)};
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
            immer_archive::vector_one<int>{}, 350, [&](const auto& vec) {
                auto ar           = immer_archive::rbts::archive_save<int>{};
                auto id1          = immer_archive::rbts::node_id{};
                std::tie(ar, id1) = save_to_archive(vec, ar);

                {
                    // Loads correctly
                    auto loader =
                        immer_archive::rbts::loader<int>{fix_leaf_nodes(ar)};
                    const auto loaded1 = loader.load_vector(id1);
                    REQUIRE(loaded1 == vec);
                }
            });
    }

    SECTION("keep archiving into the same archive")
    {
        auto ar = immer_archive::rbts::archive_save<int>{};
        for_each_generated_length(
            immer_archive::vector_one<int>{}, 350, [&](const auto& vec) {
                auto id1          = immer_archive::rbts::node_id{};
                std::tie(ar, id1) = save_to_archive(vec, ar);

                {
                    // Loads correctly
                    auto loader =
                        immer_archive::rbts::loader<int>{fix_leaf_nodes(ar)};
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
            immer_archive::flex_vector_one<int>{}, 350, [&](const auto& vec) {
                auto ar           = immer_archive::rbts::archive_save<int>{};
                auto id1          = immer_archive::rbts::node_id{};
                std::tie(ar, id1) = save_to_archive(vec, ar);

                {
                    // Loads correctly
                    auto loader =
                        immer_archive::rbts::loader<int>{fix_leaf_nodes(ar)};
                    const auto loaded1 = loader.load_flex_vector(id1);
                    REQUIRE(loaded1 == vec);
                }
            });
    }

    SECTION("one archive for all")
    {
        auto ar = immer_archive::rbts::archive_save<int>{};
        for_each_generated_length_flex(
            immer_archive::flex_vector_one<int>{}, 350, [&](const auto& vec) {
                auto id1          = immer_archive::rbts::node_id{};
                std::tie(ar, id1) = save_to_archive(vec, ar);

                {
                    // Loads correctly
                    auto loader =
                        immer_archive::rbts::loader<int>{fix_leaf_nodes(ar)};
                    const auto loaded1 = loader.load_flex_vector(id1);
                    REQUIRE(loaded1 == vec);
                }
            });
    }
}

TEST_CASE("Invalid root id")
{
    const auto json = std::string{R"({
            "value0": {
                "leaves": [
                    { "key": 1, "value": [ 6 ] }, { "key": 2, "value": [ 0, 1 ] }, { "key": 3, "value": [ 2, 3 ] }, { "key": 4, "value": [ 4, 5 ] }
                ],
                "inners": [
                    { "key": 0, "value": { "children": [ 2, 3, 4 ], "relaxed": false } }
                ],
                "vectors": [
                    { "key": 0, "value": { "root": 1, "tail": 1, "shift": 1 } }
                ],
                "flex_vectors": []
            }
        })"};
    REQUIRE_THROWS_AS(load_vec(json, 0), immer_archive::invalid_node_id);
}

TEST_CASE("Invalid tail id")
{
    const auto json = std::string{R"({
            "value0": {
                "leaves": [
                    { "key": 1, "value": [ 6 ] }, { "key": 2, "value": [ 0, 1 ] }, { "key": 3, "value": [ 2, 3 ] }, { "key": 4, "value": [ 4, 5 ] }
                ],
                "inners": [
                    { "key": 0, "value": { "children": [ 2, 3, 4 ], "relaxed": false } }
                ],
                "vectors": [
                    { "key": 0, "value": { "root": 0, "tail": 999, "shift": 1 } }
                ],
                "flex_vectors": []
            }
        })"};
    REQUIRE_THROWS_AS(load_vec(json, 0), immer_archive::invalid_node_id);
}

TEST_CASE("Node has itself as a child")
{
    const auto json = std::string{R"({
            "value0": {
                "leaves": [
                    { "key": 1, "value": [ 6 ] }, { "key": 2, "value": [ 0, 1
                    ] }, { "key": 3, "value": [ 2, 3 ] }, { "key": 4,
                    "value": [ 4, 5 ] }
                ],
                "inners": [
                    { "key": 0, "value": { "children": [ 2, 0, 4 ], "relaxed": false } }
                ],
                "vectors": [
                    { "key": 0, "value": { "root": 0, "tail": 1,
                    "shift": 1 } }
                ],
                "flex_vectors": []
            }
        })"};
    REQUIRE_THROWS_AS(load_vec(json, 0), immer_archive::archive_has_cycles);
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
                    "tail": 1,
                    "shift": 6
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
    using rbtree_t =
        std::decay_t<decltype(immer_archive::vector_one<T>{}.impl())>;
    using node_t = typename rbtree_t::node_t;
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

    const auto small_vec = gen(immer_archive::vector_one<big_object>{}, 67);

    auto ar           = immer_archive::rbts::archive_save<big_object>{};
    auto id1          = immer_archive::rbts::node_id{};
    std::tie(ar, id1) = save_to_archive(small_vec, ar);

    {
        // Loads correctly
        auto loader =
            immer_archive::rbts::loader<big_object>{fix_leaf_nodes(ar)};
        const auto loaded1 = loader.load_vector(id1);
        REQUIRE(loaded1 == small_vec);
    }

    // REQUIRE(to_json(ar) == "");
}

TEST_CASE("Test simple valid vector")
{
    const auto json = std::string{R"({
            "value0": {
                "leaves": [
                    { "key": 1, "value": [ 6 ] }, { "key": 2, "value": [ 0, 1
                    ] }, { "key": 3, "value": [ 2, 3 ] }, { "key": 4,
                    "value": [ 4, 5 ] }
                ],
                "inners": [
                    { "key": 0, "value": { "children": [ 2, 3, 4 ], "relaxed": false } }
                ],
                "vectors": [
                    { "key": 0, "value": { "root": 0, "tail": 1,
                    "shift": 1 } }
                ],
                "flex_vectors": []
            }
        })"};

    const auto vec = immer_archive::vector_one<int>{0, 1, 2, 3, 4, 5, 6};
    REQUIRE(load_vec(json, 0) == vec);
}

TEST_CASE("Test simple valid flex vector")
{
    const auto json = std::string{R"({
        "value0": {
            "leaves": [
            {"key": 1, "value": [6, 99]},
            {"key": 2, "value": [0, 1]},
            {"key": 3, "value": [2, 3]},
            {"key": 4, "value": [4, 5]},
            {"key": 5, "value": [6]}
            ],
            "inners": [
            {
                "key": 0,
                "value": {
                "children": [ 2, 3, 4, 5, 2, 3, 4 ],
                "relaxed": true
                }
            }
            ],
            "vectors": [],
            "flex_vectors": [
            {"key": 0, "value": {"root": 0, "tail": 1, "shift": 1}}
            ]
        }
        })"};

    const auto small_vec = gen(immer_archive::flex_vector_one<int>{}, 7);
    const auto vec =
        small_vec + small_vec + immer_archive::flex_vector_one<int>{99};
    // {
    //     auto ar           = immer_archive::rbts::archive_save<int>{};
    //     auto id1          = immer_archive::rbts::node_id{};
    //     std::tie(ar, id1) = save_to_archive(vec, ar);

    //     REQUIRE(to_json(ar) == "");
    // }
    REQUIRE(load_flex_vec(json, 0) == vec);
}

TEST_CASE("A leaf with too few elements")
{
    // Leaf #3 should have two elements, but it has only one.
    const auto json = std::string{R"({
            "value0": {
                "leaves": [
                    { "key": 1, "value": [ 6 ] }, { "key": 2, "value": [ 0, 1
                    ] }, { "key": 3, "value": [ 2 ] }, { "key": 4,
                    "value": [ 4, 5 ] }
                ],
                "inners": [
                    { "key": 0, "value": { "children": [ 2, 3, 4 ], "relaxed": false } }
                ],
                "vectors": [
                    { "key": 0, "value": { "root": 0, "tail": 1,
                    "shift": 1 } }
                ],
                "flex_vectors": []
            }
        })"};
    REQUIRE_THROWS_AS(load_vec(json, 0),
                      immer_archive::rbts::vector_corrupted_exception);
}

TEST_CASE("A leaf with too few elements, flex")
{
    // Leaf #3 should have two elements, but it has only one.
    // It works fine with the relaxed node.
    const auto json = std::string{R"({
        "value0": {
            "leaves": [
            {"key": 1, "value": [6]},
            {"key": 2, "value": [0, 1]},
            {"key": 3, "value": [2]},
            {"key": 4, "value": [4, 5]}
            ],
            "inners": [
            {
                "key": 0,
                "value": {
                "children": [ 2, 3, 4, 1, 2, 3, 4 ],
                "relaxed": true
                }
            }
            ],
            "vectors": [],
            "flex_vectors": [
            {"key": 0, "value": {"root": 0, "tail": 1, "shift": 1}}
            ]
        }
        })"};

    const auto vec =
        immer_archive::flex_vector_one<int>{0, 1, 2, 4, 5, 6, 0, 1, 2, 4, 5, 6};
    REQUIRE(load_flex_vec(json, 0) == vec);
}

TEST_CASE("A leaf with no elements, flex")
{
    // Leaf #3 is empty.
    // It works fine with the relaxed node.
    const auto json = std::string{R"({
        "value0": {
            "leaves": [
            {"key": 1, "value": [6]},
            {"key": 2, "value": [0, 1]},
            {"key": 3, "value": []},
            {"key": 4, "value": [4, 5]}
            ],
            "inners": [
            {
                "key": 0,
                "value": {
                "children": [ 2, 3, 4, 1, 2, 3, 4 ],
                "relaxed": true
                }
            }
            ],
            "vectors": [],
            "flex_vectors": [
            {"key": 0, "value": {"root": 0, "tail": 1, "shift": 1}}
            ]
        }
        })"};

    const auto vec =
        immer_archive::flex_vector_one<int>{0, 1, 4, 5, 6, 0, 1, 4, 5, 6};
    REQUIRE(load_flex_vec(json, 0) == vec);
}

TEST_CASE("A tail with too few elements")
{
    // Leaf #1 should have one element but it has none.
    const auto json = std::string{R"({
            "value0": {
                "leaves": [
                    { "key": 1, "value": [] }, { "key": 2, "value": [ 0, 1
                    ] }, { "key": 3, "value": [ 2, 3 ] }, { "key": 4,
                    "value": [ 4, 5 ] }
                ],
                "inners": [
                    { "key": 0, "value": { "children": [ 2, 3, 4 ], "relaxed": false } }
                ],
                "vectors": [
                    { "key": 0, "value": { "root": 0, "tail": 1,
                    "shift": 1 } }
                ],
                "flex_vectors": []
            }
        })"};

    REQUIRE_THROWS_AS(load_vec(json, 0),
                      immer_archive::rbts::vector_corrupted_exception);
}

TEST_CASE("A leaf with too many elements")
{
    // Leaf #1 has three elements. Slightly different error: a node with so many
    // elements can't even be created.
    const auto json = std::string{R"({
            "value0": {
                "leaves": [
                    { "key": 1, "value": [6,7,8] }, { "key": 2, "value": [ 0, 1
                    ] }, { "key": 3, "value": [ 2, 3 ] }, { "key": 4,
                    "value": [ 4, 5 ] }
                ],
                "inners": [
                    { "key": 0, "value": { "children": [ 2, 3, 4 ], "relaxed": false } }
                ],
                "vectors": [
                    { "key": 0, "value": { "root": 0, "tail": 1,
                    "shift": 1 } }
                ],
                "flex_vectors": []
            }
        })"};

    REQUIRE_THROWS_AS(load_vec(json, 0), immer_archive::invalid_children_count);
}

TEST_CASE("An inner node with too many elements")
{
    const auto json = std::string{R"({
            "value0": {
                "leaves": [
                    { "key": 1, "value": [ 6 ] }, { "key": 2, "value": [ 0, 1
                    ] }, { "key": 3, "value": [ 2, 3 ] }, { "key": 4,
                    "value": [ 4, 5 ] }
                ],
                "inners": [
                    { "key": 0, "value": { "children": [
                        0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
                        0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
                        0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
                        0, 1, 2, 3, 4, 5, 6, 7, 8, 9
                         ], "relaxed": false } }
                ],
                "vectors": [
                    { "key": 0, "value": { "root": 0, "tail": 1,
                    "shift": 1 } }
                ],
                "flex_vectors": []
            }
        })"};
    REQUIRE_THROWS_AS(load_vec(json, 0), immer_archive::invalid_children_count);
}

TEST_CASE("A relaxed node with too many elements")
{
    const auto json = std::string{R"({
        "value0": {
            "leaves": [
            {"key": 1, "value": [6, 99]},
            {"key": 2, "value": [0, 1]},
            {"key": 3, "value": [2, 3]},
            {"key": 4, "value": [4, 5]},
            {"key": 5, "value": [6]}
            ],
            "inners": [
            {
                "key": 0,
                "value": {
                "children": [
                    2, 3,
                    4, 5,
                    2, 3,
                    4, 2,
                    3, 4,
                    5, 2,
                    3, 4,
                    2, 3,
                    4, 5,
                    2, 3,
                    4, 2,
                    3, 4,
                    5, 2,
                    3, 4,
                    2, 3,
                    4, 5,
                    2, 3,
                    4, 2,
                    3, 4,
                    5, 2,
                    3, 4
                ],
                "relaxed": true
                }
            }
            ],
            "vectors": [],
            "flex_vectors": [
            {"key": 0, "value": {"root": 0, "tail": 1, "shift": 1}}
            ]
        }
        })"};
    REQUIRE_THROWS_AS(load_flex_vec(json, 0),
                      immer_archive::invalid_children_count);
}

TEST_CASE("Too few children")
{
    // Node 0 had children 2, 3, 4. 3 is removed.
    // Still works though.
    const auto json = std::string{R"({
            "value0": {
                "leaves": [
                    { "key": 1, "value": [ 6 ] }, { "key": 2, "value": [ 0, 1
                    ] }, { "key": 3, "value": [ 2, 3 ] }, { "key": 4,
                    "value": [ 4, 5 ] }
                ],
                "inners": [
                    { "key": 0, "value": { "children": [ 2, 4 ], "relaxed": false } }
                ],
                "vectors": [
                    { "key": 0, "value": { "root": 0, "tail": 1,
                    "shift": 1 } }
                ],
                "flex_vectors": []
            }
        })"};
    const auto vec  = immer_archive::vector_one<int>{0, 1, 4, 5, 6};
    REQUIRE(load_vec(json, 0) == vec);
}

TEST_CASE("Flex, removed one of children")
{
    const auto json = std::string{R"({
        "value0": {
            "leaves": [
            {"key": 1, "value": [6, 99]},
            {"key": 2, "value": [0, 1]},
            {"key": 3, "value": [2, 3]},
            {"key": 4, "value": [4, 5]},
            {"key": 5, "value": [6]}
            ],
            "inners": [
            {
                "key": 0,
                "value": {
                    "children": [ 2, 3, 4, 2, 3, 4 ],
                    "relaxed": true
                }
            }
            ],
            "vectors": [],
            "flex_vectors": [
            {"key": 0, "value": {"root": 0, "tail": 1, "shift":
            1}}
            ]
        }
        })"};

    const auto children_234 =
        immer_archive::flex_vector_one<int>{0, 1, 2, 3, 4, 5};
    const auto vec = children_234 + children_234 +
                     immer_archive::flex_vector_one<int>{6, 99};
    REQUIRE(load_flex_vec(json, 0) == vec);
}

TEST_CASE("Test unknown child")
{
    const auto json = std::string{R"({
            "value0": {
                "leaves": [
                    { "key": 1, "value": [ 6 ] }, { "key": 2, "value": [ 0, 1
                    ] }, { "key": 3, "value": [ 2, 3 ] }, { "key": 4,
                    "value": [ 4, 5 ] }
                ],
                "inners": [
                    { "key": 0, "value": { "children": [ 2, 3, 9 ], "relaxed": false } }
                ],
                "vectors": [
                    { "key": 0, "value": { "root": 0, "tail": 1,
                    "shift": 1 } }
                ],
                "flex_vectors": []
            }
        })"};
    REQUIRE_THROWS_AS(load_vec(json, 0), immer_archive::invalid_node_id);
}

// TEST_CASE("Test corrupted shift")
// {
//     const auto json = std::string{R"({
//             "value0": {
//                 "leaves": [
//                     { "key": 1, "value": [ 6 ] }, { "key": 2, "value": [ 0, 1
//                     ] }, { "key": 3, "value": [ 2, 3 ] }, { "key": 4,
//                     "value": [ 4, 5 ] }
//                 ],
//                 "inners": [
//                     { "key": 0, "value": { "children": [ 2, 3, 4 ] } }
//                 ],
//                 "relaxed_inners": [],
//                 "vectors": [
//                     { "key": 0, "value": { "root": 0, "tail": 1,
//                     "shift": 0 } }
//                 ],
//                 "flex_vectors": []
//             }
//         })"};

//     const auto vec = immer_archive::vector_one<int>{0, 1, 2, 3, 4, 5, 6};
//     REQUIRE(load_vec(json, 0).value() == vec);
// }
