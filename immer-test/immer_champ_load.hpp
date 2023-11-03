#pragma once

#include "immer_champ_archive.hpp"
#include <node_ptr.hpp>

#include <spdlog/spdlog.h>

namespace immer {
/**
 * This probably should be moved into immer
 */
template <class Map>
struct map_access
{
    using value_t   = typename Map::value_t;
    using hash_key  = typename Map::hash_key;
    using equal_key = typename Map::equal_key;
    using impl_t    = typename Map::impl_t;
};
} // namespace immer

namespace immer_archive {
namespace champ {

template <class T,
          typename Hash                  = std::hash<T>,
          typename Equal                 = std::equal_to<T>,
          typename MemoryPolicy          = immer::default_memory_policy,
          immer::detail::hamts::bits_t B = immer::default_bits>
class nodes_loader
{
public:
    using champ_t =
        immer::detail::hamts::champ<T, Hash, Equal, MemoryPolicy, B>;
    using node_t         = typename champ_t::node_t;
    using node_ptr       = immer_archive::node_ptr<node_t>;
    using inner_node_ptr = immer_archive::inner_node_ptr<node_t>;

    explicit nodes_loader(nodes_load<T, B> archive)
        : archive_{std::move(archive)}
    {
    }

    node_ptr load_collision(node_id id)
    {
        if (auto* p = collisions_.find(id)) {
            return *p;
        }

        auto* node_info = archive_.collisions.find(id);
        if (!node_info) {
            return nullptr;
        }

        const auto n = node_info->data.size();
        auto node    = node_ptr{node_t::make_collision_n(n),
                             [](auto* ptr) { node_t::delete_collision(ptr); }};
        immer::detail::uninitialized_copy(node_info->data.begin(),
                                          node_info->data.end(),
                                          node.get()->collisions());
        collisions_ = std::move(collisions_).set(id, node);
        return node;
    }

    node_ptr load_inner(node_id id)
    {
        if (auto* p = inners_.find(id)) {
            return p->node;
        }

        auto* node_info = archive_.inners.find(id);
        if (!node_info) {
            return nullptr;
        }

        const auto n  = node_info->children.size();
        const auto nv = node_info->values.data.size();
        auto inner    = node_ptr{node_t::make_inner_n(n, nv),
                              [](auto* ptr) { node_t::delete_inner(ptr); }};
        inner.get()->impl.d.data.inner.nodemap = node_info->nodemap;
        inner.get()->impl.d.data.inner.datamap = node_info->datamap;

        // XXX Loading validation
        assert(inner.get()->children_count() == n);
        assert(inner.get()->data_count() == nv);

        // Values
        if (nv) {
            immer::detail::uninitialized_copy(node_info->values.data.begin(),
                                              node_info->values.data.end(),
                                              inner.get()->values());
        }

        // Children
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
            inner.get()->children()[index] = raw_ptr;
            ++index;
        }

        inners_ = std::move(inners_).set(id,
                                         inner_node_ptr{
                                             .node     = inner,
                                             .children = std::move(children),
                                         });
        return inner;
    }

    node_ptr load_some_node(node_id id)
    {
        if (archive_.inners.count(id)) {
            return load_inner(id);
        }
        if (archive_.collisions.count(id)) {
            return load_collision(id);
        }
        return nullptr;
    }

private:
    const nodes_load<T, B> archive_;
    immer::map<node_id, node_ptr> collisions_;
    immer::map<node_id, inner_node_ptr> inners_;
};

template <class T,
          typename Hash                  = std::hash<T>,
          typename Equal                 = std::equal_to<T>,
          typename MemoryPolicy          = immer::default_memory_policy,
          immer::detail::hamts::bits_t B = immer::default_bits>
class set_loader
{
public:
    explicit set_loader(set_archive_load<T, B> archive)
        : archive_{std::move(archive)}
        , nodes_{archive_.nodes}
    {
    }

    std::optional<immer::set<T, Hash>> load_set(node_id id)
    {
        auto* info = archive_.sets.find(id);
        if (!info) {
            return std::nullopt;
        }

        auto root = nodes_.load_inner(info->root);
        if (!root) {
            return std::nullopt;
        }

        auto impl =
            immer::detail::hamts::champ<T, Hash, Equal, MemoryPolicy, B>{
                root.release(), info->size};

        // XXX This ctor is not public in immer.
        auto set = immer::set<T, Hash>{std::move(impl)};
        return set;
    }

private:
    const set_archive_load<T, B> archive_;
    nodes_loader<T, Hash, Equal, MemoryPolicy, B> nodes_;
};

template <class K,
          class V,
          typename Hash                  = std::hash<K>,
          typename Equal                 = std::equal_to<K>,
          typename MemoryPolicy          = immer::default_memory_policy,
          immer::detail::hamts::bits_t B = immer::default_bits>
class map_loader
{
public:
    using map_t = immer::map<K, V, Hash, Equal, MemoryPolicy, B>;

    explicit map_loader(map_archive_load<K, V, B> archive)
        : archive_{std::move(archive)}
        , nodes_{archive_.nodes}
    {
    }

    std::optional<map_t> load_map(node_id id);

private:
    const map_archive_load<K, V, B> archive_;

    using map_access = immer::map_access<map_t>;
    nodes_loader<typename map_access::value_t,
                 typename map_access::hash_key,
                 typename map_access::equal_key,
                 MemoryPolicy,
                 B>
        nodes_;
};

template <class K,
          class V,
          typename Hash,
          typename Equal,
          typename MemoryPolicy,
          immer::detail::hamts::bits_t B>
inline std::optional<
    typename map_loader<K, V, Hash, Equal, MemoryPolicy, B>::map_t>
map_loader<K, V, Hash, Equal, MemoryPolicy, B>::load_map(node_id id)
{
    auto* info = archive_.maps.find(id);
    if (!info) {
        return std::nullopt;
    }

    auto root = nodes_.load_inner(info->root);
    if (!root) {
        return std::nullopt;
    }

    auto impl = typename map_access::impl_t{root.release(), info->size};

    // XXX This ctor is not public in immer.
    auto map = map_t{std::move(impl)};
    return map;
}

} // namespace champ
} // namespace immer_archive
