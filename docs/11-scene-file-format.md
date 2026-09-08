# Scene file format (`.rt`)

`include/core/SceneParser.hpp` / `srcs/core/SceneParser.cpp`.

This is the "external file for scene description" option from the subject's
options list (chapter VI), and doubles as the "own tooling" mechanism the
subject requires for live scene manipulation during defence: `./rt
scene.rt` swaps the whole scene with no rebuild.

## Why this format

It's the same line-oriented format used by the classic 42 **miniRT**
project (`C`/`A`/`L` + `sp`/`pl`/`cy`), extended with a `co` (cone)
identifier since this project's mandatory primitive set includes a cone
that miniRT's didn't. Reusing a well-known format rather than inventing a
new one keeps scene files easy to write/read by hand and immediately
recognizable to anyone who's done miniRT.

## Grammar

One element per line. Blank lines and `#` comments (to end of line) are
ignored. Fields are whitespace-separated; vectors and colors are
comma-separated with **no spaces** inside them.

```
A  ratio               R,G,B          ambient light (ratio in [0,1])
C  x,y,z  dx,dy,dz     fov            camera (fov in degrees, ]0,180[)
L  x,y,z  brightness   [R,G,B]        point light (brightness in [0,1])
sp x,y,z  diameter     R,G,B          sphere
pl x,y,z  dx,dy,dz     R,G,B          plane (point + normal)
cy x,y,z  dx,dy,dz  diameter  height  R,G,B  cylinder (center + axis)
co x,y,z  dx,dy,dz  diameter  height  R,G,B  cone (apex + axis toward base)
```

- Exactly one `A` and one `C` line are required per file.
- `L` accepts an optional trailing color; omitted, it defaults to white.
- Direction/normal/axis vectors (`dx,dy,dz`) must have every component in
  `[-1, 1]` and must not be the zero vector; they're normalized internally,
  so they don't need to already be unit length.
- Colors are `r,g,b` with each channel `0`–`255`.
- `sp`/`cy`/`co` take a **diameter**, not a radius (matching miniRT
  convention) — the parser halves it before constructing the object.
- `co`'s `x,y,z` is the apex position (the cone's local origin per
  [04-primitives.md](04-primitives.md)), and its axis vector points from
  the apex toward the base.
- Object identifiers (`sp`/`pl`/`cy`/`co`) and light (`L`) lines may repeat
  any number of times.

## How orientation is applied

`sp` has no orientation field — spheres are rotationally symmetric, so only
`Transform::setTranslation` is used. `pl`/`cy`/`co` give an orientation as a
direction vector instead of Euler angles, so the parser computes the
rotation that takes each primitive's canonical local axis — `(0,1,0)`, same
for plane normal, cylinder axis, and cone axis (see
[03-transforms.md](03-transforms.md)) — onto the declared vector, via
`Mat3::fromToRotation` (Rodrigues' rotation formula; see
[02-math.md](02-math.md)), then calls `Transform::setRotation` with the
result.

## Error handling

Every malformed line raises `std::runtime_error("scene file, line N: ...")`
— wrong field count, a non-numeric value, an out-of-range color/ratio/fov,
a zero-length or out-of-range direction vector, a duplicate `A`/`C`, or an
unknown identifier. Missing `A`/`C` (and an unreadable/nonexistent file) are
reported the same way once the whole file has been read. `App`'s
constructor catches this, prints the message to stderr, tears down the
partially-created window, and exits — see
[08-app-controls.md](08-app-controls.md).

## Sample scenes (`scenes/`)

| File | What it exercises |
|---|---|
| `basic.rt` | All 4 primitives + 2 lights, mirroring `Scene::buildDefault` |
| `graveyard.rt` | Multiple cones sharing one orientation (identity-rotation path in `fromToRotation`) |
| `watching_eye.rt` | Minimal scene: one sphere, one light, zero ambient |
| `blood_altar.rt` | A tilted, non-unit-length axis vector (`cy` with `0,1,1`), confirming normalization |

Run any of them with `./rt -v scenes/<name>.rt` — `-v` prints render timing
alongside the load (see [08-app-controls.md](08-app-controls.md)).
