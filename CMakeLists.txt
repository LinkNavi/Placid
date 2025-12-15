cmake_minimum_required(VERSION 3.10)
project(Placid LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Put runtime binaries under the build directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Make sure the project can find your local headers
include_directories(${CMAKE_SOURCE_DIR}/include)

# Build glad as a small static library from the provided C source
add_library(glad STATIC src/gl.c)
target_include_directories(glad PUBLIC ${CMAKE_SOURCE_DIR}/include)

# Find system OpenGL
find_package(OpenGL REQUIRED)

# Find GLFW
find_package(glfw3 3.3 REQUIRED)

# Prevent GLFW from including OpenGL headers (we use GLAD)
add_definitions(-DGLFW_INCLUDE_NONE)

# ImGui - Look for it in multiple possible locations
set(IMGUI_SEARCH_PATHS
    ${CMAKE_SOURCE_DIR}/include/imgui
    ${CMAKE_SOURCE_DIR}/external/imgui
    ${CMAKE_SOURCE_DIR}/imgui
)

set(IMGUI_FOUND FALSE)
foreach(SEARCH_PATH ${IMGUI_SEARCH_PATHS})
    if(EXISTS ${SEARCH_PATH}/imgui.h)
        set(IMGUI_DIR ${SEARCH_PATH})
        set(IMGUI_FOUND TRUE)
        message(STATUS "Found ImGui at: ${IMGUI_DIR}")
        break()
    endif()
endforeach()

if(IMGUI_FOUND)
    set(IMGUI_SOURCES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    )
    add_library(imgui STATIC ${IMGUI_SOURCES})
    target_include_directories(imgui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)
    target_link_libraries(imgui PUBLIC glfw OpenGL::GL)
    target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD)
    message(STATUS "ImGui will be built and linked")
else()
    message(WARNING "ImGui not found. Checked: ${IMGUI_SEARCH_PATHS}")
    message(WARNING "Download ImGui: git clone https://github.com/ocornut/imgui.git include/imgui")
    message(WARNING "Editor will build but UI will not be available.")
    
    # Create a dummy imgui target so the build doesn't fail
    add_library(imgui INTERFACE)
endif()

# Main executable sources
set(PLACID_SOURCES
    src/main.cpp
    src/Renderer.cpp
    src/EditorApp.cpp
    src/GameMode.cpp
)

# Main executable
add_executable(placid ${PLACID_SOURCES})

# Make sure the executable can find your local headers
target_include_directories(placid PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Link everything together
target_link_libraries(placid PRIVATE 
    glad 
    glfw 
    OpenGL::GL
    imgui
)

# Nice messages
message(STATUS "CMake generator: ${CMAKE_GENERATOR}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
