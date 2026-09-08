# Requirements mapping

Traceability from `TODO.md`'s "Mandatory part" checklist to the code that
implements each item. Use this to quickly point at proof during defence.

| Requirement | Implementation |
|---|---|
| Implement the ray tracing method | `Renderer::tracePixel` / `Renderer::shade` (`srcs/core/Renderer.cpp`) casting one primary ray per pixel via `Camera::rayForPixel`, closest-hit via `Scene::trace` |
| ≥ 4 basic, non-composed primitives (plane, sphere, cylinder, cone) | `include/objects/{Plane,Sphere,Cylinder,Cone}.hpp` + matching `.cpp`, see [04-primitives.md](04-primitives.md) |
| Translation + rotation on objects before display | `Transform` (`include/core/Transform.hpp`), set per-object via `object->transform.setTranslation(...)` / `setRotationEulerXYZ(...)` in `Scene::buildDefault` (`srcs/core/Scene.cpp`) — see [03-transforms.md](03-transforms.md) |
| Camera/eye position & direction easily changeable | `Camera::setPosition` / `Camera::setDirection` (`include/core/Camera.hpp`), driven live by `App::update` (`srcs/core/App.cpp`) on held WASD + arrow keys — see [06-camera.md](06-camera.md), [08-app-controls.md](08-app-controls.md) |
| Redraw (or part of) without recalculating the whole image | `App::redraw()` (blit-only, MLX expose hook) vs. `App::rerender()` (full re-trace), `FrameBuffer` persists the last frame — see [07-rendering.md](07-rendering.md) |
| Light management — variable brightness | `Light::brightness` field, multiplied into diffuse/specular in `Renderer::shade` |
| Light management — shadows | `Scene::isOccluded` shadow ray test, called per light per pixel in `Renderer::shade` |
| Light management — multiple light sources ("multi-spot") | `Scene::lights` is a `std::vector<Light>`; `Scene::buildDefault` populates 2 lights (`key`, `fill`) with overlapping illumination/shadows |
| Light management — shine / specular highlight | Blinn-Phong half-vector term in `Renderer::shade`, tuned per-material via `Material::specular` / `Material::shininess` |
| Code in C/C++/Rust, up-to-date practices | C++17 throughout, `Makefile` enforces `-std=c++17 -Wall -Wextra` |
| No memory leaks | RAII ownership only (`std::unique_ptr<Object>` in `Scene::objects`, `std::unique_ptr<FrameBuffer>` in `App`); no raw `new`/manual `delete` anywhere in the render path — see [01-overview.md](01-overview.md) "Object lifetime" |
| Executable named `rt` | `Makefile`: `NAME = rt` |
| GPU not used for final image | Software rasterization into an MLX image buffer (`FrameBuffer::setPixel`); no GL/Metal/Vulkan/DirectX shader anywhere in the codebase |
| **Option:** external file format for scene description | `SceneParser::parseFile` (`include/core/SceneParser.hpp`, `srcs/core/SceneParser.cpp`), a miniRT-style `.rt` text format — see [11-scene-file-format.md](11-scene-file-format.md) |
| **Defence requirement:** live scene manipulation with own tooling | `./rt path/to/scene.rt` swaps the whole scene with no rebuild; `scenes/*.rt` are ready-made examples covering all 4 primitives, multiple lights, and non-trivial rotations |

## Reference scenes

`Scene::buildDefault` (`srcs/core/Scene.cpp`) hardcodes one scene combining
all four primitives with two lights positioned to overlap their shadows,
intended to match the subject's suggested reference renders (Fig VI.1/VI.2
style: 4 objects + 2 spots + shadows + shine; Fig VI.3 style: shadow
mixing). Camera position/direction there is just the `Camera` constructor
call — moving the "second camera viewpoint" is exactly the WASD/arrow-key
live movement described in [08-app-controls.md](08-app-controls.md), no
code change or restart needed.
