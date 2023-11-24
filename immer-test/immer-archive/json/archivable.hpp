#pragma once

#include <immer-archive/json/json_immer.hpp>

#include <immer-archive/to_string.hpp>

namespace immer_archive {

template <class Container>
struct archivable
{
    std::optional<Container> container;
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

    Container get_or_default() const { return container.value_or(Container{}); }

    friend bool operator==(const archivable& left, const archivable& right)
    {
        return left.container == right.container;
    }

    friend std::string to_string(const archivable& value)
    {
        return fmt::format(
            "(container = {}, container_id = {}, needs_inflating = {})",
            to_string(value.get_or_default()),
            value.container_id,
            value.needs_inflating);
    }
};

template <class ImmerArchives, class Container>
void save(json_immer_output_archive<ImmerArchives>& ar,
          const archivable<Container>& value)
{
    auto& save_archive =
        ar.get_archives().template get_save_archive<Container>();
    auto [archive, id] =
        save_to_archive(value.container.value(), std::move(save_archive));
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
    value.container = std::move(*vector);
    SPDLOG_INFO("loaded inflated container ID {} [{}] {}",
                vector_id,
                immer_archive::to_string(value.container.value()),
                typeid(Container{}).name());
}

template <class Archive, class Container>
void load(Archive& ar, archivable<Container>& value)
{
    node_id vector_id;
    ar(vector_id);
    value.container_id    = vector_id;
    value.needs_inflating = true;
    SPDLOG_INFO("loaded uninflated container ID {} {}",
                vector_id,
                typeid(Container{}).name());
}

} // namespace immer_archive
