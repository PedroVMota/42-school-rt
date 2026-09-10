# RT — Ray Tracer

C++ ray tracer built on top of [MiniLibX](https://github.com/42Paris/minilibx-linux) by default, buildable on both Linux and macOS from the same `Makefile`. An optional second backend (`make GPU=1`) swaps in SDL3 windowing and an SDL3 GPU **compute**-shader port of the ray tracer — see [`docs/12-gpu-compute.md`](docs/12-gpu-compute.md). See [`docs/README.md`](docs/README.md) for implementation notes and [`TODO.md`](TODO.md) for the requirements checklist.

## Structure

```
.
├── Makefile
├── minilibx-linux.tgz          # Linux MiniLibX source (X11)
├── minilibx_macos_opengl.tgz   # macOS MiniLibX source (OpenGL, Objective-C) — used by the Makefile
├── minilibx_macos_metal.tgz    # macOS MiniLibX source (Metal, Swift) — not used, kept for reference
├── SDL3-<version>/             # fetched + built by `make GPU=1` on first run, gitignored — not committed
├── docs/                       # implementation documentation, see docs/README.md
├── scenes/                     # sample .rt scene files, see docs/11-scene-file-format.md
├── shaders/
│   └── raytrace.msl            # GPU compute-shader port of the ray tracer (GPU=1 build only), see docs/12-gpu-compute.md
├── include/
│   ├── mlx_wrapper.hpp         # wraps mlx.h in extern "C"
│   ├── sdl_wrapper.hpp         # GPU_COMPUTING_COMPATIBILITY-gated <SDL3/SDL.h> include point
│   ├── math/                   # Vec3, Mat3
│   ├── core/                   # Ray, Transform, Camera, Light, Material, Scene, Renderer, FrameBuffer, App, SDLUtils
│   └── objects/                # Object, Plane, Sphere, Cylinder, Cone
└── srcs/                       # mirrors include/, one .cpp per non-header-only class
```

`srcs/` and `include/` are scanned **recursively** by the `Makefile` — add as many subfolders as you want, no need to edit anything. Every class that touches windowing or rendering (`App`, `FrameBuffer`, `Renderer`, `Keys.hpp`, plus the GPU-side data layout on `Vec3`/`Mat3`/`Transform`/`Material`/`Light`/`Camera`/the primitives) has a `#ifdef GPU_COMPUTING_COMPATIBILITY` branch alongside its default MiniLibX/CPU implementation; `SDLUtils.hpp`/`.cpp` and everything under `shaders/` only exist for the `GPU=1` build.

### Why `mlx_wrapper.hpp`?

`mlx.h` has no `extern "C"` guard. Including it directly in a `.cpp` file makes the C++ compiler mangle the symbol names it expects, which fails to link against the C/Objective-C-compiled `libmlx.a` (`undefined reference to mlx_init`, etc.). Always include `mlx_wrapper.hpp` instead of `mlx.h` directly.

Also note the MiniLibX API takes `char *` (not `const char *`) in places like `mlx_new_window` — pass a mutable buffer, not a string literal, or you'll hit `-fpermissive` errors.

## How the build works

The `Makefile` detects the OS with `uname -s` and:

1. Extracts the matching `.tgz` (`minilibx-linux.tgz` on Linux, `minilibx_macos_opengl.tgz` on macOS) if it hasn't been extracted yet.
2. Builds `libmlx.a` using MiniLibX's **own** build system (`./configure && make` on Linux, `make` on macOS).
3. Compiles every `.cpp` under `srcs/` (mirroring the folder structure into `obj/`), using every subfolder under `includes/` as an `-I` path.
4. Links everything into the `rt` binary.

## Requirements

**Linux**
- `gcc`/`g++`, `make`
- X11 dev headers: `xorg`
- XShm extension: `libxext-dev`
- BSD utility functions: `libbsd-dev`

```bash
sudo apt-get install gcc g++ make xorg libxext-dev libbsd-dev
```

**macOS**
- Xcode Command Line Tools (`clang`/`clang++`, `make`)
- The OpenGL/AppKit frameworks used by MiniLibX ship with macOS — no extra install needed.

```bash
xcode-select --install
```

**Additionally, for `make GPU=1`** (either OS): `cmake` and `curl`, used to
fetch and build SDL3 from source the same way MiniLibX is built — no
`brew`/`apt` SDL3 package needed, see
[`docs/12-gpu-compute.md`](docs/12-gpu-compute.md#sdl3-is-fetched-and-built-by-the-makefile-not-a-system-dependency).

## Build

```bash
make            # default: MiniLibX backend (extracts + builds MiniLibX on first run, then the project)
make GPU=1      # SDL3 backend + SDL3 GPU compute-shader ray tracing (fetches + builds SDL3 on first run)
make clean      # remove object files
make fclean     # clean + remove the binary and the extracted MiniLibX/SDL3 sources
make re         # fclean + make
```

`make GPU=1` produces the exact same `rt` executable name and CLI, just
built against a different backend — see
[`docs/12-gpu-compute.md`](docs/12-gpu-compute.md) for what actually
changes under the hood and why it still satisfies the subject's "no GPU
rasterization pipeline for the final image" rule.

Run it with:

```bash
./rt                     # built-in default scene
./rt scenes/basic.rt      # load a scene from an .rt file
./rt -v scenes/basic.rt   # -v/--verbose: print each render pass's timing
```

Any non-flag argument is treated as a `.rt` scene file; see
[`docs/11-scene-file-format.md`](docs/11-scene-file-format.md) for the file
format and [`scenes/`](scenes/) for sample scenes. Malformed scene files
fail fast with a `line N: ...` error message instead of rendering.

Controls: `W`/`A`/`S`/`D` move the camera, arrow keys look around (movement
is continuous while a key is held down), `Esc` quits. See
[`docs/08-app-controls.md`](docs/08-app-controls.md) for details.