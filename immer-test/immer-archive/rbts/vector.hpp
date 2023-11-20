#pragma once

#include <immer-archive/rbts/load.hpp>
#include <immer-archive/rbts/save.hpp>
#include <immer-archive/traits.hpp>

namespace immer_archive {

template <class T>
struct container_traits<vector_one<T>>
{
    using save_archive_t = archive_save<T>;
    using load_archive_t = archive_load<T>;
    using loader_t       = vector_loader<T>;
};

template <class T>
struct container_traits<flex_vector_one<T>>
{
    using save_archive_t = archive_save<T>;
    using load_archive_t = archive_load<T>;
    using loader_t       = flex_vector_loader<T>;
};

} // namespace immer_archive
