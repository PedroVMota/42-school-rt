# Lighting model

`include/core/Material.hpp`, `include/core/Light.hpp`,
`srcs/core/Renderer.cpp` (`Renderer::shade`). Covers the mandatory
requirement: "different brightness, shadows, multi-spot, shine effect."

## Material

```cpp
struct Material {
    Vec3  color     = Vec3(1, 1, 1);
    float diffuse   = 0.8f;
    float specular  = 0.4f;
    float shininess = 32.0f;
};
```

A Blinn-Phong material: base `color`, how much it scatters light evenly
(`diffuse`), how strong/tight its specular highlight is (`specular` /
`shininess`). Copied by value into `HitRecord` at intersection time.

## Light

```cpp
struct Light {
    Vec3  position;
    Vec3  color      = Vec3(1, 1, 1);
    float brightness = 1.0f;
};
```

A point light ("spot" in the subject's wording). `Scene::lights` is a plain
`std::vector<Light>` — the **multi-spot** requirement is just "loop over more
than one entry", not special-cased anywhere in the shading code.
`brightness` is the **variable brightness** knob, multiplied straight into
each light's diffuse/specular contribution.

## Shading (`Renderer::shade`)

Per pixel, once the closest `HitRecord` is known:

```
color = material.color * ambientColor * ambientBrightness      // 1. ambient
for each light:
    if occluded(point, light.position): continue                // 2. shadow test
    diffuse  = material.color * light.color
             * max(0, N·L) * material.diffuse * light.brightness
    half     = normalize(L + V)
    specular = light.color * pow(max(0, N·half), shininess)
             * material.specular * light.brightness
    color += diffuse + specular
color = clamp01(color)
```

- **Ambient** (step 1) is `Scene::ambientBrightness` / `Scene::ambientColor`
  applied unconditionally, so occluded surfaces are dim, never pure black —
  this is the "variable brightness" floor independent of any single light.
- **Shadows** (step 2): `Scene::isOccluded` casts a ray from the hit point
  toward the light and asks `Scene::trace` if *anything* blocks it before
  reaching the light's distance. If so, that light's diffuse+specular
  contribution is skipped entirely for this pixel — a classic hard shadow.
  Multiple lights being independently occluded/visible is exactly how
  overlapping/mixed shadows (subject Fig VI.3) fall out for free: each
  light's shadow is computed and summed independently.
- **Specular / "shine"** uses the Blinn-Phong half-vector (`normalize(L +
  V)`) rather than the classical Phong reflection vector — cheaper (no
  `reflect()` call needed) and avoids the artifacts Phong has at grazing
  angles.
- `Vec3::clamp01` at the end prevents overexposed pixels from wrapping
  around when multiple lights stack (e.g. two bright specular highlights
  landing on the same pixel).

## Shadow-ray epsilon ("shadow acne")

```cpp
Ray shadowRay(point, toLight / distance);
return trace(shadowRay, 1e-3f, distance - 1e-3f, tmp);
```

The `1e-3f` lower bound keeps the shadow ray from immediately
re-intersecting the exact surface it was cast from (floating-point error
can put the computed hit point a hair below the true surface). The
`distance - 1e-3f` upper bound stops objects sitting *at or beyond* the
light itself from casting a shadow onto that same light. Both bounds are in
world units, so they're tuned for the current scene scale (~1-10 units); a
much larger/smaller scene would need to rescale them.
