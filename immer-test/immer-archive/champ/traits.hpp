#pragma once

#include <immer-archive/champ/archive_champ.hpp>
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
    using loader_t = immer_archive::champ::container_loader<Container>;
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

} // namespace immer_archive
