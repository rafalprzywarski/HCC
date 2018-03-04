include_directories("/opt/vc/include")
link_directories("/opt/vc/lib/")

add_library(hcc_graphics MODULE graphics.cpp)
target_link_libraries(hcc_graphics brcmEGL brcmGLESv2)
