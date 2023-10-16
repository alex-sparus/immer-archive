#pragma once

#include <immer/map.hpp>
#include <immer/vector.hpp>

#include <cereal/cereal.hpp>

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
};

template <class T>
struct archive
{
    immer::map<node_id, leaf_node<T>> leaves;
    immer::map<node_id, inner_node> inners;
    immer::map<node_id, vector> vectors;
};

/**
 * Serialization functions.
 */
template <class Archive, class T>
void serialize(Archive& ar, leaf_node<T>& value)
{
    std::vector<T> dump{value.begin, value.end};
    ar(cereal::make_nvp("values", dump));
}

template <class Archive>
void serialize(Archive& ar, inner_node& value)
{
    std::vector<node_id> dump{value.children.begin(), value.children.end()};
    ar(cereal::make_nvp("children", dump));
}

template <class Archive>
void serialize(Archive& ar, vector& value)
{
    auto& root = value.root;
    auto& tail = value.tail;
    auto& size = value.size;
    ar(CEREAL_NVP(root), CEREAL_NVP(tail), CEREAL_NVP(size));
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
