# Build system

`Makefile` (repo root).

## Modes

```bash
make            # MODE defaults to debug (see MODE ?= debug)
make dev        # alias for MODE=debug
make release    # optimized build
make MODE=debug / MODE=release   # explicit
```

Object files live under separate `obj/debug/` and `obj/release/` trees so
switching modes never links stale objects built with different flags
against each other.

| | debug | release |
|---|---|---|
| Optimization | `-O0` | `-O3 -ffast-math -funroll-loops` |
| Sanitizers | `-fsanitize=address,undefined -fno-omit-frame-pointer` | none |
| Symbols | `-g3` | none |
| Defines | `-DDEBUG` | `-DNDEBUG` |

Both modes always carry `-Wall -Wextra -std=c++17 -pthread` (the last one
required for `Renderer`'s `std::thread` use, both for compiling and for
linking — see `MLX_LDFLAGS` below).

`-ffast-math` is deliberately **release-only**: it relaxes IEEE float
semantics (which is what lets the compiler auto-vectorize `Vec3` operations
per [02-math.md](02-math.md)) but would fight with
`-fsanitize=undefined`'s strict-float checks in debug, so debug stays at
standard-compliant float math where sanitizer output is meaningful.
`-march=native` was deliberately **not** added despite being tempting for
extra SIMD width: it would make `release` binaries only portable to the
exact CPU family they were built on, which is a bad trade-off for a
project that has to run/be defended on a machine that might not match the
dev machine.

## MiniLibX packaging

```
UNAME_S := $(shell uname -s)
Linux  -> minilibx-linux.tgz        (./configure && make)
Darwin -> minilibx_macos_opengl.tgz (make)
```

The matching archive is extracted (once — the extracted directory is an
order-only prerequisite of the static lib target, so `tar` never re-runs
once the folder exists) and built with **its own** build system, producing
`libmlx.a`. Link flags differ per OS (`-lXext -lX11 -lbsd` for Linux's X11
backend vs. `-framework OpenGL -framework AppKit -lz` for macOS).

Link ordering matters: `-L$(MLX_DIR) -lmlx $(SYS_LDFLAGS)` puts `libmlx.a`
**before** the system libraries it depends on. `ld` resolves symbols
left-to-right and drops libraries it's already processed once their
symbols are no longer needed, so put X11/Xext after `-lmlx` and the linker
would have already discarded them by the time `libmlx.a` asks for their
symbols — the fix is exactly this ordering, not `--start-group`/
`--end-group`, since it's simpler and the dependency direction is one-way.

## Backend toggle: MiniLibX (default) vs. SDL3 GPU compute (`make GPU=1`)

```bash
make            # default: MiniLibX backend, exactly as above
make GPU=1      # SDL3 backend + SDL3 GPU compute-shader ray tracing
make GPU=1 MODE=release
```

`GPU` is a Makefile variable, `GPU ?= 0`. It controls three things at once:

```make
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

1. Which static windowing library gets built and linked (`$(SDL_LIB)` vs.
   `$(MLX_LIB)`, both prerequisites of `$(NAME)` — see the `$(NAME):` rule).
2. Which directory gets added as an `-I` include root (SDL3's `include/`
   vs. the extracted MLX directory).
3. Whether `-DGPU_COMPUTING_COMPATIBILITY` is defined at all — the single
   macro every `#ifdef`-guarded GPU-backend code path in the project checks.
   `GPU=0` (the default) never defines it, so none of that code is even
   parsed by the preprocessor in a default build.

**`make` with no `GPU=1` is byte-for-byte the same build this project
always had** — nothing about the default path changed by adding this
option; `GPU=1` is purely additive.

See [12-gpu-compute.md](12-gpu-compute.md) for the full architecture this
enables (SDL3 windowing, the GPU compute-shader ray tracer, and how it stays
within the subject's rules); this section only covers how the Makefile
fetches and builds the extra dependency.

### SDL3 packaging — fetched and built by the Makefile, not a system dependency

Unlike MiniLibX (whose `.tgz` archives are committed to this repo), SDL3 is
fetched from its own GitHub releases the first time `make GPU=1` runs, then
built from source with its own CMake build — deliberately mirroring the
MiniLibX pattern above (self-contained, no `brew`/`apt`/`pkg-config`
dependency) rather than assuming a system install:

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

Same idempotency as the MLX rule above: `$(SDL_SRC_DIR)` is an order-only
prerequisite of `$(SDL_LIB)`, so `curl`/`tar` only run once, on whichever
`make GPU=1` invocation first needs the library — a plain `make` (`GPU=0`)
never triggers network access at all. `-DSDL_STATIC=ON -DSDL_SHARED=OFF`
mirrors linking MiniLibX statically: the built `rt` shouldn't need an SDL3
shared library installed system-wide to run.

`SDL_VERSION`/`SDL_ARCHIVE`/`SDL_SRC_DIR`/`SDL_LIB` are defined
**unconditionally**, outside the `ifeq ($(GPU),1)` block — so `make fclean`
(see below) can remove a previously-fetched SDL3 tree regardless of which
mode the *next* `make` invocation runs in.

`cmake` and `curl` are the two extra host tools `GPU=1` needs beyond what
every other build mode already requires.

### Extra link flags on macOS

```make
SDL_SYS_LDFLAGS = -framework Cocoa -framework Metal -framework QuartzCore \
		  -framework CoreVideo -framework CoreAudio -framework AudioToolbox \
		  -framework ForceFeedback -framework GameController -framework CoreHaptics \
		  -framework IOKit -framework Carbon -framework UniformTypeIdentifiers \
		  -framework CoreMedia -framework AVFoundation
```

SDL3's static build compiles in far more subsystems than MiniLibX does
(audio, haptics, game controllers, camera capture...), so it needs a much
longer framework list than MiniLibX's `-framework OpenGL -framework AppKit
-lz`. `-framework CoreMedia -framework AVFoundation` specifically were
needed to resolve `SDL_camera_coremedia.m.o`'s `AVCaptureDevice`/
`CMSampleBuffer` symbols — SDL3's camera-capture backend, compiled in even
though this project never opens a camera. Same link-ordering rule as
`libmlx.a` applies: `$(SDL_LIB)` must precede these frameworks on the link
line.

## Project sources

```make
SRCS := $(shell find $(SRCS_DIR) -name '*.cpp')
INC_DIRS := $(shell find $(INC_DIR) -type d) $(BACKEND_INC)
```

Both are discovered recursively, so adding a new `.cpp` under `srcs/` or a
new subfolder under `include/` needs no Makefile edit — this is why
`math/`, `core/`, `objects/` all "just work" as separate include roots.
`-MMD -MP` generates per-object `.d` dependency files (`-include $(DEPS)`)
so header changes correctly trigger recompilation of everything that
includes them, without a manual dependency list. `$(BACKEND_INC)` is
`$(MLX_DIR)` or `$(SDL_SRC_DIR)/include` depending on `GPU` (see above) —
every backend-specific `.cpp` (`SDLUtils.cpp`, the GPU branches inside
`App.cpp`/`Renderer.cpp`/etc.) is discovered by the same recursive `find`
as everything else; there's no separate file list to maintain per backend,
only `#ifdef` guards inside the files themselves.

## Executable name

The binary is named `rt` (`NAME = rt`), matching the subject's explicit
requirement ("the executable file must be named `rt`").
