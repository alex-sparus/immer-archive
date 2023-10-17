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
#include <bnz/immer_vector.hpp>
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

template <typename T>
T from_json(std::string input)
{
    auto is = std::istringstream{input};
    auto ar = cereal::JSONInputArchive{is};
    auto r  = T{};
    ar(r);
    return r;
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

void save_to_file(std::string_view filename, std::string_view data)
{
    auto os = std::ofstream{filename};
    if (!os) {
        throw std::runtime_error{fmt::format("Failed to save to {}", filename)};
    }
    os.write(data.data(), data.size());
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
        save_to_file("vec01.json", to_json(ar));
    }

    {
        // 66 is the biggest vector with only one inner node
        const auto vec = gen(example_vector{}, 66);
        auto ar        = save_vector(vec, {});
        save_to_file("vec66.json", to_json(ar));
    }

    // {
    //     auto vec = example_vector{};
    //     for (auto size = std::size_t{1}; size < 10000; ++size) {
    //         vec     = std::move(vec).push_back(size);
    //         auto ar = save_vector(vec, {});
    //         if (ar.inners.size() >= 2) {
    //             SPDLOG_DEBUG("size is {}, ar = {}", size, to_json(ar));
    //             break;
    //         }
    //     }
    // }

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
    save_to_file("huge_vectors.json", to_json(ar));
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

void test_save_and_load()
{
    auto test_vectors = std::vector<example_vector>{// gen(example_vector{}, 67)
                                                    example_vector{}};
    auto counter      = std::size_t{};
    auto ar           = immer_archive::archive<int>{};
    const auto save_and_load = [&]() {
        auto vec = test_vectors.back().push_back(++counter);
        test_vectors.push_back(vec);
        ar           = immer_archive::save_vector(vec, ar);
        auto ar_json = to_json(ar);
        SPDLOG_DEBUG("ar = {}", ar_json);

        auto loader = immer_archive::loader<int>{
            from_json<immer_archive::archive<int>>(ar_json)};
        auto loaded_vec = loader.load_vector(ar.vectors.begin()->first);
        assert(loaded_vec.has_value());
        assert(*loaded_vec == vec);

        // save it again, causing the full traversal and crash
        // SPDLOG_DEBUG("size = {}", size);
        // SPDLOG_DEBUG("vec = {}", to_json(loaded_vec.value()));
        // auto ar2 = immer_archive::save_vector(loaded_vec.value(), {});
        // SPDLOG_DEBUG("ar = {}, ar2 = {}", to_json(ar), to_json(ar2));
    };
    for (int i = 0; i < 10; ++i) {
        save_and_load();
    }
}

void test_read_vector()
{
    const bool xcode = false;
    const auto load  = [](auto name) {
        SPDLOG_DEBUG("loading {}", name);
        auto prefix =
            (xcode ? std::string{"../"} : "") + "../immer-test/test/data/";
        return load_archive(prefix + name);
    };

    const auto check_file = [&](auto name) {
        auto ar     = load(name);
        auto loader = immer_archive::loader<int>{ar};
        for (const auto& [vector_id, vector_info] : ar.vectors) {
            auto vec = loader.load_vector(vector_id);
            assert(vec.value().impl().check_tree());

            // Iterate over the whole read vector, just to test.
            immer::vector<int> test;
            for (int i : vec.value()) {
                test = std::move(test).push_back(i);
            }
            SPDLOG_DEBUG("tested vector size {}", test.size());
        }
    };

    check_file("vec66.json");
    check_file("vec01.json");
    check_file("huge_vectors.json");
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
    // test_save_and_load();
    test_read_vector();

    return 0;
}
