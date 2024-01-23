#include <catch2/catch_test_macros.hpp>

#include <immer-archive/xxhash/xxhash.hpp>

#include <xxhash.h>

TEST_CASE("Test hash strings")
{
    auto str = std::string{};
    REQUIRE(immer_archive::xx_hash<std::string>{}(str) == 3244421341483603138);
    REQUIRE(XXH3_64bits(str.c_str(), str.size()) == 3244421341483603138);

    str = "hello";
    REQUIRE(immer_archive::xx_hash<std::string>{}(str) ==
            10760762337991515389UL);
    REQUIRE(XXH3_64bits(str.c_str(), str.size()) == 10760762337991515389UL);
}
