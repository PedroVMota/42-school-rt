# Camera

`include/core/Camera.hpp` — header-only. Satisfies "position and direction
of the camera can be changed easily."

## Basis construction

Given a position and a look direction, `setup()`/`setDirection()` build an
orthonormal basis:

```cpp
_forward = lookDir.normalized();
Vec3 worldUp = |forward · (0,1,0)| > 0.999 ? (0,0,1) : (0,1,0);  // avoid gimbal flip
_right   = forward.cross(worldUp).normalized();
_up      = right.cross(forward).normalized();
```

The `worldUp` fallback to `(0,0,1)` only kicks in when the camera looks
almost straight up or down, where `forward × (0,1,0)` would otherwise be a
near-zero vector (undefined "right" direction).

## Ray generation (`rayForPixel`)

```cpp
ndcX = (2*(px+0.5)/width  - 1) * aspect * fovScale
ndcY = (1 - 2*(py+0.5)/height)         * fovScale
dir  = normalize(forward + right*ndcX + up*ndcY)
```

- `fovScale = tan(fov/2)` is precomputed once in `setup()`, not per pixel.
- `+0.5` samples the pixel *center*, standard practice to avoid a
  half-pixel offset bias in the image.
- `ndcY` is flipped (`1 - 2*...` instead of `2*...  - 1`) because image row
  `0` is the top of the screen but `+up` should correspond to the top —
  without the flip the rendered image would come out vertically mirrored.
- The resulting `dir` is exactly what `Renderer`/`Object::hit` assume
  everywhere: a **normalized** ray direction (see [04-primitives.md](04-primitives.md)
  and [02-math.md](02-math.md) — `Ray::dir` is documented as "must be kept
  normalized").

## Changing the camera live

`setPosition`/`setDirection` are cheap, pure setters — no recomputation
happens until the next `Renderer::render` call. `App::handleKey` (see
[08-app-controls.md](08-app-controls.md)) calls these on WASD/arrow input
and only then triggers a re-render, so moving the camera is just "mutate two
`Vec3`s, rebuild the basis, re-trace."
