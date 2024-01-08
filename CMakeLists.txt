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

if(BUILD_TESTS)
  enable_testing()
  find_package(Catch2 REQUIRED)
  add_subdirectory(test)
endif()

include(GNUInstallDirs)
install(DIRECTORY immer-archive DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
