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

# Compiling
add_executable(
  vulkan
  src/main.cpp
  src/vulkan/vulkan.cpp
  src/vulkan/instance.cpp
  src/vulkan/devices.cpp
  src/vulkan/queues.cpp
  src/vulkan/swapchain.cpp
  src/vulkan/images.cpp
  src/vulkan/commands.cpp
  src/vulkan/synchronization.cpp
  src/vulkan/timestamps.cpp
  src/vulkan/shaders.cpp
  src/vulkan/pipelines.cpp)

target_link_libraries(vulkan PRIVATE glfw Vulkan::Vulkan)
target_compile_definitions(vulkan PRIVATE GLFW_INCLUDE_NONE)

target_compile_options(vulkan PRIVATE -Wall -Wextra -Wpedantic)

add_dependencies(vulkan shaders)

install(TARGETS vulkan RUNTIME DESTINATION bin)
install(FILES ${shader_outputs} DESTINATION bin/shaders)
