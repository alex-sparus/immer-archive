//
//  main.cpp
//  immer-test
//
//  Created by Alex Shabalin on 11/10/2023.
//

#include <iostream>

#include "immer_save.hpp"

#include <spdlog/spdlog.h>

#include <bnz/immer_map.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>

#include <immer/algorithm.hpp>
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

} // namespace

int main(int argc, const char* argv[])
{
    spdlog::set_level(spdlog::level::debug);

    //    auto vec = example_vector{1};
    //    auto v2  = vec.push_back(2);
    //    save_vector(v2);

    const auto gen = [](auto init, int count) {
        for (int i = 0; i < count; ++i) {
            init = std::move(init).push_back(i);
        }
        return init;
    };

    const auto v65  = gen(example_vector{}, 67);
    const auto v66  = v65.push_back(1337);
    const auto v67  = v66.push_back(1338);
    const auto v68  = v67.push_back(1339);
    const auto v900 = gen(v68, 9999);

    using immer_archive::save_vector;
    auto ar = save_vector(v65, {});
    ar      = save_vector(v66, ar);
    ar      = save_vector(v67, ar);
    ar      = save_vector(v68, ar);
    ar      = save_vector(v900, ar);

    SPDLOG_DEBUG("archive = {}", to_json(ar));

    // auto big_branches = gen(immer::vector<std::uint64_t>{}, 67);
    // traverse_nodes(big_branches.impl());

    return 0;
}
