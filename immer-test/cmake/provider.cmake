macro(mycomp_provide_dependency method dep_name)
  if("${dep_name}" MATCHES "^(antlr_cpp|zxcv)$")
    add_subdirectory($ENV{GOOGLE_FUZZTEST_ANTLR} antlr)
  elseif("${dep_name}" MATCHES "^(abseil-cpp)$")
    add_subdirectory($ENV{ABSEIL_CPP} abseil-cpp)
  elseif("${dep_name}" MATCHES "^(googletest)$")
    add_subdirectory($ENV{GOOGLE_FUZZTEST_GTEST} googletest)
  else()
    find_package(${dep_name} REQUIRED)
  endif()
endmacro()

cmake_language(SET_DEPENDENCY_PROVIDER mycomp_provide_dependency
               SUPPORTED_METHODS FETCHCONTENT_MAKEAVAILABLE_SERIAL)
