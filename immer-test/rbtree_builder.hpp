#pragma once

#include <immer/detail/rbts/node.hpp>

namespace immer_archive {

/**
 * B must be fixed and the same as during serialization, otherwise nothing would
 * make sense.
 */
template <typename T,
          typename MemoryPolicy         = immer::default_memory_policy,
          immer::detail::rbts::bits_t B = immer::default_bits>
struct rbtree_builder
{
    static constexpr auto BL = immer::detail::rbts::bits_t{1};
    using rbtree = immer::detail::rbts::rbtree<T, MemoryPolicy, B, BL>;
    using node_t = typename rbtree::node_t;

    size_t size;
    immer::detail::rbts::shift_t shift;
    node_t* root;
    node_t* tail;

    rbtree_builder(size_t size_, immer::detail::rbts::shift_t shift_)
        : size{size_}
        , shift{shift_}
        , root{rbtree::empty_root()}
        , tail{rbtree::empty_tail()}
    {
    }

    auto tail_offset() const
    {
        using immer::detail::rbts::mask;
        return size ? (size - 1) & ~mask<BL> : 0;
    }

    template <typename Visitor, typename... Args>
    void traverse(Visitor v, Args&&... args) const
    {
        auto tail_off  = tail_offset();
        auto tail_size = size - tail_off;

        if (tail_off)
            make_regular_sub_pos(root, shift, tail_off).visit(v, args...);
        else
            make_empty_regular_pos(root).visit(v, args...);

        make_leaf_sub_pos(tail, tail_size).visit(v, args...);
    }

    void build()
    {
        traverse(visitor_helper{}, [](auto& pos) {
            using Pos = decltype(pos);
            if constexpr (is_regular_pos<Pos>) {
                // save.regular(pos);
            } else if constexpr (is_leaf_pos<Pos>) {
                // save.leaf(pos);
            }
            static_assert(is_regular_pos<Pos> || is_leaf_pos<Pos>);
        });
    }
};

// XXX: Ignoring ref counting completely for now, memory will leak
template <class T,
          typename MemoryPolicy         = immer::default_memory_policy,
          immer::detail::rbts::bits_t B = immer::default_bits>
struct loader
{
    static constexpr auto BL = immer::detail::rbts::bits_t{1};
    using builder            = rbtree_builder<T, MemoryPolicy, B>;
    using node_t             = typename builder::node_t;

    const archive_load<T> ar;
    immer::map<node_id, node_t*> leaves;
    immer::map<node_id, node_t*> strict_inners;
    immer::map<node_id, node_t*> relaxed_inners;

    node_t* load_leaf(node_id id)
    {
        if (auto* p = leaves.find(id)) {
            node_t* node = *p;
            return node->inc();
        }

        auto* node_info = ar.leaves.find(id);
        if (!node_info) {
            return nullptr;
        }

        const auto n = node_info->data.size();
        auto* leaf   = node_t::make_leaf_n(n);
        IMMER_TRY {
            immer::detail::uninitialized_copy(
                node_info->data.begin(), node_info->data.end(), leaf->leaf());
        }
        IMMER_CATCH (...) {
            node_t::heap::deallocate(node_t::sizeof_leaf_n(n), leaf);
            IMMER_RETHROW;
        }
        // XXX inc here
        leaves = std::move(leaves).set(id, leaf->inc());
        return leaf;
    }

    node_t* load_strict(node_id id)
    {
        if (auto* p = strict_inners.find(id)) {
            node_t* node = *p;
            return node->inc();
        }

        auto* node_info = ar.inners.find(id);
        if (!node_info) {
            return nullptr;
        }

        const auto n = node_info->children.size();
        auto* inner  = node_t::make_inner_n(n);
        IMMER_TRY {
            auto index = std::size_t{};
            for (const auto& child_node_id : node_info->children) {
                auto* child = load_some_node(child_node_id);
                if (!child) {
                    throw std::invalid_argument{fmt::format(
                        "Failed to load node ID {}", child_node_id)};
                }
                inner->inner()[index] = child;
                ++index;
            }
        }
        IMMER_CATCH (...) {
            node_t::delete_inner(inner, n);
            IMMER_RETHROW;
        }
        // XXX inc
        strict_inners = std::move(strict_inners).set(id, inner->inc());

        return inner;
    }

    node_t* load_relaxed(node_id id)
    {
        if (auto* p = relaxed_inners.find(id)) {
            node_t* node = *p;
            return node->inc();
        }

        auto* node_info = ar.relaxed_inners.find(id);
        if (!node_info) {
            return nullptr;
        }

        const auto n                = node_info->children.size();
        auto* relaxed               = node_t::make_inner_r_n(n);
        relaxed->relaxed()->d.count = n;
        IMMER_TRY {
            auto index = std::size_t{};
            auto sizes = std::size_t{};
            for (const auto& child_node_id : node_info->children) {
                auto* child = load_some_node(child_node_id);
                if (!child) {
                    throw std::invalid_argument{fmt::format(
                        "Failed to load node ID {}", child_node_id)};
                }
                relaxed->inner()[index] = child;
                sizes += get_count(child_node_id);
                relaxed->relaxed()->d.sizes[index] = sizes;
                ++index;
            }
        }
        IMMER_CATCH (...) {
            node_t::delete_inner_r(relaxed, n);
            IMMER_RETHROW;
        }
        // XXX inc
        strict_inners = std::move(strict_inners).set(id, relaxed->inc());

        return relaxed;
    }

    node_t* load_some_node(node_id id)
    {
        // Unknown type: leaf, inner or relaxed
        if (ar.leaves.count(id)) {
            return load_leaf(id);
        }
        if (ar.inners.count(id)) {
            return load_strict(id);
        }
        if (ar.relaxed_inners.count(id)) {
            return load_relaxed(id);
        }
        return nullptr;
    }

    std::optional<vector_one<T, MemoryPolicy, B>> load_vector(node_id id)
    {
        auto* info = ar.vectors.find(id);
        if (!info) {
            return std::nullopt;
        }

        // auto b = builder{info->size, info->shift};
        // b.build();

        auto* root = load_strict(info->root);
        auto* tail = load_leaf(info->tail);
        assert(root);
        assert(tail);
        auto impl = immer::detail::rbts::rbtree<T, MemoryPolicy, B, BL>{};
        // XXX add a way to construct it directly, this will lead to memory leak
        impl.size  = info->size;
        impl.shift = info->shift;
        impl.root  = root;
        impl.tail  = tail;
        return vector_one<T, MemoryPolicy, B>{std::move(impl)};
    }

    std::optional<flex_vector_one<T, MemoryPolicy, B>>
    load_flex_vector(node_id id)
    {
        auto* info = ar.flex_vectors.find(id);
        if (!info) {
            return std::nullopt;
        }

        auto* root = load_some_node(info->root);
        auto* tail = load_leaf(info->tail);
        assert(root);
        assert(tail);
        auto impl = immer::detail::rbts::rbtree<T, MemoryPolicy, B, BL>{};
        // XXX add a way to construct it directly, this will lead to memory leak
        impl.size  = info->size;
        impl.shift = info->shift;
        impl.root  = root;
        impl.tail  = tail;
        return vector_one<T, MemoryPolicy, B>{std::move(impl)};
    }

    std::size_t get_count(node_id id)
    {
        if (auto* p = ar.leaves.find(id)) {
            return p->data.size();
        }
        if (auto* p = ar.inners.find(id)) {
            return p->children.size();
        }
        if (auto* p = ar.relaxed_inners.find(id)) {
            return p->children.size();
        }
        return 0;
    }
};

} // namespace immer_archive
