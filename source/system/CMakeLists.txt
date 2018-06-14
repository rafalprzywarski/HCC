find_package(Freetype REQUIRED)

include_directories("/opt/vc/include" ${FREETYPE_INCLUDE_DIRS})

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  find_package(SFML REQUIRED system window graphics)
  include_directories(${SFML_INCLUDE_DIR})
  add_library(hcc_system MODULE system_macos.cpp)
  target_link_libraries(hcc_system ${SFML_LIBRARIES})
else()
  link_directories("/opt/vc/lib/")
  add_library(hcc_system MODULE graphics.cpp input.cpp system.cpp)
  target_link_libraries(hcc_system brcmEGL brcmGLESv2 ${FREETYPE_LIBRARIES})
endif()
