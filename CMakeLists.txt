project(hcc)
cmake_minimum_required(VERSION 2.8)

add_definitions("-std=c++14 -Wall")
if(APPLE)
	add_definitions("-Wno-unused-const-variable")
endif(APPLE)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake_modules")

set(GoogleMock_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/libraries/GoogleMock/include")

add_subdirectory("source")
add_subdirectory("test")
add_subdirectory("libraries")
