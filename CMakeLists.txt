cmake_minimum_required(VERSION 3.24)

project(Vulkan LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Vulkan
find_package(Vulkan 1.3 REQUIRED)

find_program(
  GLSLC_EXECUTABLE
  NAMES glslc
  HINTS "$ENV{VULKAN_SDK}/Bin" "$ENV{VULKAN_SDK}/bin" REQUIRED)

# GLFW
include(FetchContent)

set(GLFW_BUILD_DOCS
    OFF
    CACHE BOOL "" FORCE)

set(GLFW_BUILD_TESTS
    OFF
    CACHE BOOL "" FORCE)

set(GLFW_BUILD_EXAMPLES
    OFF
    CACHE BOOL "" FORCE)

set(GLFW_INSTALL
    OFF
    CACHE BOOL "" FORCE)

FetchContent_Declare(
  glfw URL https://github.com/glfw/glfw/archive/refs/tags/3.4.tar.gz
           DOWNLOAD_EXTRACT_TIMESTAMP TRUE)

FetchContent_MakeAvailable(glfw)

# Shaders
set(shader_dir "${CMAKE_CURRENT_BINARY_DIR}/shaders")
file(MAKE_DIRECTORY "${shader_dir}")

file(
  GLOB
  shaders
  CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.vert.glsl"
  "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.frag.glsl"
  "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.comp.glsl")

foreach(source IN LISTS shaders)
  get_filename_component(filename "${source}" NAME)
  string(REGEX REPLACE "\\.glsl$" "" shader "${filename}")
  get_filename_component(stage "${shader}" LAST_EXT)
  string(SUBSTRING "${stage}" 1 -1 stage)

  set(output "${shader_dir}/${shader}.spv")

  add_custom_command(
    OUTPUT "${output}"
    COMMAND "${GLSLC_EXECUTABLE}" -fshader-stage=${stage} --target-env=vulkan1.3
            -Werror -MD -MF "${output}.d" -c "${source}" -o "${output}"
    DEPENDS "${source}"
    DEPFILE "${output}.d"
    VERBATIM)

  list(APPEND shader_outputs "${output}")
endforeach()

add_custom_target(shaders DEPENDS ${shader_outputs})

# Engine
add_library(
  engine STATIC
  engine/engine.cpp
  engine/hot_reload.cpp
  engine/vulkan/caches.cpp
  engine/vulkan/vulkan.cpp
  engine/vulkan/instance.cpp
  engine/vulkan/devices.cpp
  engine/vulkan/queues.cpp
  engine/vulkan/swapchain.cpp
  engine/vulkan/images.cpp
  engine/vulkan/commands.cpp
  engine/vulkan/synchronization.cpp
  engine/vulkan/timestamps.cpp
  engine/vulkan/shaders.cpp
  engine/vulkan/pipelines.cpp)

target_include_directories(engine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(engine PUBLIC cxx_std_20)
target_link_libraries(
  engine
  PUBLIC glfw
  PRIVATE Vulkan::Vulkan)
target_compile_definitions(engine PUBLIC GLFW_INCLUDE_NONE)

target_compile_options(engine PRIVATE -Wall -Wextra -Wpedantic
                                      -Wno-missing-field-initializers)

# Game
add_executable(game main.cpp scripts/controller.cpp)
target_link_libraries(game PRIVATE engine)
target_compile_options(game PRIVATE -Wall -Wextra -Wpedantic)
target_compile_definitions(
  game PRIVATE GAME_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
               GAME_GLSLC="${GLSLC_EXECUTABLE}")
add_dependencies(game shaders)

install(TARGETS game RUNTIME DESTINATION bin)
install(FILES ${shader_outputs} DESTINATION bin/shaders)
