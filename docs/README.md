# RT documentation

Implementation notes for the ray tracer, split by concern. Read them in this
order the first time; after that, jump straight to the file you need.

## Installation & requirements

**Linux**: `gcc`/`g++`, `make`, X11 dev headers (`xorg`), `libxext-dev`, `libbsd-dev`

```bash
sudo apt-get install gcc g++ make xorg libxext-dev libbsd-dev
```

**macOS**: Xcode Command Line Tools (`clang`/`clang++`, `make`); the OpenGL/AppKit
frameworks MiniLibX needs ship with macOS already

```bash
xcode-select --install
```

**Additionally, for `make GPU=1`** (either OS): `cmake` and `curl`, used to fetch
and build SDL3 from source, no `brew`/`apt` SDL3 package needed. SDL3's GPU
API also needs a working graphics driver on the machine to actually run the
compute shader at startup:

- **macOS**: Metal, always present on Metal-capable Macs, no extra install.
  This is the only platform the shipped shader (`shaders/raytrace.msl`)
  currently targets.
- **Linux**: Vulkan, a Vulkan-capable GPU + driver (`libvulkan1`/the
  appropriate GPU driver package) is needed for `SDL_CreateGPUDevice` to
  succeed, but **no SPIR-V shader is shipped yet**, so `make GPU=1`'s
  windowing works on Linux while the GPU compute-shader path itself does
  not. See the "Known limitations" section of
  [12-gpu-compute.md](12-gpu-compute.md#known-limitations).

```bash
make                    # default: MiniLibX backend, MODE=debug
make MODE=release       # optimized build (-O3, no sanitizers)
make GPU=1              # SDL3 backend + SDL3 GPU compute-shader ray tracing
make GPU=1 MODE=release # both combined
make clean              # remove object files
make fclean             # clean + remove the binary and the extracted MiniLibX/SDL3 sources
make re                 # fclean + make

./rt                      # built-in default scene
./rt scenes/basic.rt       # load a scene from an .rt file
./rt -v scenes/basic.rt    # -v/--verbose: print each render pass's timing
```

`debug` (the default) builds with `-O0 -g3` and
`-fsanitize=address,undefined`; `release` builds with
`-O3 -ffast-math -funroll-loops` and no sanitizers. Object files are kept in
separate `obj/debug/`/`obj/release/` trees so switching modes never links
stale objects built with different flags. See
[09-build-system.md](09-build-system.md#modes) for the full flag table and
rationale.

See the root [`README.md`](../README.md) for the full project layout and
[09-build-system.md](09-build-system.md) for how the Makefile does all this
under the hood.

## Docs index

1. [Overview](01-overview.md): data flow from `main()` to a pixel on screen.
2. [Math (Vec3 / Mat3)](02-math.md): the vector/matrix layer everything else builds on.
3. [Transforms](03-transforms.md): how translation/rotation is applied to every primitive.
4. [Primitives](04-primitives.md): Plane, Sphere, Cylinder, Cone intersection math.
5. [Lighting model](05-lighting.md): Material, Light, ambient/diffuse/specular, shadows.
6. [Camera](06-camera.md): pinhole camera and primary ray generation.
7. [Rendering & framebuffer](07-rendering.md): Renderer, multithreading, MLX image buffer.
8. [App & controls](08-app-controls.md): window/event loop, expose handling, keybindings.
9. [Build system](09-build-system.md): Makefile, build modes, MiniLibX packaging.
10. [Requirements mapping](10-requirements-mapping.md): mandatory-part checklist to code.
11. [Scene file format (`.rt`)](11-scene-file-format.md): the external scene description format and its parser.
12. [GPU compute backend](12-gpu-compute.md): the optional `make GPU=1` build. SDL3 windowing, an SDL3 GPU compute-shader port of the ray tracer, and why it stays within the subject's "no GPU rasterization pipeline for the final image" rule.

See [`../TODO.md`](../TODO.md) for the requirements checklist itself (derived
from `en.subject.pdf`); these docs explain *how* the checked items are met,
not what's still outstanding.
