#pragma once

#include <immer-archive/rbts/load.hpp>
#include <json_immer.hpp>

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
    friend immer_archive::archive_save<T>& get_save_archive(archives_save& ars)
    {
        return ars.storage[hana::type_c<T>];
    }
};

template <class T>
struct archive_type_load
{
    archive_load<T> archive;
    std::optional<loader<T>> loader;
};

template <class Storage, class Names>
struct archives_load
{
    using names_t = Names;

    Storage storage;

    template <class T>
    friend loader<T>& get_loader(archives_load& ars)
    {
        auto& load = ars.storage[hana::type_c<T>];
        if (!load.loader) {
            load.loader.emplace(load.archive);
        }
        return *load.loader;
    }

    template <class Archive>
    void load(Archive& ar)
    {
        constexpr auto keys = hana::keys(names_t{});
        hana::for_each(keys, [&](auto key) {
            constexpr auto name = names_t{}[key];
            ar(cereal::make_nvp(name.c_str(), storage[key].archive));
        });
    }
};

inline auto generate_archives_save(auto type_names)
{
    auto storage =
        hana::fold_left(type_names, hana::make_map(), [](auto map, auto pair) {
            using Type = typename decltype(+hana::first(pair))::type;
            return hana::insert(
                map,
                hana::make_pair(hana::first(pair),
                                immer_archive::archive_save<Type>{}));
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
    }
    return std::make_pair(os.str(), std::move(archives));
}

template <typename T>
T from_json_with_archive(const std::string& input)
{
    using Archives = decltype(detail::generate_archives_load(
        get_archives_types(std::declval<T>())));
    auto archives  = Archives{};

    {
        auto is = std::istringstream{input};
        auto ar = cereal::JSONInputArchive{is};
        ar(CEREAL_NVP(archives));
    }

    auto is = std::istringstream{input};
    auto ar = immer_archive::json_immer_input_archive<Archives>{
        std::move(archives), is};
    auto r = T{};
    ar(r);
    return r;
}

} // namespace immer_archive
