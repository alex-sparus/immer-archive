#pragma once

#include <immer-archive/rbts/load.hpp>
#include <with_archive.hpp>

/**
 * This approach is using the standard JSONOutputArchive and does some special
 * handling in-between.
 *
 * Doesn't seem to be a good approach.
 */

namespace test {

struct info
{
    immer_archive::vector_with_archive<int> ints;
    immer_archive::vector_with_archive<std::string> strings;

    auto tie() const { return std::tie(ints, strings); }

    friend bool operator==(const info& left, const info& right)
    {
        return left.tie() == right.tie();
    }
};

struct with_archive
{
    info data;
    immer_archive::archive_save<int> ints;
    immer_archive::archive_save<std::string> strings;

    immer_archive::archive_load<int> load_ints;
    immer_archive::archive_load<std::string> load_strings;

    void inject_save()
    {
        data.ints.archive_save    = &ints;
        data.strings.archive_save = &strings;
    }

    void load_from_archive()
    {
        auto ints_loader = immer_archive::loader{load_ints};
        data.ints.vector = ints_loader.load_vector(data.ints.vector_id).value();

        auto strings_loader = immer_archive::loader{load_strings};
        data.strings.vector =
            strings_loader.load_vector(data.strings.vector_id).value();
    }
};

template <class Archive>
void serialize(Archive& ar, info& value)
{
    auto& ints    = value.ints;
    auto& strings = value.strings;
    ar(CEREAL_NVP(ints), CEREAL_NVP(strings));
}

template <class Archive>
void save(Archive& ar, const with_archive& value)
{
    auto& data    = value.data;
    auto& ints    = value.ints;
    auto& strings = value.strings;
    ar(CEREAL_NVP(data), CEREAL_NVP(ints), CEREAL_NVP(strings));
}

template <class Archive>
void load(Archive& ar, with_archive& value)
{
    auto& data    = value.data;
    auto& ints    = value.load_ints;
    auto& strings = value.load_strings;
    ar(CEREAL_NVP(data), CEREAL_NVP(ints), CEREAL_NVP(strings));
}

} // namespace test
