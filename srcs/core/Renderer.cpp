#include "core/Renderer.hpp"
#include <thread>
#include <vector>
#include <algorithm>
#include <cmath>

Vec3	Renderer::shade(const Scene &scene, const HitRecord &rec, const Vec3 &viewDir)
{
	/* Ambient term: constant floor brightness so shadowed surfaces are
	** never fully black (variable-brightness ambient light). */
	Vec3 color = rec.material.color * scene.ambientColor * scene.ambientBrightness;

	for (const Light &light : scene.lights)
	{
		if (scene.isOccluded(rec.point, light.position))
			continue; /* fully shadowed w.r.t. this light */

		Vec3 toLight = (light.position - rec.point).normalized();
		float diffuseFactor = std::max(0.0f, rec.normal.dot(toLight));
		Vec3 diffuse = rec.material.color * light.color * (diffuseFactor * rec.material.diffuse * light.brightness);

		/* Blinn-Phong specular ("shine") highlight. */
		Vec3 halfVec = (toLight + viewDir).normalized();
		float specAngle = std::max(0.0f, rec.normal.dot(halfVec));
		float specularFactor = std::pow(specAngle, rec.material.shininess);
		Vec3 specular = light.color * (specularFactor * rec.material.specular * light.brightness);

		color += diffuse + specular;
	}

	return Vec3::clamp01(color);
}

Vec3	Renderer::traceRay(const Scene &scene, const Ray &ray, int depth)
{
	HitRecord rec;

	if (!scene.trace(ray, 1e-4f, 1e30f, rec))
		return Vec3(0.05f, 0.05f, 0.08f); /* background color */

	Vec3 viewDir = -ray.dir;
	Vec3 color = shade(scene, rec, viewDir);

	if (depth < kMaxDepth && rec.material.reflectivity > 0.0f)
	{
		/* Mirror-bounce the incoming ray around the surface normal and
		** recurse; offset the origin along the normal to avoid the new
		** ray immediately re-hitting the same surface (reflection acne). */
		Vec3 reflectDir = ray.dir.reflect(rec.normal).normalized();
		Ray reflectRay(rec.point + rec.normal * 1e-4f, reflectDir);
		Vec3 reflected = traceRay(scene, reflectRay, depth + 1);
		color = color * (1.0f - rec.material.reflectivity) + reflected * rec.material.reflectivity;
	}

	return color;
}

Vec3	Renderer::tracePixel(const Scene &scene, const Camera &camera, int x, int y, int width, int height)
{
	Ray ray = camera.rayForPixel(x, y, width, height);
	return traceRay(scene, ray, 0);
}

void	Renderer::render(const Scene &scene, const Camera &camera, FrameBuffer &fb)
{
	int width = fb.width();
	int height = fb.height();

	unsigned int threadCount = std::thread::hardware_concurrency();
	if (threadCount == 0)
		threadCount = 4;
	threadCount = std::min(threadCount, (unsigned int)height);

	std::vector<std::thread> workers;
	workers.reserve(threadCount);

	auto renderRows = [&](int rowStart, int rowEnd)
	{
		for (int y = rowStart; y < rowEnd; ++y)
			for (int x = 0; x < width; ++x)
				fb.setPixel(x, y, tracePixel(scene, camera, x, y, width, height));
	};

	int rowsPerThread = height / (int)threadCount;
	int remainder = height % (int)threadCount;
	int row = 0;
	for (unsigned int i = 0; i < threadCount; ++i)
	{
		int count = rowsPerThread + (i < (unsigned int)remainder ? 1 : 0);
		workers.emplace_back(renderRows, row, row + count);
		row += count;
	}
	for (auto &t : workers)
		t.join();
}
