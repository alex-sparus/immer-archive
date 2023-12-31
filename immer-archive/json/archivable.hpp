#pragma once

#include <immer-archive/errors.hpp>
#include <immer-archive/json/json_immer.hpp>
#include <immer-archive/traits.hpp>

#include <fmt/format.h>

#include <boost/hana/type.hpp>

namespace immer_archive {

template <class Container>
struct archivable
{
    Container container;

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

    friend auto begin(const archivable& value)
    {
        return value.container.begin();
    }

    friend auto end(const archivable& value) { return value.container.end(); }
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
    using container_id = typename container_traits<Container>::container_id;
    auto id            = container_id{};
    ar(id);

    auto& loader = ar.get_archives().template get_loader<Container>();

    constexpr auto is_optional = boost::hana::is_valid(
        [](auto&& x) -> decltype((void) x.has_value()) {});
    using IsOptional = decltype(is_optional(loader.load(id)));

    if constexpr (IsOptional::value) {
        auto container = loader.load(id);
        if (!container) {
            throw ::cereal::Exception{fmt::format(
                "Failed to load a container ID {} from the archive", id)};
        }
        value.container = std::move(*container);
    } else {
        try {
            value.container = loader.load(id);
        } catch (const archive_exception& ex) {
            throw ::cereal::Exception{fmt::format(
                "Failed to load a container ID {} from the archive: {}",
                id,
                ex.what())};
        }
    }
}

template <class Archive, class Container>
void load(Archive& ar, archivable<Container>& value)
{
    using container_id = typename container_traits<Container>::container_id;
    auto id            = container_id{};
    ar(id);
}

} // namespace immer_archive
