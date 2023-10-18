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
struct leaf_node_load
{
    const T* begin;
    const T* end;
    immer::array<T> data; // NOTE: data is only used while reading the archive
};

template <class T>
struct leaf_node_save
{
    const T* begin;
    const T* end;
};

struct inner_node
{
    immer::vector<node_id> children;
};

struct relaxed_child
{
    node_id node;
    std::size_t size;
};

struct relaxed_inner_node
{
    immer::vector<relaxed_child> children;
};

template <class T>
struct vector
{
    node_id root;
    node_id tail;
    std::size_t size;
    immer::detail::rbts::shift_t shift;

    // Saving the archived vector, so that no mutations are allowed to happen.
    vector_one<T> vector;
};

template <class T>
struct flex_vector
{
    node_id root;
    node_id tail;
    std::size_t size;
    immer::detail::rbts::shift_t shift;

    // Saving the archived vector, so that no mutations are allowed to happen.
    flex_vector_one<T> vector;
};

template <class T, class Leaf>
struct archive
{
    immer::map<node_id, Leaf> leaves;
    immer::map<node_id, inner_node> inners;
    immer::map<node_id, relaxed_inner_node> relaxed_inners;
    immer::map<node_id, vector<T>> vectors;
    immer::map<node_id, flex_vector<T>> flex_vectors;
};

template <class T>
using archive_load = archive<T, leaf_node_load<T>>;

template <class T>
using archive_save = archive<T, leaf_node_save<T>>;

// This is needed to be able to use the archive that was not read from JSON
// because .data is set only while reading from JSON.
template <class T>
archive_load<T> fix_leaf_nodes(archive_save<T> ar)
{
    auto leaves = immer::map<node_id, leaf_node_load<T>>{};
    for (const auto& item : ar.leaves) {
        auto data = immer::array<T>{item.second.begin, item.second.end};
        auto leaf = leaf_node_load<T>{
            .begin = data.begin(),
            .end   = data.end(),
            .data  = data,
        };
        leaves = std::move(leaves).set(item.first, leaf);
    }

    return {
        .leaves  = std::move(leaves),
        .inners  = std::move(ar.inners),
        .vectors = std::move(ar.vectors),
    };
}

/**
 * Serialization functions.
 */
template <class Archive, class T>
void save(Archive& ar, const leaf_node_save<T>& value)
{
    ar(cereal::make_size_tag(
        static_cast<cereal::size_type>(value.end - value.begin)));
    for (auto p = value.begin; p != value.end; ++p) {
        ar(*p);
    }
}

template <class Archive, class T>
void load(Archive& ar, leaf_node_load<T>& m)
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
void serialize(Archive& ar, relaxed_child& value)
{
    auto& node = value.node;
    auto& size = value.size;
    ar(CEREAL_NVP(node), CEREAL_NVP(size));
}

template <class Archive>
void serialize(Archive& ar, relaxed_inner_node& value)
{
    auto& children = value.children;
    ar(CEREAL_NVP(children));
}

template <class Archive, class T>
void serialize(Archive& ar, vector<T>& value)
{
    auto& root  = value.root;
    auto& tail  = value.tail;
    auto& size  = value.size;
    auto& shift = value.shift;
    ar(CEREAL_NVP(root), CEREAL_NVP(tail), CEREAL_NVP(size), CEREAL_NVP(shift));
}

template <class Archive, class T>
void serialize(Archive& ar, flex_vector<T>& value)
{
    auto& root  = value.root;
    auto& tail  = value.tail;
    auto& size  = value.size;
    auto& shift = value.shift;
    ar(CEREAL_NVP(root), CEREAL_NVP(tail), CEREAL_NVP(size), CEREAL_NVP(shift));
}

template <class Archive, class T, class Leaf>
void serialize(Archive& ar, archive<T, Leaf>& value)
{
    auto& leaves         = value.leaves;
    auto& inners         = value.inners;
    auto& relaxed_inners = value.relaxed_inners;
    auto& vectors        = value.vectors;
    auto& flex_vectors   = value.flex_vectors;
    ar(CEREAL_NVP(leaves),
       CEREAL_NVP(inners),
       CEREAL_NVP(relaxed_inners),
       CEREAL_NVP(vectors),
       CEREAL_NVP(flex_vectors));
}

} // namespace immer_archive
