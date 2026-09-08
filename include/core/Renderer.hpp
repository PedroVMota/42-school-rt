#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "core/Scene.hpp"
#include "core/Camera.hpp"
#include "core/FrameBuffer.hpp"

class Renderer
{
	public:
		/* Traces one primary ray per pixel, splitting the rows across the
		** machine's hardware threads: independent pixels, no shared
		** mutable state, so this is an easy, safe win that keeps the
		** interactive redraw fast without touching the GPU. */
		static void	render(const Scene &scene, const Camera &camera, FrameBuffer &fb);

	private:
		static Vec3	tracePixel(const Scene &scene, const Camera &camera, int x, int y, int width, int height);
		static Vec3	shade(const Scene &scene, const HitRecord &rec, const Vec3 &viewDir);
};

#endif
