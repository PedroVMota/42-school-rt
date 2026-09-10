#include "core/Renderer.hpp"

#ifdef GPU_COMPUTING_COMPATIBILITY

#include "core/SDLUtils.hpp"
#include <cstdio>
#include <cstdint>
#include <vector>

namespace
{
	/* Mirrors shaders/raytrace.msl's SceneUniform struct exactly (see that
	** file's header comment for the full layout rationale) - bundles the
	** per-frame camera with the scene-wide scalars the kernel needs that
	** don't belong on any single primitive. */
	struct SceneUniformGPU
	{
		Camera::GPUCamera	camera;
		Vec3				ambientColor;
		float				ambientBrightness;
		uint32_t			numPrimitives;
		uint32_t			numLights;
		uint32_t			width;
		uint32_t			height;
		uint32_t			_pad0;
		uint32_t			_pad1;
		uint32_t			_pad2;
	};

	/* Everything the GPU path needs to persist between frames: built lazily
	** on first render and only rebuilt when the device, scene contents, or
	** image size actually change - camera movement (the common case, one
	** rerender per keypress) touches none of this, only the tiny per-frame
	** uniform push in Renderer::render below. */
	struct GPUState
	{
		SDL_GPUDevice			*device = nullptr;
		SDL_GPUComputePipeline	*pipeline = nullptr;
		SDL_GPUBuffer			*primitiveBuffer = nullptr;
		SDL_GPUBuffer			*lightBuffer = nullptr;
		SDL_GPUBuffer			*outputBuffer = nullptr;
		SDL_GPUTransferBuffer	*downloadTransfer = nullptr;
		uint32_t				numPrimitives = 0;
		uint32_t				numLights = 0;
		int						width = 0;
		int						height = 0;
		const Scene				*lastScene = nullptr;
	};

	GPUState	g_state;

	void	releaseGPUState()
	{
		if (!g_state.device)
			return;
		SDL_UTILS::releaseBuffer(g_state.device, g_state.primitiveBuffer);
		SDL_UTILS::releaseBuffer(g_state.device, g_state.lightBuffer);
		SDL_UTILS::releaseBuffer(g_state.device, g_state.outputBuffer);
		if (g_state.downloadTransfer)
			SDL_ReleaseGPUTransferBuffer(g_state.device, g_state.downloadTransfer);
		if (g_state.pipeline)
			SDL_ReleaseGPUComputePipeline(g_state.device, g_state.pipeline);
		g_state = GPUState{};
	}

	bool	ensureOutputTarget(int width, int height)
	{
		if (g_state.outputBuffer && g_state.width == width && g_state.height == height)
			return true;

		SDL_UTILS::releaseBuffer(g_state.device, g_state.outputBuffer);
		if (g_state.downloadTransfer)
			SDL_ReleaseGPUTransferBuffer(g_state.device, g_state.downloadTransfer);

		uint32_t	byteSize = (uint32_t)(width * height * sizeof(uint32_t));

		g_state.outputBuffer = SDL_UTILS::createStorageBuffer(g_state.device, byteSize, true);

		SDL_GPUTransferBufferCreateInfo	dlInfo{};

		dlInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
		dlInfo.size = byteSize;
		g_state.downloadTransfer = SDL_CreateGPUTransferBuffer(g_state.device, &dlInfo);

		g_state.width = width;
		g_state.height = height;
		return g_state.outputBuffer && g_state.downloadTransfer;
	}

	bool	ensureSceneBuffers(const Scene &scene)
	{
		if (g_state.lastScene == &scene)
			return true;

		std::vector<Object::GPUPrimitive>	prims;
		std::vector<Light::GPULight>		lights;

		prims.reserve(scene.objects.size());
		for (const auto &obj : scene.objects)
			prims.push_back(obj->toGPU());
		lights.reserve(scene.lights.size());
		for (const auto &light : scene.lights)
			lights.push_back(light.toGPU());

		SDL_UTILS::releaseBuffer(g_state.device, g_state.primitiveBuffer);
		SDL_UTILS::releaseBuffer(g_state.device, g_state.lightBuffer);

		/* Buffers can't be zero-sized; a scene with no lights/objects still
		** needs a valid (if unused - numPrimitives/numLights gate the
		** shader's loops) buffer to bind. */
		uint32_t	primBytes = (uint32_t)(prims.size() * sizeof(Object::GPUPrimitive));
		uint32_t	lightBytes = (uint32_t)(lights.size() * sizeof(Light::GPULight));

		g_state.primitiveBuffer = SDL_UTILS::createStorageBuffer(g_state.device,
			primBytes > 0 ? primBytes : sizeof(Object::GPUPrimitive), false);
		g_state.lightBuffer = SDL_UTILS::createStorageBuffer(g_state.device,
			lightBytes > 0 ? lightBytes : sizeof(Light::GPULight), false);

		bool	ok = g_state.primitiveBuffer && g_state.lightBuffer;

		if (ok && primBytes > 0)
			ok = SDL_UTILS::uploadToBuffer(g_state.device, g_state.primitiveBuffer, prims.data(), primBytes);
		if (ok && lightBytes > 0)
			ok = SDL_UTILS::uploadToBuffer(g_state.device, g_state.lightBuffer, lights.data(), lightBytes);

		g_state.numPrimitives = (uint32_t)prims.size();
		g_state.numLights = (uint32_t)lights.size();
		g_state.lastScene = &scene;
		return ok;
	}

	bool	ensureGPUState(SDL_GPUDevice *device, const Scene &scene, int width, int height)
	{
		if (g_state.device != device)
		{
			releaseGPUState();
			g_state.device = device;
		}

		if (!g_state.pipeline)
		{
			SDL_GPUComputePipelineCreateInfo	info{};

			info.num_uniform_buffers = 1;
			info.num_readonly_storage_buffers = 2;
			info.num_readwrite_storage_buffers = 1;
			info.threadcount_x = 8;
			info.threadcount_y = 8;
			info.threadcount_z = 1;
			g_state.pipeline = SDL_UTILS::createComputePipeline(device, "shaders/raytrace", "raytrace_main", info);
			if (g_state.pipeline)
				std::fprintf(stderr, "rt: GPU compute pipeline ready\n");
			else
				return false;
		}

		return ensureOutputTarget(width, height) && ensureSceneBuffers(scene);
	}
}

void	Renderer::render(const Scene &scene, const Camera &camera, FrameBuffer &fb, SDL_GPUDevice *gpuDevice)
{
	int	width = fb.width();
	int	height = fb.height();

	if (!gpuDevice || !ensureGPUState(gpuDevice, scene, width, height))
	{
		std::fprintf(stderr, "rt: GPU render path unavailable this frame\n");
		return;
	}

	SceneUniformGPU	uniform{};

	uniform.camera = camera.toGPU();
	uniform.ambientColor = scene.ambientColor;
	uniform.ambientBrightness = scene.ambientBrightness;
	uniform.numPrimitives = g_state.numPrimitives;
	uniform.numLights = g_state.numLights;
	uniform.width = (uint32_t)width;
	uniform.height = (uint32_t)height;

	SDL_GPUCommandBuffer	*cmd = SDL_AcquireGPUCommandBuffer(gpuDevice);

	SDL_PushGPUComputeUniformData(cmd, 0, &uniform, sizeof(uniform));

	SDL_GPUStorageBufferReadWriteBinding	writeBinding{};

	writeBinding.buffer = g_state.outputBuffer;
	writeBinding.cycle = false;

	SDL_GPUComputePass	*pass = SDL_BeginGPUComputePass(cmd, nullptr, 0, &writeBinding, 1);

	SDL_BindGPUComputePipeline(pass, g_state.pipeline);

	SDL_GPUBuffer	*readBuffers[2] = { g_state.primitiveBuffer, g_state.lightBuffer };

	SDL_BindGPUComputeStorageBuffers(pass, 0, readBuffers, 2);

	uint32_t	groupsX = (uint32_t)((width + 7) / 8);
	uint32_t	groupsY = (uint32_t)((height + 7) / 8);

	SDL_DispatchGPUCompute(pass, groupsX, groupsY, 1);
	SDL_EndGPUComputePass(pass);

	SDL_GPUCopyPass			*copyPass = SDL_BeginGPUCopyPass(cmd);
	SDL_GPUBufferRegion			srcRegion{ g_state.outputBuffer, 0, (uint32_t)(width * height * sizeof(uint32_t)) };
	SDL_GPUTransferBufferLocation	dstLoc{ g_state.downloadTransfer, 0 };

	SDL_DownloadFromGPUBuffer(copyPass, &srcRegion, &dstLoc);
	SDL_EndGPUCopyPass(copyPass);

	SDL_GPUFence	*fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);

	SDL_WaitForGPUFences(gpuDevice, true, &fence, 1);
	SDL_ReleaseGPUFence(gpuDevice, fence);

	void	*mapped = SDL_MapGPUTransferBuffer(gpuDevice, g_state.downloadTransfer, false);

	if (mapped)
	{
		fb.setPixelsRaw(reinterpret_cast<const uint32_t *>(mapped));
		SDL_UnmapGPUTransferBuffer(gpuDevice, g_state.downloadTransfer);
	}
}

#else

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

#endif /* GPU_COMPUTING_COMPATIBILITY */
