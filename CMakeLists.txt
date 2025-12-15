cmake_minimum_required(VERSION 3.10)
project(Placid LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# Put runtime binaries under the build directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


# Prefer out-of-source builds; this file assumes you'll use the provided run script


# Make sure the project can find your local headers
include_directories(${CMAKE_SOURCE_DIR}/include)


# Build glad as a small static library from the provided C source
add_library(glad STATIC src/gl.c)
target_include_directories(glad PUBLIC ${CMAKE_SOURCE_DIR}/include)


# Find system OpenGL
find_package(OpenGL REQUIRED)


# Find GLFW (common CMake config target). If your system doesn't provide the
# glfw3 CMake package, install the development package (e.g. `glfw` or `glfw3`).
find_package(glfw3 3.3 REQUIRED)


# Main executable
add_executable(placid src/main.cpp)


# Make sure the executable can find your local headers (glad/HERO/KHR)
target_include_directories(placid PRIVATE ${CMAKE_SOURCE_DIR}/include)


# Link everything together
# - glad (local static lib)
# - glfw (system)
# - OpenGL::GL (from find_package(OpenGL))
target_link_libraries(placid PRIVATE glad glfw OpenGL::GL)


# Nice messages
message(STATUS "CMake generator: ${CMAKE_GENERATOR}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
