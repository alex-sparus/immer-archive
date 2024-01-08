#pragma once

#include <immer-archive/rbts/traverse.hpp>

#include <spdlog/spdlog.h>

namespace immer_archive::rbts {

namespace detail {

template <typename T,
          typename MemoryPolicy,
          immer::detail::rbts::bits_t B,
          immer::detail::rbts::bits_t BL>
std::pair<archive_save<T, MemoryPolicy, B, BL>, node_id>
get_node_id(archive_save<T, MemoryPolicy, B, BL> ar,
            const immer::detail::rbts::node<T, MemoryPolicy, B, BL>* ptr)
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

template <typename T,
          typename MemoryPolicy,
          immer::detail::rbts::bits_t B,
          immer::detail::rbts::bits_t BL>
struct archive_builder
{
    archive_save<T, MemoryPolicy, B, BL> ar;

    template <class Pos>
    void operator()(regular_pos_tag, Pos& pos, auto&& visit)
    {
        auto id = get_node_id(pos.node());
        if (ar.inners.count(id)) {
            return;
        }

        auto node_info = inner_node{};
        // Explicit this-> call to workaround an "unused this" warning.
        pos.each(visitor_helper{},
                 [&node_info, &visit, this](
                     auto any_tag, auto& child_pos, auto&&) mutable {
                     node_info.children =
                         std::move(node_info.children)
                             .push_back(this->get_node_id(child_pos.node()));
                     visit(child_pos);
                 });
        ar.inners = std::move(ar.inners).set(id, node_info);
    }

    template <class Pos>
    void operator()(relaxed_pos_tag, Pos& pos, auto&& visit)
    {
        auto id = get_node_id(pos.node());
        if (ar.inners.count(id)) {
            return;
        }

        auto node_info = inner_node{
            .relaxed = true,
        };

        auto* node = pos.node();
        auto* r    = node->relaxed();
        auto index = std::size_t{};
        pos.each(visitor_helper{},
                 [&](auto any_tag, auto& child_pos, auto&&) mutable {
                     node_info.children =
                         std::move(node_info.children)
                             .push_back(get_node_id(child_pos.node()));
                     ++index;

                     visit(child_pos);
                 });

        assert(node_info.children.size() == r->d.count);

        ar.inners = std::move(ar.inners).set(id, node_info);
    }

    template <class Pos>
    void operator()(leaf_pos_tag, Pos& pos, auto&& visit)
    {
        T* first = pos.node()->leaf();
        auto id  = get_node_id(pos.node());
        if (ar.leaves.count(id)) {
            // SPDLOG_DEBUG("already seen leaf node {}", id);
            return;
        }

        // SPDLOG_DEBUG("leaf node {}", id);

        auto info = leaf_node_save<T>{
            .begin = first,
            .end   = first + pos.count(),
        };
        ar.leaves = std::move(ar.leaves).set(id, std::move(info));
    }

    node_id get_node_id(immer::detail::rbts::node<T, MemoryPolicy, B, BL>* ptr)
    {
        auto [ar2, id] =
            immer_archive::rbts::detail::get_node_id(std::move(ar), ptr);
        ar = std::move(ar2);
        return id;
    }
};

template <typename T,
          typename MemoryPolicy,
          immer::detail::rbts::bits_t B,
          immer::detail::rbts::bits_t BL,
          class Archive>
auto save_nodes(const immer::detail::rbts::rbtree<T, MemoryPolicy, B, BL>& tree,
                Archive ar)
{
    auto save = archive_builder<T, MemoryPolicy, B, BL>{
        .ar = std::move(ar),
    };
    tree.traverse(visitor_helper{}, save);
    return std::move(save.ar);
}

template <typename T,
          typename MemoryPolicy,
          immer::detail::rbts::bits_t B,
          immer::detail::rbts::bits_t BL,
          class Archive>
auto save_nodes(
    const immer::detail::rbts::rrbtree<T, MemoryPolicy, B, BL>& tree,
    Archive ar)
{
    auto save = archive_builder<T, MemoryPolicy, B, BL>{
        .ar = std::move(ar),
    };
    tree.traverse(visitor_helper{}, save);
    return std::move(save.ar);
}

} // namespace detail

template <typename T,
          typename MemoryPolicy,
          immer::detail::rbts::bits_t B,
          immer::detail::rbts::bits_t BL>
std::pair<archive_save<T, MemoryPolicy, B, BL>, node_id>
save_to_archive(immer::vector<T, MemoryPolicy, B, BL> vec,
                archive_save<T, MemoryPolicy, B, BL> archive)
{
    const auto& impl = vec.impl();
    auto root_id     = node_id{};
    auto tail_id     = node_id{};
    std::tie(archive, root_id) =
        detail::get_node_id(std::move(archive), impl.root);
    std::tie(archive, tail_id) =
        detail::get_node_id(std::move(archive), impl.tail);
    const auto tree_id = rbts_id{
        .root = root_id,
        .tail = tail_id,
    };

    if (auto* p = archive.rbts_to_id.find(tree_id)) {
        // Already been saved
        auto vector_id = *p;
        return {std::move(archive), vector_id};
    }

    archive = detail::save_nodes(impl, std::move(archive));

    assert(archive.inners.count(root_id));
    assert(archive.leaves.count(tail_id));

    const auto vector_id = archive.rbts_to_id.size();
    assert(archive.vectors.count(vector_id) == 0);

    archive.rbts_to_id = std::move(archive.rbts_to_id).set(tree_id, vector_id);
    archive.vectors    = std::move(archive.vectors)
                          .set(vector_id,
                               vector_save<T, MemoryPolicy, B, BL>{
                                   .rbts =
                                       rbts_info{
                                           .root = root_id,
                                           .tail = tail_id,
                                       },
                                   .vector = std::move(vec),
                               });

    return {std::move(archive), vector_id};
}

template <typename T,
          typename MemoryPolicy,
          immer::detail::rbts::bits_t B,
          immer::detail::rbts::bits_t BL>
std::pair<archive_save<T, MemoryPolicy, B, BL>, node_id>
save_to_archive(immer::flex_vector<T, MemoryPolicy, B, BL> vec,
                archive_save<T, MemoryPolicy, B, BL> archive)
{
    const auto& impl = vec.impl();
    auto root_id     = node_id{};
    auto tail_id     = node_id{};
    std::tie(archive, root_id) =
        detail::get_node_id(std::move(archive), impl.root);
    std::tie(archive, tail_id) =
        detail::get_node_id(std::move(archive), impl.tail);
    const auto tree_id = rbts_id{
        .root = root_id,
        .tail = tail_id,
    };

    if (auto* p = archive.rbts_to_id.find(tree_id)) {
        // Already been saved
        auto vector_id = *p;
        return {std::move(archive), vector_id};
    }

    archive = detail::save_nodes(impl, std::move(archive));

    assert(archive.inners.count(root_id));
    assert(archive.leaves.count(tail_id));

    const auto vector_id = archive.rbts_to_id.size();
    assert(archive.flex_vectors.count(vector_id) == 0);

    archive.rbts_to_id = std::move(archive.rbts_to_id).set(tree_id, vector_id);
    archive.flex_vectors = std::move(archive.flex_vectors)
                               .set(vector_id,
                                    flex_vector_save<T, MemoryPolicy, B, BL>{
                                        .rbts =
                                            rbts_info{
                                                .root = root_id,
                                                .tail = tail_id,
                                            },
                                        .vector = std::move(vec),
                                    });

    return {std::move(archive), vector_id};
}

} // namespace immer_archive::rbts
