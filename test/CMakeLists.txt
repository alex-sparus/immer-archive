add_executable(
  tests test_vectors.cpp test_special_archive.cpp test_champ.cpp
        test_includes.cpp test_xxhash.cpp ../immer-archive/xxhash/xxhash_64.cpp)
target_include_directories(tests PRIVATE ../)
target_link_libraries(tests PRIVATE spdlog::spdlog Catch2::Catch2WithMain
                                    xxHash::xxhash)

include(CTest)
include(Catch)
catch_discover_tests(tests)

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

if(UNIX AND NOT APPLE)
  set(LINUX TRUE)
endif()

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
  target_compile_definitions(
    tests PRIVATE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE IMMER_NO_FREE_LIST=1)
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
  target_compile_definitions(
    tests PRIVATE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE IMMER_NO_FREE_LIST=1)
endif()

add_subdirectory(fuzz)

install(TARGETS tests DESTINATION bin)
install(FILES valgrind.supp DESTINATION bin)
