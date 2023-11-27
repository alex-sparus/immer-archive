#include <catch2/catch_test_macros.hpp>

#include "test/test_data.hpp"
#include <test/utils.hpp>

using namespace test;

// "[.]" is to not run it by default
TEST_CASE("Test saving and loading a structure", "[.]")
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
