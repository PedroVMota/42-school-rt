#ifndef SDL_UTILS_HPP
#define SDL_UTILS_HPP

#ifdef GPU_COMPUTING_COMPATIBILITY

#include "sdl_wrapper.hpp"
#include <cstdint>
#include <vector>

/*
** Thin, reusable wrappers around the parts of SDL3's GPU *compute* API
** (SDL3/SDL_gpu.h) the ray tracer needs to accelerate intersection/shading
** work on the GPU: device setup, loading precompiled compute-shader
** bytecode, and moving primitive/light/camera data to and from GPU storage
** buffers. Nothing here touches SDL's graphics (vertex/fragment) pipeline -
** the subject forbids the final image being produced by a GPU rasterization
** pipeline, so only the compute pass is GPU-accelerated; its output is read
** back and blitted like any other pixel buffer (see FrameBuffer/App).
** Every other class that needs GPU compute goes through these functions
** rather than calling raw SDL_GPU* entry points itself.
*/
namespace SDL_UTILS
{
	/* Creates a GPU device claimed against `window`, offering every shader
	** format SDL3 knows how to consume (SPIR-V/MSL/metallib/DXIL) so the
	** platform's default driver can pick the one it needs. Returns nullptr
	** and logs via SDL_GetError() on failure. */
	SDL_GPUDevice	*createDevice(SDL_Window *window);

	/* Releases the window claim and destroys the device. Safe to call with
	** either pointer null. */
	void	destroyDevice(SDL_GPUDevice *device, SDL_Window *window);

	/* Picks the single shader format bit `device`'s driver actually
	** consumes, in the order this project ships bytecode for. Returns
	** SDL_GPU_SHADERFORMAT_INVALID if none of them are supported. */
	SDL_GPUShaderFormat	selectShaderFormat(SDL_GPUDevice *device);

	/* Reads "<basePath><ext for format>" (see shaders/) off disk into `out`
	** — e.g. basePath "shaders/raytrace" + SPIR-V reads
	** "shaders/raytrace.spv". Returns false (leaving `out` untouched) if the
	** format has no known extension or the file can't be read. */
	bool	loadShaderBytecode(const char *basePath, SDL_GPUShaderFormat format,
			std::vector<uint8_t> &out);

	/* Loads <basePath>'s bytecode for whichever format `device` supports and
	** builds a compute pipeline from it. `info` must have every field set
	** except code/code_size/format/entrypoint, which this function fills in
	** itself before calling SDL_CreateGPUComputePipeline. Returns nullptr on
	** failure (unsupported format, missing file, or pipeline creation
	** failure - all logged via SDL_GetError()/stderr). */
	SDL_GPUComputePipeline	*createComputePipeline(SDL_GPUDevice *device, const char *basePath,
			const char *entrypoint, SDL_GPUComputePipelineCreateInfo info);

	/* Creates a GPU-visible storage buffer of `size` bytes. `writable`
	** selects COMPUTE_STORAGE_WRITE (the compute shader's output, e.g. the
	** rendered pixel buffer) vs. COMPUTE_STORAGE_READ (its inputs: packed
	** primitives, lights, camera). */
	SDL_GPUBuffer	*createStorageBuffer(SDL_GPUDevice *device, uint32_t size, bool writable);

	/* Uploads `size` bytes from `data` into `buffer` (must have been created
	** with createStorageBuffer(..., writable=false)) via the transfer-
	** buffer round trip SDL's GPU API requires for any CPU->GPU copy.
	** Returns false on failure. */
	bool	uploadToBuffer(SDL_GPUDevice *device, SDL_GPUBuffer *buffer, const void *data, uint32_t size);

	/* Frees a buffer created by createStorageBuffer. Safe to call with
	** either pointer null. */
	void	releaseBuffer(SDL_GPUDevice *device, SDL_GPUBuffer *buffer);
}

#endif /* GPU_COMPUTING_COMPATIBILITY */

#endif
