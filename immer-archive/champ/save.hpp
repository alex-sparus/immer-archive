#pragma once

#include <immer-archive/champ/archive.hpp>

#include <spdlog/spdlog.h>

namespace immer_archive {
namespace champ {

template <class T, immer::detail::hamts::bits_t B>
struct nodes_archive_builder
{
    nodes_save<T, B> ar;

    void visit_inner(const auto* node, auto depth)
    {
        auto id = get_node_id(node);
        if (ar.inners.count(id)) {
            return;
        }

        auto node_info = inner_node_save<T, B>{
            .nodemap = node->nodemap(),
            .datamap = node->datamap(),
        };

        if (node->datamap()) {
            node_info.values = {node->values(),
                                node->values() + node->data_count()};
        }
        if (node->nodemap()) {
            auto fst = node->children();
            auto lst = fst + node->children_count();
            for (; fst != lst; ++fst) {
                node_info.children =
                    std::move(node_info.children).push_back(get_node_id(*fst));
                visit(*fst, depth + 1);
            }
        }

        ar.inners = std::move(ar.inners).set(id, node_info);
    }

    void visit_collision(const auto* node)
    {
        auto id = get_node_id(node);
        if (ar.inners.count(id)) {
            return;
        }

        ar.inners = std::move(ar.inners).set(
            id,
            inner_node_save<T, B>{
                .values     = {node->collisions(),
                               node->collisions() + node->collision_count()},
                .collisions = true,
            });
    }

    void visit(const auto* node, immer::detail::hamts::count_t depth)
    {
        using immer::detail::hamts::max_depth;

        if (depth < max_depth<B>) {
            visit_inner(node, depth);
        } else {
            visit_collision(node);
        }
    }

    node_id get_node_id(auto* ptr)
    {
        auto [ar2, id] = immer_archive::champ::get_node_id(std::move(ar), ptr);
        ar             = std::move(ar2);
        return id;
    }
};

template <typename T,
          typename Hash,
          typename Equal,
          typename MemoryPolicy,
          immer::detail::hamts::bits_t B,
          class Archive>
auto save_nodes(
    const immer::detail::hamts::champ<T, Hash, Equal, MemoryPolicy, B>& champ,
    Archive ar)
{
    using champ_t = std::decay_t<decltype(champ)>;
    using node_t  = typename champ_t::node_t;

    auto save = nodes_archive_builder<typename node_t::value_t, B>{
        .ar = std::move(ar),
    };
    save.visit(champ.root, 0);

    return std::move(save.ar);
}

} // namespace champ
} // namespace immer_archive
