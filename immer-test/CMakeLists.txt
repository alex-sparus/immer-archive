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

add_executable(MyExample main.cpp test/test_struct_with_archive.cpp)
target_include_directories(MyExample PRIVATE immer .)
target_link_libraries(MyExample PRIVATE spdlog::spdlog Catch2::Catch2WithMain)

if(CMAKE_GENERATOR STREQUAL "Xcode")
  target_compile_options(MyExample PRIVATE -O0 -fno-optimize-sibling-calls -g
                                           -fno-omit-frame-pointer)
  target_compile_definitions(MyExample PRIVATE BOOST_USE_ASAN=1)
  target_compile_definitions(MyExample
                             PRIVATE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)
else()
  # ninja && ASAN_OPTIONS=detect_leaks=1 LSAN_OPTIONS=suppressions=../suppr.txt
  # ./MyExample
  target_compile_options(
    MyExample
    PRIVATE -O0 -fno-optimize-sibling-calls -g -fno-omit-frame-pointer
            -fsanitize-coverage=trace-pc-guard -fsanitize=leak)
  target_compile_definitions(MyExample PRIVATE BOOST_USE_ASAN=1)
  target_compile_definitions(MyExample
                             PRIVATE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)
  target_link_options(MyExample PRIVATE -fsanitize=leak)
endif()
