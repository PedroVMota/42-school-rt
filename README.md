# RT — Ray Tracer

C++ ray tracer built on top of [MiniLibX](https://github.com/42Paris/minilibx-linux), buildable on both Linux and macOS from the same `Makefile`. See [`docs/README.md`](docs/README.md) for implementation notes and [`TODO.md`](TODO.md) for the requirements checklist.

## Structure

```
.
├── Makefile
├── minilibx-linux.tgz          # Linux MiniLibX source (X11)
├── minilibx_macos_opengl.tgz   # macOS MiniLibX source (OpenGL, Objective-C) — used by the Makefile
├── minilibx_macos_metal.tgz    # macOS MiniLibX source (Metal, Swift) — not used, kept for reference
├── docs/                       # implementation documentation, see docs/README.md
├── include/
│   ├── mlx_wrapper.hpp         # wraps mlx.h in extern "C"
│   ├── math/                   # Vec3, Mat3
│   ├── core/                   # Ray, Transform, Camera, Light, Material, Scene, Renderer, FrameBuffer, App
│   └── objects/                # Object, Plane, Sphere, Cylinder, Cone
└── srcs/                       # mirrors include/, one .cpp per non-header-only class
```

`srcs/` and `include/` are scanned **recursively** by the `Makefile` — add as many subfolders as you want, no need to edit anything.

### Why `mlx_wrapper.hpp`?

`mlx.h` has no `extern "C"` guard. Including it directly in a `.cpp` file makes the C++ compiler mangle the symbol names it expects, which fails to link against the C/Objective-C-compiled `libmlx.a` (`undefined reference to mlx_init`, etc.). Always include `mlx_wrapper.hpp` instead of `mlx.h` directly.

Also note the MiniLibX API takes `char *` (not `const char *`) in places like `mlx_new_window` — pass a mutable buffer, not a string literal, or you'll hit `-fpermissive` errors.

## How the build works

The `Makefile` detects the OS with `uname -s` and:

1. Extracts the matching `.tgz` (`minilibx-linux.tgz` on Linux, `minilibx_macos_opengl.tgz` on macOS) if it hasn't been extracted yet.
2. Builds `libmlx.a` using MiniLibX's **own** build system (`./configure && make` on Linux, `make` on macOS).
3. Compiles every `.cpp` under `srcs/` (mirroring the folder structure into `obj/`), using every subfolder under `includes/` as an `-I` path.
4. Links everything into the `program` binary.

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

## Build

```bash
make        # build (extracts + builds MiniLibX on first run, then the project)
make clean  # remove object files
make fclean # clean + remove the binary and the extracted MiniLibX source
make re     # fclean + make
```

Run it with:

```bash
./rt
```

Controls: `W`/`A`/`S`/`D` move the camera, arrow keys look around, `Esc` quits. See [`docs/08-app-controls.md`](docs/08-app-controls.md) for details.