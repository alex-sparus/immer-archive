#pragma once

#include "immer_save.hpp"
#include "rbtree_builder.hpp"

#include <adapter/adapter.hpp>

/**
 * Adapted from cereal/archives/adapters.hpp
 */

namespace immer_archive {

template <class U, class A>
U& get_archives_load(A&);

template <class ImmerArchives, class Archive>
class with_archives_adapter_load : public Archive
{
public:
    template <class... Args>
    with_archives_adapter_load(ImmerArchives archives_, Args&&... args)
        : Archive{std::forward<Args>(args)...}
        , archives{std::move(archives_)}
    {
    }

private:
    friend ImmerArchives& get_archives_load<ImmerArchives>(Archive& ar);

private:
    ImmerArchives archives;
};

// template <class T>
// inline void CEREAL_LOAD_FUNCTION_NAME(JSONInputArchive& ar, NameValuePair<T>&
// t)
// {
//     ar.setNextName(t.name);
//     ar(t.value);
// }

template <class ImmerArchives, class Archive>
ImmerArchives& get_archives_load(Archive& ar)
{
    try {
        return dynamic_cast<
                   with_archives_adapter_load<ImmerArchives, Archive>&>(ar)
            .archives;
    } catch (const std::bad_cast&) {
        throw ::cereal::Exception{
            "Attempting to get ImmerArchives from archive "
            "not wrapped in with_archives_adapter_load"};
    }
}

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
void save(with_archives_adapter_save<ImmerArchives>& ar,
          const vector_one_archivable<T, MemoryPolicy, B>& value)
{
    auto& save_archive = get_save_archive<T>(ar.get_archives());
    auto [archive, id] = save_vector(value.vector, std::move(save_archive));
    save_archive       = std::move(archive);
    ar(id);
}

template <class Archive, class T>
void load(Archive& ar, vector_one_archivable<T>& value)
{
    // node_id vector_id;
    // ar(vector_id);

    // auto& loader = get_loader<T>(get_archives_load<archives_load>(ar));
    // auto vector  = loader.load_vector(vector_id);
    // if (!vector) {
    //     throw ::cereal::Exception{fmt::format(
    //         "Failed to load a vector ID {} from the archive", vector_id)};
    // }
    // value = std::move(*vector);
}

} // namespace immer_archive
