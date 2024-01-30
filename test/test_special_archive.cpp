#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>

#include <test/utils.hpp>

#include <boost/hana.hpp>
#include <immer-archive/champ/traits.hpp>
#include <immer-archive/json/archivable.hpp>
#include <immer-archive/json/json_with_archive.hpp>
#include <immer-archive/rbts/vector.hpp>

// to save std::pair
#include <cereal/types/utility.hpp>

#include <boost/hana/ext/std/tuple.hpp>

namespace {

namespace hana = boost::hana;

/**
 * Some user data type that contains some vector_one_archivable, which should be
 * serialized in a special way.
 */

using test::flex_vector_one;
using test::test_value;
using test::vector_one;

template <class T>
std::string string_via_tie(const T& value)
{
    std::string result;
    hana::for_each(value.tie(), [&](const auto& item) {
        using Item = std::decay_t<decltype(item)>;
        result += (result.empty() ? "" : ", ") +
                  Catch::StringMaker<Item>::convert(item);
    });
    return result;
}

struct meta_meta
{
    immer_archive::archivable<vector_one<int>> ints;
    immer_archive::archivable<immer::table<test_value>> table;

    auto tie() const { return std::tie(ints, table); }

    friend bool operator==(const meta_meta& left, const meta_meta& right)
    {
        return left.tie() == right.tie();
    }

    friend std::ostream& operator<<(std::ostream& s, const meta_meta& value)
    {
        return s << string_via_tie(value);
    }

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(ints), CEREAL_NVP(table));
    }
};

struct meta
{
    immer_archive::archivable<vector_one<int>> ints;
    immer_archive::archivable<vector_one<meta_meta>> metas;

    auto tie() const { return std::tie(ints, metas); }

    friend bool operator==(const meta& left, const meta& right)
    {
        return left.tie() == right.tie();
    }

    friend std::ostream& operator<<(std::ostream& s, const meta& value)
    {
        return s << string_via_tie(value);
    }

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(ints), CEREAL_NVP(metas));
    }
};

struct test_data
{
    immer_archive::archivable<vector_one<int>> ints;
    immer_archive::archivable<vector_one<std::string>> strings;

    immer_archive::archivable<flex_vector_one<int>> flex_ints;
    immer_archive::archivable<immer::map<int, std::string>> map;

    immer_archive::archivable<vector_one<meta>> metas;

    // Also test having meta directly, not inside an archivable type
    meta single_meta;

    auto tie() const
    {
        return std::tie(ints, strings, flex_ints, map, metas, single_meta);
    }

    friend bool operator==(const test_data& left, const test_data& right)
    {
        return left.tie() == right.tie();
    }

    /**
     * Serialization function is defined as normal.
     */
    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(ints),
           CEREAL_NVP(strings),
           CEREAL_NVP(flex_ints),
           CEREAL_NVP(map),
           CEREAL_NVP(metas),
           CEREAL_NVP(single_meta));
    }
};

/**
 * A special function that enumerates which types of immer-archives are
 * required. Explicitly name each type, for simplicity.
 */
inline auto get_archives_types(const test_data&)
{
    auto names = hana::make_map(
        hana::make_pair(hana::type_c<vector_one<int>>,
                        BOOST_HANA_STRING("ints")),
        hana::make_pair(hana::type_c<vector_one<std::string>>,
                        BOOST_HANA_STRING("strings")),
        hana::make_pair(hana::type_c<flex_vector_one<int>>,
                        BOOST_HANA_STRING("flex_ints")),
        hana::make_pair(hana::type_c<immer::map<int, std::string>>,
                        BOOST_HANA_STRING("int_string_map")),
        hana::make_pair(hana::type_c<vector_one<meta>>,
                        BOOST_HANA_STRING("metas")),
        hana::make_pair(hana::type_c<vector_one<meta_meta>>,
                        BOOST_HANA_STRING("meta_metas")),
        hana::make_pair(hana::type_c<immer::table<test_value>>,
                        BOOST_HANA_STRING("table_test_value"))

    );
    return names;
}

inline auto get_archives_types(const std::pair<test_data, test_data>&)
{
    return get_archives_types(test_data{});
}

} // namespace

template <>
struct fmt::formatter<meta_meta> : ostream_formatter
{};

template <>
struct fmt::formatter<meta> : ostream_formatter
{};

namespace Catch {
template <>
struct StringMaker<test_data>
{
    static std::string convert(const test_data& value)
    {
        return string_via_tie(value);
    }
};

template <>
struct StringMaker<meta>
{
    static std::string convert(const meta& value)
    {
        return string_via_tie(value);
    }
};

template <>
struct StringMaker<meta_meta>
{
    static std::string convert(const meta_meta& value)
    {
        return string_via_tie(value);
    }
};
} // namespace Catch

TEST_CASE("Special archive minimal test")
{
    const auto ints1 = vector_one<int>{
        1,
        2,
        3,
        4,
        5,
    };
    const auto test1 = test_data{
        .metas =
            {
                meta{
                    .ints = ints1,
                    .metas =
                        {
                            meta_meta{
                                .ints = ints1,
                            },
                        },
                },
            },
    };

    const auto [json_str, archives] =
        immer_archive::to_json_with_archive(test1);

    // REQUIRE(json_str == "");

    {
        auto full_load =
            immer_archive::from_json_with_archive<test_data>(json_str);
        REQUIRE(full_load == test1);
        // REQUIRE(immer_archive::to_json_with_archive(full_load).first == "");
    }
}

TEST_CASE("Save with a special archive")
{
    spdlog::set_level(spdlog::level::debug);

    const auto ints1 = test::gen(test::example_vector{}, 3);
    const auto test1 = test_data{
        .ints      = ints1,
        .strings   = {"one", "two"},
        .flex_ints = flex_vector_one<int>{ints1},
        .map =
            {
                {1, "_one_"},
                {2, "two__"},
            },
        .metas = {vector_one<meta>{
            meta{
                .ints =
                    {
                        1,
                        2,
                        3,
                        4,
                        5,
                    },
            },
            meta{
                .ints = ints1,
            },
        }},
        .single_meta =
            meta{
                .ints = {66, 50, 55},
            },
    };

    const auto [json_str, archives] =
        immer_archive::to_json_with_archive(test1);
    SECTION("Try to save and load the archive")
    {
        const auto archives_json = [&archives = archives] {
            auto os = std::ostringstream{};
            {
                auto ar = immer_archive::json_immer_output_archive<
                    std::decay_t<decltype(archives)>>{os};
                ar(123);
                ar(CEREAL_NVP(archives));
            }
            return os.str();
        }();
        // REQUIRE(archives_json == "");
        const auto archives_loaded = [&archives_json] {
            using Archives =
                decltype(immer_archive::detail::generate_archives_load(
                    get_archives_types(test_data{})));
            auto archives = Archives{};

            {
                auto is = std::istringstream{archives_json};
                auto ar = cereal::JSONInputArchive{is};
                ar(CEREAL_NVP(archives));
            }

            {
                auto is = std::istringstream{archives_json};
                auto ar = immer_archive::json_immer_input_archive<Archives>{
                    archives, is};
                ar(CEREAL_NVP(archives));
            }

            return archives;
        }();
        REQUIRE(archives_loaded.storage[hana::type_c<vector_one<int>>]
                    .archive.leaves.size() == 7);
    }

    // REQUIRE(json_str == "");

    {
        auto full_load =
            immer_archive::from_json_with_archive<test_data>(json_str);
        REQUIRE(full_load == test1);
        // REQUIRE(immer_archive::to_json_with_archive(full_load).first == "");
    }
}

TEST_CASE("Save with a special archive, special type is enclosed")
{
    const auto map = immer::map<int, std::string>{
        {1, "_one_"},
        {2, "two__"},
    };
    const auto ints1 = test::gen(test::example_vector{}, 3);
    const auto ints5 = vector_one<int>{
        1,
        2,
        3,
        4,
        5,
    };
    const auto metas = vector_one<meta>{
        meta{
            .ints = ints5,
        },
        meta{
            .ints = ints1,
        },
    };
    const auto test1 = test_data{
        .ints      = ints1,
        .strings   = {"one", "two"},
        .flex_ints = flex_vector_one<int>{ints1},
        .map       = map,
        .metas     = metas,
    };
    const auto test2 = test_data{
        .ints      = ints1,
        .strings   = {"three"},
        .flex_ints = flex_vector_one<int>{ints1},
        .map       = map.set(3, "__three"),
        .metas =
            {
                meta{
                    .ints = ints5,
                },
            },
    };

    // At the beginning, the vector is shared, it's the same data.
    REQUIRE(test1.ints.container.identity() == test2.ints.container.identity());
    REQUIRE(test1.flex_ints.container.identity() ==
            test2.flex_ints.container.identity());

    const auto [json_str, archives] =
        immer_archive::to_json_with_archive(std::make_pair(test1, test2));

    // REQUIRE(json_str == "");

    {
        auto [loaded1, loaded2] = immer_archive::from_json_with_archive<
            std::pair<test_data, test_data>>(json_str);
        REQUIRE(loaded1 == test1);
        REQUIRE(loaded2 == test2);

        REQUIRE(loaded1.metas.container.size() == 2);
        REQUIRE(loaded1.metas.container[0].ints == ints5);
        REQUIRE(loaded1.metas.container[1].ints == ints1);

        // After loading, two vectors are still reused.
        REQUIRE(loaded1.ints.container.identity() ==
                loaded2.ints.container.identity());
        REQUIRE(loaded1.flex_ints.container.identity() ==
                loaded2.flex_ints.container.identity());

        REQUIRE(loaded1.metas.container[0].ints.container.identity() ==
                loaded2.metas.container[0].ints.container.identity());
    }
}

TEST_CASE("Special archive must load and save types that have no archive")
{
    const auto val1  = test_value{123, "value1"};
    const auto val2  = test_value{234, "value2"};
    const auto value = std::make_pair(val1, val2);

    const auto json_archive_str =
        immer_archive::to_json_with_archive(value).first;
    REQUIRE(json_archive_str == test::to_json(value));

    {
        auto loaded = immer_archive::from_json_with_archive<
            std::decay_t<decltype(value)>>(json_archive_str);
        REQUIRE(loaded == value);
    }
}

TEST_CASE("Special archive loads empty test_data")
{
    const auto value = test_data{};

    // const auto json_archive_str =
    //     immer_archive::to_json_with_archive(value).first;
    // REQUIRE(json_archive_str == "");

    const auto json_archive_str = R"({
  "value0": {
    "ints": 0,
    "strings": 0,
    "flex_ints": 0,
    "map": 0,
    "metas": 0,
    "single_meta": {"ints": 0, "metas": 0}
  },
  "archives": {
    "ints": {
      "leaves": [{"key": 1, "value": []}],
      "inners": [{"key": 0, "value": {"children": [], "relaxed": false}}],
      "vectors": [{"key": 0, "value": {"root": 0, "tail": 1}}],
      "flex_vectors": []
    },
    "strings": {
      "leaves": [{"key": 1, "value": []}],
      "inners": [{"key": 0, "value": {"children": [], "relaxed": false}}],
      "vectors": [{"key": 0, "value": {"root": 0, "tail": 1}}],
      "flex_vectors": []
    },
    "flex_ints": {
      "leaves": [{"key": 1, "value": []}],
      "inners": [{"key": 0, "value": {"children": [], "relaxed": false}}],
      "vectors": [],
      "flex_vectors": [{"key": 0, "value": {"root": 0, "tail": 1}}]
    },
    "int_string_map": [
          {"values": [], "children": [], "nodemap": 0, "datamap": 0, "collisions": false}
    ],
    "metas": {
      "leaves": [{"key": 1, "value": []}],
      "inners": [{"key": 0, "value": {"children": [], "relaxed": false}}],
      "vectors": [{"key": 0, "value": {"root": 0, "tail": 1}}],
      "flex_vectors": []
    },
    "meta_metas": {
      "leaves": [{"key": 1, "value": []}],
      "inners": [{"key": 0, "value": {"children": [], "relaxed": false}}],
      "vectors": [{"key": 0, "value": {"root": 0, "tail": 1}}],
      "flex_vectors": []
    },
    "table_test_value": []
  }
})";

    {
        auto loaded = immer_archive::from_json_with_archive<
            std::decay_t<decltype(value)>>(json_archive_str);
        REQUIRE(loaded == value);
    }
}

TEST_CASE("Special archive throws cereal::Exception")
{
    const auto value = test_data{};

    // const auto json_archive_str =
    //     immer_archive::to_json_with_archive(value).first;
    // REQUIRE(json_archive_str == "");

    const auto json_archive_str = R"({
  "value0": {
    "ints": 99,
    "strings": 0,
    "flex_ints": 0,
    "map": 0,
    "metas": 0,
    "single_meta": {"ints": 0, "metas": 0}
  },
  "archives": {
    "ints": {
      "leaves": [{"key": 1, "value": []}],
      "inners": [{"key": 0, "value": {"children": [], "relaxed": false}}],
      "vectors": [{"key": 0, "value": {"root": 0, "tail": 1, "shift": 1}}],
      "flex_vectors": []
    },
    "strings": {
      "leaves": [{"key": 1, "value": []}],
      "inners": [{"key": 0, "value": {"children": [], "relaxed": false}}],
      "vectors": [{"key": 0, "value": {"root": 0, "tail": 1, "shift": 1}}],
      "flex_vectors": []
    },
    "flex_ints": {
      "leaves": [{"key": 1, "value": []}],
      "inners": [{"key": 0, "value": {"children": [], "relaxed": false}}],
      "vectors": [],
      "flex_vectors": [{"key": 0, "value": {"root": 0, "tail": 1, "shift": 1}}]
    },
    "int_string_map": [
        {"values": [], "children": [], "nodemap": 0, "datamap": 0, "collisions": false}
    ],
    "metas": {
      "leaves": [{"key": 1, "value": []}],
      "inners": [{"key": 0, "value": {"children": [], "relaxed": false}}],
      "vectors": [{"key": 0, "value": {"root": 0, "tail": 1, "shift": 1}}],
      "flex_vectors": []
    },
    "meta_metas": {
      "leaves": [{"key": 1, "value": []}],
      "inners": [{"key": 0, "value": {"children": [], "relaxed": false}}],
      "vectors": [{"key": 0, "value": {"root": 0, "tail": 1, "shift": 1}}],
      "flex_vectors": []
    },
    "table_test_value": []
  }
})";

    REQUIRE_THROWS_MATCHES(
        immer_archive::from_json_with_archive<test_data>(json_archive_str),
        ::cereal::Exception,
        Catch::Matchers::Message("Failed to load a container ID 99 from the "
                                 "archive: Unknown vector ID 99"));
}
