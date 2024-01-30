#pragma once

#include <immer-archive/champ/archive.hpp>
#include <immer-archive/champ/champ.hpp>
#include <immer-archive/traits.hpp>

namespace immer_archive {

template <class Container>
struct champ_traits
{
    using save_archive_t =
        immer_archive::champ::container_archive_save<Container>;
    using load_archive_t =
        immer_archive::champ::container_archive_load<Container>;
    using loader_t     = immer_archive::champ::container_loader<Container>;
    using container_id = immer_archive::champ::node_id;
};

template <typename K,
          typename T,
          typename Hash,
          typename Equal,
          typename MemoryPolicy,
          immer::detail::hamts::bits_t B>
struct container_traits<immer::map<K, T, Hash, Equal, MemoryPolicy, B>>
    : champ_traits<immer::map<K, T, Hash, Equal, MemoryPolicy, B>>
{};

template <typename T,
          typename KeyFn,
          typename Hash,
          typename Equal,
          typename MemoryPolicy,
          immer::detail::hamts::bits_t B>
struct container_traits<immer::table<T, KeyFn, Hash, Equal, MemoryPolicy, B>>
    : champ_traits<immer::table<T, KeyFn, Hash, Equal, MemoryPolicy, B>>
{};

} // namespace immer_archive
