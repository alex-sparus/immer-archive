#pragma once

#include "immer_archive.hpp"

#include <immer/detail/rbts/rrbtree.hpp>

#include <spdlog/spdlog.h>

namespace immer_archive {

struct is_regular_pos_func
{
    template <class... Rest>
    constexpr bool
    operator()(const immer::detail::rbts::regular_sub_pos<Rest...>&)
    {
        return true;
    }

    template <class... Rest>
    constexpr bool operator()(immer::detail::rbts::full_pos<Rest...>&)
    {
        return true;
    }

    template <class... Rest>
    constexpr bool operator()(immer::detail::rbts::regular_pos<Rest...>&)
    {
        return true;
    }

    template <class... Rest>
    constexpr bool operator()(immer::detail::rbts::empty_regular_pos<Rest...>&)
    {
        return true;
    }
};

struct is_leaf_pos_func
{
    template <class... Rest>
    constexpr bool operator()(immer::detail::rbts::leaf_sub_pos<Rest...>&)
    {
        return true;
    }

    template <class... Rest>
    constexpr bool operator()(immer::detail::rbts::full_leaf_pos<Rest...>&)
    {
        return true;
    }

    template <class... Rest>
    constexpr bool operator()(immer::detail::rbts::leaf_pos<Rest...>&)
    {
        return true;
    }

    template <class... Rest>
    constexpr bool operator()(immer::detail::rbts::empty_leaf_pos<Rest...>&)
    {
        return true;
    }
};

struct is_relaxed_pos_func
{
    template <class... Rest>
    constexpr bool operator()(immer::detail::rbts::relaxed_pos<Rest...>&)
    {
        return true;
    }
};

template <class T>
constexpr bool is_regular_pos = std::is_invocable_v<is_regular_pos_func, T>;

template <class T>
constexpr bool is_leaf_pos = std::is_invocable_v<is_leaf_pos_func, T>;

template <class T>
constexpr bool is_relaxed_pos = std::is_invocable_v<is_relaxed_pos_func, T>;

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

template <typename T,
          typename MemoryPolicy,
          immer::detail::rbts::bits_t B,
          immer::detail::rbts::bits_t BL>
node_id get_leaf_id(immer::detail::rbts::node<T, MemoryPolicy, B, BL>& node)
{
    T* first = node.leaf();
    return reinterpret_cast<node_id>(static_cast<void*>(first));
}

template <typename T,
          typename MemoryPolicy,
          immer::detail::rbts::bits_t B,
          immer::detail::rbts::bits_t BL>
node_id get_inner_id(immer::detail::rbts::node<T, MemoryPolicy, B, BL>& node)
{
    using node_t  = immer::detail::rbts::node<T, MemoryPolicy, B, BL>;
    node_t* inner = *node.inner();
    return reinterpret_cast<node_id>(static_cast<void*>(inner));
}

template <typename T,
          typename MemoryPolicy,
          immer::detail::rbts::bits_t B,
          immer::detail::rbts::bits_t BL>
node_id get_relaxed_id(immer::detail::rbts::node<T, MemoryPolicy, B, BL>& node)
{
    using node_t    = immer::detail::rbts::node<T, MemoryPolicy, B, BL>;
    using relaxed_t = typename node_t::relaxed_t;
    relaxed_t* ptr  = node.relaxed();
    return reinterpret_cast<node_id>(static_cast<void*>(ptr));
}

template <class T>
struct archive_builder
{
    archive<T> ar;

    template <class Pos>
    void regular(Pos& pos)
    {
        auto id = get_inner_id(*pos.node());
        if (ar.inners.count(id)) {
            return;
        }

        auto node_info = inner_node{};
        pos.each(visitor_helper{}, [&node_info, this](auto& child_pos) mutable {
            using ChildPos = decltype(child_pos);
            if constexpr (is_regular_pos<ChildPos>) {
                node_info.children =
                    std::move(node_info.children)
                        .push_back(get_inner_id(*child_pos.node()));
                regular(child_pos);
            } else if constexpr (is_leaf_pos<ChildPos>) {
                node_info.children =
                    std::move(node_info.children)
                        .push_back(get_leaf_id(*child_pos.node()));
                leaf(child_pos);
            }

            static_assert(is_regular_pos<ChildPos> || is_leaf_pos<ChildPos>);
        });
        ar.inners = std::move(ar.inners).set(id, node_info);
    }

    template <class Pos>
    void relaxed(Pos& pos)
    {
        auto id = get_relaxed_id(*pos.node());
        if (ar.inners.count(id)) {
            return;
        }

        auto node_info = inner_node{};
        pos.each(visitor_helper{}, [&node_info, this](auto& child_pos) mutable {
            using ChildPos = decltype(child_pos);
            if constexpr (is_regular_pos<ChildPos>) {
                node_info.children =
                    std::move(node_info.children)
                        .push_back(get_inner_id(*child_pos.node()));
                regular(child_pos);
            } else if constexpr (is_relaxed_pos<ChildPos>) {
                node_info.children =
                    std::move(node_info.children)
                        .push_back(get_relaxed_id(*child_pos.node()));
                relaxed(child_pos);
            } else if constexpr (is_leaf_pos<ChildPos>) {
                node_info.children =
                    std::move(node_info.children)
                        .push_back(get_leaf_id(*child_pos.node()));
                leaf(child_pos);
            }

            static_assert(is_regular_pos<ChildPos> ||
                          is_relaxed_pos<ChildPos> || is_leaf_pos<ChildPos>);
        });
        ar.inners = std::move(ar.inners).set(id, node_info);
    }

    template <class Pos>
    void leaf(Pos& pos)
    {
        T* first = pos.node()->leaf();
        auto id  = get_leaf_id(*pos.node());
        if (ar.leaves.count(id)) {
            // SPDLOG_DEBUG("already seen leaf node {}", id);
            return;
        }

        // SPDLOG_DEBUG("leaf node {}", id);

        auto info = leaf_node<T>{
            .begin = first,
            .end   = first + pos.count(),
        };
        ar.leaves = std::move(ar.leaves).set(id, std::move(info));
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

    tree.traverse(visitor_helper{}, [&save](auto& pos) {
        using Pos = decltype(pos);
        if constexpr (is_regular_pos<Pos>) {
            save.regular(pos);
        } else if constexpr (is_leaf_pos<Pos>) {
            save.leaf(pos);
        }
        static_assert(is_regular_pos<Pos> || is_leaf_pos<Pos>);
    });

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

    tree.traverse(visitor_helper{}, [&save](auto& pos) {
        using Pos = decltype(pos);
        if constexpr (is_regular_pos<Pos>) {
            save.regular(pos);
        } else if constexpr (is_relaxed_pos<Pos>) {
            save.relaxed(pos);
        } else if constexpr (is_leaf_pos<Pos>) {
            save.leaf(pos);
        }
        static_assert(is_regular_pos<Pos> || is_relaxed_pos<Pos> ||
                      is_leaf_pos<Pos>);
    });

    auto result = std::move(save.ar);
    return result;
}

template <class T>
archive<T> save_vector(const vector_one<T>& vec, archive<T> archive)
{
    const auto& impl = vec.impl();
    archive          = save_nodes(impl, std::move(archive));

    const auto root_id = get_inner_id(*impl.root);
    assert(archive.inners.count(root_id));

    const auto tail_id = get_leaf_id(*impl.tail);
    assert(archive.leaves.count(tail_id));

    const auto vector_id =
        reinterpret_cast<node_id>(static_cast<const void*>(&impl));
    archive.vectors = std::move(archive.vectors)
                          .set(vector_id,
                               vector{
                                   .root = root_id,
                                   .tail = tail_id,
                                   .size = impl.size,
                               });

    return archive;
}

template <class T>
archive<T> save_vector(const flex_vector_one<T>& vec, archive<T> archive)
{
    const auto& impl = vec.impl();
    archive          = save_nodes(impl, std::move(archive));

    const auto root_id = [&] {
        if (impl.root->relaxed()) {
            return get_relaxed_id(*impl.root);
        } else {
            return get_inner_id(*impl.root);
        }
    }();
    assert(archive.inners.count(root_id));

    const auto tail_id = get_leaf_id(*impl.tail);
    assert(archive.leaves.count(tail_id));

    const auto vector_id =
        reinterpret_cast<node_id>(static_cast<const void*>(&impl));
    archive.vectors = std::move(archive.vectors)
                          .set(vector_id,
                               vector{
                                   .root = root_id,
                                   .tail = tail_id,
                                   .size = impl.size,
                               });

    return archive;
}

} // namespace immer_archive
