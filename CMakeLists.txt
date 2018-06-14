project(cleo)
cmake_minimum_required(VERSION 2.8)

add_definitions("-std=c++14 -Wall")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake_modules")

add_subdirectory("source")
