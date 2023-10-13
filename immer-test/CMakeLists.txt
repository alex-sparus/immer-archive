cmake_minimum_required(VERSION 3.5.1)

project(
  ModernCMakeExample
  VERSION 1.0
  LANGUAGES CXX)


add_executable(MyExample main.cpp)
target_include_directories(MyExample PRIVATE immer)
