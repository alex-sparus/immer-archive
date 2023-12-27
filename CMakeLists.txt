cmake_minimum_required(VERSION 3.5.1)

project(
  ImmerArchive
  VERSION 1.0
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(TESTS_WITH_LEAK_SANITIZER "enable leak sanitizer for tests" yes)
option(BUILD_TESTS "enable tests" yes)

find_package(spdlog REQUIRED)
find_package(cereal REQUIRED)
find_package(Catch2 REQUIRED)

if(UNIX AND NOT APPLE)
  set(LINUX TRUE)
endif()

function(add_fuzzer bin_name file)
  add_executable(${bin_name} ${file})
  target_include_directories(${bin_name} PRIVATE immer/extra/fuzzer .)
  target_compile_options(
    ${bin_name} PRIVATE -g -O1 -fno-optimize-sibling-calls
                        -fno-omit-frame-pointer -fsanitize=fuzzer,address)
  target_link_libraries(${bin_name} PRIVATE spdlog::spdlog)
  target_link_options(${bin_name} PRIVATE -g -O1 -fsanitize=fuzzer,address)
  target_compile_options(
    ${bin_name}
    PRIVATE -Wno-unknown-warning-option
            -Werror
            -Wall
            -Wextra
            -pedantic
            -Wno-unused-parameter
            -Wno-c++20-designator
            -Wno-gnu-anonymous-struct
            -Wno-nested-anon-types
            -Wno-unused-variable
            # -Wno-nullability-extension -Wno-deprecated-declarations
  )
endfunction()

if(BUILD_TESTS)
  add_executable(tests test/test_vectors.cpp test/test_special_archive.cpp
                       test/test_champ.cpp test/test_includes.cpp)
  target_include_directories(tests PRIVATE immer .)
  target_link_libraries(tests PRIVATE spdlog::spdlog Catch2::Catch2WithMain)

  target_compile_options(
    tests
    PRIVATE -Wno-unknown-warning-option
            -Werror
            -Wall
            -Wextra
            -pedantic
            -Wno-unused-parameter
            -Wno-c++20-designator
            -Wno-gnu-anonymous-struct
            -Wno-nested-anon-types
            -Wno-unused-function
            # -Wno-nullability-extension -Wno-deprecated-declarations
  )

  if(CMAKE_GENERATOR STREQUAL "Xcode")
    target_compile_options(tests PRIVATE -O0 -fno-optimize-sibling-calls -g
                                         -fno-omit-frame-pointer)
    target_compile_definitions(tests PRIVATE BOOST_USE_ASAN=1)
    target_compile_definitions(tests
                               PRIVATE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)
  elseif(LINUX)
    target_compile_options(tests PRIVATE -O1 -fno-optimize-sibling-calls -g
                                         -fno-omit-frame-pointer)
    if(TESTS_WITH_LEAK_SANITIZER)
      target_compile_options(tests PRIVATE -fsanitize-coverage=trace-pc-guard
                                           -fsanitize=address)
      target_link_options(tests PRIVATE -fsanitize=address)
    endif()
    target_compile_definitions(tests PRIVATE BOOST_USE_ASAN=1)
    target_compile_definitions(tests
                               PRIVATE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)
  else()
    # macOS but not Xcode ninja && ASAN_OPTIONS=detect_leaks=1
    # LSAN_OPTIONS=suppressions=../suppr.txt ./tests
    target_compile_options(tests PRIVATE -O0 -fno-optimize-sibling-calls -g
                                         -fno-omit-frame-pointer)
    if(TESTS_WITH_LEAK_SANITIZER)
      target_compile_options(tests PRIVATE -fsanitize-coverage=trace-pc-guard
                                           -fsanitize=address)
      target_link_options(tests PRIVATE -fsanitize=address)
    endif()
    target_compile_definitions(tests PRIVATE BOOST_USE_ASAN=1)
    target_compile_definitions(tests
                               PRIVATE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)
  endif()

  # Fuzz
  add_fuzzer(fuzz_set test/fuzz/fuzz_set.cpp)
  add_fuzzer(flex-vector test/fuzz/flex-vector.cpp)
endif()

include(GNUInstallDirs)
install(DIRECTORY immer-archive DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

if(BUILD_TESTS)
  include(CTest)
  include(Catch)
  catch_discover_tests(tests)
endif()
