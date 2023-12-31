#pragma once

#include <immer-archive/errors.hpp>
#include <immer-archive/node_ptr.hpp>
#include <immer-archive/rbts/archive.hpp>
#include <immer-archive/rbts/traverse.hpp>

#include <boost/hana.hpp>
#include <immer/set.hpp>
#include <immer/vector.hpp>
#include <optional>

#include <spdlog/spdlog.h>

namespace immer_archive::rbts {

template <immer::detail::rbts::bits_t B,
          immer::detail::rbts::bits_t BL,
          immer::detail::rbts::count_t Depth>
inline constexpr auto get_max_vector_size_for_depth()
{
    namespace hana = boost::hana;

    constexpr auto depth =
        hana::integral_c<immer::detail::rbts::count_t, Depth>;

    constexpr auto children_inner =
        hana::integral_c<immer::detail::rbts::size_t,
                         immer::detail::rbts::branches<B>>;
    constexpr auto children_leaf =
        hana::integral_c<immer::detail::rbts::size_t,
                         immer::detail::rbts::branches<BL>>;
    constexpr auto root_size =
        hana::power(children_inner, depth) * children_leaf;
    return root_size + children_leaf;
}

template <immer::detail::rbts::bits_t B,
          immer::detail::rbts::bits_t BL,
          immer::detail::rbts::count_t Depth>
inline constexpr auto get_shift_for_depth()
{
    namespace hana = boost::hana;

    constexpr auto bits = hana::integral_c<immer::detail::rbts::shift_t, B>;
    constexpr auto bits_leaf =
        hana::integral_c<immer::detail::rbts::shift_t, BL>;
    constexpr auto depth =
        hana::integral_c<immer::detail::rbts::shift_t, Depth>;

    return bits_leaf +
           bits * (depth - hana::integral_c<immer::detail::rbts::shift_t, 1>);
}

inline constexpr auto
get_shift_for_depth_runtime(immer::detail::rbts::bits_t b,
                            immer::detail::rbts::bits_t bl,
                            immer::detail::rbts::count_t depth)
{
    auto bits      = immer::detail::rbts::shift_t{b};
    auto bits_leaf = immer::detail::rbts::shift_t{bl};
    return bits_leaf + bits * (immer::detail::rbts::shift_t{depth} - 1);
}

template <immer::detail::rbts::bits_t B, immer::detail::rbts::bits_t BL>
inline auto get_shift_for_size(immer::detail::rbts::size_t size)
{
    namespace hana = boost::hana;

    // Let's assume depth 12 is enough.
    constexpr auto max_depth = immer::detail::rbts::count_t{12};
    constexpr auto really_big_number_of_elements = 2305843009213693954;
    static_assert(get_max_vector_size_for_depth<5, 1, max_depth>().value ==
                  really_big_number_of_elements);

    auto found = false;
    auto shift = immer::detail::rbts::shift_t{};
    hana::for_each(
        hana::range_c<immer::detail::rbts::count_t, 1, max_depth + 1>,
        [&](auto d) {
            if (found) {
                return;
            }
            constexpr auto max_size =
                get_max_vector_size_for_depth<B, BL, d.value>();
            if (size <= max_size) {
                found = true;
                shift = get_shift_for_depth<B, BL, d.value>();
            }
        });

    if (!found) {
        throw std::invalid_argument{"size must be way too big"};
    }

    return shift;
}

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

class relaxed_node_not_allowed_exception : public archive_exception
{
public:
    relaxed_node_not_allowed_exception(node_id id_)
        : archive_exception{fmt::format(
              "Non-relaxed vector can not have relaxed node ID {}", id_)}
        , id{id_}
    {
    }

    node_id id;
};

template <class T,
          typename MemoryPolicy,
          immer::detail::rbts::bits_t B,
          immer::detail::rbts::bits_t BL>
class loader
{
public:
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

    immer::vector<T, MemoryPolicy, B, BL> load_vector(node_id id)
    {
        auto* info = ar_.vectors.find(id);
        if (!info) {
            throw archive_exception{fmt::format("Unknown vector ID {}", id)};
        }

        const auto relaxed_allowed = false;
        auto root = load_inner(info->root, {}, relaxed_allowed);
        auto tail = load_leaf(info->tail);

        const auto tree_size =
            get_node_size(info->root) + get_node_size(info->tail);

        auto impl = rbtree{tree_size,
                           get_shift_for_size<B, BL>(tree_size),
                           std::move(root).release(),
                           std::move(tail).release()};

        verify_tree(impl);
        return impl;
    }

    immer::flex_vector<T, MemoryPolicy, B, BL> load_flex_vector(node_id id)
    {
        auto* info = ar_.flex_vectors.find(id);
        if (!info) {
            throw invalid_node_id{id};
        }

        const auto relaxed_allowed = true;
        auto root = load_inner(info->root, {}, relaxed_allowed);
        auto tail = load_leaf(info->tail);

        const auto tree_size =
            get_node_size(info->root) + get_node_size(info->tail);
        const auto depth = get_node_depth(info->root);
        const auto shift = get_shift_for_depth_runtime(B, BL, depth);

        auto impl = rrbtree{tree_size,
                            shift,
                            std::move(root).release(),
                            std::move(tail).release()};

        verify_tree(impl);

        return impl;
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

    node_ptr
    load_inner(node_id id, nodes_set_t loading_nodes, bool relaxed_allowed)
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

        const auto children_ids = get_node_children(*node_info);

        const auto n         = children_ids.size();
        constexpr auto max_n = immer::detail::rbts::branches<B>;
        if (n > max_n) {
            throw invalid_children_count{id};
        }

        // The code doesn't handle very well a relaxed node with size zero.
        // Pretend it's a strict node.
        const bool is_relaxed = node_info->relaxed && n > 0;

        if (is_relaxed && !relaxed_allowed) {
            throw relaxed_node_not_allowed_exception{id};
        }

        auto inner =
            is_relaxed
                ? node_ptr{node_t::make_inner_r_n(n),
                           [n](auto* ptr) { node_t::delete_inner_r(ptr, n); }}
                : node_ptr{node_t::make_inner_n(n),
                           [n](auto* ptr) { node_t::delete_inner(ptr, n); }};
        auto children = immer::vector<node_ptr>{};

        if (is_relaxed) {
            inner.get()->relaxed()->d.count = n;
            auto index                      = std::size_t{};
            auto running_size               = std::size_t{};
            for (const auto& child_node_id : children_ids) {
                auto child = load_some_node(
                    child_node_id, loading_nodes, relaxed_allowed);
                running_size += get_node_size(child_node_id);

                auto* raw_ptr = child.get();
                children      = std::move(children).push_back(std::move(child));
                inner.get()->inner()[index]            = raw_ptr;
                inner.get()->relaxed()->d.sizes[index] = running_size;
                ++index;
            }
        } else {
            auto index = std::size_t{};
            for (const auto& child_node_id : children_ids) {
                auto child = load_some_node(
                    child_node_id, loading_nodes, relaxed_allowed);
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

    node_ptr
    load_some_node(node_id id, nodes_set_t loading_nodes, bool relaxed_allowed)
    {
        // Unknown type: leaf, inner or relaxed
        if (ar_.leaves.count(id)) {
            return load_leaf(id);
        }
        if (ar_.inners.count(id)) {
            return load_inner(id, std::move(loading_nodes), relaxed_allowed);
        }
        throw invalid_node_id{id};
    }

    immer::vector<node_id> get_node_children(const inner_node& node_info)
    {
        // Ignore empty children
        auto result = immer::vector<node_id>{};
        for (const auto& child_node_id : node_info.children) {
            const auto child_size = get_node_size(child_node_id);
            if (child_size) {
                result = std::move(result).push_back(child_node_id);
            }
        }
        return result;
    }

    std::size_t get_node_size(node_id id, nodes_set_t loading_nodes = {})
    {
        if (auto* p = sizes_.find(id)) {
            return *p;
        }
        auto size = [&] {
            if (auto* p = ar_.leaves.find(id)) {
                return p->data.size();
            }
            if (auto* p = ar_.inners.find(id)) {
                auto result   = std::size_t{};
                loading_nodes = std::move(loading_nodes).insert(id);
                for (const auto& child_id : p->children) {
                    if (loading_nodes.count(child_id)) {
                        throw archive_has_cycles{child_id};
                    }
                    result += get_node_size(child_id, loading_nodes);
                }
                return result;
            }
            throw invalid_node_id{id};
        }();
        sizes_ = std::move(sizes_).set(id, size);
        return size;
    }

    immer::detail::rbts::count_t get_node_depth(node_id id)
    {
        if (auto* p = depths_.find(id)) {
            return *p;
        }
        auto depth = [&]() -> immer::detail::rbts::count_t {
            if (auto* p = ar_.leaves.find(id)) {
                return 0;
            }
            if (auto* p = ar_.inners.find(id)) {
                if (p->children.empty()) {
                    return 1;
                } else {
                    return 1 + get_node_depth(p->children.front());
                }
            }
            throw invalid_node_id{id};
        }();
        depths_ = std::move(depths_).set(id, depth);
        return depth;
    }

    template <class Tree>
    void verify_tree(const Tree& impl)
    {
        const auto check_inner = [&](auto&& pos,
                                     auto&& visit,
                                     bool visiting_relaxed) {
            const auto* id = loaded_inners_.find(pos.node());
            if (!id) {
                if (loaded_leaves_.find(pos.node())) {
                    throw std::logic_error{"A node is expected to be an inner "
                                           "node but it's actually a leaf"};
                }

                throw std::logic_error{"Inner node of a freshly loaded "
                                       "vector is unknown"};
            }
            const auto* info = ar_.inners.find(*id);
            assert(info);
            if (!info) {
                throw std::logic_error{
                    "No info for the just loaded inner node"};
            }

            /**
             * NOTE: Not sure how useful this check is.
             */
            // if (visiting_relaxed != info->relaxed) {
            //     throw std::logic_error{fmt::format(
            //         "Node {} is expected to be relaxed == {} but it is {}",
            //         *id,
            //         info->relaxed,
            //         visiting_relaxed)};
            // }

            const auto expected_count = pos.count();
            const auto real_count     = get_node_children(*info).size();
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
                    const bool visiting_relaxed = false;
                    check_inner(pos, visit, visiting_relaxed);
                },
                [&](detail::relaxed_pos_tag, auto&& pos, auto&& visit) {
                    // SPDLOG_INFO("relaxed_pos_tag");
                    const bool visiting_relaxed = true;
                    check_inner(pos, visit, visiting_relaxed);
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
                        throw vector_corrupted_exception{
                            *id, expected_count, real_count};
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
    immer::map<node_id, immer::detail::rbts::count_t> depths_;
};

template <typename T,
          typename MemoryPolicy,
          immer::detail::rbts::bits_t B,
          immer::detail::rbts::bits_t BL>
class vector_loader
{
public:
    explicit vector_loader(archive_load<T> ar)
        : loader{std::move(ar)}
    {
    }

    auto load(node_id id) { return loader.load_vector(id); }

private:
    loader<T, MemoryPolicy, B, BL> loader;
};

template <typename T,
          typename MemoryPolicy,
          immer::detail::rbts::bits_t B,
          immer::detail::rbts::bits_t BL>
vector_loader<T, MemoryPolicy, B, BL>
make_loader_for(const immer::vector<T, MemoryPolicy, B, BL>&,
                archive_load<T> ar)
{
    return vector_loader<T, MemoryPolicy, B, BL>{std::move(ar)};
}

template <typename T,
          typename MemoryPolicy,
          immer::detail::rbts::bits_t B,
          immer::detail::rbts::bits_t BL>
class flex_vector_loader
{
public:
    explicit flex_vector_loader(archive_load<T> ar)
        : loader{std::move(ar)}
    {
    }

    auto load(node_id id) { return loader.load_flex_vector(id); }

private:
    loader<T, MemoryPolicy, B, BL> loader;
};

template <typename T,
          typename MemoryPolicy,
          immer::detail::rbts::bits_t B,
          immer::detail::rbts::bits_t BL>
flex_vector_loader<T, MemoryPolicy, B, BL>
make_loader_for(const immer::flex_vector<T, MemoryPolicy, B, BL>&,
                archive_load<T> ar)
{
    return flex_vector_loader<T, MemoryPolicy, B, BL>{std::move(ar)};
}

} // namespace immer_archive::rbts
