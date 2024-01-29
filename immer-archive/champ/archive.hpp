#pragma once

#include <immer-archive/common/archive.hpp>

#include <immer/map.hpp>
#include <immer/set.hpp>
#include <immer/table.hpp>
#include <immer/vector.hpp>

#include <cereal/cereal.hpp>

#include <boost/endian/conversion.hpp>

namespace immer_archive {
namespace champ {

using node_id = std::uint64_t;

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

    auto tie() const { return std::tie(values, children, nodemap, datamap); }

    friend bool operator==(const inner_node_load& left,
                           const inner_node_load& right)
    {
        return left.tie() == right.tie();
    }
};

template <class T, immer::detail::hamts::bits_t B>
struct nodes_save
{
    immer::map<node_id, inner_node_save<T, B>> inners;
    immer::map<node_id, values_save<T>> collisions;

    immer::map<const void*, node_id> node_ptr_to_id;
};

template <class T, immer::detail::hamts::bits_t B = immer::default_bits>
struct nodes_load
{
    immer::map<node_id, inner_node_load<T, B>> inners;
    immer::map<node_id, values_load<T>> collisions;

    auto tie() const { return std::tie(inners, collisions); }

    friend bool operator==(const nodes_load& left, const nodes_load& right)
    {
        return left.tie() == right.tie();
    }
};

template <class T, immer::detail::hamts::bits_t B>
nodes_load<T, B> to_load_archive(const nodes_save<T, B>& archive)
{
    auto inners = immer::map<node_id, inner_node_load<T, B>>{};
    for (const auto& [key, inner] : archive.inners) {
        inners = std::move(inners).set(key,
                                       inner_node_load<T, B>{
                                           .values   = inner.values,
                                           .children = inner.children,
                                           .nodemap  = inner.nodemap,
                                           .datamap  = inner.datamap,
                                       });
    }

    auto collisions = immer::map<node_id, values_load<T>>{};
    for (const auto& [key, collision] : archive.collisions) {
        collisions = std::move(collisions).set(key, collision);
    }

    return {
        .inners     = std::move(inners),
        .collisions = std::move(collisions),
    };
}

/**
 * Serialization functions.
 */
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

template <class Archive, class T, immer::detail::hamts::bits_t B>
void save(Archive& ar, const nodes_save<T, B>& value)
{
    auto& inners     = value.inners;
    auto& collisions = value.collisions;
    ar(CEREAL_NVP(inners), CEREAL_NVP(collisions));
}

template <class Archive, class T, immer::detail::hamts::bits_t B>
void load(Archive& ar, nodes_load<T, B>& value)
{
    auto& inners     = value.inners;
    auto& collisions = value.collisions;
    ar(CEREAL_NVP(inners), CEREAL_NVP(collisions));
}

} // namespace champ
} // namespace immer_archive
