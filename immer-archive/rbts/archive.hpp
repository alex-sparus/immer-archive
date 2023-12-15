#pragma once

#include <immer/array.hpp>
#include <immer/flex_vector.hpp>
#include <immer/map.hpp>
#include <immer/vector.hpp>

#include <cereal/cereal.hpp>

#include <immer-archive/cereal/immer_map.hpp>
#include <immer-archive/cereal/immer_vector.hpp>

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

} // namespace immer_archive

namespace immer_archive::rbts {

using node_id = std::uint64_t;
static_assert(sizeof(void*) == sizeof(node_id));

template <class T>
struct leaf_node_load
{
    immer::array<T> data;

    friend bool operator==(const leaf_node_load& left,
                           const leaf_node_load& right)
    {
        return left.data == right.data;
    }
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
    bool relaxed = {};

    auto tie() const { return std::tie(children, relaxed); }

    friend bool operator==(const inner_node& left, const inner_node& right)
    {
        return left.tie() == right.tie();
    }
};

struct rbts_info
{
    node_id root;
    node_id tail;
    immer::detail::rbts::shift_t shift;

    auto tie() const { return std::tie(root, tail, shift); }

    friend bool operator==(const rbts_info& left, const rbts_info& right)
    {
        return left.tie() == right.tie();
    }
};

struct rbts_id
{
    node_id root;
    node_id tail;

    auto tie() const { return std::tie(root, tail); }

    friend bool operator==(const rbts_id& left, const rbts_id& right)
    {
        return left.tie() == right.tie();
    }
};

template <class T>
struct vector_save
{
    rbts_info rbts;
    // Saving the archived vector, so that no mutations are allowed to happen.
    vector_one<T> vector;
};

template <class T>
struct vector_load
{
    rbts_info rbts;

    friend bool operator==(const vector_load& left, const vector_load& right)
    {
        return left.rbts == right.rbts;
    }
};

template <class T>
struct flex_vector_save
{
    rbts_info rbts;
    // Saving the archived vector, so that no mutations are allowed to happen.
    flex_vector_one<T> vector;
};

template <class T>
struct flex_vector_load
{
    rbts_info rbts;

    friend bool operator==(const flex_vector_load& left,
                           const flex_vector_load& right)
    {
        return left.rbts == right.rbts;
    }
};

template <class T>
struct archive_save
{
    immer::map<node_id, leaf_node_save<T>> leaves;
    immer::map<node_id, inner_node> inners;
    immer::map<node_id, vector_save<T>> vectors;
    immer::map<node_id, flex_vector_save<T>> flex_vectors;

    immer::map<rbts_id, node_id> rbts_to_id;
    immer::map<const void*, node_id> node_ptr_to_id;
};

template <class T>
struct archive_load
{
    immer::map<node_id, leaf_node_load<T>> leaves;
    immer::map<node_id, inner_node> inners;
    immer::map<node_id, vector_load<T>> vectors;
    immer::map<node_id, flex_vector_load<T>> flex_vectors;

    auto tie() const { return std::tie(leaves, inners, vectors, flex_vectors); }

    friend bool operator==(const archive_load& left, const archive_load& right)
    {
        return left.tie() == right.tie();
    }
};

// This is needed to be able to use the archive that was not read from JSON
// because .data is set only while reading from JSON.
template <class T>
archive_load<T> fix_leaf_nodes(archive_save<T> ar)
{
    auto leaves = immer::map<node_id, leaf_node_load<T>>{};
    for (const auto& item : ar.leaves) {
        auto leaf = leaf_node_load<T>{
            .data = immer::array<T>{item.second.begin, item.second.end},
        };
        leaves = std::move(leaves).set(item.first, leaf);
    }

    auto vectors = immer::map<node_id, vector_load<T>>{};
    for (const auto& [id, info] : ar.vectors) {
        vectors = std::move(vectors).set(id,
                                         vector_load<T>{
                                             .rbts = info.rbts,
                                         });
    }

    auto flex_vectors = immer::map<node_id, flex_vector_load<T>>{};
    for (const auto& [id, info] : ar.flex_vectors) {
        flex_vectors = std::move(flex_vectors)
                           .set(id,
                                flex_vector_load<T>{
                                    .rbts = info.rbts,
                                });
    }

    return {
        .leaves       = std::move(leaves),
        .inners       = std::move(ar.inners),
        .vectors      = std::move(vectors),
        .flex_vectors = std::move(flex_vectors),
    };
}

/**
 * Serialization functions.
 */
template <
    class Archive,
    class T,
    typename = std::enable_if_t<
        cereal::traits::detail::count_output_serializers<T, Archive>::value !=
        0>>
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
}

template <class Archive>
void serialize(Archive& ar, inner_node& value)
{
    auto& children = value.children;
    auto& relaxed  = value.relaxed;
    ar(CEREAL_NVP(children), CEREAL_NVP(relaxed));
}

template <class Archive>
void save(Archive& ar, const rbts_info& value)
{
    auto& root  = value.root;
    auto& tail  = value.tail;
    auto& shift = value.shift;
    ar(CEREAL_NVP(root), CEREAL_NVP(tail), CEREAL_NVP(shift));
}

template <class Archive>
void load(Archive& ar, rbts_info& value)
{
    auto& root  = value.root;
    auto& tail  = value.tail;
    auto& shift = value.shift;
    ar(CEREAL_NVP(root), CEREAL_NVP(tail), CEREAL_NVP(shift));
}

template <class Archive>
void serialize(Archive& ar, rbts_id& value)
{
    auto& root = value.root;
    auto& tail = value.tail;
    ar(CEREAL_NVP(root), CEREAL_NVP(tail));
}

template <class Archive, class T>
void save(Archive& ar, const vector_save<T>& value)
{
    save(ar, value.rbts);
}

template <class Archive, class T>
void load(Archive& ar, vector_load<T>& value)
{
    load(ar, value.rbts);
}

template <class Archive, class T>
void save(Archive& ar, const flex_vector_save<T>& value)
{
    save(ar, value.rbts);
}

template <class Archive, class T>
void load(Archive& ar, flex_vector_load<T>& value)
{
    load(ar, value.rbts);
}

template <class Archive, class... T>
void save(Archive& ar, const archive_save<T...>& value)
{
    auto& leaves       = value.leaves;
    auto& inners       = value.inners;
    auto& vectors      = value.vectors;
    auto& flex_vectors = value.flex_vectors;
    ar(CEREAL_NVP(leaves),
       CEREAL_NVP(inners),
       CEREAL_NVP(vectors),
       CEREAL_NVP(flex_vectors));
}

template <class Archive, class... T>
void load(Archive& ar, archive_load<T...>& value)
{
    auto& leaves       = value.leaves;
    auto& inners       = value.inners;
    auto& vectors      = value.vectors;
    auto& flex_vectors = value.flex_vectors;
    ar(CEREAL_NVP(leaves),
       CEREAL_NVP(inners),
       CEREAL_NVP(vectors),
       CEREAL_NVP(flex_vectors));
}

} // namespace immer_archive::rbts

namespace std {

template <>
struct hash<immer_archive::rbts::rbts_id>
{
    auto operator()(const immer_archive::rbts::rbts_id& x) const
    {
        const auto boost_combine = [](std::size_t& seed, std::size_t hash) {
            seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        };

        auto seed = std::size_t{};
        boost_combine(seed, hash<immer_archive::rbts::node_id>{}(x.root));
        boost_combine(seed, hash<immer_archive::rbts::node_id>{}(x.tail));
        return seed;
    }
};

} // namespace std
