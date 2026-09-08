# Overview

## Directory layout

```
include/
  math/       Vec3, Mat3                       — pure math, no dependencies
  core/       Ray, Transform, Camera, Light,
              Material, HitRecord, Scene,
              Renderer, FrameBuffer, App, Keys  — ray tracing + windowing
  objects/    Object (base), Plane, Sphere,
              Cylinder, Cone                    — geometric primitives
srcs/         mirrors include/, one .cpp per non-header-only class
```

`math/` never includes anything from `core/` or `objects/`; `objects/`
depends on `core/` (`Ray`, `HitRecord`, `Material`, `Transform`) but not the
other way around. `Scene`/`Renderer`/`App` sit on top and wire everything
together. This keeps the intersection math testable in isolation from MLX.

## Data flow, one frame

```
main()
  -> App::App()
       -> mlx_init() / mlx_new_window()
       -> Scene::buildDefault() or SceneParser::parseFile(scene.rt)
                                        builds objects + lights + camera
       -> App::rerender()
            -> Renderer::render(scene, camera, framebuffer)
                 for each pixel (parallel across rows):
                   Camera::rayForPixel()        -> primary Ray
                   Scene::trace()                -> closest HitRecord
                   Renderer::shade()             -> ambient + per-light
                     Scene::isOccluded()         -> shadow ray
                   FrameBuffer::setPixel()        -> write into MLX image
            -> mlx_put_image_to_window()          blit the whole image
  -> App::run() -> mlx_loop()                     event loop (blocking)

  on expose event -> App::redraw()  (blit only, no ray tracing)
  on loop tick    -> App::update()  (checks currently held keys, moves the
                                      camera, calls rerender() only if
                                      something actually changed)
```

The split between `redraw()` (cheap, blit-only) and `rerender()` (expensive,
re-traces every pixel) is what satisfies the subject's "redraw without
recalculating the entire image" requirement — see
[07-rendering.md](07-rendering.md) and [08-app-controls.md](08-app-controls.md).

## Object lifetime / ownership

- `App` owns the MLX connection/window handles, the `Scene`, the `Camera`,
  and the `FrameBuffer` (via `std::unique_ptr`).
- `Scene` owns every `Object` via `std::vector<std::unique_ptr<Object>>` —
  polymorphic primitives, one vtable dispatch per intersection test.
- Nothing else allocates ray-tracing-side resources per frame: `Renderer`
  and the primitives are stateless with respect to a single render call, so
  there's nothing to leak between frames (relevant to the "no memory leaks"
  requirement).
