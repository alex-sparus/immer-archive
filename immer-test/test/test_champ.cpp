#include <catch2/catch_test_macros.hpp>

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
    REQUIRE(to_json(ar) == "");
}
