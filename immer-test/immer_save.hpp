#pragma once

#include "immer_archive.hpp"

#include <immer/detail/rbts/rrbtree.hpp>

#include <spdlog/spdlog.h>

namespace immer_archive {

struct regular_pos_tag
{};
struct leaf_pos_tag
{};
struct relaxed_pos_tag
{};

template <class T>
struct position_tag : std::false_type
{};

template <class... Rest>
struct position_tag<immer::detail::rbts::regular_sub_pos<Rest...>>
{
    using type = regular_pos_tag;
};

template <class... Rest>
struct position_tag<immer::detail::rbts::full_pos<Rest...>>
{
    using type = regular_pos_tag;
};
template <class... Rest>
struct position_tag<immer::detail::rbts::regular_pos<Rest...>>
{
    using type = regular_pos_tag;
};
template <class... Rest>
struct position_tag<immer::detail::rbts::empty_regular_pos<Rest...>>
{
    using type = regular_pos_tag;
};

template <class... Rest>
struct position_tag<immer::detail::rbts::leaf_sub_pos<Rest...>>
{
    using type = leaf_pos_tag;
};
template <class... Rest>
struct position_tag<immer::detail::rbts::full_leaf_pos<Rest...>>
{
    using type = leaf_pos_tag;
};
template <class... Rest>
struct position_tag<immer::detail::rbts::leaf_pos<Rest...>>
{
    using type = leaf_pos_tag;
};
template <class... Rest>
struct position_tag<immer::detail::rbts::empty_leaf_pos<Rest...>>
{
    using type = leaf_pos_tag;
};

template <class... Rest>
struct position_tag<immer::detail::rbts::relaxed_pos<Rest...>>
{
    using type = relaxed_pos_tag;
};

struct visitor_helper
{
    template <class T, class F>
    static void visit_regular(T&& pos, F&& fn)
    {
        fn(std::forward<T>(pos));
    }

    template <class T, class F>
    static void visit_relaxed(T&& pos, F&& fn)
    {
        fn(std::forward<T>(pos));
    }

    template <class T, class F>
    static void visit_leaf(T&& pos, F&& fn)
    {
        fn(std::forward<T>(pos));
    }
};

template <typename T, typename MemoryPolicy, immer::detail::rbts::bits_t B>
std::pair<archive_save<T>, node_id>
get_node_id(archive_save<T> ar,
            const immer::detail::rbts::node<T, MemoryPolicy, B, 1>* ptr)
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

template <class T>
struct archive_builder
{
    archive_save<T> ar;

    template <class Pos>
    void operator()(Pos& pos)
    {
        visit(pos);
    }

    template <class Pos>
    void visit(Pos& pos)
    {
        using Tag = typename position_tag<std::decay_t<Pos>>::type;
        visit(Tag{}, pos);
    }

    template <class Pos>
    void visit(regular_pos_tag, Pos& pos)
    {
        auto id = get_node_id(pos.node());
        if (ar.inners.count(id)) {
            return;
        }

        auto node_info = inner_node{};
        pos.each(visitor_helper{}, [&node_info, this](auto& child_pos) mutable {
            node_info.children = std::move(node_info.children)
                                     .push_back(get_node_id(child_pos.node()));
            visit(child_pos);
        });
        ar.inners = std::move(ar.inners).set(id, node_info);
    }

    template <class Pos>
    void visit(relaxed_pos_tag, Pos& pos)
    {
        auto id = get_node_id(pos.node());
        if (ar.relaxed_inners.count(id)) {
            return;
        }

        auto node_info = relaxed_inner_node{};

        auto* node = pos.node();
        auto* r    = node->relaxed();
        auto index = std::size_t{};
        pos.each(visitor_helper{}, [&](auto& child_pos) mutable {
            using ChildPos     = decltype(child_pos);
            node_info.children = std::move(node_info.children)
                                     .push_back(relaxed_child{
                                         .node = get_node_id(child_pos.node()),
                                         .size = r->d.sizes[index],
                                     });
            ++index;

            visit(child_pos);
        });

        assert(node_info.children.size() == r->d.count);

        ar.relaxed_inners = std::move(ar.relaxed_inners).set(id, node_info);
    }

    template <class Pos>
    void visit(leaf_pos_tag, Pos& pos)
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

    template <typename MemoryPolicy, immer::detail::rbts::bits_t B>
    node_id get_node_id(immer::detail::rbts::node<T, MemoryPolicy, B, 1>* ptr)
    {
        auto [ar2, id] = immer_archive::get_node_id(std::move(ar), ptr);
        ar             = std::move(ar2);
        return id;
    }
};

template <typename T,
          typename MemoryPolicy,
          immer::detail::rbts::bits_t B,
          class Archive>
auto save_nodes(const immer::detail::rbts::rbtree<T, MemoryPolicy, B, 1>& tree,
                Archive ar)
{
    using tree_t = std::decay_t<decltype(tree)>;
    using node_t = typename tree_t::node_t;

    auto save = archive_builder<typename node_t::value_t>{
        .ar = std::move(ar),
    };

    tree.traverse(visitor_helper{}, save);

    return std::move(save.ar);
}

template <typename T,
          typename MemoryPolicy,
          immer::detail::rbts::bits_t B,
          class Archive>
auto save_nodes(const immer::detail::rbts::rrbtree<T, MemoryPolicy, B, 1>& tree,
                Archive ar)
{
    using tree_t = std::decay_t<decltype(tree)>;
    using node_t = typename tree_t::node_t;

    auto save = archive_builder<typename node_t::value_t>{
        .ar = std::move(ar),
    };

    tree.traverse(visitor_helper{}, save);

    auto result = std::move(save.ar);
    return result;
}

template <class T>
std::pair<archive_save<T>, node_id> save_vector(vector_one<T> vec,
                                                archive_save<T> archive)
{
    const auto& impl           = vec.impl();
    auto root_id               = node_id{};
    auto tail_id               = node_id{};
    std::tie(archive, root_id) = get_node_id(std::move(archive), impl.root);
    std::tie(archive, tail_id) = get_node_id(std::move(archive), impl.tail);
    const auto tree_id         = rbts_id{
                .root = root_id,
                .tail = tail_id,
    };

    if (auto* p = archive.rbts_to_id.find(tree_id)) {
        // Already been saved
        auto vector_id = *p;
        return {std::move(archive), vector_id};
    }

    archive = save_nodes(impl, std::move(archive));

    assert(archive.inners.count(root_id));
    assert(archive.leaves.count(tail_id));

    const auto vector_id = archive.rbts_to_id.size();
    assert(archive.vectors.count(vector_id) == 0);

    archive.rbts_to_id = std::move(archive.rbts_to_id).set(tree_id, vector_id);
    archive.vectors    = std::move(archive.vectors)
                          .set(vector_id,
                               vector_save<T>{
                                   .rbts =
                                       rbts_info{
                                           .root  = root_id,
                                           .tail  = tail_id,
                                           .size  = impl.size,
                                           .shift = impl.shift,
                                       },
                                   .vector = std::move(vec),
                               });

    return {std::move(archive), vector_id};
}

template <class T>
std::pair<archive_save<T>, node_id> save_vector(flex_vector_one<T> vec,
                                                archive_save<T> archive)
{
    const auto& impl           = vec.impl();
    auto root_id               = node_id{};
    auto tail_id               = node_id{};
    std::tie(archive, root_id) = get_node_id(std::move(archive), impl.root);
    std::tie(archive, tail_id) = get_node_id(std::move(archive), impl.tail);
    const auto tree_id         = rbts_id{
                .root = root_id,
                .tail = tail_id,
    };

    if (auto* p = archive.rbts_to_id.find(tree_id)) {
        // Already been saved
        auto vector_id = *p;
        return {std::move(archive), vector_id};
    }

    archive = save_nodes(impl, std::move(archive));

    assert(archive.inners.count(root_id) ||
           archive.relaxed_inners.count(root_id));
    assert(archive.leaves.count(tail_id));

    const auto vector_id = archive.rbts_to_id.size();
    assert(archive.flex_vectors.count(vector_id) == 0);

    archive.rbts_to_id = std::move(archive.rbts_to_id).set(tree_id, vector_id);
    archive.flex_vectors = std::move(archive.flex_vectors)
                               .set(vector_id,
                                    flex_vector_save<T>{
                                        .rbts =
                                            rbts_info{
                                                .root  = root_id,
                                                .tail  = tail_id,
                                                .size  = impl.size,
                                                .shift = impl.shift,
                                            },
                                        .vector = std::move(vec),
                                    });

    return {std::move(archive), vector_id};
}

} // namespace immer_archive
