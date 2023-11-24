cmake_minimum_required(VERSION 3.5.1)

project(
  ModernCMakeExample
  VERSION 1.0
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(spdlog REQUIRED)
find_package(cereal REQUIRED)
find_package(Catch2 REQUIRED)

add_executable(
  MyExample test/test_vectors.cpp test/test_struct_with_archive.cpp
            test/test_special_archive.cpp test/test_champ.cpp)
target_include_directories(MyExample PRIVATE immer .)
target_link_libraries(MyExample PRIVATE spdlog::spdlog Catch2::Catch2WithMain)

target_compile_options(
  MyExample
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
  target_compile_options(MyExample PRIVATE -O0 -fno-optimize-sibling-calls -g
                                           -fno-omit-frame-pointer)
  target_compile_definitions(MyExample PRIVATE BOOST_USE_ASAN=1)
  target_compile_definitions(MyExample
                             PRIVATE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)
elseif(LINUX)
  target_compile_options(
    MyExample
    PRIVATE -O1 -fno-optimize-sibling-calls -g -fno-omit-frame-pointer
            -fsanitize-coverage=trace-pc-guard -fsanitize=address)
  target_compile_definitions(MyExample PRIVATE BOOST_USE_ASAN=1)
  target_compile_definitions(MyExample
                             PRIVATE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)
  target_link_options(MyExample PRIVATE -fsanitize=address)
else()
  # macOS but not Xcode ninja && ASAN_OPTIONS=detect_leaks=1
  # LSAN_OPTIONS=suppressions=../suppr.txt ./MyExample
  target_compile_options(
    MyExample
    PRIVATE -O0 -fno-optimize-sibling-calls -g -fno-omit-frame-pointer
            -fsanitize-coverage=trace-pc-guard -fsanitize=address)
  target_compile_definitions(MyExample PRIVATE BOOST_USE_ASAN=1)
  target_compile_definitions(MyExample
                             PRIVATE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)
  target_link_options(MyExample PRIVATE -fsanitize=address)
endif()

# Fuzz
add_executable(fuzz_set test/fuzz/fuzz_set.cpp)
target_include_directories(fuzz_set PRIVATE immer/extra/fuzzer immer .)
target_compile_options(
  fuzz_set PRIVATE -g -O1 -fno-optimize-sibling-calls -fno-omit-frame-pointer
                   -fsanitize=fuzzer,address)
target_link_libraries(fuzz_set PRIVATE spdlog::spdlog)
target_link_options(fuzz_set PRIVATE -g -O1 -fsanitize=fuzzer,address)
target_compile_options(
  fuzz_set
  PRIVATE -Wno-unknown-warning-option
          -Werror
          -Wall
          -Wextra
          -pedantic
          -Wno-unused-parameter
          -Wno-c++20-designator
          -Wno-gnu-anonymous-struct
          -Wno-nested-anon-types
          # -Wno-nullability-extension -Wno-deprecated-declarations
)
