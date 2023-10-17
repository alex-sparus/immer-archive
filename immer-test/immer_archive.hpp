#pragma once

#include <immer/array.hpp>
#include <immer/map.hpp>
#include <immer/vector.hpp>

#include <cereal/cereal.hpp>

#include <bnz/immer_vector.hpp>

namespace immer_archive {

// Fixing BL to 1, because by default it depends on the sizeof(T)
// If the size of T changes, it might change the number of elements stored in
// the leaf nodes, which might affect the layout of the tree.
template <typename T,
          typename MemoryPolicy         = immer::default_memory_policy,
          immer::detail::rbts::bits_t B = immer::default_bits>
using vector_one = immer::vector<T, MemoryPolicy, B, 1>;

template <typename T,
          typename MemoryPolicy         = immer::default_memory_policy,
          immer::detail::rbts::bits_t B = immer::default_bits>
using flex_vector_one = immer::flex_vector<T, MemoryPolicy, B, 1>;

using node_id = std::uint64_t;
static_assert(sizeof(void*) == sizeof(node_id));

template <class T>
struct leaf_node
{
    const T* begin;
    const T* end;
    immer::array<T> data; // NOTE: data is only used while reading the archive
};

struct inner_node
{
    immer::vector<node_id> children;
};

struct vector
{
    node_id root;
    node_id tail;
    std::size_t size;
    immer::detail::rbts::shift_t shift;
};

template <class T>
struct archive
{
    immer::map<node_id, leaf_node<T>> leaves;
    immer::map<node_id, inner_node> inners;
    immer::map<node_id, vector> vectors;
};

// This is needed to be able to use the archive that was not read from JSON
// because .data is set only while reading from JSON.
template <class T>
archive<T> fix_leaf_nodes(archive<T> ar)
{
    auto leaves = immer::map<node_id, leaf_node<T>>{};
    for (const auto& item : ar.leaves) {
        auto data = immer::array<T>{item.second.begin, item.second.end};
        auto leaf = leaf_node<T>{
            .begin = data.begin(),
            .end   = data.end(),
            .data  = data,
        };
        leaves = std::move(leaves).set(item.first, leaf);
    }
    ar.leaves = std::move(leaves);
    return ar;
}

/**
 * Serialization functions.
 */
template <class Archive, class T>
void save(Archive& ar, const leaf_node<T>& value)
{
    ar(cereal::make_size_tag(
        static_cast<cereal::size_type>(value.end - value.begin)));
    for (auto p = value.begin; p != value.end; ++p) {
        ar(*p);
    }
}

template <class Archive, class T>
void load(Archive& ar, leaf_node<T>& m)
{
    cereal::size_type size;
    ar(cereal::make_size_tag(size));

    for (auto i = cereal::size_type{}; i < size; ++i) {
        T x;
        ar(x);
        m.data = std::move(m.data).push_back(std::move(x));
    }

    m.begin = m.data.begin();
    m.end   = m.data.end();
}

template <class Archive>
void serialize(Archive& ar, inner_node& value)
{
    auto& children = value.children;
    ar(CEREAL_NVP(children));
}

template <class Archive>
void serialize(Archive& ar, vector& value)
{
    auto& root  = value.root;
    auto& tail  = value.tail;
    auto& size  = value.size;
    auto& shift = value.shift;
    ar(CEREAL_NVP(root), CEREAL_NVP(tail), CEREAL_NVP(size), CEREAL_NVP(shift));
}

template <class Archive, class T>
void serialize(Archive& ar, archive<T>& value)
{
    auto& leaves  = value.leaves;
    auto& inners  = value.inners;
    auto& vectors = value.vectors;
    ar(CEREAL_NVP(leaves), CEREAL_NVP(inners), CEREAL_NVP(vectors));
}

} // namespace immer_archive
