# Primitives

`include/objects/Object.hpp` + one header/`.cpp` pair per shape. All four
satisfy the "at least 4 basic, non-composed geometric objects" requirement:
plane, sphere, cylinder, cone.

## Object (base class)

```cpp
class Object {
public:
    virtual bool hit(const Ray &worldRay, float tMin, float tMax, HitRecord &rec) const = 0;
    Transform transform;
    Material  material;
};
```

`hit()` follows the standard ray tracer contract: return `true` and fill
`rec` only if there's an intersection with parameter `t` strictly inside
`(tMin, tMax)`. The open interval matters twice:

- `tMin` (usually a small epsilon like `1e-4f`/`1e-3f`) skips
  self-intersection at the ray's own origin — without it, secondary rays
  (shadow rays especially) would immediately "hit" the surface they were
  cast from due to floating-point rounding ("shadow acne").
- `tMax` lets `Scene::trace` shrink the search window to the closest hit
  found so far, so each subsequent object only has to beat the current
  best `t` — a cheap form of early rejection.

## Plane (`Plane.cpp`)

Canonical form: `y = 0`, normal `(0,1,0)`. Ray-plane intersection is a single
division: `t = -origin.y / dir.y` (undefined/skipped when `dir.y ≈ 0`, i.e.
the ray runs parallel to the plane). The normal is flipped to face the ray
origin (`dot(dir, normal) > 0 ⇒ negate`) so shading always sees the "front"
side regardless of which way the ray approaches from.

## Sphere (`Sphere.cpp`)

Canonical form: centered on the local origin, radius `r`. Classic quadratic
`|O + tD|² = r²` solved via the discriminant. Uses the smaller (nearer)
root first and only falls back to the larger root if the near one is outside
`(tMin, tMax)` — this is what lets a camera positioned *inside* a sphere
still see its far wall instead of reporting a miss. Local normal at the hit
point is just `point / radius` (no square root needed — already unit length
by construction since the point lies on the sphere).

## Cylinder (`Cylinder.cpp`)

Canonical form: **finite, capped**, axis = local Y, extends from
`y = -height/2` to `y = +height/2`. Two-part algorithm, both parts write into
a shared `bestT`/`found` pair so the two are naturally merged into whichever
is closer:

1. **Side surface**: `x² + z² = r²` restricted to `y` within the cylinder's
   height — a 2D circle equation in `x`/`z`, ignoring `y` in the quadratic
   itself, then rejecting roots whose `y` falls outside the cap range.
2. **End caps**: two disks at `y = ±halfHeight`; intersect the ray with each
   plane, then reject if the hit point falls outside radius `r` from the
   axis.

Capping matters for correctness under translation/rotation: an uncapped
cylinder viewed end-on would show as an empty ring with the far side visible
through it, which reads as a rendering bug rather than a cylinder.

## Cone (`Cone.cpp`)

Canonical form: **finite, capped**, apex at the local origin, axis = local
+Y, base radius `radius` at `y = height` (so `k = radius / height` is the
half-angle's slope). Implicit surface: `x² + z² = k²y²`.

Substituting the ray gives a quadratic same shape as the sphere/cylinder,
but with an important extra case: the leading coefficient `a` can be
**zero** (when the ray direction is exactly parallel to the cone's surface
along that axis), which would make the general quadratic solver divide by
zero. The code branches on `|a| > ε`: quadratic case as usual, otherwise a
linear fallback (`t = -c / b`) so that degenerate ray directions are still
handled instead of silently missing. The base is capped the same way as the
cylinder's caps (disk at `y = height`); the apex itself needs no cap since
it's a single point.

Normal on the side surface comes from the gradient of `f(x,y,z) = x² + z² -
k²y²`, which is `(2x, -2k²y, 2z)` — normalized before use since (unlike the
sphere) this isn't already unit length.

## Shared conventions

- Every `hit()` transforms the ray once via `transform.toLocal()`, solves
  the intersection in local space, then converts the point/normal back with
  `transform.pointToWorld()` / `transform.normalToWorld()` — see
  [03-transforms.md](03-transforms.md).
- Every normal is flipped toward the incoming ray before being stored, so
  `Renderer::shade()` never has to guess which side of a surface it's
  looking at.
- `material` is copied into the `HitRecord` by value — cheap (a few floats
  and a `Vec3`) and keeps `HitRecord` self-contained for shading.
