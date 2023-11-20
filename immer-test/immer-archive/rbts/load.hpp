#pragma once

#include <immer-archive/node_ptr.hpp>
#include <immer-archive/rbts/archive.hpp>

#include <immer/vector.hpp>
#include <optional>

#include <spdlog/spdlog.h>

namespace immer_archive {

template <class T,
          typename MemoryPolicy         = immer::default_memory_policy,
          immer::detail::rbts::bits_t B = immer::default_bits>
class loader
{
public:
    static constexpr auto BL = immer::detail::rbts::bits_t{1};
    using rbtree         = immer::detail::rbts::rbtree<T, MemoryPolicy, B, BL>;
    using node_t         = typename rbtree::node_t;
    using node_ptr       = node_ptr<node_t>;
    using inner_node_ptr = inner_node_ptr<node_t>;

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

        auto root = load_strict(info->rbts.root);
        auto tail = load_leaf(info->rbts.tail);
        assert(root);
        assert(tail);
        auto impl = immer::detail::rbts::rbtree<T, MemoryPolicy, B, BL>{};
        // XXX Hack: destroy the empty tree to later replace all the members.
        // XXX add a way to construct it directly.
        impl.dec();
        impl.size  = info->rbts.size;
        impl.shift = info->rbts.shift;
        impl.root  = root.release();
        impl.tail  = tail.release();
        return vector_one<T, MemoryPolicy, B>{std::move(impl)};
    }

    std::optional<flex_vector_one<T, MemoryPolicy, B>>
    load_flex_vector(node_id id)
    {
        auto* info = ar_.flex_vectors.find(id);
        if (!info) {
            return std::nullopt;
        }

        auto root = load_some_node(info->rbts.root);
        auto tail = load_leaf(info->rbts.tail);
        assert(root);
        assert(tail);
        auto impl = immer::detail::rbts::rbtree<T, MemoryPolicy, B, BL>{};
        // XXX Hack: destroy the empty tree to later replace all the members.
        // XXX add a way to construct it directly.
        impl.dec();
        impl.size  = info->rbts.size;
        impl.shift = info->rbts.shift;
        impl.root  = root.release();
        impl.tail  = tail.release();
        return flex_vector_one<T, MemoryPolicy, B>{std::move(impl)};
    }

private:
    node_ptr load_leaf(node_id id)
    {
        if (auto* p = leaves_.find(id)) {
            return *p;
        }

        auto* node_info = ar_.leaves.find(id);
        if (!node_info) {
            return nullptr;
        }

        const auto n = node_info->data.size();
        auto leaf    = node_ptr{node_t::make_leaf_n(n),
                             [n](auto* ptr) { node_t::delete_leaf(ptr, n); }};
        immer::detail::uninitialized_copy(
            node_info->data.begin(), node_info->data.end(), leaf.get()->leaf());
        leaves_ = std::move(leaves_).set(id, leaf);
        return leaf;
    }

    node_ptr load_strict(node_id id)
    {
        if (auto* p = inners_.find(id)) {
            return p->node;
        }

        auto* node_info = ar_.inners.find(id);
        if (!node_info) {
            return nullptr;
        }

        const auto n  = node_info->children.size();
        auto inner    = node_ptr{node_t::make_inner_n(n),
                              [n](auto* ptr) { node_t::delete_inner(ptr, n); }};
        auto children = immer::vector<node_ptr>{};
        auto index    = std::size_t{};
        for (const auto& child_node_id : node_info->children) {
            auto child = load_some_node(child_node_id);
            if (!child) {
                throw std::invalid_argument{
                    fmt::format("Failed to load node ID {}", child_node_id)};
            }
            auto* raw_ptr = child.get();
            children      = std::move(children).push_back(std::move(child));
            inner.get()->inner()[index] = raw_ptr;
            ++index;
        }
        inners_ = std::move(inners_).set(id,
                                         inner_node_ptr{
                                             .node     = inner,
                                             .children = std::move(children),
                                         });
        return inner;
    }

    node_ptr load_relaxed(node_id id)
    {
        if (auto* p = inners_.find(id)) {
            return p->node;
        }

        auto* node_info = ar_.relaxed_inners.find(id);
        if (!node_info) {
            return nullptr;
        }

        const auto n = node_info->children.size();
        auto relaxed = node_ptr{node_t::make_inner_r_n(n), [n](auto* ptr) {
                                    node_t::delete_inner_r(ptr, n);
                                }};
        relaxed.get()->relaxed()->d.count = n;
        auto children                     = immer::vector<node_ptr>{};
        auto index                        = std::size_t{};
        for (const auto& [child_node_id, child_size] : node_info->children) {
            auto child = load_some_node(child_node_id);
            if (!child) {
                throw std::invalid_argument{
                    fmt::format("Failed to load node ID {}", child_node_id)};
            }
            auto* raw_ptr = child.get();
            children      = std::move(children).push_back(std::move(child));
            relaxed.get()->inner()[index]            = raw_ptr;
            relaxed.get()->relaxed()->d.sizes[index] = child_size;
            ++index;
        }
        inners_ = std::move(inners_).set(id,
                                         inner_node_ptr{
                                             .node     = relaxed,
                                             .children = std::move(children),
                                         });
        return relaxed;
    }

    node_ptr load_some_node(node_id id)
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
    immer::map<node_id, node_ptr> leaves_;
    immer::map<node_id, inner_node_ptr> inners_;
};

template <class T,
          typename MemoryPolicy         = immer::default_memory_policy,
          immer::detail::rbts::bits_t B = immer::default_bits>
class vector_loader
{
public:
    explicit vector_loader(archive_load<T> ar)
        : loader{std::move(ar)}
    {
    }

    auto load(node_id id) { return loader.load_vector(id); }

private:
    loader<T, MemoryPolicy, B> loader;
};

template <class T,
          typename MemoryPolicy         = immer::default_memory_policy,
          immer::detail::rbts::bits_t B = immer::default_bits>
class flex_vector_loader
{
public:
    explicit flex_vector_loader(archive_load<T> ar)
        : loader{std::move(ar)}
    {
    }

    auto load(node_id id) { return loader.load_flex_vector(id); }

private:
    loader<T, MemoryPolicy, B> loader;
};

} // namespace immer_archive
