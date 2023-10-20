#pragma once

#include <immer/detail/rbts/node.hpp>
#include <optional>

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

        auto root = load_strict(info->rbts.root);
        auto tail = load_leaf(info->rbts.tail);
        assert(root);
        assert(tail);
        auto impl = immer::detail::rbts::rbtree<T, MemoryPolicy, B, BL>{};
        // XXX add a way to construct it directly, this will lead to memory leak
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
        return vector_one<T, MemoryPolicy, B>{std::move(impl)};
    }

private:
    class node_ptr
    {
    private:
        node_t* ptr;
        std::function<void(node_t* ptr)> deleter;

    public:
        node_ptr(node_t* ptr_, std::function<void(node_t* ptr)> deleter_)
            : ptr{ptr_}
            , deleter{std::move(deleter_)}
        {
            SPDLOG_TRACE("ctor {} with ptr {}", (void*) this, (void*) ptr);
            // Assuming the node has just been created and not calling inc() on
            // it.
        }

        node_ptr(std::nullptr_t)
            : ptr{nullptr}
        {
        }

        node_ptr(const node_ptr& other)
            : ptr{other.ptr}
            , deleter{other.deleter}
        {
            SPDLOG_TRACE("copy ctor {} from {}", (void*) this, (void*) &other);
            if (ptr) {
                ptr->inc();
            }
        }

        node_ptr(node_ptr&& other)
            : ptr{other.release()}
            , deleter{other.deleter}
        {
            SPDLOG_TRACE("move ctor {} from {}", (void*) this, (void*) &other);
        }

        node_ptr& operator=(node_ptr&& other)
        {
            SPDLOG_TRACE("move assign {} = {}", (void*) this, (void*) &other);
            auto temp = node_ptr{std::move(other)};
            using std::swap;
            swap(ptr, temp.ptr);
            swap(deleter, temp.deleter);
            return *this;
        }

        ~node_ptr()
        {
            SPDLOG_TRACE("dtor {}", (void*) this);
            if (ptr && ptr->dec()) {
                SPDLOG_TRACE("calling deleter for {}", (void*) ptr);
                deleter(ptr);
            }
        }

        operator bool() const { return ptr; }

        node_t* release()
        {
            auto result = ptr;
            ptr         = nullptr;
            return result;
        }

        node_t* get() { return ptr; }
    };

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
        if (auto* p = strict_inners_.find(id)) {
            return *p;
        }

        auto* node_info = ar_.inners.find(id);
        if (!node_info) {
            return nullptr;
        }

        const auto n = node_info->children.size();
        auto inner   = node_ptr{node_t::make_inner_n(n),
                              [n](auto* ptr) { node_t::delete_inner(ptr, n); }};

        auto index = std::size_t{};
        for (const auto& child_node_id : node_info->children) {
            auto child = load_some_node(child_node_id);
            if (!child) {
                throw std::invalid_argument{
                    fmt::format("Failed to load node ID {}", child_node_id)};
            }
            inner.get()->inner()[index] = child.release();
            ++index;
        }
        strict_inners_ = std::move(strict_inners_).set(id, inner);
        return inner;
    }

    node_ptr load_relaxed(node_id id)
    {
        if (auto* p = relaxed_inners_.find(id)) {
            return *p;
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
        auto index                        = std::size_t{};
        for (const auto& [child_node_id, child_size] : node_info->children) {
            auto child = load_some_node(child_node_id);
            if (!child) {
                throw std::invalid_argument{
                    fmt::format("Failed to load node ID {}", child_node_id)};
            }
            relaxed.get()->inner()[index]            = child.release();
            relaxed.get()->relaxed()->d.sizes[index] = child_size;
            ++index;
        }
        relaxed_inners_ = std::move(relaxed_inners_).set(id, relaxed);
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
    immer::map<node_id, node_ptr> strict_inners_;
    immer::map<node_id, node_ptr> relaxed_inners_;
};

} // namespace immer_archive
