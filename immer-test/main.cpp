//
//  main.cpp
//  immer-test
//
//  Created by Alex Shabalin on 11/10/2023.
//

#include <fstream>
#include <iostream>

#include "immer_save.hpp"
#include "rbtree_builder.hpp"

#include <spdlog/spdlog.h>

#include <bnz/immer_map.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>

#include <immer/algorithm.hpp>
#include <immer/flex_vector.hpp>
#include <immer/map.hpp>
#include <immer/vector.hpp>

namespace {
template <typename T>
std::string to_json(const T& serializable)
{
    auto os = std::ostringstream{};
    {
        auto ar = cereal::JSONOutputArchive{os};
        ar(serializable);
    }
    return os.str();
}
} // namespace

namespace {

using example_vector = immer_archive::vector_one<int>;

const auto gen = [](auto init, int count) {
    for (int i = 0; i < count; ++i) {
        init = std::move(init).push_back(i);
    }
    return init;
};

void test_vector()
{
    using immer_archive::save_vector;

    {
        // empty
        const auto empty = example_vector{};
        const auto one   = empty.push_back(123);
        auto ar          = save_vector(empty, {});
        ar               = save_vector(one, ar);
        SPDLOG_DEBUG("archive = {}", to_json(ar));
    }

    const auto v65  = gen(example_vector{}, 67);
    const auto v66  = v65.push_back(1337);
    const auto v67  = v66.push_back(1338);
    const auto v68  = v67.push_back(1339);
    const auto v900 = gen(v68, 9999);

    auto ar = save_vector(v65, {});
    ar      = save_vector(v66, ar);
    ar      = save_vector(v67, ar);
    ar      = save_vector(v68, ar);
    ar      = save_vector(v900, ar);

    SPDLOG_DEBUG("archive = {}", to_json(ar));
}

void test_flex_vector()
{
    using immer_archive::save_vector;

    {
        // empty
        const auto empty = immer_archive::flex_vector_one<int>{};
        const auto one   = empty.push_back(123);
        auto ar          = save_vector(empty, {});
        ar               = save_vector(one, ar);
        SPDLOG_DEBUG("empty and one archive = {}", to_json(ar));
    }

    const auto v1 = gen(immer_archive::flex_vector_one<int>{}, 3);
    const auto v2 = gen(immer_archive::flex_vector_one<int>{}, 4);

    const auto v3 = v1 + v2;

    auto ar = save_vector(v1, {});
    ar      = save_vector(v2, ar);
    ar      = save_vector(v3, ar);

    SPDLOG_DEBUG("archive = {}", to_json(ar));
    SPDLOG_DEBUG("archive2 = {}", to_json(save_vector(v1, {})));
    SPDLOG_DEBUG("archive2 = {}", to_json(save_vector(v3, {})));
}

immer_archive::archive<int> load_archive(std::string_view filename)
{
    auto is = std::ifstream{filename};
    if (!is) {
        throw std::runtime_error{
            fmt::format("Failed to read from {}", filename)};
    }

    auto result = immer_archive::archive<int>{};
    {
        auto ar = cereal::JSONInputArchive{is};
        ar(result);
    }
    return result;
}

void test_read_vector()
{
    auto ar = load_archive("../immer-test/vec01.json");
    SPDLOG_DEBUG("archive = {}", to_json(ar));
}

} // namespace

int main(int argc, const char* argv[])
{
    spdlog::set_level(spdlog::level::debug);

    //    auto vec = example_vector{1};
    //    auto v2  = vec.push_back(2);
    //    save_vector(v2);

    // auto big_branches = gen(immer::vector<std::uint64_t>{}, 67);
    // traverse_nodes(big_branches.impl());

    // test_vector();
    // test_flex_vector();
    test_read_vector();

    return 0;
}
