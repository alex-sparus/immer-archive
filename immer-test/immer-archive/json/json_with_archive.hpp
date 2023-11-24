#pragma once

#include <immer-archive/json/json_immer.hpp>
#include <immer-archive/traits.hpp>

#include <boost/hana.hpp>

/**
 * to_json_with_archive
 */

namespace immer_archive {

namespace detail {

namespace hana = boost::hana;

/**
 * Archives and functions to serialize types that contain immer-archivable data
 * structures.
 */
template <class Storage, class Names>
struct archives_save
{
    using names_t = Names;

    Storage storage;

    template <class Archive>
    void save(Archive& ar) const
    {
        constexpr auto keys = hana::keys(names_t{});
        hana::for_each(keys, [&](auto key) {
            constexpr auto name = names_t{}[key];
            ar(cereal::make_nvp(name.c_str(), storage[key]));
        });
    }

    template <class T>
    auto& get_save_archive()
    {
        using Contains = decltype(hana::contains(storage, hana::type_c<T>));
        constexpr bool contains = hana::value<Contains>();
        static_assert(contains,
                      "There is no archive for the given type, check the "
                      "get_archives_types function");
        return storage[hana::type_c<T>];
    }
};

template <class Container>
struct archive_type_load
{
    typename container_traits<Container>::load_archive_t archive;
    std::optional<typename container_traits<Container>::loader_t> loader;

    template <class ArchivesLoad>
    void inflate(ArchivesLoad& archives)
    {
        archive.inflate(archives);
    }
};

template <class Storage, class Names>
struct archives_load
{
    using names_t = Names;

    Storage storage;

    template <class Container>
    auto& get_loader()
    {
        auto& load = storage[hana::type_c<Container>];
        if (!load.loader) {
            load.loader.emplace(load.archive);
        }
        return *load.loader;
    }

    void inflate()
    {
        hana::for_each(hana::keys(storage),
                       [&](auto key) { storage[key].inflate(*this); });
    }

    template <class Archive>
    void load(Archive& ar)
    {
        constexpr auto keys = hana::keys(names_t{});
        hana::for_each(keys, [&](auto key) {
            constexpr auto name = names_t{}[key];
            ar(cereal::make_nvp(name.c_str(), storage[key].archive));
        });

        inflate();
    }
};

inline auto generate_archives_save(auto type_names)
{
    auto storage =
        hana::fold_left(type_names, hana::make_map(), [](auto map, auto pair) {
            using Type = typename decltype(+hana::first(pair))::type;
            return hana::insert(
                map,
                hana::make_pair(
                    hana::first(pair),
                    typename container_traits<Type>::save_archive_t{}));
        });

    using Storage = decltype(storage);
    using Names   = decltype(type_names);
    return archives_save<Storage, Names>{storage};
}

inline auto generate_archives_load(auto type_names)
{
    auto storage =
        hana::fold_left(type_names, hana::make_map(), [](auto map, auto pair) {
            using Type = typename decltype(+hana::first(pair))::type;
            return hana::insert(
                map,
                hana::make_pair(hana::first(pair), archive_type_load<Type>{}));
        });

    using Storage = decltype(storage);
    using Names   = decltype(type_names);
    return archives_load<Storage, Names>{storage};
}

} // namespace detail

/**
 * Type T must provide a callable free function get_archives_types(const T&).
 */
template <typename T>
auto to_json_with_archive(const T& serializable)
{
    auto archives =
        detail::generate_archives_save(get_archives_types(serializable));
    auto os = std::ostringstream{};
    {
        auto ar =
            immer_archive::json_immer_output_archive<decltype(archives)>{os};
        ar(serializable);
        archives = ar.get_archives();

        {
            auto os2 = std::ostringstream{};
            auto ar2 =
                immer_archive::json_immer_output_archive<decltype(archives)>{
                    archives, os2};
            ar2(archives);
            archives = ar2.get_archives();
        }

        ar.get_archives() = archives;
        ar.finalize();
    }
    return std::make_pair(os.str(), std::move(archives));
}

template <typename T>
T from_json_with_archive(const std::string& input)
{
    using Archives = decltype(detail::generate_archives_load(
        get_archives_types(std::declval<T>())));
    auto archives  = Archives{};

    SPDLOG_INFO("loading archive as json");
    {
        auto is = std::istringstream{input};
        auto ar = cereal::JSONInputArchive{is};
        ar(CEREAL_NVP(archives));
    }
    SPDLOG_INFO("done loading archive as json");

    auto archives2 = Archives{};
    SPDLOG_INFO("loading archive again");
    {
        auto is = std::istringstream{input};
        auto ar =
            immer_archive::json_immer_input_archive<Archives>{archives, is};
        auto& archives = archives2;
        ar(CEREAL_NVP(archives));
    }
    SPDLOG_INFO("done loading archive again");

    auto is = std::istringstream{input};
    auto ar = immer_archive::json_immer_input_archive<Archives>{
        std::move(archives2), is};
    auto r = T{};
    ar(r);
    SPDLOG_INFO("done loading the value");
    return r;
}

} // namespace immer_archive
