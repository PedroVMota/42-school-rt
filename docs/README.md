# RT documentation

Implementation notes for the ray tracer, split by concern. Read them in this
order the first time; after that, jump straight to the file you need.

1. [Overview](01-overview.md) — data flow from `main()` to a pixel on screen.
2. [Math (Vec3 / Mat3)](02-math.md) — the vector/matrix layer everything else builds on.
3. [Transforms](03-transforms.md) — how translation/rotation is applied to every primitive.
4. [Primitives](04-primitives.md) — Plane, Sphere, Cylinder, Cone intersection math.
5. [Lighting model](05-lighting.md) — Material, Light, ambient/diffuse/specular, shadows.
6. [Camera](06-camera.md) — pinhole camera and primary ray generation.
7. [Rendering & framebuffer](07-rendering.md) — Renderer, multithreading, MLX image buffer.
8. [App & controls](08-app-controls.md) — window/event loop, expose handling, keybindings.
9. [Build system](09-build-system.md) — Makefile, build modes, MiniLibX packaging.
10. [Requirements mapping](10-requirements-mapping.md) — mandatory-part checklist → code.
11. [Scene file format (`.rt`)](11-scene-file-format.md) — the external scene description format and its parser.
12. [GPU compute backend](12-gpu-compute.md) — the optional `make GPU=1` build: SDL3 windowing, an SDL3 GPU compute-shader port of the ray tracer, and why it stays within the subject's "no GPU rasterization pipeline for the final image" rule.

See [`../TODO.md`](../TODO.md) for the requirements checklist itself (derived
from `en.subject.pdf`); these docs explain *how* the checked items are met,
not what's still outstanding.
