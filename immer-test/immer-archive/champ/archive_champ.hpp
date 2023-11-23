#pragma once

#include "archive.hpp"

namespace immer_archive {
namespace champ {

/**
 * Container is a champ-based container.
 */

template <class Container>
struct container_save
{
    champ_info champ;
    // Saving the archived container, so that no mutations are allowed to
    // happen.
    Container container;
};

template <class Container>
struct container_archive_save
{
    using champ_t = std::decay_t<decltype(std::declval<Container>().impl())>;
    using T       = typename champ_t::node_t::value_t;

    nodes_save<T, champ_t::bits> nodes;
    immer::map<node_id, container_save<Container>> containers;
};

template <class Container>
struct container_archive_load
{
    using champ_t = std::decay_t<decltype(std::declval<Container>().impl())>;
    using T       = typename champ_t::node_t::value_t;

    nodes_load<T, champ_t::bits> nodes;
    immer::map<node_id, champ_info> containers;

    template <class ArchivesLoad>
    void inflate(ArchivesLoad&)
    {
    }
};

template <class Container>
container_archive_load<Container>
to_load_archive(const container_archive_save<Container>& archive)
{
    auto containers = immer::map<node_id, champ_info>{};
    for (const auto& [key, value] : archive.containers) {
        containers = std::move(containers).set(key, value.champ);
    }

    return {
        .nodes      = to_load_archive(archive.nodes),
        .containers = std::move(containers),
    };
}

template <class Archive, class... T>
void save(Archive& ar, const container_save<T...>& value)
{
    save(ar, value.champ);
}

template <class Archive, class Container>
void save(Archive& ar, const container_archive_save<Container>& value)
{
    auto& nodes      = value.nodes;
    auto& containers = value.containers;
    ar(CEREAL_NVP(nodes), CEREAL_NVP(containers));
}

template <class Archive, class Container>
void load(Archive& ar, container_archive_load<Container>& value)
{
    auto& nodes      = value.nodes;
    auto& containers = value.containers;
    ar(CEREAL_NVP(nodes), CEREAL_NVP(containers));
}

} // namespace champ
} // namespace immer_archive
