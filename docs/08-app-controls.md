# App & controls

`include/core/App.hpp` / `srcs/core/App.cpp`, `include/core/Keys.hpp`,
`srcs/main.cpp`.

## Lifecycle

```cpp
App::App(width, height, title):
    mlx_init() / mlx_new_window()
    _scene = Scene::buildDefault(width, height, _camera)   // also sets up _camera
    _framebuffer = make_unique<FrameBuffer>(mlx, width, height)
    rerender()                                             // first full trace + blit
    mlx_expose_hook(win, App::onExpose, this)
    mlx_key_hook(win, App::onKeyPress, this)

App::run(): mlx_loop(mlx)   // blocks, dispatches X11/Cocoa events forever
```

`_camera` is constructed with placeholder values in the member-initializer
list (it has no default constructor — a `Camera` must always describe a
valid basis) and immediately overwritten by `Scene::buildDefault`'s
`outCamera.setup(...)` call. `_scene`/`_framebuffer` are move-assigned /
`unique_ptr`-owned respectively, so there's exactly one owner of the MLX
image and the object list at all times — no double-free / leak risk on
destruction.

## Event trampolines

MLX's C API takes bare function pointers, not member functions, so
`onExpose`/`onKeyPress` are `static` and forward to the instance via the
`param` argument every `mlx_*_hook` call carries:

```cpp
static int onExpose(void *param)   { static_cast<App *>(param)->redraw(); return 0; }
static int onKeyPress(int keycode, void *param) { static_cast<App *>(param)->handleKey(keycode); return 0; }
```

Registered with an explicit `(int (*)())` cast — `mlx_expose_hook`/
`mlx_key_hook` are declared with empty-parens (`()`, meaning "unspecified
arguments", a C idiom) function pointer types in `mlx.h`, which C++'s
stricter type system won't implicitly convert to/from. This is a well-known
wart of wrapping MiniLibX from C++ and the cast is safe here because MLX
calls the pointer back with exactly the arguments each hook documents.

## Keybindings (`Keys.hpp`)

| Key | Effect |
|---|---|
| `W` / `S` | move camera forward / backward along its look direction |
| `A` / `D` | strafe camera left / right |
| `←` / `→` | yaw the look direction |
| `↑` / `↓` | pitch the look direction |
| `Esc` | destroy the window and exit |

`Keys.hpp` exists because MLX reports **raw platform keycodes**, and X11
(Linux) and Cocoa (macOS) use completely different numbering for the same
physical key — the header centralizes the `#ifdef __APPLE__` mapping so
`App.cpp` only ever deals with named constants (`KEY_W`, `KEY_ESC`, ...).

`handleKey` only calls `rerender()` if one of the recognized keys was
pressed (`moved` flag) — unrecognized keys are ignored without triggering an
expensive re-trace. This is also where the "position and direction of the
camera can be changed easily" requirement is demonstrated live: every
keypress is a couple of `Vec3` operations plus one `Renderer::render` call,
with no restart needed.

## Why `Esc` calls `mlx_destroy_window` then `std::exit(0)` directly

`mlx_loop` never returns control on its own; the conventional MiniLibX
pattern for a clean exit is to tear down the window from inside a hook and
terminate the process immediately rather than trying to unwind back out of
`mlx_loop`. `App::~App()` also guards on `_win` being non-null so it won't
double-destroy if the destructor somehow still ran.
