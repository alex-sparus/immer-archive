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
    using node_t  = immer::detail::rbts::node<T, MemoryPolicy, B, BL>;
    using edit_t  = typename node_t::edit_t;
    using owner_t = typename MemoryPolicy::transience_t::owner;

    size_t size;
    immer::detail::rbts::shift_t shift;
    node_t* root;
    node_t* tail;
};

} // namespace immer_archive
