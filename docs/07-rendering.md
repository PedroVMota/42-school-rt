# Rendering & framebuffer

`include/core/FrameBuffer.hpp`, `include/core/Renderer.hpp` /
`srcs/core/Renderer.cpp`, `include/core/Scene.hpp`.

This document covers the default CPU/MiniLibX path. Built with `make GPU=1`,
`FrameBuffer` instead wraps an SDL streaming texture and `Renderer::render`
dispatches the same ray-object intersection and shading math as an SDL3 GPU
**compute** shader (never a rasterization/graphics pipeline — see the
subject-compliance discussion in that doc) instead of tracing on the CPU.
See [12-gpu-compute.md](12-gpu-compute.md) for the full GPU architecture,
including a pixel-level diff proving the two paths render identically;
everything below is the CPU/MLX (default) behavior.

## FrameBuffer

Thin wrapper around an MLX image (`mlx_new_image` + `mlx_get_data_addr`).
Rendering writes directly into this backing buffer via `setPixel`, which
converts a `Vec3` color (0..1 floats) to a packed `0x00RRGGBB` pixel and
writes it at the right byte offset using the bpp/line-length/endian values
MLX reports for the current display:

```cpp
char *dst = _data + (y * _lineLength) + (x * (_bitsPerPixel / 8));
*reinterpret_cast<uint32_t *>(dst) = pixel;
```

This buffer is **persistent** across frames — it's not recreated per render,
only overwritten. That persistence is what makes the expose-without-recompute
requirement possible: the image already contains the last fully-rendered
frame at all times, ready to be blitted.

## Scene::trace / Scene::isOccluded

```cpp
bool trace(ray, tMin, tMax, rec) const;       // closest hit across all objects
bool isOccluded(point, lightPos) const;       // shadow test, see 05-lighting.md
```

`trace` is a linear scan over `Scene::objects`, shrinking `tMax` to the
current closest hit's `t` on every improvement — each subsequent object's
`hit()` only needs to beat that tighter bound, which is a cheap form of
early-out with zero acceleration-structure bookkeeping. For scene sizes in
the 4-10 primitive range (mandatory part), a BVH/kd-tree would add more
overhead than it saves; this is the right trade-off at this scale and is
reused identically for both primary and shadow rays.

## Renderer::render — parallelism

```cpp
threadCount = min(hardware_concurrency(), height);
split rows into `threadCount` contiguous bands (remainder rows spread over
    the first few threads so no thread gets more than one extra row);
each thread runs tracePixel() over its rows into the shared FrameBuffer;
join all threads before returning.
```

This is safe without any locking because:

- Every pixel writes to a disjoint memory address (`x, y` uniquely maps to
  one offset in `FrameBuffer`'s buffer) — no two threads ever touch the same
  byte.
- `Scene`/`Camera` are only **read** during rendering (`trace`, `isOccluded`,
  `rayForPixel` are all `const`), never mutated — so there's no shared
  mutable state to race on.

This is the "optimized algorithm" choice made instead of hand-written SIMD
intrinsics: ray tracing is embarrassingly parallel per pixel, so spreading
work across all CPU cores gives a much larger, more portable speedup than
manually vectorizing the per-pixel math would, while `Vec3`'s alignment
(see [02-math.md](02-math.md)) still lets the compiler auto-vectorize the
inner vector arithmetic on top of that. This is also exactly why porting the
same per-pixel work to a GPU **compute** shader (`make GPU=1`, see
[12-gpu-compute.md](12-gpu-compute.md)) is such a natural fit later: no
change to the parallelization *strategy*, just which kind of hardware
thread runs each independent pixel. A GPU **rasterization** pipeline for
the final image, on the other hand, is off the table per the subject's
constraints regardless of build flag.

## Redraw vs. re-render — the "expose" requirement

Two distinct entry points in `App` (detailed in
[08-app-controls.md](08-app-controls.md)):

- `redraw()` — `mlx_put_image_to_window` only. **No ray tracing.** Bound to
  the MLX expose event, so uncovering the window (e.g. another window moved
  away) repaints instantly from the cached `FrameBuffer` instead of
  recomputing every pixel.
- `rerender()` — `Renderer::render` (full re-trace) followed by `redraw()`.
  Only called when something that actually affects the image changed (scene
  build, or a camera-moving key press).

This split is the direct implementation of the subject's "manage to redraw
the view (or part of it) without recalculating the entire image" mandatory
requirement.
