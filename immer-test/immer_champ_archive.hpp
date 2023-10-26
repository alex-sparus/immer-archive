#pragma once

#include <immer/map.hpp>
#include <immer/set.hpp>
#include <immer/vector.hpp>

#include <cereal/cereal.hpp>

// #include <bnz/immer_map.hpp>
// #include <bnz/immer_vector.hpp>

namespace immer_archive {
namespace champ {

using node_id = std::uint64_t;

struct champ_info
{
    node_id root;
    std::size_t size;
};

template <class T>
struct values_save
{
    const T* begin = nullptr;
    const T* end   = nullptr;
};

template <class T>
struct inner_node_save
{
    values_save<T> values;
    immer::vector<node_id> children;
};

template <class T, class Hash>
struct set_save
{
    champ_info champ;
    // Saving the archived set, so that no mutations are allowed to happen.
    immer::set<T, Hash> set;
};

template <class T>
struct set_load
{
    champ_info champ;
};

template <class T, class Hash>
struct archive_save
{
    immer::map<node_id, inner_node_save<T>> inners;
    immer::map<node_id, values_save<T>> collisions;
    immer::map<node_id, set_save<T, Hash>> sets;

    immer::map<const void*, node_id> node_ptr_to_id;
};

template <class T>
struct archive_load
{};

// template <class T>
// archive_load<T> fix_leaf_nodes(archive_save<T> ar)
// {
//     auto leaves = immer::map<node_id, leaf_node_load<T>>{};
//     for (const auto& item : ar.leaves) {
//         auto data = immer::array<T>{item.second.begin, item.second.end};
//         auto leaf = leaf_node_load<T>{
//             .begin = data.begin(),
//             .end   = data.end(),
//             .data  = data,
//         };
//         leaves = std::move(leaves).set(item.first, leaf);
//     }

//     auto vectors = immer::map<node_id, vector_load<T>>{};
//     for (const auto& [id, info] : ar.vectors) {
//         vectors = std::move(vectors).set(id,
//                                          vector_load<T>{
//                                              .rbts = info.rbts,
//                                          });
//     }

//     auto flex_vectors = immer::map<node_id, flex_vector_load<T>>{};
//     for (const auto& [id, info] : ar.flex_vectors) {
//         flex_vectors = std::move(flex_vectors)
//                            .set(id,
//                                 flex_vector_load<T>{
//                                     .rbts = info.rbts,
//                                 });
//     }

//     return {
//         .leaves         = std::move(leaves),
//         .inners         = std::move(ar.inners),
//         .relaxed_inners = std::move(ar.relaxed_inners),
//         .vectors        = std::move(vectors),
//         .flex_vectors   = std::move(flex_vectors),
//     };
// }

/**
 * Serialization functions.
 */
template <class Archive, class T>
void serialize(Archive& ar, values_save<T>& value)
{
    ar(cereal::make_size_tag(
        static_cast<cereal::size_type>(value.end - value.begin)));
    for (auto p = value.begin; p != value.end; ++p) {
        ar(*p);
    }
}

template <class Archive, class T>
void serialize(Archive& ar, inner_node_save<T>& value)
{
    auto& values   = value.values;
    auto& children = value.children;
    ar(CEREAL_NVP(values), CEREAL_NVP(children));
}

template <class Archive>
void save(Archive& ar, const champ_info& value)
{
    auto& root = value.root;
    auto& size = value.size;
    ar(CEREAL_NVP(root), CEREAL_NVP(size));
}

template <class Archive, class... T>
void save(Archive& ar, const set_save<T...>& value)
{
    save(ar, value.champ);
}

template <class Archive, class... T>
void save(Archive& ar, const archive_save<T...>& value)
{
    auto& inners     = value.inners;
    auto& collisions = value.collisions;
    auto& sets       = value.sets;
    ar(CEREAL_NVP(inners), CEREAL_NVP(collisions), CEREAL_NVP(sets));
}

} // namespace champ
} // namespace immer_archive
