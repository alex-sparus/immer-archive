#pragma once

#include <immer/detail/rbts/node.hpp>

namespace immer_archive {

// XXX: Ignoring ref counting completely for now, memory will leak
template <class T,
          typename MemoryPolicy         = immer::default_memory_policy,
          immer::detail::rbts::bits_t B = immer::default_bits>
class loader
{
public:
    static constexpr auto BL = immer::detail::rbts::bits_t{1};
    using rbtree = immer::detail::rbts::rbtree<T, MemoryPolicy, B, BL>;
    using node_t = typename rbtree::node_t;

    explicit loader(archive_load<T> ar)
        : ar_{std::move(ar)}
    {
    }

    std::optional<vector_one<T, MemoryPolicy, B>> load_vector(node_id id)
    {
        auto* info = ar_.vectors.find(id);
        if (!info) {
            return std::nullopt;
        }

        // auto b = builder{info->size, info->shift};
        // b.build();

        auto* root = load_strict(info->rbts.root);
        auto* tail = load_leaf(info->rbts.tail);
        assert(root);
        assert(tail);
        auto impl = immer::detail::rbts::rbtree<T, MemoryPolicy, B, BL>{};
        // XXX add a way to construct it directly, this will lead to memory leak
        impl.size  = info->rbts.size;
        impl.shift = info->rbts.shift;
        impl.root  = root;
        impl.tail  = tail;
        return vector_one<T, MemoryPolicy, B>{std::move(impl)};
    }

    std::optional<flex_vector_one<T, MemoryPolicy, B>>
    load_flex_vector(node_id id)
    {
        auto* info = ar_.flex_vectors.find(id);
        if (!info) {
            return std::nullopt;
        }

        auto* root = load_some_node(info->rbts.root);
        auto* tail = load_leaf(info->rbts.tail);
        assert(root);
        assert(tail);
        auto impl = immer::detail::rbts::rbtree<T, MemoryPolicy, B, BL>{};
        // XXX add a way to construct it directly, this will lead to memory leak
        impl.size  = info->rbts.size;
        impl.shift = info->rbts.shift;
        impl.root  = root;
        impl.tail  = tail;
        return vector_one<T, MemoryPolicy, B>{std::move(impl)};
    }

private:
    node_t* load_leaf(node_id id)
    {
        if (auto* p = leaves_.find(id)) {
            node_t* node = *p;
            return node->inc();
        }

        auto* node_info = ar_.leaves.find(id);
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
        leaves_ = std::move(leaves_).set(id, leaf->inc());
        return leaf;
    }

    node_t* load_strict(node_id id)
    {
        if (auto* p = strict_inners_.find(id)) {
            node_t* node = *p;
            return node->inc();
        }

        auto* node_info = ar_.inners.find(id);
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
        strict_inners_ = std::move(strict_inners_).set(id, inner->inc());

        return inner;
    }

    node_t* load_relaxed(node_id id)
    {
        if (auto* p = relaxed_inners_.find(id)) {
            node_t* node = *p;
            return node->inc();
        }

        auto* node_info = ar_.relaxed_inners.find(id);
        if (!node_info) {
            return nullptr;
        }

        const auto n                = node_info->children.size();
        auto* relaxed               = node_t::make_inner_r_n(n);
        relaxed->relaxed()->d.count = n;
        IMMER_TRY {
            auto index = std::size_t{};
            for (const auto& [child_node_id, child_size] :
                 node_info->children) {
                auto* child = load_some_node(child_node_id);
                if (!child) {
                    throw std::invalid_argument{fmt::format(
                        "Failed to load node ID {}", child_node_id)};
                }
                relaxed->inner()[index]            = child;
                relaxed->relaxed()->d.sizes[index] = child_size;
                ++index;
            }
        }
        IMMER_CATCH (...) {
            node_t::delete_inner_r(relaxed, n);
            IMMER_RETHROW;
        }
        // XXX inc
        relaxed_inners_ = std::move(relaxed_inners_).set(id, relaxed->inc());

        return relaxed;
    }

    node_t* load_some_node(node_id id)
    {
        // Unknown type: leaf, inner or relaxed
        if (ar_.leaves.count(id)) {
            return load_leaf(id);
        }
        if (ar_.inners.count(id)) {
            return load_strict(id);
        }
        if (ar_.relaxed_inners.count(id)) {
            return load_relaxed(id);
        }
        return nullptr;
    }

private:
    const archive_load<T> ar_;
    immer::map<node_id, node_t*> leaves_;
    immer::map<node_id, node_t*> strict_inners_;
    immer::map<node_id, node_t*> relaxed_inners_;
};

} // namespace immer_archive
