# Transforms

`include/core/Transform.hpp` — header-only.

## The problem

The subject requires translation and rotation on every primitive ("a sphere
declared at (0,0,0) must be translatable to (42,42,42)"). Each primitive
type has its own intersection formula (quadratic for sphere, planar for
plane, etc.); re-deriving each formula for an arbitrarily translated and
rotated shape would be error-prone and slow.

## The trick: transform the ray, not the shape

Every primitive is defined in its own **canonical local space**:

| Primitive | Canonical local definition |
|---|---|
| Plane    | passes through the local origin, normal = `(0,1,0)` |
| Sphere   | centered on the local origin, given radius |
| Cylinder | axis = local Y, centered on the origin |
| Cone     | apex at the local origin, axis = local +Y |

`Transform` stores a translation `Vec3` and a rotation `Mat3` (plus its
transpose/inverse, precomputed once per `setRotationEulerXYZ` call, not per
ray):

```cpp
Ray  toLocal(const Ray &world) const;     // world -> object space
Vec3 pointToWorld(const Vec3 &local) const;
Vec3 normalToWorld(const Vec3 &local) const;
```

Every `Object::hit()` override follows the same three-step pattern:

```cpp
Ray local = transform.toLocal(worldRay);      // 1. bring the ray into local space
/* ... solve the canonical intersection equation in local space ... */
rec.point  = transform.pointToWorld(localPoint);   // 2. hit point back to world
rec.normal = transform.normalToWorld(localNormal); // 3. normal back to world
```

This means:

- Adding a new primitive only requires writing its canonical-space
  intersection formula — translation/rotation support comes for free.
- The transform math (one 3×3 matrix-vector product for the origin, one for
  the direction) is far cheaper per ray than transforming the surface
  equation's coefficients.

## Why normals use the rotation matrix directly (not inverse-transpose)

The general rule for transforming normals under a linear map `M` is
`(M⁻¹)ᵀ`. For a pure rotation, `M⁻¹ = Mᵀ`, so `(M⁻¹)ᵀ = (Mᵀ)ᵀ = M` — the
rotation matrix itself. That's why `normalToWorld` just applies `_rotation`
directly instead of computing a separate inverse-transpose. This shortcut
would silently produce wrong normals if a non-uniform scale were ever added
to `Transform` — worth remembering before adding a "scale" option.

## Precomputed inverse

`setRotationEulerXYZ` computes and caches `_rotationInv = _rotation.transposed()`
once, at scene-build time. `toLocal()` is called once per primitive per ray
(potentially millions of times per frame), so paying the transpose cost
once up front instead of per-ray matters for performance.
