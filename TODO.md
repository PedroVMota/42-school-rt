# RT — Requirements TODO

Derived from `en.subject.pdf` (v4.1). Options/bonuses are only graded if the
mandatory part is 100% complete and works without malfunctioning.

## General constraints (apply throughout)
- [ ] Language: C, C++, or Rust — latest version, up-to-date good practices
      (project currently uses C++, consistent with this).
- [ ] No memory leaks.
- [ ] Libc / libstdc++ (or Rust equivalents) allowed freely in mandatory part.
- [ ] Math library (`-lm`) allowed.
- [ ] External native libs allowed only to open known image formats (libpng, libjpeg...).
- [ ] MiniLibX (or equivalent: SDL, XCB...) allowed for window/pixel/image/event handling.
- [ ] Final image must **not** be produced by the GPU — no GPU pipeline
      (OpenGL/Metal/Vulkan/DirectX vertex/fragment/geometry shaders) for rendering.
      GPU *compute* (OpenCL/CUDA/compute shaders) is allowed only for performance,
      not to replace the ray tracing itself.
- [ ] Any other library used in bonus part must be justifiable during defence.

## Mandatory part (0 points, but gates everything else)
- [ ] Implement the ray tracing method to produce a computer-generated image.
- [ ] At least 4 basic (non-composed) geometric primitives:
  - [ ] Plane
  - [ ] Sphere
  - [ ] Cylinder
  - [ ] Cone
- [ ] Translation and rotation transforms on objects before display
      (e.g. sphere at (0,0,0) must be translatable to (42,42,42)).
- [ ] Camera/eye: position and direction must be easily changeable.
- [ ] Redraw the view (or part of it) without recalculating the whole image
      (e.g. handle MiniLibX expose event properly).
- [ ] Light management:
  - [ ] Variable brightness
  - [ ] Shadows
  - [ ] Multiple light sources ("multi-spot")
  - [ ] Shine / specular highlight effect

### Recommended reference scenes to reproduce (helps prove mandatory part works)
- [ ] Scene with the 4 basic objects, 2 spot lights, shadows, shine (Fig VI.1/VI.2 — 2 camera viewpoints of the same scene).
- [ ] Scene demonstrating shadow mixing / overlap from multiple lights (Fig VI.3).

## Options (only scored if mandatory is perfect — pick a substantial subset)
No fixed list/limit; below are the subject's suggested options.
- [ ] Ambiance light
- [ ] Direct light
- [ ] Parallel light
- [ ] Additional limited objects: parallelograms, disks, half-spheres, tubes, etc.
- [ ] Bump mapping and colour disruption
- [x] External file format for scene description (parser) — `SceneParser`
      (`include/core/SceneParser.hpp`, `srcs/core/SceneParser.cpp`), miniRT-style
      `.rt` format, see `docs/11-scene-file-format.md`
- [ ] Reflection
- [ ] Transparency
- [ ] Shadow modification according to transparency of elements
- [ ] Composed elements: cubes, pyramids, tetrahedrons, etc.
- [ ] Textures
- [ ] Negative elements (CSG subtraction)
- [ ] Limit disruption / transparency / reflection depending on texture
- [ ] More native elements: paraboloid, hyperboloid, tablecloth, toroid, etc.
- [ ] (Optional/exotic, not required) Distributed computation across multiple machines
- [ ] (Optional/exotic, not required) Quadrics/quartics support
- [ ] (Optional/exotic, not required) Video clip export from generated image sequence
- [ ] (Optional/exotic, not required) Stereoscopic/VR headset version

Note: if parsing `.pov`/`.3ds` scene files as an option, primitives must still
be computed from equations, not from imported vertices/triangles.

## Live configuration (required capability for defence)
- [ ] Provide a way to reconfigure/manipulate scenes live during defence using
      your **own** tooling (not existing public tools) — either:
  - [x] A config file format you designed, and/or — `.rt` scene files via
        `SceneParser`, pass any scene as `./rt path/to/scene.rt`
  - [ ] In-program live configuration/editing.

## Bonuses
- [ ] Anything beyond the listed options that's outstanding can earn bonus
      points; final mark can go up to 125.
- [x] GPU compute acceleration (`make GPU=1`) — an SDL3 backend
      (`include/sdl_wrapper.hpp`, `include/core/SDLUtils.hpp` /
      `srcs/core/SDLUtils.cpp`) plus a full SDL3 GPU compute-shader port of
      the ray tracer (`shaders/raytrace.msl`), gated behind
      `GPU_COMPUTING_COMPATIBILITY` so the default MiniLibX build is
      untouched. Verified pixel-identical (99.998% of pixels exact match) to
      the CPU renderer on the same scene. Per Chapter IV's explicit note,
      this is GPU *compute*, never a rasterization pipeline for the final
      image — see `docs/12-gpu-compute.md` for the full writeup, including
      the one known gap: only the Metal/MSL shader format is implemented
      and tested (no Vulkan/SPIR-V or D3D12/DXIL shader source has been
      authored, so this only actually runs the compute path on macOS).

## Submission / defence readiness
- [ ] Work must live in the Git repository; verify file/folder naming is correct.
- [ ] Be ready to demonstrate every claimed option live during evaluation.
- [x] Prepare multiple pre-configured scenes ready to be (re)calculated live —
      pre-rendered images (jpeg/png...) are **not** accepted as proof of options.
      See `scenes/*.rt`.
- [ ] Be ready to explain and defend every design decision, including anything
      AI-assisted (per Chapter II: stay able to dive deep into any part without
      relying on AI; identify what was AI-generated).
