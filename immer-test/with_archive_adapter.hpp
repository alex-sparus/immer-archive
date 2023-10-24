#pragma once

#include "immer_save.hpp"
#include "rbtree_builder.hpp"

/**
 * Adapted from cereal/archives/adapters.hpp
 */

namespace immer_archive {

struct archives_save
{
    archive_save<int> ints;
    archive_save<std::string> strings;

    template <class Archive>
    void save(Archive& ar) const
    {
        ar(CEREAL_NVP(ints), CEREAL_NVP(strings));
    }
};

template <class T>
archive_save<T>& get_save_archive(archives_save& ars);

template <>
archive_save<int>& get_save_archive(archives_save& ars)
{
    return ars.ints;
}

template <>
archive_save<std::string>& get_save_archive(archives_save& ars)
{
    return ars.strings;
}

struct archives_load
{
    archive_load<int> ints;
    archive_load<std::string> strings;

    std::optional<loader<int>> int_loader;
    std::optional<loader<std::string>> strings_loader;

    template <class Archive>
    void load(Archive& ar)
    {
        ar(CEREAL_NVP(ints), CEREAL_NVP(strings));
    }
};

template <class T>
loader<T>& get_loader(archives_load& ars);

template <>
loader<int>& get_loader(archives_load& ars)
{
    if (!ars.int_loader) {
        ars.int_loader.emplace(ars.ints);
    }
    return *ars.int_loader;
}

template <>
loader<std::string>& get_loader(archives_load& ars)
{
    if (!ars.strings_loader) {
        ars.strings_loader.emplace(ars.strings);
    }
    return *ars.strings_loader;
}

template <class U, class A>
U& get_archives_save(A&);

template <class U, class A>
U& get_archives_load(A&);

template <class ImmerArchives, class Archive>
class with_archives_adapter_save : public Archive
{
public:
    template <class... Args>
    with_archives_adapter_save(Args&&... args)
        : Archive{std::forward<Args>(args)...}
    {
    }

    ~with_archives_adapter_save() { Archive::operator()(CEREAL_NVP(archives)); }

private:
    friend ImmerArchives& get_archives_save<ImmerArchives>(Archive& ar);

private:
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

template <class ImmerArchives, class Archive>
ImmerArchives& get_archives_save(Archive& ar)
{
    try {
        return dynamic_cast<
                   with_archives_adapter_save<ImmerArchives, Archive>&>(ar)
            .archives;
    } catch (const std::bad_cast&) {
        throw ::cereal::Exception{
            "Attempting to get ImmerArchives from archive "
            "not wrapped in with_archives_adapter_save"};
    }
}

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

template <class Archive,
          class T,
          class MemoryPolicy,
          immer::detail::rbts::bits_t B>
void save(Archive& ar, const vector_one_archivable<T, MemoryPolicy, B>& value)
{
    auto& save_archive =
        get_save_archive<T>(get_archives_save<archives_save>(ar));
    auto [archive, id] = save_vector(value.vector, std::move(save_archive));
    save_archive       = std::move(archive);
    ar(id);
}

template <class Archive, class T>
void load(Archive& ar, vector_one_archivable<T>& value)
{
    node_id vector_id;
    ar(vector_id);

    auto& loader = get_loader<T>(get_archives_load<archives_load>(ar));
    auto vector  = loader.load_vector(vector_id);
    if (!vector) {
        throw ::cereal::Exception{fmt::format(
            "Failed to load a vector ID {} from the archive", vector_id)};
    }
    value = std::move(*vector);
}

} // namespace immer_archive
