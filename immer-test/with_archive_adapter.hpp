#pragma once

#include "immer_save.hpp"
#include "rbtree_builder.hpp"

#include <json_immer.hpp>

namespace immer_archive {

template <typename T,
          typename MemoryPolicy         = immer::default_memory_policy,
          immer::detail::rbts::bits_t B = immer::default_bits>
struct vector_one_archivable
{
    vector_one<T, MemoryPolicy, B> vector;

    vector_one_archivable() = default;

    vector_one_archivable(std::initializer_list<T> values)
        : vector{std::move(values)}
    {
    }

    vector_one_archivable(vector_one<T, MemoryPolicy, B> vector_)
        : vector{std::move(vector_)}
    {
    }

    friend bool operator==(const vector_one_archivable& left,
                           const vector_one_archivable& right)
    {
        return left.vector == right.vector;
    }
};

template <class ImmerArchives,
          class T,
          class MemoryPolicy,
          immer::detail::rbts::bits_t B>
void save(json_immer_output_archive<ImmerArchives>& ar,
          const vector_one_archivable<T, MemoryPolicy, B>& value)
{
    auto& save_archive = get_save_archive<T>(ar.get_archives());
    auto [archive, id] = save_vector(value.vector, std::move(save_archive));
    save_archive       = std::move(archive);
    ar(id);
}

template <class ImmerArchives, class T>
void load(json_immer_input_archive<ImmerArchives>& ar,
          vector_one_archivable<T>& value)
{
    node_id vector_id;
    ar(vector_id);

    auto& loader = get_loader<T>(ar.get_archives());
    auto vector  = loader.load_vector(vector_id);
    if (!vector) {
        throw ::cereal::Exception{fmt::format(
            "Failed to load a vector ID {} from the archive", vector_id)};
    }
    value = std::move(*vector);
}

} // namespace immer_archive
