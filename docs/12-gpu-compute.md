# GPU compute backend (`GPU_COMPUTING_COMPATIBILITY`)

`Makefile` (`GPU=1`), `include/sdl_wrapper.hpp`, `include/core/SDLUtils.hpp` /
`srcs/core/SDLUtils.cpp`, `shaders/raytrace.msl`, plus an
`#ifdef GPU_COMPUTING_COMPATIBILITY` branch in every class that touches
windowing or rendering: `App`, `Keys.hpp`, `FrameBuffer`, `Renderer`, and the
GPU-side data layout added to `Vec3`, `Mat3`, `Transform`, `Material`,
`Light`, `Camera`, `Object` and each primitive.

This is a second, complete backend, selected at **compile time**, not a
runtime option. The default `make` build is completely untouched by any of
this — same MiniLibX binary, same object files, same link line as before
this backend existed. Everything below only exists when the project is built
with `make GPU=1`.

## Why this exists, and what it's allowed to do

The subject (`en.subject.pdf`, Chapter IV "General Instructions") draws a
specific line:

> you can use all functions of the MiniLibX or their equivalent in another
> graphic library (SDL, XCB, ...): open a window, lit a pixel, put an image
> on a window, and manage events. **The final image cannot be created by the
> GPU: rendering via a GPU pipeline (vertex/fragment/geometry shaders using
> OpenGL, Metal, Vulkan, DirectX ...) is forbidden.**
>
> Note: GPU computing (OpenCL, CUDA, or compute shaders with OpenGL, Vulkan,
> Metal, ..) may be used in order to increase performances.

Two separate permissions, easy to conflate:

1. **Windowing library swap** — MiniLibX vs. SDL vs. XCB are interchangeable
   for opening a window, blitting a finished image, and reading input
   events. Freely allowed either way.
2. **GPU acceleration of the ray-tracing computation itself** — allowed, but
   only through a **compute** pipeline (a kernel that reads/writes buffers),
   never a **graphics/rasterization** pipeline (vertex/fragment/geometry
   shaders that *draw* the final image). The distinguishing question the
   subject cares about is "did a rasterizer produce the pixels you see," not
   "did the GPU touch any data."

This backend does both, and keeps them architecturally separate on purpose:

- Windowing/pixel-blit (`App`, `Keys.hpp`, `FrameBuffer`'s `SDL_Texture` +
  `SDL_RenderTexture`) uses SDL3's ordinary 2D renderer API. This is
  permission (1) — a straight MiniLibX-for-SDL swap, nothing about it is
  GPU-compute-specific, and it works identically whether or not the compute
  path below is even reachable.
- The actual ray/primitive intersection and shading math is ported to an
  SDL3 **GPU compute** kernel (`shaders/raytrace.msl`, dispatched via
  `SDL_DispatchGPUCompute`). This is permission (2). Its output is a plain
  storage buffer of packed pixels — never a texture bound as a render
  target, never touched by a vertex or fragment shader. `FrameBuffer` reads
  that buffer back and blits it exactly the same way it would blit a
  CPU-computed frame. From the subject's perspective, "the GPU produced some
  numbers that got copied into a pixel buffer" is indistinguishable from
  "the CPU produced those numbers" — no rasterization pipeline is ever in
  the path that puts a pixel on screen.

The CPU path (`make`, no flag) remains the default and the one MiniLibX/the
42 grading environment expects; this backend is additive, not a replacement.

## Backend selection: one macro, one Makefile flag

```make
GPU ?= 0
...
ifeq ($(GPU),1)
    BACKEND_LIB     = $(SDL_LIB)
    BACKEND_LDFLAGS = $(SDL_LDFLAGS)
    BACKEND_INC     = $(SDL_SRC_DIR)/include
    CXXFLAGS       += -DGPU_COMPUTING_COMPATIBILITY
else
    BACKEND_LIB     = $(MLX_LIB)
    BACKEND_LDFLAGS = $(MLX_LDFLAGS)
    BACKEND_INC     = $(MLX_DIR)
endif
```

`make GPU=1` is the only thing that defines `GPU_COMPUTING_COMPATIBILITY`.
Every `#ifdef GPU_COMPUTING_COMPATIBILITY` block in the codebase is dead code
— never even parsed by the preprocessor — unless that flag is passed, so
there is zero risk of the GPU path leaking into a default `make` build
through some forgotten branch.

**Naming note:** the originally-requested macro name was
`3D_GPU_COMPUTING_COMPATIBILITY`. That is not a legal C/C++ preprocessor
identifier — identifiers (and therefore macro names) cannot start with a
digit; `-D3D_GPU_COMPUTING_COMPATIBILITY` fails to compile
(`error: macro name must be an identifier`). `GPU_COMPUTING_COMPATIBILITY`
was chosen as the closest legal equivalent once this was caught during
implementation.

## SDL3 is fetched and built by the Makefile, not a system dependency

Exactly like MiniLibX's `.tgz` files already committed to this repo, SDL3 is
**not** assumed to be installed via a package manager (`brew`/`apt`) or
found via `pkg-config`. `make GPU=1` fetches SDL3's own pinned release
source tarball and builds a static library from it, the same way `make`
(any mode) already extracts and builds MiniLibX from the committed archives:

```make
SDL_VERSION   = 3.4.16
SDL_ARCHIVE   = SDL3-$(SDL_VERSION).tar.gz
SDL_URL       = https://github.com/libsdl-org/SDL/releases/download/release-$(SDL_VERSION)/$(SDL_ARCHIVE)
SDL_SRC_DIR   = SDL3-$(SDL_VERSION)
SDL_BUILD_DIR = $(SDL_SRC_DIR)/build
SDL_LIB       = $(SDL_BUILD_DIR)/libSDL3.a

$(SDL_LIB): | $(SDL_SRC_DIR)
	cmake -S $(SDL_SRC_DIR) -B $(SDL_BUILD_DIR) -DCMAKE_BUILD_TYPE=Release \
		-DSDL_STATIC=ON -DSDL_SHARED=OFF -DSDL_TEST_LIBRARY=OFF
	cmake --build $(SDL_BUILD_DIR) --parallel

$(SDL_SRC_DIR):
	curl -fL -o $(SDL_ARCHIVE) $(SDL_URL)
	tar -xzf $(SDL_ARCHIVE)
```

Differences from the MiniLibX pattern, and why:

- SDL3 doesn't ship a tarball in this repo (unlike the MLX ones, which are
  small and 42-provided) — it's `curl`ed from SDL's own GitHub releases the
  first time `make GPU=1` runs, then cached on disk (`SDL3-3.4.16/`,
  gitignored) exactly like the extracted MLX directories are. A clean
  checkout with `GPU=0` (the default) never touches the network at all;
  only `GPU=1` triggers the fetch, and only once.
- MiniLibX ships its own hand-rolled build system (`./configure && make` on
  Linux, plain `make` on macOS); SDL3 ships a CMake build, so the build
  rule uses `cmake` instead. This is the one extra host tool `GPU=1` needs
  beyond what `MODE=debug`/`MODE=release` already require — `curl` and
  `cmake`, both ordinary developer-machine tools, not anything exotic.
- `-DSDL_STATIC=ON -DSDL_SHARED=OFF` mirrors linking `libmlx.a` statically:
  the built `rt` binary shouldn't depend on an SDL3 shared library being
  installed system-wide at runtime, for the same "just build and run it"
  reason MiniLibX is linked statically.

`SDL_VERSION`/`SDL_ARCHIVE`/`SDL_SRC_DIR`/`SDL_LIB` are defined
**unconditionally** in the Makefile (outside the `ifeq ($(GPU),1)` block),
specifically so `make fclean` can remove a previously-fetched SDL3 tree
regardless of which mode the *next* invocation runs in — `fclean` shouldn't
need `GPU=1` tacked on to fully clean up after a `GPU=1` build that happened
earlier.

### Linking SDL3's static library — the framework list

On macOS, SDL3's static build pulls in considerably more system frameworks
than MiniLibX does, because SDL3 is a much larger library (windowing, GPU,
audio, haptics, game controllers, camera capture, etc. all compiled in):

```make
SDL_SYS_LDFLAGS = -framework Cocoa -framework Metal -framework QuartzCore \
		  -framework CoreVideo -framework CoreAudio -framework AudioToolbox \
		  -framework ForceFeedback -framework GameController -framework CoreHaptics \
		  -framework IOKit -framework Carbon -framework UniformTypeIdentifiers \
		  -framework CoreMedia -framework AVFoundation
```

The first pass at this list (everything except the last line) linked fine
right up until `SDL_camera_coremedia.m.o`'s symbols — SDL3's camera-capture
backend, compiled in even though this project never opens a camera —
produced `Undefined symbols ... AVCaptureDevice...`/`CMSampleBuffer...` at
link time. `-framework CoreMedia -framework AVFoundation` resolved it. This
is the practical cost of linking a "give me everything" static SDL3 build:
you link against every subsystem's frameworks whether you use that
subsystem or not. (A leaner option — `-DSDL_CAMERA=OFF` etc. at the CMake
step — would trim this, but wasn't pursued since the current list is a
one-time, already-solved cost.)

Order matters here exactly like it does for `libmlx.a`
(see [09-build-system.md](09-build-system.md)): `$(SDL_LIB)` must appear on
the link line **before** these frameworks, so the linker still has SDL3's
undefined symbols in its work list when it reaches the frameworks that
resolve them.

## `include/sdl_wrapper.hpp`

The SDL3 equivalent of `include/mlx_wrapper.hpp`, but structurally simpler:
unlike `mlx.h`, SDL3's own headers already have proper `extern "C"` guards
for C++, so no linkage wrapping is needed. Its only job is to make the
`GPU_COMPUTING_COMPATIBILITY` gate a single, consistent include point:

```cpp
#ifdef GPU_COMPUTING_COMPATIBILITY
	#include <SDL3/SDL.h>
#endif
```

Every class with a GPU branch includes `sdl_wrapper.hpp` instead of
`<SDL3/SDL.h>` directly, so `<SDL3/SDL.h>` is never even reachable from a
default `make` build's preprocessor — there's exactly one place that could
possibly pull in SDL3 headers, and it's inert unless `GPU=1`.

## Windowing & events: `App`, `Keys.hpp`

Full detail on the shared (non-GPU-specific) `App` design — lifecycle,
keybindings, the redraw-vs-rerender split, why `Esc` exits the way it does —
lives in [08-app-controls.md](08-app-controls.md); this section only covers
what's different under `GPU_COMPUTING_COMPATIBILITY`.

### Keys.hpp

```cpp
#ifdef GPU_COMPUTING_COMPATIBILITY
	# define KEY_ESC	SDL_SCANCODE_ESCAPE
	# define KEY_W		SDL_SCANCODE_W
	...
#else
	// MLX's raw platform keycodes, split #if defined(__APPLE__) / X11
#endif
```

SDL3 reports **scancodes** — physical key position, already normalized
across platforms by SDL itself — so unlike the MLX branch there's no
`#ifdef __APPLE__` split needed here at all: one set of `SDL_SCANCODE_*`
constants works on every OS SDL3 supports.

### App — construction, event loop, teardown

MLX's model is callback-hook-based (`mlx_expose_hook`, `mlx_hook` for
key events, `mlx_loop_hook` for the idle tick, and `mlx_loop` never
returning control to the caller). SDL3's model is a plain poll loop, so
`App::run()` looks structurally different even though it drives the exact
same `handleKeyDown`/`handleKeyUp`/`update()`/`redraw()` private methods
`08-app-controls.md` describes:

```cpp
void App::run()
{
	bool running = true;
	SDL_Event event;
	while (running)
	{
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_EVENT_QUIT)
				running = false;
			else if (event.type == SDL_EVENT_KEY_DOWN)
				handleKeyDown((int)event.key.scancode);
			else if (event.type == SDL_EVENT_KEY_UP)
				handleKeyUp((int)event.key.scancode);
		}
		if (!running)
			break;
		update();
		redraw();
	}
}
```

Because there's no separate "expose" concept driving a callback the way MLX
has one, `run()` simply calls `update()` (which internally calls
`rerender()` only if a held key actually moved/turned the camera — see
`08-app-controls.md`'s "why `update()` only calls `rerender()` when
something changed") and `redraw()` (a cheap texture re-present, see
`FrameBuffer` below) every loop iteration. The MLX build's
"redraw-without-recomputing" requirement is satisfied by the expose hook
specifically because MLX ties `redraw()` to a real OS repaint event; the
SDL build satisfies the same requirement structurally instead — `redraw()`
never re-traces, full stop, regardless of what triggers it, so presenting
it every tick is exactly as cheap as presenting it only-on-expose would be.

Construction swaps `mlx_init()`/`mlx_new_window()` for
`SDL_Init(SDL_INIT_VIDEO)` / `SDL_CreateWindow(title, width, height, 0)` /
`SDL_CreateRenderer(win, nullptr)` (the last with a `nullptr` driver name,
i.e. "let SDL pick the best available 2D renderer backend" — Metal on
macOS, typically Vulkan or OpenGL on Linux). It also creates the
`SDL_GPUDevice` used by the compute path (`SDL_UTILS::createDevice`, see
below) up front, once, for the lifetime of the `App`.

The MLX-only static trampolines (`onExpose`/`onKeyDown`/`onKeyUp`/
`onLoopTick`, needed because `mlx_*_hook` only accepts bare function
pointers) are `#ifndef GPU_COMPUTING_COMPATIBILITY`-guarded out of the SDL
build entirely — SDL's poll loop calls the instance methods directly, so
those trampolines would just be permanently-unused code under `GPU=1`.

`Esc` tears down via `SDL_DestroyRenderer` / `SDL_DestroyWindow` /
`SDL_Quit()` then `std::exit(0)`, the SDL-idiomatic equivalent of
`08-app-controls.md`'s "why `Esc` calls `mlx_destroy_window` then
`std::exit(0)` directly" reasoning — SDL's poll loop, like `mlx_loop`,
has no natural "unwind and return" exit path either.

## `FrameBuffer` — SDL streaming texture

```cpp
class FrameBuffer   // GPU_COMPUTING_COMPATIBILITY branch
{
	SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XRGB8888,
		SDL_TEXTUREACCESS_STREAMING, width, height);
	std::vector<uint32_t> _pixels;   // CPU-side scratch, same 0x00RRGGBB packing as the MLX branch

	void setPixel(x, y, Vec3 color)   { _pixels[y*width+x] = packed; }         // per-pixel, CPU path only
	void setPixelsRaw(const uint32_t *packed) { memcpy(_pixels.data(), ...); } // bulk, GPU path
	void present() { SDL_UpdateTexture(_texture, nullptr, _pixels.data(), ...); }
	const uint32_t *rawPixels() const { return _pixels.data(); }
};
```

The MLX branch's `setPixel` writes straight into MLX's own backing memory —
there's no separate "upload" step, the image *is* the buffer. SDL's
`SDL_TEXTUREACCESS_STREAMING` texture isn't directly writable like that
(driver-managed GPU memory, not a plain `char*`), so this branch keeps a
CPU-side `std::vector<uint32_t>` as the buffer `setPixel` (CPU render path)
or `setPixelsRaw` (GPU render path, see `Renderer` below — the compute
shader already produces packed `0x00RRGGBB` pixels, so this is a straight
`memcpy` instead of `setPixel`'s per-pixel float-to-byte conversion) writes
into, and a single `present()` call per re-trace pushes the whole buffer to
the texture in one `SDL_UpdateTexture`. `App::rerender()` calls `present()`
once, right after rendering and before the first `redraw()` — see
[07-rendering.md](07-rendering.md) for why that split (one upload per
re-trace, arbitrarily many free re-presents) is what makes the
"redraw-without-recomputing" requirement hold here too.

`redraw()` itself becomes `SDL_RenderClear` + `SDL_RenderTexture` (a plain
texture blit onto the window's own renderer) + `SDL_RenderPresent` — no
shader, no draw call touching vertex/fragment stages, just SDL2-API-style
"copy this texture onto the screen," which is what keeps this firmly on the
"lit a pixel, put an image on a window" side of the subject's rule quoted
above.

`rawPixels()` is read-only access to that same buffer, added for
tooling/verification rather than any rendering need — it's what the
CPU-vs-GPU pixel-diff described later in this document used to compare a
GPU-rendered frame against the CPU reference without needing a screenshot.

## `SDL_UTILS` — the GPU compute plumbing namespace

`include/core/SDLUtils.hpp` / `srcs/core/SDLUtils.cpp`, entirely
`#ifdef GPU_COMPUTING_COMPATIBILITY`-guarded (an empty translation unit
under the default build). This is the "extract SDL functions, e.g. for
compiling shaders, into their own namespace, compiled only under the
define" piece: every other class that needs SDL's GPU API goes through
these functions and never calls a raw `SDL_*GPU*` entry point itself.

```cpp
namespace SDL_UTILS
{
	SDL_GPUDevice           *createDevice(SDL_Window *window);
	void                      destroyDevice(SDL_GPUDevice *device, SDL_Window *window);
	SDL_GPUShaderFormat       selectShaderFormat(SDL_GPUDevice *device);
	bool                      loadShaderBytecode(const char *basePath, SDL_GPUShaderFormat format,
	                              std::vector<uint8_t> &out);
	SDL_GPUComputePipeline   *createComputePipeline(SDL_GPUDevice *device, const char *basePath,
	                              const char *entrypoint, SDL_GPUComputePipelineCreateInfo info);
	SDL_GPUBuffer            *createStorageBuffer(SDL_GPUDevice *device, uint32_t size, bool writable);
	bool                      uploadToBuffer(SDL_GPUDevice *device, SDL_GPUBuffer *buffer,
	                              const void *data, uint32_t size);
	void                      releaseBuffer(SDL_GPUDevice *device, SDL_GPUBuffer *buffer);
}
```

### A correction made while reading SDL's actual headers

The original design sketch (before any of Phase 3's code was written)
assumed a `loadComputeShaderBytecode` function returning an `SDL_GPUShader*`
— i.e. "compile the shader, then separately build a pipeline from it,"
mirroring how graphics (vertex/fragment) shaders work in SDL3's API.
Reading `SDL3-3.4.16/include/SDL3/SDL_gpu.h` directly (rather than going
from memory/assumption) showed this doesn't apply to compute: `SDL_GPUShader`
objects only exist for `SDL_GPU_SHADERSTAGE_VERTEX`/`FRAGMENT` — a graphics
pipeline shader stage. A **compute** pipeline takes raw bytecode straight
into `SDL_CreateGPUComputePipeline(device, &SDL_GPUComputePipelineCreateInfo{code, code_size, entrypoint, format, ...})`
in one step; there's no intermediate "compute shader object." So
`SDL_UTILS` has no `SDL_GPUShader`-returning function at all — only
`createComputePipeline`, which goes straight from a bytecode file to a
usable pipeline. This also happens to reinforce the compute-vs-graphics
separation the subject cares about: this codebase never touches
`SDL_GPUShader`/`SDL_GPUShaderStage` at all, because it never builds a
graphics pipeline.

### `createDevice` / `destroyDevice`

```cpp
SDL_GPUDevice *createDevice(SDL_Window *window)
{
	SDL_GPUShaderFormat formats = SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL
		| SDL_GPU_SHADERFORMAT_METALLIB | SDL_GPU_SHADERFORMAT_DXIL;
	SDL_GPUDevice *device = SDL_CreateGPUDevice(formats, false, nullptr);
	...
	SDL_ClaimWindowForGPUDevice(device, window);
	...
}
```

Offers every shader format SDL3 knows about up front (there's no cost to
claiming support for formats this project doesn't ship bytecode for — see
`createComputePipeline`'s format-fallback logic below) and lets SDL pick the
platform's actual default backend (`nullptr` driver name) — Metal on macOS,
typically Vulkan on Linux. `App` calls this once in its constructor and logs
the result:

```
rt: GPU device ready (driver=metal, shader formats=0x30)
```

`0x30` = `SDL_GPU_SHADERFORMAT_MSL (1<<4) | SDL_GPU_SHADERFORMAT_METALLIB (1<<5)`
— confirmation, straight from the real driver on the machine this was
developed on, that Metal only accepts MSL/metallib, never SPIR-V/DXIL
(those bits are absent from what it reports supporting).

### `createComputePipeline` — format preference with fallback

The device may claim to support more shader formats than this project
actually ships bytecode for. `createComputePipeline` walks a fixed
preference order and uses the first format that both the device supports
**and** has a matching bytecode file on disk, instead of trusting
`selectShaderFormat`'s device-only pick and failing outright if that
format's file doesn't exist:

```cpp
static const SDL_GPUShaderFormat kPreferenceOrder[] = {
	SDL_GPU_SHADERFORMAT_SPIRV, SDL_GPU_SHADERFORMAT_METALLIB,
	SDL_GPU_SHADERFORMAT_MSL,   SDL_GPU_SHADERFORMAT_DXIL,
};
for (SDL_GPUShaderFormat candidate : kPreferenceOrder)
{
	if (!(supported & candidate))
		continue;
	if (loadShaderBytecode(basePath, candidate, bytecode))
	{
		format = candidate;
		break;
	}
}
```

This mattered immediately in practice: this project ships
`shaders/raytrace.msl` (source text) but not `shaders/raytrace.metallib`
(precompiled bytecode — see the shader section below for why), and Metal's
driver reports supporting *both* formats. Without the fallback, picking
METALLIB first (a reasonable "prefer precompiled over source" default) would
make every run fail with "file not found." With it, the real observed
startup log is:

```
rt: could not open shader bytecode 'shaders/raytrace.metallib'
rt: GPU compute pipeline ready
```

— tried METALLIB, missed, fell through to MSL, succeeded. Exactly the
intended behavior, not a silent bug.

### `loadShaderBytecode`

Maps a format to a file extension (`.spv`/`.metallib`/`.msl`/`.dxil`) and
reads `<basePath><ext>` off disk as raw bytes — for `.msl` this is just the
shader's UTF-8 source text read as a byte blob, which is exactly what
`SDL_GPUShaderCreateInfo`/`SDL_GPUComputePipelineCreateInfo`'s
`code`/`code_size` fields expect regardless of whether the format is
"compiled bytecode" or "source text"; SDL's own backend decides how to
interpret those bytes per-format (see the shader section below for what
Metal specifically does with `SDL_GPU_SHADERFORMAT_MSL`).

### `createStorageBuffer` / `uploadToBuffer` / `releaseBuffer`

`createStorageBuffer(device, size, writable)` wraps `SDL_CreateGPUBuffer`
with the right usage flag —
`SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE` for the shader's output,
`SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ` for its inputs (primitives,
lights). `uploadToBuffer` does the full transfer-buffer round trip SDL's GPU
API requires for any CPU→GPU copy in one call: create an
`SDL_GPUTransferBuffer` (`UPLOAD` usage), `SDL_MapGPUTransferBuffer` it,
`memcpy` the caller's data in, unmap, then a copy pass
(`SDL_BeginGPUCopyPass` / `SDL_UploadToGPUBuffer` / `SDL_EndGPUCopyPass`) on
a freshly-acquired command buffer, submitted synchronously
(`SDL_SubmitGPUCommandBuffer`). `Renderer`'s scene-buffer builder (below)
calls this once per primitive/light array upload; nothing else in this
codebase touches `SDL_GPUTransferBuffer` directly.

## GPU-side data layout (Phase 4/5): mirroring the CPU structs byte-for-byte

Every CPU struct the compute shader needs to read gets an `#ifdef`-guarded
GPU mirror plus a `toGPU()` method, added directly alongside the CPU type it
mirrors (`Mat3::GPUMat3`, `Transform::GPUTransform`, `Material::GPUMaterial`,
`Light::GPULight`, `Camera::GPUCamera`, `Object::GPUPrimitive`) rather than
collected in one file — keeping each mirror next to the struct it copies
keeps the "what does this field mean" question answerable by reading one
place, the same way `Object`'s own `hit()` is already colocated with the
shape it belongs to (see [04-primitives.md](04-primitives.md)).

### `Vec3` needs no wrapper at all

```cpp
struct alignas(16) Vec3 { float x, y, z, w; ... };   // include/math/Vec3.hpp
```

`Vec3` was already 16-byte aligned with a padded, always-zero 4th lane —
originally purely for the CPU SIMD auto-vectorization win described in
[02-math.md](02-math.md). That layout (four floats, 16-byte aligned) is
*also* exactly what GLSL/HLSL's std430 rules — and, separately, Metal's
own alignment rules — require for a `vec4`/`float4`. So `Vec3` is already
GPU-storage-buffer-ready as-is; every GPU mirror struct below uses `Vec3`
directly for anything that would otherwise be a "vec3 slot," instead of a
separate `GPUVec4` type. This is a case where a decision made for an
unrelated CPU-performance reason turned out to double as the right GPU
layout for free.

### Why the other structs *do* need mirrors, and exact sizes

| CPU struct | GPU mirror | Size | Why it can't just be reused as-is |
|---|---|---|---|
| `Mat3` (`float m[3][3]`, 36B, row-major, no padding) | `Mat3::GPUMat3` (3× `Vec3` rows) | 48B | Raw `float[3][3]` has no 16-byte row alignment; GLSL/HLSL/MSL all expect a 3×3 matrix's rows/columns individually 16-byte-aligned. |
| `Transform` (translation + rotation + rotationInv, private `Mat3` members) | `Transform::GPUTransform` | 112B | Bundles a `Vec3` translation with both `GPUMat3`s — the shader needs the already-computed inverse rotation too (see `to_local` below), since inverting a `mat3` cheaply inside the kernel isn't worth it when the CPU already has it. |
| `Material` (`Vec3 color` + 4 floats) | `Material::GPUMaterial` | 32B | Layout-compatible already in principle (`color`'s 16-byte alignment leaves the 4 trailing floats packed with no gap) — mirrored anyway for a name distinct from the CPU type and to keep the "every uploaded struct has an explicit, documented GPU shape" pattern consistent. |
| `Light` (`Vec3 position`, `Vec3 color`, `float brightness`, 36B unpadded) | `Light::GPULight` | 48B | Lights are uploaded as an **array**; std430 requires an array element's stride to be a multiple of the element's own 16-byte base alignment. 36 isn't a multiple of 16, so 12 bytes (`float _pad[3]`) of explicit tail padding round it up to 48. |
| `Camera` (4× `Vec3` + 2 floats, private members) | `Camera::GPUCamera` | 80B | Same story as `Light`: 64 + 8 = 72 isn't a multiple of 16, so 8 bytes (`float _pad[2]`) round it up to 80. |

Every size above was **verified**, not just hand-computed: a standalone
check compiled against the real headers and asserted `sizeof`/`alignof`/
`offsetof` for each struct matched these numbers exactly (see
"Verification" below) — compiler-inserted padding can differ from what you'd
expect on paper, so this was checked rather than assumed.

### `Object::GPUPrimitive` — the tagged struct every shape packs into

```cpp
enum : uint32_t { GPU_PRIMITIVE_SPHERE = 0, GPU_PRIMITIVE_PLANE = 1,
                   GPU_PRIMITIVE_CYLINDER = 2, GPU_PRIMITIVE_CONE = 3 };

struct GPUPrimitive
{
	uint32_t                 type;
	uint32_t                 _pad0[3];   // pad to 16B before `transform`, which itself needs 16B alignment
	Transform::GPUTransform  transform;  // 112B
	Material::GPUMaterial    material;   // 32B
	float                    params[4];  // 16B — see table below
};                                       // 176B total
```

`type` tags which local-space intersection routine the compute shader
should run for a given array element — the GPU equivalent of the vtable
dispatch `Object::hit()`'s override gets for free on the CPU
(see [01-overview.md](01-overview.md)/[04-primitives.md](04-primitives.md)
for how that dispatch works there). `params` holds whatever scalars a given
primitive kind needs on top of the shared transform/material, matching
exactly what each `hit()` override actually reads:

| Primitive | `params[0]` | `params[1]` | Why (matches the CPU `hit()` math exactly) |
|---|---|---|---|
| Sphere | radius | — | `\|local point\|² = radius²` |
| Plane | — | — | Fully described by the shared transform (`y=0` in local space) |
| Cylinder | radius | halfHeight | Side test (`x²+z²=radius²`, `\|y\|≤halfHeight`) + cap disks |
| Cone | radius | height | Side test (`x²+z²=k²y²`, `k=radius/height`, `0≤y≤height`) + base cap |

`Object` gains a protected, non-virtual `packGPU(type)` helper that fills
`type`/`transform`/`material` (identical for every subclass) and returns a
zero-initialized `params`; each subclass's `toGPU()` override is then just
`packGPU(GPU_PRIMITIVE_X)` plus filling however many `params` slots that
shape needs — e.g. `Sphere::toGPU()`:

```cpp
Object::GPUPrimitive Sphere::toGPU() const
{
	Object::GPUPrimitive p = packGPU(GPU_PRIMITIVE_SPHERE);
	p.params[0] = _radius;
	return p;
}
```

## The compute shader: `shaders/raytrace.msl`

### Why MSL, and why source text instead of precompiled `metallib`

SDL's Metal backend supports two GPU shader formats:
`SDL_GPU_SHADERFORMAT_METALLIB` (precompiled bytecode, normally produced by
`xcrun metal`/`metallib`) and `SDL_GPU_SHADERFORMAT_MSL` (raw source text).
Reading `SDL3-3.4.16/src/gpu/metal/SDL_gpu_metal.m` directly (rather than
assuming) shows exactly what each does:

```objc
if (format == SDL_GPU_SHADERFORMAT_MSL) {
	NSString *codeString = [[NSString alloc] initWithBytes:code length:codeSize
	                                                encoding:NSUTF8StringEncoding];
	library = [renderer->device newLibraryWithSource:codeString options:nil error:&error];
} else if (format == SDL_GPU_SHADERFORMAT_METALLIB) {
	...
	library = [renderer->device newLibraryWithData:data error:&error];
}
```

`newLibraryWithSource:` is a real, OS-level runtime shader compiler built
into the Metal framework itself — it needs no `xcrun metal`/full Xcode
install (which this development machine doesn't have; only Command Line
Tools). So `shaders/raytrace.msl` ships as plain source text, compiled by
the OS at `SDL_CreateGPUComputePipeline` time via this exact code path —
which also means the pipeline-creation verification described below is a
genuine compile of real, unmodified MSL source, not a pre-baked binary.

### Struct layout: `float4`, not MSL's native `float3`

Every struct in the shader mirrors its C++ counterpart using `float4` for
any "vec3" slot (`.xyz` used, `.w` ignored) instead of MSL's native
`float3`. MSL's `float3` has size 12 but alignment 16 — meaning a `float3`
member's *own* footprint is 12 bytes, but if the compiler needs to align a
*following* member, it may insert padding that's hard to predict without
careful cross-referencing of the spec. Since the C++ side already always
reserves a full 16 bytes for every `Vec3` (deliberately, as covered above),
using `float4` in MSL for the same slots makes every struct's layout
byte-identical to its C++ counterpart by construction, with no reliance on
either language's implicit padding rules matching the other's. Where
explicit tail padding is needed (`GPULight`, `GPUCamera`, `SceneUniform`),
it's spelled out as individual scalar fields (`float _pad0; float _pad1;
...`), for the same "no implicit-alignment guessing" reason.

### Resource bindings — following SDL's documented MSL convention

`SDL_CreateGPUComputePipeline`'s own doc comment specifies a required
binding order per shader format; for MSL/metallib:

> `[[buffer]]`: Uniform buffers, followed by read-only storage buffers,
> followed by read-write storage buffers.

which is exactly what the kernel signature follows:

```cpp
kernel void raytrace_main(
	constant SceneUniform  &scene       [[buffer(0)]],   // uniform
	const device GPUPrimitive *primitives [[buffer(1)]],  // read-only storage
	const device GPULight  *lights      [[buffer(2)]],   // read-only storage
	device uint             *outPixels   [[buffer(3)]],   // read-write storage
	uint2 gid [[thread_position_in_grid]])
```

No textures/samplers are used at all — the kernel's only output is a plain
`device uint*` storage buffer of packed `0x00RRGGBB` pixels, deliberately
avoiding anything that looks like a render target (see "Why this exists"
above for why that distinction matters to the subject's rules).

`SceneUniform` (128 bytes) is this shader's own addition on top of the
Phase 4/5 GPU structs — it bundles the per-frame `GPUCamera` with scene-wide
scalars the kernel needs that don't belong on any single primitive (ambient
light, primitive/light counts, image dimensions). Its C++-side mirror
(`SceneUniformGPU`, matching field-for-field) lives in `Renderer.cpp`, not
in a header, since only `Renderer::render` ever constructs one.

### Porting the math: what mirrors what

Every helper function in the shader is a direct, deliberately literal port
of the equivalent CPU function, with a comment naming its source:

| Shader function | Mirrors (CPU) |
|---|---|
| `mat3_mul`, `to_local`, `point_to_world`, `normal_to_world` | `Mat3::operator*(Vec3)`, `Transform::toLocal`/`pointToWorld`/`normalToWorld` (`include/core/Transform.hpp`) |
| `hit_sphere` | `Sphere::hit` (`srcs/objects/Sphere.cpp`) |
| `hit_plane` | `Plane::hit` (`srcs/objects/Plane.cpp`) |
| `hit_cylinder` | `Cylinder::hit` (`srcs/objects/Cylinder.cpp`) |
| `hit_cone` | `Cone::hit` (`srcs/objects/Cone.cpp`) |
| `hit_primitive` | The type-dispatch + local→world conversion every `Object::hit()` override does after calling `transform.toLocal()` |
| `trace_closest` | `Scene::trace` (`include/core/Scene.hpp`) |
| `trace_shadow` | `Scene::isOccluded` |
| `shade` | `Renderer::shade` (`srcs/core/Renderer.cpp`), including the Blinn-Phong half-vector specular term — see [05-lighting.md](05-lighting.md) |
| `raytrace_main`'s bounce loop | `Renderer::traceRay`'s recursive mirror-bounce |

Field-for-field, the same local-space equations, the same epsilon values
(`1e-4`, `1e-6`, `1e-8`), the same `tMin`/`tMax` shrinking-bound closest-hit
search. The one structural difference is unavoidable: **compute kernels
can't recurse** the way `Renderer::traceRay(scene, ray, depth)` does, so the
mirror-bounce chain

```cpp
// CPU, recursive:
color = shade(rec) * (1 - refl) + traceRay(reflectRay, depth+1) * refl
```

is unrolled into an iterative accumulate-by-weight loop that produces the
exact same expanded sum:

```cpp
float3 accumColor = float3(0.0), weight = float3(1.0);
for (int depth = 0; depth <= kMaxDepth; ++depth)
{
	HitInfo hit;
	if (!trace_closest(..., hit)) { accumColor += weight * kBackground; break; }
	float3 local = shade(..., hit, viewDir);
	accumColor += weight * local * (1.0 - hit.reflectivity);
	if (depth == kMaxDepth || hit.reflectivity <= 0.0) break;
	weight *= hit.reflectivity;
	origin = hit.point + hit.normal * 1e-4; dir = normalize(reflect(dir, hit.normal));
}
```

`kMaxDepth = 5` matches `Renderer::kMaxDepth` exactly, and the loop's
iteration count (0..5 inclusive, i.e. up to 6 traces: 1 primary + 5
reflection bounces) matches the CPU recursion's actual depth bound for the
same reason: `traceRay` only recurses while `depth < kMaxDepth`, so the
deepest possible call chain is depths 0 through 5.

## `Renderer`'s GPU dispatch path — lazy, persistent state

`Renderer::render` gains a second overload, only declared under
`GPU_COMPUTING_COMPATIBILITY`:

```cpp
static void render(const Scene &scene, const Camera &camera, FrameBuffer &fb, SDL_GPUDevice *gpuDevice);
```

Unlike the CPU path (a pure function — no state persists between calls,
see [07-rendering.md](07-rendering.md)), the GPU path needs pipeline and
buffer objects that are expensive to create and should be reused across
frames. Since `Renderer` has always been a stateless-static-API class (no
instance, `App` never constructs one), the GPU branch keeps this state in
an anonymous-namespace `static GPUState` inside `Renderer.cpp` rather than
changing `Renderer`'s public shape into a class-with-instance-state — the
smallest change that gets persistence without disturbing every call site.

```cpp
struct GPUState
{
	SDL_GPUDevice          *device;
	SDL_GPUComputePipeline *pipeline;
	SDL_GPUBuffer           *primitiveBuffer, *lightBuffer, *outputBuffer;
	SDL_GPUTransferBuffer  *downloadTransfer;
	uint32_t                 numPrimitives, numLights;
	int                       width, height;
	const Scene              *lastScene;   // pointer identity, not deep comparison
};
```

`ensureGPUState(device, scene, width, height)` runs at the top of every
`render()` call and does three independent lazy rebuilds:

1. **Pipeline** — built once, the first time `render()` is ever called
   (`g_state.pipeline` starts null); never rebuilt after that unless the
   device itself changes (which doesn't happen in normal operation — one
   `App`, one device, for the process lifetime).
2. **Output target** (`outputBuffer` + its matching download
   `SDL_GPUTransferBuffer`) — rebuilt only if `width`/`height` differ from
   last time (this project doesn't currently support live window resizing,
   so in practice this also only runs once, but the check exists so it
   wouldn't silently render into a stale, wrongly-sized buffer if that
   changed).
3. **Scene buffers** (`primitiveBuffer`, `lightBuffer`) — rebuilt only if
   `&scene` (pointer identity) differs from the last call's scene. `App`
   owns exactly one `Scene` for its whole lifetime (see
   [08-app-controls.md](08-app-controls.md)), so in the current codebase
   this uploads primitives/lights **exactly once**, on the very first
   `render()` call, and every subsequent camera-movement-triggered
   `rerender()` (the overwhelmingly common case — see `App::update()`)
   reuses them untouched.

This split matters for interactive performance: WASD/arrow-key movement
(the "camera position and direction must be easily changeable" requirement,
demonstrated live) only ever needs step 3's work skipped and a tiny
per-frame uniform push — no buffer rebuild, no re-upload, on the hot path.

### Per-frame work

```cpp
SceneUniformGPU uniform{ camera.toGPU(), scene.ambientColor, scene.ambientBrightness,
                          g_state.numPrimitives, g_state.numLights, (uint32_t)width, (uint32_t)height };

SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(gpuDevice);
SDL_PushGPUComputeUniformData(cmd, 0, &uniform, sizeof(uniform));

SDL_GPUComputePass *pass = SDL_BeginGPUComputePass(cmd, nullptr, 0, &writeBinding /* outputBuffer */, 1);
SDL_BindGPUComputePipeline(pass, g_state.pipeline);
SDL_BindGPUComputeStorageBuffers(pass, 0, {primitiveBuffer, lightBuffer}, 2);
SDL_DispatchGPUCompute(pass, ceil(width/8), ceil(height/8), 1);   // one thread per pixel, 8x8 groups
SDL_EndGPUComputePass(pass);

SDL_GPUCopyPass *copyPass = SDL_BeginGPUCopyPass(cmd);
SDL_DownloadFromGPUBuffer(copyPass, outputBuffer, downloadTransfer);
SDL_EndGPUCopyPass(copyPass);

SDL_GPUFence *fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);
SDL_WaitForGPUFences(gpuDevice, true, &fence, 1);   // block until the GPU is actually done
SDL_ReleaseGPUFence(gpuDevice, fence);

fb.setPixelsRaw(reinterpret_cast<const uint32_t *>(SDL_MapGPUTransferBuffer(gpuDevice, downloadTransfer, false)));
SDL_UnmapGPUTransferBuffer(gpuDevice, downloadTransfer);
```

`SDL_PushGPUComputeUniformData` — not a full buffer-upload round trip — is
used for the per-frame camera/scene-scalar data specifically because it's
documented as lightweight, command-buffer-scoped data, unlike
`SDL_UTILS::uploadToBuffer`'s heavier transfer-buffer dance (appropriate for
the primitive/light arrays, which are genuinely large and infrequently
updated, but wasteful for 128 bytes of per-frame camera state).

The dispatch → download → **synchronous fence wait** → map/copy/unmap
sequence is intentionally blocking: `App::rerender()` (see
[08-app-controls.md](08-app-controls.md)) expects `Renderer::render` to
return with a fully up-to-date `FrameBuffer`, exactly like the CPU path's
`std::thread::join()` calls block until every worker thread has finished —
the GPU path's fence wait is the same contract, just waiting on the GPU
timeline instead of CPU threads.

## Verification

Every phase of this backend was checked against something concrete rather
than "it compiled" — the two checks worth recording in detail:

### GPU vs. CPU pixel diff

A scratch harness rendered `Scene::buildDefault` through the real
`Renderer::render` GPU overload and dumped `FrameBuffer::rawPixels()` to a
PPM file; a second harness rendered the *same* scene through the unmodified
CPU `Renderer::render` overload (still using real MLX, via `mlx_init()` with
no window shown) and dumped that to a second PPM. Diffing them
channel-by-channel:

```
pixels: 120000, pixels with diff>2: 3 (0.00%), max channel diff: 9, mean diff: 0.000
```

3 pixels out of 120,000 differ at all (by at most 9/255 — almost certainly
float-rounding at a primitive silhouette edge, where CPU and GPU `sqrt`/
transcendental implementations can legitimately disagree by an ULP or two
right at a discriminant boundary), everything else bit-for-bit identical.
Both images show the expected scene: red sphere, green rotated cylinder,
blue cone, gray ground plane, shadows from both lights — matching
`Scene::buildDefault`'s description and the subject's own reference-figure
style (Fig VI.1-VI.3).

### Compute pipeline load, on real hardware

Before the dispatch path existed (Phase 6), a standalone check confirmed
`shaders/raytrace.msl` actually compiles through Metal's real runtime
compiler and that its declared resource counts are self-consistent, by
just building the pipeline and checking it's non-null — genuine evidence
the shader is syntactically valid MSL with a correct binding layout, not a
guess:

```
rt: GPU device ready (driver=metal, shader formats=0x30)
rt: could not open shader bytecode 'shaders/raytrace.metallib'
rt: GPU compute pipeline ready
```

### GPU struct layout

A standalone check compiled against the real headers and asserted
`sizeof`/`alignof`/`offsetof` for every GPU mirror struct
(`Mat3::GPUMat3`, `Transform::GPUTransform`, `Material::GPUMaterial`,
`Light::GPULight`, `Camera::GPUCamera`) matched the hand-computed sizes in
the table above exactly — `ALL_LAYOUTS_OK`, no compiler-inserted surprise
padding.

## Known limitations

- **Metal (macOS) only, currently.** `shaders/raytrace.msl` is the only
  shader source this project ships. A Linux (Vulkan/SPIR-V) or Windows
  (D3D12/DXIL) build of the same kernel would need its own GLSL/HLSL source
  and a toolchain-based offline compile step (`glslc`, `dxc`, or similar) —
  `SDL_UTILS` is already format-agnostic for this (`createComputePipeline`'s
  preference-order fallback would pick SPIR-V/DXIL automatically the moment
  a `shaders/raytrace.spv`/`.dxil` file exists), but no other format's
  bytecode has been authored or tested, since this development machine only
  has a Metal-capable GPU to verify against.
- **No live window resizing.** `ensureGPUState`'s output-target rebuild
  logic handles a size change correctly in principle, but nothing in `App`
  currently triggers one (the window is created at a fixed size and never
  resized), so this path is implemented but not exercised.
- **One scene per process lifetime.** `ensureSceneBuffers`' "rebuild only if
  the `Scene` pointer changed" check is correct for how `App` actually
  works today (one `Scene`, built once at startup) but would need a scene
  *content* comparison (or an explicit "dirty" flag) instead of pointer
  identity if in-program live scene editing (`TODO.md`'s "in-program live
  configuration" option) were added later — reloading a *different* `.rt`
  file today goes through a whole new `App`/process, which naturally gets a
  new `Scene` at a new address.
