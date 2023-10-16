#pragma once

#include "immer_archive.hpp"

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
};

template <class T>
constexpr bool is_regular_pos = std::is_invocable_v<is_regular_pos_func, T>;

template <class T>
constexpr bool is_leaf_pos = std::is_invocable_v<is_leaf_pos_func, T>;

struct visitor_helper
{
    template <class F, class... T>
    static void visit_regular(immer::detail::rbts::regular_sub_pos<T...>& pos,
                              F&& fn)
    {
        fn(pos);
    }

    template <class F, class... T>
    static void visit_regular(immer::detail::rbts::full_pos<T...>& pos, F&& fn)
    {
        fn(pos);
    }

    template <class F, class... T>
    static void visit_regular(immer::detail::rbts::regular_pos<T...>& pos,
                              F&& fn)
    {
        fn(pos);
    }

    template <class F, class... T>
    static void visit_leaf(immer::detail::rbts::leaf_sub_pos<T...>& pos, F&& fn)
    {
        fn(pos);
    }

    template <class F, class... T>
    static void visit_leaf(immer::detail::rbts::full_leaf_pos<T...>& pos,
                           F&& fn)
    {
        fn(pos);
    }

    template <class F, class... T>
    static void visit_leaf(immer::detail::rbts::leaf_pos<T...>& pos, F&& fn)
    {
        fn(pos);
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
    immer::detail::rbts::node<T, MemoryPolicy, B, BL>* inner = *node.inner();
    return reinterpret_cast<node_id>(static_cast<void*>(inner));
}

template <class T>
struct archive_builder
{
    archive<T> ar;

    template <class Pos>
    bool regular(Pos& pos)
    {
        using node_t = typename std::decay_t<decltype(pos)>::node_t;
        auto id      = get_inner_id(*pos.node());
        if (ar.inners.count(id)) {
            const bool have_seen = true;
            return have_seen;
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

        const bool have_seen = false;
        return have_seen;
    }

    template <class Pos>
    bool leaf(Pos& pos)
    {
        using node_t = typename std::decay_t<decltype(pos)>::node_t;
        T* first     = pos.node()->leaf();
        auto id      = get_leaf_id(*pos.node());
        if (ar.leaves.count(id)) {
            // SPDLOG_DEBUG("already seen leaf node {}", id);
            const bool have_seen = true;
            return have_seen;
        }

        // SPDLOG_DEBUG("leaf node {}", id);

        ar.leaves            = std::move(ar.leaves).set(id,
                                             leaf_node<T>{
                                                            .begin = first,
                                                            .end   = first + pos.count(),
                                             });
        const bool have_seen = false;
        return have_seen;
    }
};

template <class Tree, class Archive>
auto save_nodes(const Tree& rbtree, Archive ar)
{
    using immer::detail::rbts::branches;

    using node_t      = typename Tree::node_t;
    constexpr auto BL = node_t::bits_leaf;

    auto save = archive_builder<typename node_t::value_t>{
        .ar = std::move(ar),
    };
    auto fn = [&save](auto& pos) {
        using Pos = decltype(pos);
        if constexpr (is_regular_pos<Pos>) {
            save.regular(pos);
        } else if constexpr (is_leaf_pos<Pos>) {
            save.leaf(pos);
        }
        static_assert(is_regular_pos<Pos> || is_leaf_pos<Pos>);
    };

    if (rbtree.size > branches<BL>) {
        make_regular_sub_pos(rbtree.root, rbtree.shift, rbtree.tail_offset())
            .visit(visitor_helper{}, fn);
    }

    make_leaf_sub_pos(rbtree.tail, rbtree.tail_size())
        .visit(visitor_helper{}, fn);

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

} // namespace immer_archive
