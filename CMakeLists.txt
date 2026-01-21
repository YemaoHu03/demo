cmake_minimum_required(VERSION 3.15)
project(text_element_demo CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Freetype REQUIRED)

add_library(demo_core
    src/image/rgba_surface.cpp
    src/image/blit.cpp
    src/image/ppm_writer.cpp

    src/text/font_library.cpp
    src/text/font_face.cpp
    src/text/glyph_cache.cpp
    src/text/text_layout.cpp
    src/text/text_rasterizer.cpp

    src/elements/element_renderer.cpp
    src/renderer/scene_renderer.cpp
)

target_include_directories(demo_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${FREETYPE_INCLUDE_DIRS}
)

target_link_libraries(demo_core PUBLIC
    ${FREETYPE_LIBRARIES}
)

add_executable(demo src/main.cpp)
target_link_libraries(demo PRIVATE demo_core)
