# App & controls

`include/core/App.hpp` / `srcs/core/App.cpp`, `include/core/Keys.hpp`,
`srcs/main.cpp`.

## Lifecycle

```cpp
App::App(width, height, title, verbose, sceneFile):
    mlx_init() / mlx_new_window()
    try:
        _scene = sceneFile ? SceneParser::parseFile(sceneFile, width, height, _camera)
                            : Scene::buildDefault(width, height, _camera)
    catch (std::exception &e):
        print e.what() to stderr, destroy window, exit(1)
    _framebuffer = make_unique<FrameBuffer>(mlx, width, height)
    rerender()                                             // first full trace + blit
    mlx_expose_hook(win, App::onExpose, this)
    mlx_hook(win, KeyPress,   KeyPressMask,   App::onKeyDown, this)
    mlx_hook(win, KeyRelease, KeyReleaseMask, App::onKeyUp,   this)
    mlx_loop_hook(mlx, App::onLoopTick, this)

App::run(): mlx_loop(mlx)   // blocks, dispatches X11/Cocoa events forever
```

`_camera` is constructed with placeholder values in the member-initializer
list (it has no default constructor — a `Camera` must always describe a
valid basis) and immediately overwritten by `Scene::buildDefault`'s or
`SceneParser::parseFile`'s `outCamera.setup(...)` call. `_scene`/
`_framebuffer` are move-assigned / `unique_ptr`-owned respectively, so
there's exactly one owner of the MLX image and the object list at all
times — no double-free / leak risk on destruction.

A malformed scene file throws `std::runtime_error` from `SceneParser`; the
constructor catches it, tears down the (already-created) window, prints the
message, and exits — this happens before the MLX event loop ever starts, so
there's no partially-initialized `App` left running.

## CLI flags (`srcs/main.cpp`)

```bash
./rt                     # built-in default scene (Scene::buildDefault)
./rt scene.rt             # load scene.rt instead (any non-flag argument)
./rt -v                   # -v / --verbose: print each render's timing to stderr
./rt -v scene.rt           # both together, any order
```

`main` does its own tiny arg parse: `-v`/`--verbose` sets a flag, any other
`-`-prefixed argument is rejected as unknown, and at most one non-flag
argument is accepted as the scene file path. See
[11-scene-file-format.md](11-scene-file-format.md) for the `.rt` format
itself.

## Event trampolines

MLX's C API takes bare function pointers, not member functions, so
`onExpose`/`onKeyDown`/`onKeyUp`/`onLoopTick` are `static` and forward to
the instance via the `param` argument every `mlx_*_hook` call carries:

```cpp
static int onExpose(void *param)   { static_cast<App *>(param)->redraw(); return 0; }
static int onKeyDown(int keycode, void *param) { static_cast<App *>(param)->handleKeyDown(keycode); return 0; }
static int onKeyUp(int keycode, void *param)   { static_cast<App *>(param)->handleKeyUp(keycode);   return 0; }
static int onLoopTick(void *param) { static_cast<App *>(param)->update(); return 0; }
```

Registered with an explicit `(int (*)())` cast — `mlx_hook`/`mlx_loop_hook`
are declared with empty-parens (`()`, meaning "unspecified arguments", a C
idiom) function pointer types in `mlx.h`, which C++'s stricter type system
won't implicitly convert to/from. This is a well-known wart of wrapping
MiniLibX from C++ and the cast is safe here because MLX calls the pointer
back with exactly the arguments each hook documents.

## Why `mlx_key_hook` isn't used

`mlx_key_hook` fires on key **release**, not key press, on both minilibx
backends — confirmed by reading the implementation directly:
`mlx_new_window.m`'s `setEvent:3` on macOS and `mlx_key_hook.c`'s
`win->hooks[KeyRelease]` on Linux both bind it to the release slot. That
gives at best one discrete step per full press-then-release, with no way to
tell a held key from a tapped one.

Instead, `App` hooks `KeyPress`/`KeyRelease` directly via the generic
`mlx_hook(win, x_event, x_mask, funct, param)` (event codes/masks are
hardcoded in `Keys.hpp` as `MLX_KEYPRESS`/`MLX_KEYRELEASE` — these are
stable X11 protocol constants shared by both backends, avoiding a
dependency on X11 headers that don't exist on the Cocoa/macOS build) and
tracks currently-held keys in `_pressedKeys` (a `std::unordered_set<int>`).
`mlx_loop_hook` then polls that set once per event-loop tick via
`App::update()`.

## Keybindings (`Keys.hpp`)

| Key | Effect |
|---|---|
| `W` / `S` | move camera forward / backward along its look direction |
| `A` / `D` | strafe camera left / right |
| `←` / `→` | yaw the look direction |
| `↑` / `↓` | pitch the look direction |
| `Esc` | destroy the window and exit |

Movement is **continuous while a key is held down** (driven by
`App::update()` on every loop tick), and multiple held keys combine — e.g.
`W` + `D` moves diagonally — since `update()` checks every tracked key each
tick instead of reacting to one keycode per event.

`Keys.hpp` exists because MLX reports **raw platform keycodes**, and X11
(Linux) and Cocoa (macOS) use completely different numbering for the same
physical key — the header centralizes the `#ifdef __APPLE__` mapping so
`App.cpp` only ever deals with named constants (`KEY_W`, `KEY_ESC`, ...).

`update()` only calls `rerender()` if at least one held key actually moved
or turned the camera this tick — an empty `_pressedKeys` (or only unrelated
keys held) skips the expensive re-trace entirely. This is also where the
"position and direction of the camera can be changed easily" requirement is
demonstrated live: each tick is a couple of `Vec3` operations plus one
`Renderer::render` call, with no restart needed. Because a full re-trace
currently costs hundreds of milliseconds to over a second (see
[07-rendering.md](07-rendering.md)), the loop naturally self-throttles: the
next tick — and thus the next `update()` call — can't run until the
in-flight `rerender()` returns, so held-key movement advances the camera
roughly once per render rather than flooding it with redundant re-traces.

`Esc` is handled directly in `handleKeyDown` (not deferred to `update()`),
so it exits immediately on press rather than waiting for a tick.

## Why `Esc` calls `mlx_destroy_window` then `std::exit(0)` directly

`mlx_loop` never returns control on its own; the conventional MiniLibX
pattern for a clean exit is to tear down the window from inside a hook and
terminate the process immediately rather than trying to unwind back out of
`mlx_loop`. `App::~App()` also guards on `_win` being non-null so it won't
double-destroy if the destructor somehow still ran.

## Verbose render timing (`-v`/`--verbose`)

`App::rerender()` wraps the `Renderer::render` call in
`std::chrono::steady_clock` timestamps when `_verbose` is set, and prints
`[verbose] render took N.NN ms` to stderr. Since `rerender()` is the single
choke point every camera-changing path (initial render, held-key movement,
a future scene reload) goes through, this covers every re-trace without
needing to instrument each caller separately.
