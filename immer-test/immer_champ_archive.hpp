#pragma once

#include <immer/array.hpp>
#include <immer/map.hpp>
#include <immer/set.hpp>
#include <immer/vector.hpp>

#include <cereal/cereal.hpp>

#include <boost/endian/conversion.hpp>

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
struct values_load
{
    immer::array<T> data;
};

template <class T, immer::detail::hamts::bits_t B>
struct inner_node_save
{
    using bitmap_t = typename immer::detail::hamts::get_bitmap_type<B>::type;

    values_save<T> values;
    immer::vector<node_id> children;
    bitmap_t nodemap;
    bitmap_t datamap;
};

template <class T, immer::detail::hamts::bits_t B>
struct inner_node_load
{
    using bitmap_t = typename immer::detail::hamts::get_bitmap_type<B>::type;

    values_load<T> values;
    immer::vector<node_id> children;
    bitmap_t nodemap;
    bitmap_t datamap;
};

template <class T, class Hash>
struct set_save
{
    champ_info champ;
    // Saving the archived set, so that no mutations are allowed to happen.
    immer::set<T, Hash> set;
};

template <class T, class Hash, immer::detail::hamts::bits_t B>
struct archive_save
{
    immer::map<node_id, inner_node_save<T, B>> inners;
    immer::map<node_id, values_save<T>> collisions;
    immer::map<node_id, set_save<T, Hash>> sets;

    immer::map<const void*, node_id> node_ptr_to_id;
};

template <class T, immer::detail::hamts::bits_t B = immer::default_bits>
struct archive_load
{
    immer::map<node_id, inner_node_load<T, B>> inners;
    immer::map<node_id, values_load<T>> collisions;
    immer::map<node_id, champ_info> sets;
};

template <class T, class Hash, immer::detail::hamts::bits_t B>
archive_load<T, B> to_load_archive(const archive_save<T, Hash, B>& archive)
{
    auto inners = immer::map<node_id, inner_node_load<T, B>>{};
    for (const auto& [key, inner] : archive.inners) {
        inners = std::move(inners).set(
            key,
            inner_node_load<T, B>{
                .values   = {immer::array<T>{inner.values.begin,
                                             inner.values.end}},
                .children = inner.children,
                .nodemap  = inner.nodemap,
                .datamap  = inner.datamap,
            });
    }

    auto collisions = immer::map<node_id, values_load<T>>{};
    for (const auto& [key, collision] : archive.collisions) {
        collisions = std::move(collisions)
                         .set(key,
                              values_load<T>{
                                  .data = immer::array<T>{collision.begin,
                                                          collision.end},
                              });
    }

    auto sets = immer::map<node_id, champ_info>{};
    for (const auto& [key, set] : archive.sets) {
        sets = std::move(sets).set(key, set.champ);
    }

    return {
        .inners     = std::move(inners),
        .collisions = std::move(collisions),
        .sets       = std::move(sets),
    };
}

/**
 * Serialization functions.
 */
template <class Archive, class T>
void save(Archive& ar, const values_save<T>& value)
{
    ar(cereal::make_size_tag(
        static_cast<cereal::size_type>(value.end - value.begin)));
    for (auto p = value.begin; p != value.end; ++p) {
        ar(*p);
    }
}

template <class Archive, class T>
void load(Archive& ar, values_load<T>& m)
{
    cereal::size_type size;
    ar(cereal::make_size_tag(size));

    for (auto i = cereal::size_type{}; i < size; ++i) {
        T x;
        ar(x);
        m.data = std::move(m.data).push_back(std::move(x));
    }
}

template <class Archive, class T, immer::detail::hamts::bits_t B>
void save(Archive& ar, const inner_node_save<T, B>& value)
{
    auto& values       = value.values;
    auto& children     = value.children;
    const auto nodemap = boost::endian::native_to_big(value.nodemap);
    const auto datamap = boost::endian::native_to_big(value.datamap);
    ar(CEREAL_NVP(values),
       CEREAL_NVP(children),
       CEREAL_NVP(nodemap),
       CEREAL_NVP(datamap));
}

template <class Archive, class T, immer::detail::hamts::bits_t B>
void load(Archive& ar, inner_node_load<T, B>& value)
{
    auto& values   = value.values;
    auto& children = value.children;
    auto& nodemap  = value.nodemap;
    auto& datamap  = value.datamap;
    ar(CEREAL_NVP(values),
       CEREAL_NVP(children),
       CEREAL_NVP(nodemap),
       CEREAL_NVP(datamap));
    boost::endian::big_to_native_inplace(nodemap);
    boost::endian::big_to_native_inplace(datamap);
}

template <class Archive>
void save(Archive& ar, const champ_info& value)
{
    auto& root = value.root;
    auto& size = value.size;
    ar(CEREAL_NVP(root), CEREAL_NVP(size));
}

template <class Archive>
void load(Archive& ar, champ_info& value)
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

template <class Archive, class T, class Hash, immer::detail::hamts::bits_t B>
void save(Archive& ar, const archive_save<T, Hash, B>& value)
{
    auto& inners     = value.inners;
    auto& collisions = value.collisions;
    auto& sets       = value.sets;
    ar(CEREAL_NVP(inners), CEREAL_NVP(collisions), CEREAL_NVP(sets));
}

template <class Archive, class T, immer::detail::hamts::bits_t B>
void load(Archive& ar, archive_load<T, B>& value)
{
    auto& inners     = value.inners;
    auto& collisions = value.collisions;
    auto& sets       = value.sets;
    ar(CEREAL_NVP(inners), CEREAL_NVP(collisions), CEREAL_NVP(sets));
}

} // namespace champ
} // namespace immer_archive
