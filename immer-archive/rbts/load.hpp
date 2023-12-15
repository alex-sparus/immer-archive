#pragma once

#include <immer-archive/errors.hpp>
#include <immer-archive/node_ptr.hpp>
#include <immer-archive/rbts/archive.hpp>
#include <immer-archive/rbts/traverse.hpp>

#include <boost/hana/functional/overload.hpp>
#include <immer/set.hpp>
#include <immer/vector.hpp>
#include <optional>

#include <spdlog/spdlog.h>

namespace immer_archive::rbts {

class vector_corrupted_exception : public archive_exception
{
public:
    vector_corrupted_exception(node_id id_,
                               std::size_t expected_count_,
                               std::size_t real_count_)
        : archive_exception{fmt::format("Loaded vector is corrupted. Inner "
                                        "node ID {} should "
                                        "have {} children but it has {}",
                                        id_,
                                        expected_count_,
                                        real_count_)}
        , id{id_}
        , expected_count{expected_count_}
        , real_count{real_count_}
    {
    }

    node_id id;
    std::size_t expected_count;
    std::size_t real_count;
};

template <class T,
          typename MemoryPolicy         = immer::default_memory_policy,
          immer::detail::rbts::bits_t B = immer::default_bits>
class loader
{
public:
    static constexpr auto BL = immer::detail::rbts::bits_t{1};
    using rbtree         = immer::detail::rbts::rbtree<T, MemoryPolicy, B, BL>;
    using rrbtree        = immer::detail::rbts::rrbtree<T, MemoryPolicy, B, BL>;
    using node_t         = typename rbtree::node_t;
    using node_ptr       = node_ptr<node_t>;
    using inner_node_ptr = inner_node_ptr<node_t>;
    using nodes_set_t    = immer::set<node_id>;

    explicit loader(archive_load<T> ar)
        : ar_{std::move(ar)}
    {
    }

    vector_one<T, MemoryPolicy, B> load_vector(node_id id)
    {
        auto* info = ar_.vectors.find(id);
        if (!info) {
            throw archive_exception{fmt::format("Unknown vector ID {}", id)};
        }

        auto root = load_strict(info->rbts.root, {});
        auto tail = load_leaf(info->rbts.tail);

        const auto tree_size =
            get_node_size(info->rbts.root) + get_node_size(info->rbts.tail);

        auto impl = rbtree{tree_size,
                           info->rbts.shift,
                           std::move(root).release(),
                           std::move(tail).release()};

        verify_tree(impl);
        return vector_one<T, MemoryPolicy, B>{std::move(impl)};
    }

    flex_vector_one<T, MemoryPolicy, B> load_flex_vector(node_id id)
    {
        auto* info = ar_.flex_vectors.find(id);
        if (!info) {
            throw invalid_node_id{id};
        }

        auto root = load_some_node(info->rbts.root, {});
        auto tail = load_leaf(info->rbts.tail);

        const auto tree_size =
            get_node_size(info->rbts.root) + get_node_size(info->rbts.tail);

        auto impl = rrbtree{tree_size,
                            info->rbts.shift,
                            std::move(root).release(),
                            std::move(tail).release()};

        verify_tree(impl);

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
            throw invalid_node_id{id};
        }

        const auto n         = node_info->data.size();
        constexpr auto max_n = immer::detail::rbts::branches<BL>;
        if (n > max_n) {
            throw invalid_children_count{id};
        }

        auto leaf = node_ptr{node_t::make_leaf_n(n),
                             [n](auto* ptr) { node_t::delete_leaf(ptr, n); }};
        immer::detail::uninitialized_copy(
            node_info->data.begin(), node_info->data.end(), leaf.get()->leaf());
        leaves_        = std::move(leaves_).set(id, leaf);
        loaded_leaves_ = std::move(loaded_leaves_).set(leaf.get(), id);
        return leaf;
    }

    node_ptr load_strict(node_id id, nodes_set_t loading_nodes)
    {
        if (loading_nodes.count(id)) {
            throw archive_has_cycles{id};
        }

        if (auto* p = inners_.find(id)) {
            return p->node;
        }

        auto* node_info = ar_.inners.find(id);
        if (!node_info) {
            throw invalid_node_id{id};
        }

        loading_nodes = std::move(loading_nodes).insert(id);

        const auto n         = node_info->children.size();
        constexpr auto max_n = immer::detail::rbts::branches<B>;
        if (n > max_n) {
            throw invalid_children_count{id};
        }

        auto inner =
            node_info->relaxed
                ? node_ptr{node_t::make_inner_r_n(n),
                           [n](auto* ptr) { node_t::delete_inner_r(ptr, n); }}
                : node_ptr{node_t::make_inner_n(n),
                           [n](auto* ptr) { node_t::delete_inner(ptr, n); }};
        auto children = immer::vector<node_ptr>{};

        if (node_info->relaxed) {
            inner.get()->relaxed()->d.count = n;
            auto index                      = std::size_t{};
            auto running_size               = std::size_t{};
            for (const auto& child_node_id : node_info->children) {
                auto child = load_some_node(child_node_id, loading_nodes);
                running_size += get_node_size(child_node_id);

                auto* raw_ptr = child.get();
                children      = std::move(children).push_back(std::move(child));
                inner.get()->inner()[index]            = raw_ptr;
                inner.get()->relaxed()->d.sizes[index] = running_size;
                ++index;
            }
        } else {
            auto index = std::size_t{};
            for (const auto& child_node_id : node_info->children) {
                auto child    = load_some_node(child_node_id, loading_nodes);
                auto* raw_ptr = child.get();
                children      = std::move(children).push_back(std::move(child));
                inner.get()->inner()[index] = raw_ptr;
                ++index;
            }
        }

        inners_        = std::move(inners_).set(id,
                                         inner_node_ptr{
                                                    .node     = inner,
                                                    .children = std::move(children),
                                         });
        loaded_inners_ = std::move(loaded_inners_).set(inner.get(), id);
        return inner;
    }

    node_ptr load_some_node(node_id id, nodes_set_t loading_nodes)
    {
        // Unknown type: leaf, inner or relaxed
        if (ar_.leaves.count(id)) {
            return load_leaf(id);
        }
        if (ar_.inners.count(id)) {
            return load_strict(id, std::move(loading_nodes));
        }
        throw invalid_node_id{id};
    }

    std::size_t get_node_size(node_id id)
    {
        if (auto* p = sizes_.find(id)) {
            return *p;
        }
        auto size = [&] {
            if (auto* p = ar_.leaves.find(id)) {
                return p->data.size();
            }
            if (auto* p = ar_.inners.find(id)) {
                auto result = std::size_t{};
                for (const auto& child_id : p->children) {
                    result += get_node_size(child_id);
                }
                return result;
            }
            throw invalid_node_id{id};
        }();
        sizes_ = std::move(sizes_).set(id, size);
        return size;
    }

    template <class Tree>
    void verify_tree(const Tree& impl) const
    {
        const auto check_inner = [&](auto&& pos, auto&& visit) {
            const auto* id = loaded_inners_.find(pos.node());
            assert(id);
            if (!id) {
                throw std::logic_error{"Inner node of a freshly loaded "
                                       "vector is unknown"};
            }
            const auto* info = ar_.inners.find(*id);
            assert(info);
            if (!info) {
                throw std::logic_error{
                    "No info for the just loaded inner node"};
            }

            const auto expected_count = pos.count();
            const auto real_count     = info->children.size();
            if (expected_count != real_count) {
                throw vector_corrupted_exception{
                    *id, expected_count, real_count};
            }

            pos.each(detail::visitor_helper{},
                     [&visit](auto any_tag, auto& child_pos, auto&&) {
                         visit(child_pos);
                     });
        };

        impl.traverse(
            detail::visitor_helper{},
            boost::hana::overload(
                [&](detail::regular_pos_tag, auto&& pos, auto&& visit) {
                    // SPDLOG_INFO("regular_pos_tag");
                    check_inner(pos, visit);
                },
                [&](detail::relaxed_pos_tag, auto&& pos, auto&& visit) {
                    // SPDLOG_INFO("relaxed_pos_tag");
                    check_inner(pos, visit);
                },
                [&](detail::leaf_pos_tag, auto&& pos, auto&& visit) {
                    // SPDLOG_INFO("leaf_pos_tag");
                    const auto* id = loaded_leaves_.find(pos.node());
                    assert(id);
                    if (!id) {
                        throw std::logic_error{
                            "Leaf of a freshly loaded vector is unknown"};
                    }
                    const auto* info = ar_.leaves.find(*id);
                    assert(info);
                    if (!info) {
                        throw std::logic_error{
                            "No info for the just loaded leaf"};
                    }

                    const auto expected_count = pos.count();
                    const auto real_count     = info->data.size();
                    if (expected_count != real_count) {
                        throw std::invalid_argument{fmt::format(
                            "Loaded vector is corrupted. Leaf ID {} should "
                            "have {} elements but it has {}",
                            *id,
                            expected_count,
                            real_count)};
                    }
                }));
    }

private:
    const archive_load<T> ar_;
    immer::map<node_id, node_ptr> leaves_;
    immer::map<node_id, inner_node_ptr> inners_;
    immer::map<node_t*, node_id> loaded_leaves_;
    immer::map<node_t*, node_id> loaded_inners_;
    immer::map<node_id, std::size_t> sizes_;
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

} // namespace immer_archive::rbts
