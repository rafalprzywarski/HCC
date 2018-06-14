find_package(Freetype REQUIRED)

include_directories("/opt/vc/include" ${FREETYPE_INCLUDE_DIRS})
link_directories("/opt/vc/lib/")

add_library(hcc_system MODULE graphics.cpp input.cpp system.cpp)
target_link_libraries(hcc_system brcmEGL brcmGLESv2 ${FREETYPE_LIBRARIES})
