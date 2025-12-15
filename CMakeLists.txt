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

# ImGui detection
set(IMGUI_CANDIDATES
    ${CMAKE_SOURCE_DIR}/include/imgui
    ${CMAKE_SOURCE_DIR}/imgui
    ${CMAKE_SOURCE_DIR}/external/imgui
)

foreach(CANDIDATE ${IMGUI_CANDIDATES})
    if(EXISTS ${CANDIDATE}/imgui.h)
        set(IMGUI_DIR ${CANDIDATE})
        break()
    endif()
endforeach()

if(NOT IMGUI_DIR)
    message(FATAL_ERROR "ImGui not found. Please run setup.sh first.")
endif()

message(STATUS "Using ImGui from: ${IMGUI_DIR}")

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# GLAD detection
set(GLAD_CANDIDATES
    ${CMAKE_SOURCE_DIR}/include/glad/src/gl.c
    ${CMAKE_SOURCE_DIR}/src/glad/gl.c
    ${CMAKE_SOURCE_DIR}/glad/src/gl.c
    ${CMAKE_SOURCE_DIR}/external/glad/src/gl.c
)

foreach(CANDIDATE ${GLAD_CANDIDATES})
    if(EXISTS ${CANDIDATE})
        set(GLAD_SOURCES ${CANDIDATE})
        get_filename_component(GLAD_DIR ${CANDIDATE} DIRECTORY)
        get_filename_component(GLAD_DIR ${GLAD_DIR} DIRECTORY)
        break()
    endif()
endforeach()

if(NOT GLAD_SOURCES)
    message(FATAL_ERROR "GLAD not found. Searched: ${GLAD_CANDIDATES}")
endif()

message(STATUS "Using GLAD from: ${GLAD_SOURCES}")

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${GLAD_DIR}/include
    ${ENET_INCLUDE_DIRS}
)

# Main executable
add_executable(placid
    src/main.cpp
    src/EditorApp.cpp
    src/GameMode.cpp
    src/Renderer.cpp
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

# Optional: simple_hero_test (if you want to keep it)
if(EXISTS ${CMAKE_SOURCE_DIR}/src/simple_hero_test.cpp)
    add_executable(simple_hero_test
        src/simple_hero_test.cpp
    )
    target_link_libraries(simple_hero_test
        pthread
    )
endif()
