#pragma once

#include "archive.hpp"

namespace immer_archive {
namespace champ {

template <class K, class V, class Hash>
struct map_save
{
    champ_info champ;
    // Saving the archived map, so that no mutations are allowed to happen.
    immer::map<K, V, Hash> map;
};

template <class K, class V, class Hash, immer::detail::hamts::bits_t B>
struct map_archive_save
{
    nodes_save<std::pair<K, V>, B> nodes;
    immer::map<node_id, map_save<K, V, Hash>> maps;
};

template <class K,
          class V,
          immer::detail::hamts::bits_t B = immer::default_bits>
struct map_archive_load
{
    nodes_load<std::pair<K, V>, B> nodes;
    immer::map<node_id, champ_info> maps;
};

template <class K, class V, class Hash, immer::detail::hamts::bits_t B>
map_archive_load<K, V, B>
to_load_archive(const map_archive_save<K, V, Hash, B>& archive)
{
    auto maps = immer::map<node_id, champ_info>{};
    for (const auto& [key, map] : archive.maps) {
        maps = std::move(maps).set(key, map.champ);
    }

    return {
        .nodes = to_load_archive(archive.nodes),
        .maps  = std::move(maps),
    };
}

template <class Archive, class... T>
void save(Archive& ar, const map_save<T...>& value)
{
    save(ar, value.champ);
}

template <class Archive,
          class K,
          class V,
          class Hash,
          immer::detail::hamts::bits_t B>
void save(Archive& ar, const map_archive_save<K, V, Hash, B>& value)
{
    auto& nodes = value.nodes;
    auto& maps  = value.maps;
    ar(CEREAL_NVP(nodes), CEREAL_NVP(maps));
}

template <class Archive, class K, class V, immer::detail::hamts::bits_t B>
void load(Archive& ar, map_archive_load<K, V, B>& value)
{
    auto& nodes = value.nodes;
    auto& maps  = value.maps;
    ar(CEREAL_NVP(nodes), CEREAL_NVP(maps));
}

} // namespace champ
} // namespace immer_archive
