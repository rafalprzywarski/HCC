find_package(Freetype REQUIRED)
find_package(PNG REQUIRED)

include_directories("/opt/vc/include" ${FREETYPE_INCLUDE_DIRS} ${PNG_INCLUDE_DIRS})

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  find_package(SFML REQUIRED system window graphics)
  FIND_LIBRARY(OpenGL_LIBRARY OpenGL )
  include_directories(${SFML_INCLUDE_DIR})
  add_library(hcc_system MODULE graphics.cpp system_macos.cpp)
  target_link_libraries(hcc_system ${SFML_LIBRARIES} ${FREETYPE_LIBRARIES} ${OpenGL_LIBRARY} ${PNG_LIBRARIES})
  message("SFML Libraries: ${SFML_LIBRARIES}")
else()
  link_directories("/opt/vc/lib/")
  add_library(hcc_system MODULE graphics.cpp input.cpp system.cpp)
  target_link_libraries(hcc_system brcmEGL brcmGLESv2 ${FREETYPE_LIBRARIES} ${PNG_LIBRARIES})
endif()

add_library(hcc_system_stub MODULE system_stub.cpp)
