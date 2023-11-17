#pragma once

#include "archive_set.hpp"
#include "load.hpp"

namespace immer_archive {
namespace champ {

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

template <typename T,
          typename Hash,
          typename Equal,
          typename MemoryPolicy,
          immer::detail::hamts::bits_t B>
std::pair<set_archive_save<T, Hash, B>, node_id>
save_set(immer::set<T, Hash, Equal, MemoryPolicy, B> set,
         set_archive_save<T, Hash, B> archive)
{
    const auto& impl = set.impl();
    auto root_id     = node_id{};
    std::tie(archive.nodes, root_id) =
        get_node_id(std::move(archive.nodes), impl.root);

    if (auto* p = archive.sets.find(root_id)) {
        // Already been saved
        return {std::move(archive), root_id};
    }

    archive.nodes = save_nodes(impl, std::move(archive.nodes));
    assert(archive.nodes.inners.count(root_id));

    archive.sets = std::move(archive.sets)
                       .set(root_id,
                            set_save<T, Hash>{
                                .champ =
                                    champ_info{
                                        .root = root_id,
                                        .size = impl.size,
                                    },
                                .set = std::move(set),
                            });

    return {std::move(archive), root_id};
}

} // namespace champ
} // namespace immer_archive
