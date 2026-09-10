#ifndef RENDERER_HPP
#define RENDERER_HPP

#ifdef GPU_COMPUTING_COMPATIBILITY
	#include "sdl_wrapper.hpp"
#endif
#include "core/Scene.hpp"
#include "core/Camera.hpp"
#include "core/FrameBuffer.hpp"

class Renderer
{
	public:
#ifdef GPU_COMPUTING_COMPATIBILITY
		/* Dispatches shaders/raytrace.msl's compute kernel (one thread per
		** pixel) instead of tracing on the CPU, then reads the result back
		** into `fb`. `gpuDevice` is App's SDL_GPUDevice (see core/SDLUtils.hpp);
		** the compute pipeline and primitive/light/output GPU buffers are
		** built lazily on first call and reused/rebuilt only when the scene
		** or image size actually changes (see Renderer.cpp's ensureGPUState). */
		static void	render(const Scene &scene, const Camera &camera, FrameBuffer &fb, SDL_GPUDevice *gpuDevice);
#else
		/* Traces one primary ray per pixel, splitting the rows across the
		** machine's hardware threads: independent pixels, no shared
		** mutable state, so this is an easy, safe win that keeps the
		** interactive redraw fast without touching the GPU. */
		static void	render(const Scene &scene, const Camera &camera, FrameBuffer &fb);

	private:
		static constexpr int	kMaxDepth = 5; /* bounces allowed between mirrors facing each other */

		static Vec3	tracePixel(const Scene &scene, const Camera &camera, int x, int y, int width, int height);
		static Vec3	traceRay(const Scene &scene, const Ray &ray, int depth);
		static Vec3	shade(const Scene &scene, const HitRecord &rec, const Vec3 &viewDir);
#endif
};

#endif
