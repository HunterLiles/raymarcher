# Vulkan raymarching starter

A C++20, data-oriented Vulkan shell for writing your own GLSL SDF raymarcher.
The current shader generates camera rays and displays their directions as RGB.
It deliberately leaves the distance function and marching loop for you to implement.

The only third-party application library is **GLFW 3.4**, plus Vulkan itself.
There is no GLM, Volk, VMA, shaderc, glslang, or heavy GUI framework.
All application source is C++; the C Vulkan API is used directly. All shaders
are GLSL compiled by **glslc**.

## Build

Requirements:

- A C++20 compiler (GCC, Clang, or Visual Studio 2022).
- CMake 3.24 or newer.
- Vulkan development headers and loader, and a Vulkan 1.3 driver supporting dynamic rendering.
- glslc (included with the Vulkan SDK or as part of the glslang package).
- Internet access on the first configure to fetch the pinned GLFW source.
- Linux: GLFW's platform development packages. For both backends these include X11,
  Xrandr, Xinerama, Xcursor, Xi, Wayland, wayland-scanner, wayland-protocols, and xkbcommon.
  See [GLFW's build instructions](https://www.glfw.org/docs/3.4/compile.html).

Vulkan validation layers are optional. Debug and Release builds request validation by
default; if the layer is missing, a message is printed and execution continues.
Use `--no-validation` for timing without validation overhead.

### Linux

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
./build/vulkan_starter
```

If `glslc` is not on PATH, specify it via `-DGLSLC_EXECUTABLE=/absolute/path/to/glslc`.
For X11-only builds, add `-DGLFW_BUILD_WAYLAND=OFF` to configure. For Wayland-only,
add `-DGLFW_BUILD_X11=OFF`. GLFW's defaults enable both on Linux.

### Windows

Install Visual Studio 2022 with Desktop development with C++, the Vulkan SDK,
and glslc. Run in a developer terminal:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug --parallel
.\build\Debug\vulkan_starter.exe
```

Adjust the glslc path to your installation if needed. `VULKAN_SDK/Bin` is also searched.
Shader binaries are embedded at compile time. Paths are resolved
from the executable location, so launching from another working directory works.

Edit GLSL, rebuild with the ordinary `cmake --build` command, and restart the app.
The default build updates shader files even when no C++ relink is needed.
There is no runtime shader compiler or hot reload in this starting version.

## Your raymarcher starts here

Open **`shaders/raymarch.frag.glsl`**. The fragment entry point already provides:

- `ray_origin`: camera position in world space.
- `ray_direction`: normalized world-space direction for this pixel.
- `frame.origin_time.w`: elapsed active-loop seconds.
- `frame.viewport.xy`: framebuffer size, including high-DPI scaling.

Replace the final ray-direction color with your tracing result. Camera translation
updates the origin; this initial direction-only preview changes visually with rotation,
not translation. That is expected until you trace something located in world space.

The full-screen triangle only schedules the pixel shader across the window. It is not
scene geometry. There are no scene vertex/index buffers, projected objects, depth
attachments, or depth passes. Vulkan's graphics pipeline is simply the host for your
per-pixel ray code.

The starter does not choose an SDF representation, marching algorithm, spatial tree,
scene ownership scheme, or compute-dispatch architecture for you.

## Module boundaries

| File/module | Responsibility |
| --- | --- |
| `src/main.cpp` | GLFW initialization, surface connection, loop, subsystem wiring, lifetime order |
| `src/input.*` | GLFW input snapshot; no renderer dependency |
| `src/camera.*` | Camera update and render constants; no GLFW/Vulkan calls |
| `src/render_data.hpp` | Plain 80-byte camera data shared with the shader |
| `src/math.hpp` | Custom vectors and matrices; no external math library |
| `src/stats.hpp` | Plain diagnostics snapshot |
| `src/platform.*` | Executable-directory lookup on Linux/Windows |
| `src/vulkan/vulkan.*` | Master Vulkan module; opaque state and frame orchestration |
| `src/vulkan/instance.*` | Instance and validation messenger |
| `src/vulkan/devices.*` | Physical/logical device and memory-type selection |
| `src/vulkan/queues.*` | Graphics/presentation queue-family discovery |
| `src/vulkan/swapchain.*` | Swapchain images, views, and per-image presentation semaphores |
| `src/vulkan/buffers.*` | Buffer allocation, host writes/flushes, cleanup for future SDF data |
| `src/vulkan/images.*` | Image-layout transitions and dependencies |
| `src/vulkan/commands.*` | Command pools, buffers, and recording startup |
| `src/vulkan/synchronization.*` | Per-frame acquire semaphore and completion fence |
| `src/vulkan/timestamps.*` | GPU query allocation/readback, valid-bit wrapping |
| `src/vulkan/shaders.*` | SPIR-V file loading and shader modules |
| `src/vulkan/pipelines.*` | Full-screen pipeline and push-constant layout |
| `shaders/fullscreen.vert.glsl` | Vertex-ID-generated full-screen triangle |
| `shaders/raymarch.frag.glsl` | Camera rays; your future SDF raymarcher |
| `cmake/EmbedSpirv.cmake` | Embeds glslc output using CMake only |

State is held in plain structs and contiguous resource arrays, with free functions
operating on the data they need. There are no per-object rendering classes, singleton
engine, or inheritance hierarchy. `main.cpp` holds small subsystem state/handles to
coordinate their lifetimes; it does not allocate Vulkan resources or implement input,
camera, or rendering behavior.

`vulkan_create()` creates the instance; `main.cpp` connects GLFW using
`glfwCreateWindowSurface()`. `vulkan_initialize()` takes ownership of that surface.
Shutdown waits for the device, then destroys Vulkan resources, the window, and GLFW.

The buffer module is available for your future SDF/storage data and is unused by the
ray preview. Its caller owns each `Buffer`, must destroy it, and must synchronize writes
against GPU readers. `buffer_write()` requires host-visible memory and flushes memory
when it is not host-coherent. Device-local-only data needs a staging-copy path, which is
not implemented here. Descriptor sets for scene buffers are also left for your scene design.

## Camera and math

WASD moves, Q/E moves down/up, Shift increases speed, right mouse drag looks,
and Escape exits.

The world is right-handed with +Y up. The initial camera faces roughly -Z. Rays are
constructed from the camera basis, so this path requires no projection-matrix inverse.
The custom math library also supplies multiplication, translation, Y rotation, look-at,
and perspective matrices for later use. Matrices store columns contiguously and multiply
column vectors. Perspective uses Vulkan depth [0, 1] and flips Y for a positive-height viewport.

## Diagnostics

Performance stats (FPS, frame time, CPU work) are output to the console via a single-line
carriage-return update.

## Vulkan details and current limits

- Vulkan 1.3 dynamic rendering; no render-pass/framebuffer objects.
- Two frame slots; command pools/fences/acquire semaphores and queries are frame-local.
- Presentation semaphores are indexed by acquired **swapchain image**, not frame slot.
- Resize/out-of-date handling rebuilds the swapchain and pipeline.
  Minimized windows wait for events instead of rendering with a zero-size framebuffer.
- `vkDeviceWaitIdle()` is used for resize/shutdown. This is the conventional portable
  starter approach. Explicit presentation-completion guarantees would require an
  extension such as swapchain maintenance1; this project does not enable it.
- No MSAA, compute path, scene descriptors, resource streaming, or runtime shader reload.

## Verification performed

Built with GCC 13.3, CMake 4.4.3, Vulkan headers/loader 1.3.275, and glslc.
Both GLFW X11 and Wayland backends compiled. All GLSL shaders compiled with warnings
treated as errors.

Ran the application on Linux/X11 in a virtual display with Mesa llvmpipe, including
300 frames and two window resizes, with validation and synchronization validation
requested. No validation messages were emitted; GPU timestamp readback worked and the
camera-ray preview was visually inspected. These are software-device
functional checks, not representative hardware performance numbers.

Windows/MSVC compilation and native Windows/Wayland execution were not tested here.

For a finite local smoke test:

```sh
./build/vulkan_starter --frames 300 --resize-test
```

See [SOURCES.md](SOURCES.md) for the primary API references used.
