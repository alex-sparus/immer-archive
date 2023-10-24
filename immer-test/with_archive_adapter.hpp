#pragma once

#include "immer_save.hpp"
#include "rbtree_builder.hpp"

/**
 * Adapted from cereal/archives/adapters.hpp
 */

namespace immer_archive {

template <class U, class A>
U& get_archives_load(A&);

template <class ImmerArchives, class Archive>
class with_archives_adapter_save
    : public cereal::OutputArchive<
          with_archives_adapter_save<ImmerArchives, Archive>>
    , public cereal::traits::TextArchive
{
public:
    template <class... Args>
    with_archives_adapter_save(Args&&... args)
        : cereal::OutputArchive<
              with_archives_adapter_save<ImmerArchives, Archive>>{this}
        , archive{std::forward<Args>(args)...}
    {
    }

    ~with_archives_adapter_save() { archive(CEREAL_NVP(archives)); }

    template <class T>
    void save_nvp(const cereal::NameValuePair<T>& t)
    {
        archive.setNextName(t.name);
        cereal::OutputArchive<
            with_archives_adapter_save<ImmerArchives,
                                       Archive>>::operator()(t.value);
    }

    template <class T>
    void save(const T& t)
    {
        archive(t);
    }

    ImmerArchives& get_archives() { return archives; }

private:
    Archive archive;
    ImmerArchives archives;
};

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

template <class ImmerArchives, class Archive, class T>
inline void CEREAL_SAVE_FUNCTION_NAME(
    with_archives_adapter_save<ImmerArchives, Archive>& ar,
    cereal::NameValuePair<T> const& t)
{
    ar.save_nvp(t);
}

/**
 * Forward saving types like "unsigned long long" etc.
 */
template <class ImmerArchives,
          class Archive,
          class T,
          typename = std::void_t<decltype(CEREAL_SAVE_FUNCTION_NAME(
              std::declval<Archive&>(), std::declval<T>()))>>
inline void CEREAL_SAVE_FUNCTION_NAME(
    with_archives_adapter_save<ImmerArchives, Archive>& ar, const T& t)
{
    ar.save(t);
}

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
          class Archive,
          class T,
          class MemoryPolicy,
          immer::detail::rbts::bits_t B>
void save(with_archives_adapter_save<ImmerArchives, Archive>& ar,
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
