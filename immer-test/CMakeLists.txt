cmake_minimum_required(VERSION 3.5.1)

project(
  ModernCMakeExample
  VERSION 1.0
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(spdlog REQUIRED)
find_package(cereal REQUIRED)

add_compile_definitions(SPDLOG_ACTIVE_LEVEL TRACE)

add_executable(MyExample main.cpp)
target_include_directories(MyExample PRIVATE immer .)
target_link_libraries(MyExample PRIVATE spdlog::spdlog)

target_compile_options(MyExample INTERFACE
    -fno-omit-frame-pointer -fsanitize-coverage=trace-pc-guard -fsanitize=address)
target_compile_definitions(MyExample INTERFACE BOOST_USE_ASAN=1)
target_link_options(MyExample INTERFACE -fsanitize=address)

target_compile_options(MyExample PRIVATE
    $<$<CXX_COMPILER_ID:Clang>:-g>
    )
