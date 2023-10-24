#pragma once

namespace immer_archive {

template <class ImmerArchives>
class with_archives_adapter_save
    : public cereal::OutputArchive<with_archives_adapter_save<ImmerArchives>>
    , public cereal::traits::TextArchive
{
public:
    template <class... Args>
    with_archives_adapter_save(Args&&... args)
        : cereal::OutputArchive<with_archives_adapter_save<ImmerArchives>>{this}
        , archive{std::forward<Args>(args)...}
    {
    }

    ~with_archives_adapter_save() { archive(CEREAL_NVP(archives)); }

    template <class T>
    void save_nvp(const cereal::NameValuePair<T>& t)
    {
        archive.setNextName(t.name);
        cereal::OutputArchive<
            with_archives_adapter_save<ImmerArchives>>::operator()(t.value);
    }

    void startNode() { archive.startNode(); }
    void writeName() { archive.writeName(); }
    void finishNode() { archive.finishNode(); }

    template <class T>
    void saveValue(const T& value)
    {
        archive.saveValue(value);
    }

    ImmerArchives& get_archives() { return archives; }

private:
    cereal::JSONOutputArchive archive;
    ImmerArchives archives;
};

} // namespace immer_archive
