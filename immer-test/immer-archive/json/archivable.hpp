#pragma once

#include <immer-archive/json/json_immer.hpp>

#include <fmt/format.h>
#include <fmt/ranges.h>

namespace immer_archive {

inline std::string to_string(int val) { return fmt::format("{}", val); }

template <typename T,
          typename MemoryPolicy         = immer::default_memory_policy,
          immer::detail::rbts::bits_t B = immer::default_bits>
std::string to_string(const immer::vector<T, MemoryPolicy, B, 1>& value)
{
    std::vector<std::string> str;
    for (const auto& item : value) {
        str.push_back(to_string(item));
    }
    return fmt::format("[{}]", fmt::join(str, ", "));
}

template <class Container>
struct archivable
{
    Container container;
    node_id container_id = {};
    bool needs_inflating = false;

    archivable() = default;

    archivable(std::initializer_list<typename Container::value_type> values)
        : container{std::move(values)}
    {
    }

    archivable(Container container_)
        : container{std::move(container_)}
    {
    }

    friend bool operator==(const archivable& left, const archivable& right)
    {
        return left.container == right.container;
    }

    friend std::string to_string(const archivable& value)
    {
        return fmt::format(
            "(container = {}, container_id = {}, needs_inflating = {})",
            to_string(value.container),
            value.container_id,
            value.needs_inflating);
    }

    template <class ArchivesLoad>
    void inflate(ArchivesLoad& archives)
    {
        if (!needs_inflating) {
            return;
        }

        auto& loader         = archives.template get_loader<Container>();
        auto maybe_container = loader.load(container_id);
        if (!maybe_container) {
            throw ::cereal::Exception{fmt::format(
                "Failed to inflate a container ID {} from the archive",
                container_id)};
        }
        container       = std::move(*maybe_container);
        needs_inflating = false;
    }
};

template <class ImmerArchives, class Container>
void save(json_immer_output_archive<ImmerArchives>& ar,
          const archivable<Container>& value)
{
    auto& save_archive =
        ar.get_archives().template get_save_archive<Container>();
    auto [archive, id] =
        save_to_archive(value.container, std::move(save_archive));
    save_archive = std::move(archive);
    ar(id);
}

template <class ImmerArchives, class Container>
void load(json_immer_input_archive<ImmerArchives>& ar,
          archivable<Container>& value)
{
    node_id vector_id;
    ar(vector_id);

    auto& loader = ar.get_archives().template get_loader<Container>();
    auto vector  = loader.load(vector_id);
    if (!vector) {
        throw ::cereal::Exception{fmt::format(
            "Failed to load a vector ID {} from the archive", vector_id)};
    }
    value = std::move(*vector);
}

template <class Archive, class Container>
void load(Archive& ar, archivable<Container>& value)
{
    node_id vector_id;
    ar(vector_id);
    value.container_id    = vector_id;
    value.needs_inflating = true;
}

} // namespace immer_archive
