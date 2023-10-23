#pragma once

#include "immer_save.hpp"

namespace immer_archive {

template <typename T,
          typename MemoryPolicy         = immer::default_memory_policy,
          immer::detail::rbts::bits_t B = immer::default_bits>
struct vector_with_archive
{
    vector_one<T, MemoryPolicy, B> vector;
    archive_save<T>* archive_save = nullptr;

    node_id vector_id;
    bool loaded = false;

    vector_with_archive() = default;

    vector_with_archive(std::initializer_list<T> values)
        : vector{std::move(values)}
    {
    }

    vector_with_archive(vector_one<T, MemoryPolicy, B> vector_)
        : vector{std::move(vector_)}
    {
    }

    friend bool operator==(const vector_with_archive& left,
                           const vector_with_archive& right)
    {
        return left.vector == right.vector;
    }
};

template <class Archive,
          class T,
          class MemoryPolicy,
          immer::detail::rbts::bits_t B>
void save(Archive& ar, const vector_with_archive<T, MemoryPolicy, B>& value)
{
    assert(value.archive_save);
    auto [archive, id] =
        save_vector(value.vector, std::move(*value.archive_save));
    *value.archive_save = std::move(archive);
    ar(id);
}

template <class Archive,
          class T,
          class MemoryPolicy,
          immer::detail::rbts::bits_t B>
void load(Archive& ar, vector_with_archive<T, MemoryPolicy, B>& value)
{
    ar(value.vector_id);
    value.loaded = true;
}

} // namespace immer_archive
