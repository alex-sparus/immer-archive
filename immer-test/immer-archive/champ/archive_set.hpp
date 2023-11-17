#pragma once

#include "archive.hpp"

namespace immer_archive {
namespace champ {

template <class T, class Hash>
struct set_save
{
    champ_info champ;
    // Saving the archived set, so that no mutations are allowed to happen.
    immer::set<T, Hash> set;
};

template <class T, class Hash, immer::detail::hamts::bits_t B>
struct set_archive_save
{
    nodes_save<T, B> nodes;
    immer::map<node_id, set_save<T, Hash>> sets;
};

template <class T, immer::detail::hamts::bits_t B = immer::default_bits>
struct set_archive_load
{
    nodes_load<T, B> nodes;
    immer::map<node_id, champ_info> sets;
};

template <class T, class Hash, immer::detail::hamts::bits_t B>
set_archive_load<T, B>
to_load_archive(const set_archive_save<T, Hash, B>& archive)
{
    auto sets = immer::map<node_id, champ_info>{};
    for (const auto& [key, set] : archive.sets) {
        sets = std::move(sets).set(key, set.champ);
    }

    return {
        .nodes = to_load_archive(archive.nodes),
        .sets  = std::move(sets),
    };
}

template <class Archive, class... T>
void save(Archive& ar, const set_save<T...>& value)
{
    save(ar, value.champ);
}

template <class Archive, class T, class Hash, immer::detail::hamts::bits_t B>
void save(Archive& ar, const set_archive_save<T, Hash, B>& value)
{
    auto& nodes = value.nodes;
    auto& sets  = value.sets;
    ar(CEREAL_NVP(nodes), CEREAL_NVP(sets));
}

template <class Archive, class T, immer::detail::hamts::bits_t B>
void load(Archive& ar, set_archive_load<T, B>& value)
{
    auto& nodes = value.nodes;
    auto& sets  = value.sets;
    ar(CEREAL_NVP(nodes), CEREAL_NVP(sets));
}

} // namespace champ
} // namespace immer_archive
