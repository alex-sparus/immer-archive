#pragma once

#include "save_archive.hpp"

namespace immer_archive {

//! Prologue for NVPs for JSON archives
/*! NVPs do not start or finish nodes - they just set up the names */
template <class T, class ImmerArchives>
inline void prologue(with_archives_adapter_save<ImmerArchives>&,
                     cereal::NameValuePair<T> const&)
{
}

// ######################################################################
//! Epilogue for NVPs for JSON archives
/*! NVPs do not start or finish nodes - they just set up the names */
template <class T, class ImmerArchives>
inline void epilogue(with_archives_adapter_save<ImmerArchives>&,
                     cereal::NameValuePair<T> const&)
{
}

// ######################################################################
//! Prologue for deferred data for JSON archives
/*! Do nothing for the defer wrapper */
template <class T, class ImmerArchives>
inline void prologue(with_archives_adapter_save<ImmerArchives>&,
                     cereal::DeferredData<T> const&)
{
}

// ######################################################################
//! Epilogue for deferred for JSON archives
/*! NVPs do not start or finish nodes - they just set up the names */
template <class T, class ImmerArchives>
inline void epilogue(with_archives_adapter_save<ImmerArchives>&,
                     cereal::DeferredData<T> const&)
{
}

// ######################################################################
//! Prologue for SizeTags for JSON archives
/*! SizeTags are strictly ignored for JSON, they just indicate
    that the current node should be made into an array */
template <class T, class ImmerArchives>
inline void prologue(with_archives_adapter_save<ImmerArchives>& ar,
                     cereal::SizeTag<T> const&)
{
    ar.makeArray();
}

// ######################################################################
//! Epilogue for SizeTags for JSON archives
/*! SizeTags are strictly ignored for JSON */
template <class T, class ImmerArchives>
inline void epilogue(with_archives_adapter_save<ImmerArchives>&,
                     cereal::SizeTag<T> const&)
{
}

// ######################################################################
//! Prologue for all other types for JSON archives (except minimal types)
/*! Starts a new node, named either automatically or by some NVP,
    that may be given data by the type about to be archived

    Minimal types do not start or finish nodes */
template <class ImmerArchives,
          class T,
          cereal::traits::EnableIf<
              !std::is_arithmetic<T>::value,
              !cereal::traits::has_minimal_base_class_serialization<
                  T,
                  cereal::traits::has_minimal_output_serialization,
                  with_archives_adapter_save<ImmerArchives>>::value,
              !cereal::traits::has_minimal_output_serialization<
                  T,
                  with_archives_adapter_save<ImmerArchives>>::value> =
              cereal::traits::sfinae>
inline void prologue(with_archives_adapter_save<ImmerArchives>& ar, T const&)
{
    ar.startNode();
}

// ######################################################################
//! Epilogue for all other types other for JSON archives (except minimal types)
/*! Finishes the node created in the prologue

    Minimal types do not start or finish nodes */
template <class ImmerArchives,
          class T,
          cereal::traits::EnableIf<
              !std::is_arithmetic<T>::value,
              !cereal::traits::has_minimal_base_class_serialization<
                  T,
                  cereal::traits::has_minimal_output_serialization,
                  with_archives_adapter_save<ImmerArchives>>::value,
              !cereal::traits::has_minimal_output_serialization<
                  T,
                  with_archives_adapter_save<ImmerArchives>>::value> =
              cereal::traits::sfinae>
inline void epilogue(with_archives_adapter_save<ImmerArchives>& ar, T const&)
{
    ar.finishNode();
}

// ######################################################################
//! Prologue for arithmetic types for JSON archives
template <class ImmerArchives>
inline void prologue(with_archives_adapter_save<ImmerArchives>& ar,
                     std::nullptr_t const&)
{
    ar.writeName();
}

// ######################################################################
//! Epilogue for arithmetic types for JSON archives
template <class ImmerArchives>
inline void epilogue(with_archives_adapter_save<ImmerArchives>&,
                     std::nullptr_t const&)
{
}

// ######################################################################
//! Prologue for arithmetic types for JSON archives
template <class T,
          class ImmerArchives,
          cereal::traits::EnableIf<std::is_arithmetic<T>::value> =
              cereal::traits::sfinae>
inline void prologue(with_archives_adapter_save<ImmerArchives>& ar, T const&)
{
    ar.writeName();
}

// ######################################################################
//! Epilogue for arithmetic types for JSON archives
template <class ImmerArchives,
          class T,
          cereal::traits::EnableIf<std::is_arithmetic<T>::value> =
              cereal::traits::sfinae>
inline void epilogue(with_archives_adapter_save<ImmerArchives>&, T const&)
{
}

// ######################################################################
//! Prologue for strings for JSON archives
template <class ImmerArchives, class CharT, class Traits, class Alloc>
inline void prologue(with_archives_adapter_save<ImmerArchives>& ar,
                     std::basic_string<CharT, Traits, Alloc> const&)
{
    ar.writeName();
}

// ######################################################################
//! Epilogue for strings for JSON archives
template <class ImmerArchives, class CharT, class Traits, class Alloc>
inline void epilogue(with_archives_adapter_save<ImmerArchives>&,
                     std::basic_string<CharT, Traits, Alloc> const&)
{
}

// ######################################################################
// Common JSONArchive serialization functions
// ######################################################################
//! Serializing NVP types to JSON
template <class ImmerArchives, class T>
inline void
CEREAL_SAVE_FUNCTION_NAME(with_archives_adapter_save<ImmerArchives>& ar,
                          cereal::NameValuePair<T> const& t)
{
    ar.save_nvp(t);
}

//! Saving for nullptr to JSON
template <class ImmerArchives>
inline void
CEREAL_SAVE_FUNCTION_NAME(with_archives_adapter_save<ImmerArchives>& ar,
                          std::nullptr_t const& t)
{
    ar.saveValue(t);
}

//! Saving for arithmetic to JSON
template <class ImmerArchives,
          class T,
          cereal::traits::EnableIf<std::is_arithmetic<T>::value> =
              cereal::traits::sfinae>
inline void
CEREAL_SAVE_FUNCTION_NAME(with_archives_adapter_save<ImmerArchives>& ar,
                          T const& t)
{
    ar.saveValue(t);
}

//! saving string to JSON
template <class ImmerArchives, class CharT, class Traits, class Alloc>
inline void
CEREAL_SAVE_FUNCTION_NAME(with_archives_adapter_save<ImmerArchives>& ar,
                          std::basic_string<CharT, Traits, Alloc> const& str)
{
    ar.saveValue(str);
}

// ######################################################################
//! Saving SizeTags to JSON
template <class ImmerArchives, class T>
inline void
CEREAL_SAVE_FUNCTION_NAME(with_archives_adapter_save<ImmerArchives>&,
                          cereal::SizeTag<T> const&)
{
    // nothing to do here, we don't explicitly save the size
}

} // namespace immer_archive
