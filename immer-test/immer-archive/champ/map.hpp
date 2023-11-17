#pragma once

#include "archive_map.hpp"
#include "load.hpp"
#include "save.hpp"

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

template <typename K,
          typename V,
          typename Hash,
          typename Equal,
          typename MemoryPolicy,
          immer::detail::hamts::bits_t B>
std::pair<map_archive_save<K, V, Hash, B>, node_id>
save_map(immer::map<K, V, Hash, Equal, MemoryPolicy, B> map,
         map_archive_save<K, V, Hash, B> archive)
{
    const auto& impl = map.impl();
    auto root_id     = node_id{};
    std::tie(archive.nodes, root_id) =
        get_node_id(std::move(archive.nodes), impl.root);

    if (auto* p = archive.maps.find(root_id)) {
        // Already been saved
        return {std::move(archive), root_id};
    }

    archive.nodes = save_nodes(impl, std::move(archive.nodes));
    assert(archive.nodes.inners.count(root_id));

    archive.maps = std::move(archive.maps)
                       .set(root_id,
                            map_save<K, V, Hash>{
                                .champ =
                                    champ_info{
                                        .root = root_id,
                                        .size = impl.size,
                                    },
                                .map = std::move(map),
                            });

    return {std::move(archive), root_id};
}

} // namespace champ
} // namespace immer_archive
