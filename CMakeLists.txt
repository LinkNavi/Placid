cmake_minimum_required(VERSION 3.10)
project(Placid)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Find packages
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(ENET REQUIRED libenet)

# ImGui - check multiple locations
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/include/imgui)
if(NOT EXISTS ${IMGUI_DIR})
    set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/imgui)
endif()

if(EXISTS ${IMGUI_DIR})
    set(IMGUI_SOURCES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    )
else()
    message(FATAL_ERROR "ImGui not found. Please run setup.sh")
endif()

# GLAD
set(GLAD_SOURCES ${CMAKE_SOURCE_DIR}/src/gl.c)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${ENET_INCLUDE_DIRS}
)

# Main executable
add_executable(placid
    src/main.cpp
    src/EditorApp.cpp
    src/GameMode.cpp
    src/Renderer.cpp
    src/LocalPlayer.cpp
    src/GameScene.cpp
    ${IMGUI_SOURCES}
    ${GLAD_SOURCES}
)

target_link_libraries(placid
    glfw
    OpenGL::GL
    ${ENET_LIBRARIES}
    ${CMAKE_DL_LIBS}
    pthread
)

# Test executables
add_executable(test_network
    src/test_network.cpp
)

target_link_libraries(test_network
    ${ENET_LIBRARIES}
    pthread
)

if(EXISTS ${CMAKE_SOURCE_DIR}/src/simple_hero_test.cpp)
    add_executable(simple_hero_test
        src/simple_hero_test.cpp
    )
    target_link_libraries(simple_hero_test
        pthread
    )
endif()
