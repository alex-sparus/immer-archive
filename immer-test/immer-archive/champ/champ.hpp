#pragma once

#include "archive_champ.hpp"
#include "load.hpp"
#include "save.hpp"

namespace immer_archive {
namespace champ {

template <class Node>
struct node_traits
{
    template <typename T>
    struct impl;

    template <typename T,
              typename Hash,
              typename Equal,
              typename MemoryPolicy,
              immer::detail::hamts::bits_t B>
    struct impl<immer::detail::hamts::node<T, Hash, Equal, MemoryPolicy, B>>
    {
        using equal_t              = Equal;
        using hash_t               = Hash;
        using memory_t             = MemoryPolicy;
        static constexpr auto bits = B;
    };

    using Hash                 = typename impl<Node>::hash_t;
    using Equal                = typename impl<Node>::equal_t;
    using MemoryPolicy         = typename impl<Node>::memory_t;
    static constexpr auto bits = impl<Node>::bits;
};

template <class Container>
class container_loader
{
    using champ_t = std::decay_t<decltype(std::declval<Container>().impl())>;
    using node_t  = typename champ_t::node_t;
    using traits  = node_traits<node_t>;

public:
    explicit container_loader(container_archive_load<Container> archive)
        : archive_{std::move(archive)}
        , nodes_{archive_.nodes}
    {
    }

    std::optional<Container> load(node_id id)
    {
        auto* info = archive_.containers.find(id);
        if (!info) {
            return std::nullopt;
        }

        auto root = nodes_.load_inner(info->root);
        if (!root) {
            return std::nullopt;
        }

        auto impl = champ_t{root.release(), info->size};

        // XXX This ctor is not public in immer.
        auto container = Container{std::move(impl)};
        return container;
    }

private:
    const container_archive_load<Container> archive_;
    nodes_loader<typename node_t::value_t,
                 typename traits::Hash,
                 typename traits::Equal,
                 typename traits::MemoryPolicy,
                 traits::bits>
        nodes_;
};

template <class Container>
std::pair<container_archive_save<Container>, node_id>
save_container(Container container, container_archive_save<Container> archive)
{
    const auto& impl = container.impl();
    auto root_id     = node_id{};
    std::tie(archive.nodes, root_id) =
        get_node_id(std::move(archive.nodes), impl.root);

    if (auto* p = archive.containers.find(root_id)) {
        // Already been saved
        return {std::move(archive), root_id};
    }

    archive.nodes = save_nodes(impl, std::move(archive.nodes));
    assert(archive.nodes.inners.count(root_id));

    archive.containers = std::move(archive.containers)
                             .set(root_id,
                                  container_save<Container>{
                                      .champ =
                                          champ_info{
                                              .root = root_id,
                                              .size = impl.size,
                                          },
                                      .container = std::move(container),
                                  });

    return {std::move(archive), root_id};
}

} // namespace champ
} // namespace immer_archive
