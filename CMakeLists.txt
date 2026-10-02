cmake_minimum_required(VERSION 3.24)
project(
  VulkanStarter
  VERSION 0.1
  LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT WIN32 AND NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
  message(FATAL_ERROR "This starter targets Windows and Linux")
endif()
find_package(Vulkan 1.3 REQUIRED)
find_program(
  GLSLC_EXECUTABLE
  NAMES glslc
  HINTS "$ENV{VULKAN_SDK}/Bin" "$ENV{VULKAN_SDK}/bin" REQUIRED)
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
set(generated "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${generated}")
set(shader_dir "${CMAKE_CURRENT_BINARY_DIR}/compiled-shaders")
file(MAKE_DIRECTORY "${shader_dir}")
foreach(shader fullscreen.vert raymarch.frag)
  add_custom_command(
    OUTPUT "${shader_dir}/${shader}.spv"
    COMMAND
      "${GLSLC_EXECUTABLE}" -std=glsl-4.60 -mfmt=generic -c -o
      "${shader_dir}/${shader}.spv"
      "${CMAKE_CURRENT_SOURCE_DIR}/shaders/${shader}.glsl"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/${shader}.glsl"
    VERBATIM)
  list(APPEND shader_outputs "${shader_dir}/${shader}.spv")
endforeach()
add_custom_target(shaders DEPENDS ${shader_outputs})
add_executable(
  vulkan_starter
  src/main.cpp
  src/input.cpp
  src/camera.cpp
  src/platform.cpp
  src/vulkan/vulkan.cpp
  src/vulkan/instance.cpp
  src/vulkan/devices.cpp
  src/vulkan/queues.cpp
  src/vulkan/swapchain.cpp
  src/vulkan/buffers.cpp
  src/vulkan/images.cpp
  src/vulkan/commands.cpp
  src/vulkan/synchronization.cpp
  src/vulkan/timestamps.cpp
  src/vulkan/shaders.cpp
  src/vulkan/pipelines.cpp)
target_compile_definitions(vulkan_starter PRIVATE GLFW_INCLUDE_NONE NOMINMAX)
if(MSVC)
  target_compile_options(vulkan_starter PRIVATE /W4 /permissive-)
else()
  target_compile_options(vulkan_starter PRIVATE -Wall -Wextra -Wpedantic
                                                -Wno-missing-field-initializers)
endif()
# An ALL target also copies shader-only rebuilds; POST_BUILD alone would miss
# those.
add_custom_target(
  runtime_shaders ALL
  COMMAND "${CMAKE_COMMAND}" -E make_directory
          "$<TARGET_FILE_DIR:vulkan_starter>/shaders"
  COMMAND
    "${CMAKE_COMMAND}" -E copy_if_different "${shader_dir}/raymarch.frag.spv"
    "$<TARGET_FILE_DIR:vulkan_starter>/shaders"
  DEPENDS shaders vulkan_starter
  VERBATIM)
install(TARGETS vulkan_starter RUNTIME DESTINATION bin)
install(FILES "${shader_dir}/raymarch.frag.spv" DESTINATION bin/shaders)
