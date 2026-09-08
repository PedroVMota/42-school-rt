# Math: Vec3 / Mat3

`include/math/Vec3.hpp`, `include/math/Mat3.hpp` — header-only, no `.cpp`.

## Vec3

```cpp
struct alignas(16) Vec3 { float x, y, z, w; ... };
```

Used for both 3D points/directions **and** RGB color (0..1 range), which is
the standard ray tracer trick: `Material::color * Light::color` is just a
component-wise `Vec3::operator*`.

Why 16-byte aligned with an unused `w`:

- A `Vec3` then occupies exactly one SSE/NEON register's worth of memory
  (4 × float = 16 bytes), so the compiler can lower `operator+`, `operator*`,
  `dot()`, `cross()` etc. to a single vector instruction instead of three
  scalar ones, with no manual intrinsics required.
- `-O3 -ffast-math` (release build only, see [09-build-system.md](09-build-system.md))
  gives the compiler permission to reorder float operations, which is what
  actually triggers this auto-vectorization for the reduction in `dot()`.
- `w` is always 0 and never touched by any operator, so it can never leak
  into a dot product or a color channel.
- Debug builds intentionally do **not** get `-ffast-math` (it would fight
  with `-fsanitize=undefined`'s strict float semantics), so debug is
  slower — that trade-off is fine since debug exists for correctness, not
  performance.

Operations provided: `+ - * / (scalar) * (component-wise) dot cross length
lengthSquared normalized reflect clamp01`. `reflect(normal)` implements the
standard `I - 2(I·N)N` formula and is the same building block that a future
"reflection" option would reuse.

## Mat3

Plain row-major 3×3 matrix (`m[row][col]`), used only to rotate `Vec3`s. No
4×4/homogeneous machinery: the subject only asks for translation + rotation,
never scaling or perspective projection of *objects* (the camera does its
own perspective divide directly in `Camera::rayForPixel`, see
[06-camera.md](06-camera.md)), so a 3×3 matrix plus a separate translation
`Vec3` (stored in `Transform`, see [03-transforms.md](03-transforms.md)) is
sufficient and cheaper than carrying a 4×4 matrix through every intersection
test.

- `rotationX/Y/Z(radians)` — single-axis rotation matrices.
- `fromEulerXYZ(rx, ry, rz)` — composes them as `Rx * Ry * Rz`, applied to a
  vector as `Rx * (Ry * (Rz * v))`. This is the convention `Transform`
  assumes; if you add a scene parser, angles must be supplied in that order.
- `transposed()` — for an **orthonormal** rotation matrix (which is all
  `Mat3` ever represents here — pure rotations, no shear/scale), the
  inverse equals the transpose. `Transform` uses this to get the
  world-to-local rotation for free instead of computing a general 3×3
  inverse.
