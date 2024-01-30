#pragma once

#include "archive.hpp"

namespace immer_archive {
namespace champ {

/**
 * Container is a champ-based container.
 */
template <class Container>
struct container_archive_save
{
    using champ_t = std::decay_t<decltype(std::declval<Container>().impl())>;
    using T       = typename champ_t::node_t::value_t;

    nodes_save<T, champ_t::bits> nodes;

    // Saving the archived container, so that no mutations are allowed to
    // happen.
    immer::vector<Container> containers;
};

template <class Container>
struct container_archive_load
{
    using champ_t = std::decay_t<decltype(std::declval<Container>().impl())>;
    using T       = typename champ_t::node_t::value_t;

    nodes_load<T, champ_t::bits> nodes;

    friend bool operator==(const container_archive_load& left,
                           const container_archive_load& right)
    {
        return left.nodes == right.nodes;
    }
};

template <class Container>
container_archive_load<Container>
to_load_archive(const container_archive_save<Container>& archive)
{
    return {
        .nodes = to_load_archive(archive.nodes),
    };
}

template <class Archive, class Container>
void save(Archive& ar, const container_archive_save<Container>& value)
{
    save(ar, value.nodes);
}

template <class Archive, class Container>
void load(Archive& ar, container_archive_load<Container>& value)
{
    load(ar, value.nodes);
}

} // namespace champ
} // namespace immer_archive
