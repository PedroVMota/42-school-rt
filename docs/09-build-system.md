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

## Project sources

```make
SRCS := $(shell find $(SRCS_DIR) -name '*.cpp')
INC_DIRS := $(shell find $(INC_DIR) -type d) $(MLX_DIR)
```

Both are discovered recursively, so adding a new `.cpp` under `srcs/` or a
new subfolder under `include/` needs no Makefile edit — this is why
`math/`, `core/`, `objects/` all "just work" as separate include roots.
`-MMD -MP` generates per-object `.d` dependency files (`-include $(DEPS)`)
so header changes correctly trigger recompilation of everything that
includes them, without a manual dependency list.

## Executable name

The binary is named `rt` (`NAME = rt`), matching the subject's explicit
requirement ("the executable file must be named `rt`").
