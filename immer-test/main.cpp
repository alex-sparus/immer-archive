//
//  main.cpp
//  immer-test
//
//  Created by Alex Shabalin on 11/10/2023.
//

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
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

std::vector<immer_archive::archive_load<int>>
load_archive(const std::filesystem::path& filename)
{
    const auto open = [&] {
        auto is = std::ifstream{filename};
        if (!is) {
            throw std::runtime_error{
                fmt::format("Failed to read from {}", filename.c_str())};
        }
        return is;
    };

    try {
        auto result = immer_archive::archive_load<int>{};
        auto is     = open();
        {
            auto ar = cereal::JSONInputArchive{is};
            ar(result);
        }
        return {result};
    } catch (const cereal::Exception&) {
        auto result = std::vector<immer_archive::archive_load<int>>{};
        auto is     = open();
        {
            auto ar = cereal::JSONInputArchive{is};
            ar(result);
        }
        return result;
    }
}

auto load(auto name)
{
    const bool xcode = false;
    SPDLOG_DEBUG("loading {}", name);
    auto prefix =
        (xcode ? std::string{"../"} : "") + "../immer-test/test/data/";
    return load_archive(prefix + name);
}

void save_to_file(const std::filesystem::path& filename, std::string_view data)
{
    auto os = std::ofstream{filename};
    if (!os) {
        throw std::runtime_error{
            fmt::format("Failed to save to {}", filename.c_str())};
    }
    os.write(data.data(), data.size());
}

using example_vector      = immer_archive::vector_one<int>;
using example_flex_vector = immer_archive::flex_vector_one<int>;
using immer_archive::save_vector;

const auto gen = [](auto init, int count) {
    for (int i = 0; i < count; ++i) {
        init = std::move(init).push_back(i);
    }
    return init;
};

} // namespace

TEST_CASE("Saving vectors")
{
    {
        // empty
        const auto empty = example_vector{};
        const auto one   = empty.push_back(123);
        auto ar          = save_vector(empty, {}).first;
        ar               = save_vector(one, ar).first;
        save_to_file("vec01.json", to_json(ar));
    }

    {
        const auto vec = gen(example_vector{}, 7);
        auto ar        = save_vector(vec, {}).first;
        save_to_file("vec7.json", to_json(ar));
    }

    {
        // 66 is the biggest vector with only one inner node
        const auto vec = gen(example_vector{}, 66);
        auto ar        = save_vector(vec, {}).first;
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

    auto ar = save_vector(v65, {}).first;
    ar      = save_vector(v66, ar).first;
    ar      = save_vector(v67, ar).first;
    ar      = save_vector(v68, ar).first;
    ar      = save_vector(v900, ar).first;
    save_to_file("huge_vectors.json", to_json(ar));
}

TEST_CASE("Saving flex_vectors")
{
    {
        // empty
        const auto empty = immer_archive::flex_vector_one<int>{};
        const auto one   = empty.push_back(123);
        auto ar          = save_vector(empty, {}).first;
        ar               = save_vector(one, ar).first;
        save_to_file("flex_vec01.json", to_json(ar));
    }

    const auto v1 = gen(immer_archive::flex_vector_one<int>{}, 3);
    const auto v2 = gen(immer_archive::flex_vector_one<int>{}, 4);
    const auto v3 = v1 + v2;

    const auto v50  = gen(v3, 50);
    const auto v100 = v50 + v50;
    const auto v52  = v50 + v2;

    auto ar = save_vector(v1, {}).first;
    ar      = save_vector(v2, ar).first;
    ar      = save_vector(v3, ar).first;
    ar      = save_vector(v50, ar).first;
    ar      = save_vector(v100, ar).first;
    ar      = save_vector(v52, ar).first;

    auto archives = std::vector<immer_archive::archive_save<int>>{
        ar,
        save_vector(v1, {}).first,
        save_vector(v3, {}).first,
    };
    save_to_file("flex_concat.json", to_json(archives));
}

TEST_CASE("Save and load multiple times into the same archive")
{
    auto test_vectors = std::vector<example_vector>{// gen(example_vector{}, 67)
                                                    example_vector{}};
    auto counter      = std::size_t{};
    auto ar           = immer_archive::archive_save<int>{};
    const auto save_and_load = [&]() {
        const auto vec = test_vectors.back().push_back(++counter);
        test_vectors.push_back(vec);

        auto vector_id          = immer_archive::node_id{};
        std::tie(ar, vector_id) = immer_archive::save_vector(vec, ar);

        auto loader     = immer_archive::loader<int>{fix_leaf_nodes(ar)};
        auto loaded_vec = loader.load_vector(vector_id);
        REQUIRE(loaded_vec.has_value());
        REQUIRE(*loaded_vec == vec);

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

TEST_CASE("Read vectors")
{
    const auto check_file = [&](auto name) {
        const auto archives = load(name);
        for (const auto& ar : archives) {
            auto loader = immer_archive::loader<int>{ar};
            for (const auto& [vector_id, vector_info] : ar.vectors) {
                auto vec = loader.load_vector(vector_id);
                REQUIRE(vec.value().impl().check_tree());

                // Iterate over the whole read vector, just to test.
                immer::vector<int> test;
                for (int i : vec.value()) {
                    test = std::move(test).push_back(i);
                }
                SPDLOG_DEBUG("tested vector size {}", test.size());
            }
        }
    };

    check_file("vec66.json");
    check_file("vec01.json");
    check_file("huge_vectors.json");
}

TEST_CASE("Read flex vectors")
{
    const auto check_file = [&](auto name) {
        const auto archives = load(name);
        for (const auto& ar : archives) {
            auto loader = immer_archive::loader<int>{ar};
            for (const auto& [vector_id, vector_info] : ar.flex_vectors) {
                auto vec = loader.load_flex_vector(vector_id);
                REQUIRE(vec.value().impl().check_tree());

                // Iterate over the whole read vector, just to test.
                auto test = example_vector{};
                for (int i : vec.value()) {
                    test = std::move(test).push_back(i);
                }
                REQUIRE(test == vec.value());
                SPDLOG_DEBUG("tested vector size {}", test.size());
                SPDLOG_DEBUG("vec = {}", to_json(vec.value()));
            }
        }
    };

    check_file("flex_vec01.json");
    check_file("flex_concat.json");
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
        -> std::pair<immer_archive::archive_save<int>,
                     std::vector<immer_archive::node_id>> {
        auto ar  = immer_archive::archive_save<int>{};
        auto ids = std::vector<immer_archive::node_id>{};
        for (const auto& v : vectors) {
            auto [ar2, id] = save_vector(v, ar);
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
    auto loader = immer_archive::loader<int>{fix_leaf_nodes(ar)};
    std::vector<example_vector> loaded;
    auto index = std::size_t{};
    for (const auto& id : ids) {
        auto v = loader.load_vector(id);
        REQUIRE(v.has_value());
        REQUIRE(v.value() == vectors[index]);
        loaded.push_back(v.value());
        ++index;
    }

    SPDLOG_DEBUG("loaded == vectors {}", loaded == vectors);
    REQUIRE(loaded == vectors);

    loaded = {};
    // Now the loader should deallocated all the nodes it has.
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
    const auto save_vectors = [](immer_archive::archive_save<int> ar,
                                 const auto& vectors)
        -> std::pair<immer_archive::archive_save<int>,
                     std::vector<immer_archive::node_id>> {
        auto ids = std::vector<immer_archive::node_id>{};
        for (const auto& v : vectors) {
            auto [ar2, id] = save_vector(v, ar);
            ar             = ar2;
            ids.push_back(id);
        }
        REQUIRE(ids.size() == vectors.size());
        return {std::move(ar), std::move(ids)};
    };

    auto ar                        = immer_archive::archive_save<int>{};
    const auto vectors             = generate_vectors();
    const auto flex_vectors        = generate_flex_vectors();
    auto vector_ids                = std::vector<immer_archive::node_id>{};
    auto flex_vectors_ids          = std::vector<immer_archive::node_id>{};
    std::tie(ar, vector_ids)       = save_vectors(ar, vectors);
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
    auto loader = immer_archive::loader<int>{fix_leaf_nodes(ar)};
    auto loaded = [&] {
        auto result = std::vector<example_vector>{};
        auto index  = std::size_t{};
        for (const auto& id : vector_ids) {
            auto v = loader.load_vector(id);
            REQUIRE(v.has_value());
            REQUIRE(v.value() == vectors[index]);
            result.push_back(v.value());
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
            REQUIRE(v.has_value());
            REQUIRE(v.value() == flex_vectors[index]);
            result.push_back(v.value());
            ++index;
        }
        return result;
    }();
    REQUIRE(loaded_flex == flex_vectors);

    loaded = {};
    // Now the loader should deallocated all the nodes it has.
}
