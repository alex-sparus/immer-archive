#include <catch2/catch_test_macros.hpp>

#include <test/utils.hpp>

#include <boost/hana.hpp>
#include <immer-archive/json/archivable.hpp>
#include <immer-archive/json/json_with_archive.hpp>
#include <immer-archive/rbts/vector.hpp>

// to save std::pair
#include <cereal/types/utility.hpp>

namespace {

namespace hana = boost::hana;

/**
 * Some user data type that contains some vector_one_archivable, which should be
 * serialized in a special way.
 */
struct test_data
{
    immer_archive::archivable<immer_archive::vector_one<int>> ints;
    immer_archive::archivable<immer_archive::vector_one<std::string>> strings;

    immer_archive::archivable<immer_archive::flex_vector_one<int>> flex_ints;

    auto tie() const { return std::tie(ints, strings, flex_ints); }

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
        ar(CEREAL_NVP(ints), CEREAL_NVP(strings), CEREAL_NVP(flex_ints));
    }
};

/**
 * A special function that enumerates which types of immer-archives are
 * required. Explicitly name each type, for simplicity.
 */
inline auto get_archives_types(const test_data&)
{
    auto names = hana::make_map(
        hana::make_pair(hana::type_c<immer_archive::vector_one<int>>,
                        BOOST_HANA_STRING("ints")),
        hana::make_pair(hana::type_c<immer_archive::vector_one<std::string>>,
                        BOOST_HANA_STRING("strings")),
        hana::make_pair(hana::type_c<immer_archive::flex_vector_one<int>>,
                        BOOST_HANA_STRING("flex_ints"))

    );
    return names;
}

inline auto get_archives_types(const std::pair<test_data, test_data>&)
{
    return get_archives_types(test_data{});
}

} // namespace

TEST_CASE("Save with a special archive")
{
    const auto ints1 = test::gen(test::example_vector{}, 3);
    const auto test1 = test_data{
        .ints      = ints1,
        .strings   = {"one", "two"},
        .flex_ints = immer_archive::flex_vector_one<int>{ints1},
    };

    const auto [json_str, archives] =
        immer_archive::to_json_with_archive(test1);
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
            auto is = std::istringstream{archives_json};
            auto ar = cereal::JSONInputArchive{is};
            using Archives =
                decltype(immer_archive::detail::generate_archives_load(
                    get_archives_types(test_data{})));
            auto archives = Archives{};
            ar(CEREAL_NVP(archives));
            return archives;
        }();
        REQUIRE(archives_loaded
                    .storage[hana::type_c<immer_archive::vector_one<int>>]
                    .archive.leaves.size() == 2);
    }

    // REQUIRE(json_str == "");

    {
        auto full_load =
            immer_archive::from_json_with_archive<test_data>(json_str);
        REQUIRE(full_load == test1);
    }
}

TEST_CASE("Save with a special archive, special type is enclosed")
{
    const auto ints1 = test::gen(test::example_vector{}, 3);
    const auto test1 = test_data{
        .ints      = ints1,
        .strings   = {"one", "two"},
        .flex_ints = immer_archive::flex_vector_one<int>{ints1},
    };
    const auto test2 = test_data{
        .ints      = ints1,
        .strings   = {"three"},
        .flex_ints = immer_archive::flex_vector_one<int>{ints1},
    };

    // At the beginning, the vector is shared, it's the same data.
    REQUIRE(test1.ints.container.identity() == test2.ints.container.identity());
    REQUIRE(test1.flex_ints.container.identity() ==
            test2.flex_ints.container.identity());

    const auto [json_str, archives] =
        immer_archive::to_json_with_archive(std::make_pair(test1, test2));
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
            auto is = std::istringstream{archives_json};
            auto ar = cereal::JSONInputArchive{is};
            using Archives =
                decltype(immer_archive::detail::generate_archives_load(
                    get_archives_types(test_data{})));
            auto archives = Archives{};
            ar(CEREAL_NVP(archives));
            return archives;
        }();
        REQUIRE(archives_loaded
                    .storage[hana::type_c<immer_archive::vector_one<int>>]
                    .archive.leaves.size() == 2);
    }

    // REQUIRE(json_str == "");

    {
        auto [loaded1, loaded2] = immer_archive::from_json_with_archive<
            std::pair<test_data, test_data>>(json_str);
        REQUIRE(loaded1 == test1);
        REQUIRE(loaded2 == test2);

        // After loading, two vectors are still reused.
        REQUIRE(loaded1.ints.container.identity() ==
                loaded2.ints.container.identity());
        REQUIRE(loaded1.flex_ints.container.identity() ==
                loaded2.flex_ints.container.identity());
    }
}
