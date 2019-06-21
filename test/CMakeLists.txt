include_directories(${GoogleMock_INCLUDE_DIRS})

add_executable(hcc_test
  circle_coverage_test.cpp
  main.cpp
)

target_link_libraries(hcc_test gmock pthread)
