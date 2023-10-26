#pragma once

#include "immer_champ_archive.hpp"

#include <spdlog/spdlog.h>

namespace immer_archive {
namespace champ {

template <typename T,
          typename Hash,
          typename Equal,
          typename MemoryPolicy,
          immer::detail::hamts::bits_t B>
std::pair<archive_save<T, Hash>, node_id> get_node_id(
    archive_save<T, Hash> ar,
    const immer::detail::hamts::node<T, Hash, Equal, MemoryPolicy, B>* ptr)
{
    auto* ptr_void = static_cast<const void*>(ptr);
    if (auto* maybe_id = ar.node_ptr_to_id.find(ptr_void)) {
        auto id = *maybe_id;
        return {std::move(ar), id};
    }

    const auto id     = ar.node_ptr_to_id.size();
    ar.node_ptr_to_id = std::move(ar.node_ptr_to_id).set(ptr_void, id);
    return {std::move(ar), id};
}

template <class T, class Hash, immer::detail::hamts::bits_t B>
struct archive_builder
{
    archive_save<T, Hash> ar;

    void visit_inner(const auto* node, auto depth)
    {
        auto id = get_node_id(node);
        if (ar.inners.count(id)) {
            return;
        }

        auto node_info = inner_node_save<T>{};

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
        if (ar.collisions.count(id)) {
            return;
        }

        auto info = values_save<T>{
            .begin = node->collisions(),
            .end   = node->collisions() + node->collision_count(),
        };
        ar.collisions = std::move(ar.collisions).set(id, std::move(info));
    }

    void visit(const auto* node, auto depth)
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

    auto save = archive_builder<typename node_t::value_t, Hash, B>{
        .ar = std::move(ar),
    };
    save.visit(champ.root, 0);

    return std::move(save.ar);
}

template <class T, class Hash>
std::pair<archive_save<T, Hash>, node_id>
save_set(immer::set<T, Hash> set, archive_save<T, Hash> archive)
{
    const auto& impl           = set.impl();
    auto root_id               = node_id{};
    std::tie(archive, root_id) = get_node_id(std::move(archive), impl.root);

    if (auto* p = archive.sets.find(root_id)) {
        // Already been saved
        return {std::move(archive), root_id};
    }

    archive = save_nodes(impl, std::move(archive));
    assert(archive.inners.count(root_id));

    archive.sets = std::move(archive.sets)
                       .set(root_id,
                            set_save<T, Hash>{
                                .champ =
                                    champ_info{
                                        .root = root_id,
                                        .size = impl.size,
                                    },
                                .set = std::move(set),
                            });

    return {std::move(archive), root_id};
}

} // namespace champ
} // namespace immer_archive
