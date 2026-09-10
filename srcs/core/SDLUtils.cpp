#include "core/SDLUtils.hpp"

#ifdef GPU_COMPUTING_COMPATIBILITY

#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>

namespace
{
	const char	*extensionForFormat(SDL_GPUShaderFormat format)
	{
		switch (format)
		{
			case SDL_GPU_SHADERFORMAT_SPIRV:
				return ".spv";
			case SDL_GPU_SHADERFORMAT_METALLIB:
				return ".metallib";
			case SDL_GPU_SHADERFORMAT_MSL:
				return ".msl";
			case SDL_GPU_SHADERFORMAT_DXIL:
				return ".dxil";
			default:
				return nullptr;
		}
	}
}

SDL_GPUDevice	*SDL_UTILS::createDevice(SDL_Window *window)
{
	SDL_GPUShaderFormat	formats = SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL
		| SDL_GPU_SHADERFORMAT_METALLIB | SDL_GPU_SHADERFORMAT_DXIL;
	SDL_GPUDevice		*device = SDL_CreateGPUDevice(formats, false, nullptr);

	if (!device)
	{
		std::fprintf(stderr, "rt: SDL_CreateGPUDevice failed: %s\n", SDL_GetError());
		return nullptr;
	}
	if (!SDL_ClaimWindowForGPUDevice(device, window))
	{
		std::fprintf(stderr, "rt: SDL_ClaimWindowForGPUDevice failed: %s\n", SDL_GetError());
		SDL_DestroyGPUDevice(device);
		return nullptr;
	}
	std::fprintf(stderr, "rt: GPU device ready (driver=%s, shader formats=0x%x)\n",
		SDL_GetGPUDeviceDriver(device), (unsigned)SDL_GetGPUShaderFormats(device));
	return device;
}

void	SDL_UTILS::destroyDevice(SDL_GPUDevice *device, SDL_Window *window)
{
	if (!device)
		return;
	if (window)
		SDL_ReleaseWindowFromGPUDevice(device, window);
	SDL_DestroyGPUDevice(device);
}

SDL_GPUShaderFormat	SDL_UTILS::selectShaderFormat(SDL_GPUDevice *device)
{
	SDL_GPUShaderFormat	supported = SDL_GetGPUShaderFormats(device);

	if (supported & SDL_GPU_SHADERFORMAT_SPIRV)
		return SDL_GPU_SHADERFORMAT_SPIRV;
	if (supported & SDL_GPU_SHADERFORMAT_METALLIB)
		return SDL_GPU_SHADERFORMAT_METALLIB;
	if (supported & SDL_GPU_SHADERFORMAT_MSL)
		return SDL_GPU_SHADERFORMAT_MSL;
	if (supported & SDL_GPU_SHADERFORMAT_DXIL)
		return SDL_GPU_SHADERFORMAT_DXIL;
	return SDL_GPU_SHADERFORMAT_INVALID;
}

bool	SDL_UTILS::loadShaderBytecode(const char *basePath, SDL_GPUShaderFormat format,
		std::vector<uint8_t> &out)
{
	const char	*ext = extensionForFormat(format);

	if (!ext)
		return false;

	std::string		path = std::string(basePath) + ext;
	std::ifstream	file(path, std::ios::binary | std::ios::ate);

	if (!file)
	{
		std::fprintf(stderr, "rt: could not open shader bytecode '%s'\n", path.c_str());
		return false;
	}

	std::streamsize	size = file.tellg();
	file.seekg(0, std::ios::beg);
	out.resize(static_cast<size_t>(size));
	if (!file.read(reinterpret_cast<char *>(out.data()), size))
	{
		std::fprintf(stderr, "rt: failed reading shader bytecode '%s'\n", path.c_str());
		return false;
	}
	return true;
}

SDL_GPUComputePipeline	*SDL_UTILS::createComputePipeline(SDL_GPUDevice *device, const char *basePath,
		const char *entrypoint, SDL_GPUComputePipelineCreateInfo info)
{
	/* Preference order, not just "whatever selectShaderFormat() picked":
	** this project doesn't necessarily ship bytecode for every format a
	** device claims to support (e.g. Metal accepts both MSL and metallib,
	** but only raytrace.msl exists), so fall through to the next format
	** the device supports until one actually has a file on disk. */
	static const SDL_GPUShaderFormat	kPreferenceOrder[] = {
		SDL_GPU_SHADERFORMAT_SPIRV,
		SDL_GPU_SHADERFORMAT_METALLIB,
		SDL_GPU_SHADERFORMAT_MSL,
		SDL_GPU_SHADERFORMAT_DXIL,
	};
	SDL_GPUShaderFormat		supported = SDL_GetGPUShaderFormats(device);
	SDL_GPUShaderFormat		format = SDL_GPU_SHADERFORMAT_INVALID;
	std::vector<uint8_t>	bytecode;

	for (SDL_GPUShaderFormat candidate : kPreferenceOrder)
	{
		if (!(supported & candidate))
			continue;
		if (loadShaderBytecode(basePath, candidate, bytecode))
		{
			format = candidate;
			break;
		}
	}
	if (format == SDL_GPU_SHADERFORMAT_INVALID)
	{
		std::fprintf(stderr, "rt: no usable shader bytecode found for '%s' (device supports 0x%x)\n",
			basePath, (unsigned)supported);
		return nullptr;
	}

	info.code = bytecode.data();
	info.code_size = bytecode.size();
	info.format = format;
	info.entrypoint = entrypoint;

	SDL_GPUComputePipeline	*pipeline = SDL_CreateGPUComputePipeline(device, &info);

	if (!pipeline)
		std::fprintf(stderr, "rt: SDL_CreateGPUComputePipeline failed: %s\n", SDL_GetError());
	return pipeline;
}

SDL_GPUBuffer	*SDL_UTILS::createStorageBuffer(SDL_GPUDevice *device, uint32_t size, bool writable)
{
	SDL_GPUBufferCreateInfo	info{};

	info.usage = writable ? SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE : SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ;
	info.size = size;

	SDL_GPUBuffer	*buffer = SDL_CreateGPUBuffer(device, &info);

	if (!buffer)
		std::fprintf(stderr, "rt: SDL_CreateGPUBuffer failed: %s\n", SDL_GetError());
	return buffer;
}

bool	SDL_UTILS::uploadToBuffer(SDL_GPUDevice *device, SDL_GPUBuffer *buffer, const void *data, uint32_t size)
{
	SDL_GPUTransferBufferCreateInfo	transferInfo{};

	transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	transferInfo.size = size;

	SDL_GPUTransferBuffer	*transfer = SDL_CreateGPUTransferBuffer(device, &transferInfo);

	if (!transfer)
	{
		std::fprintf(stderr, "rt: SDL_CreateGPUTransferBuffer failed: %s\n", SDL_GetError());
		return false;
	}

	void	*mapped = SDL_MapGPUTransferBuffer(device, transfer, false);

	if (!mapped)
	{
		std::fprintf(stderr, "rt: SDL_MapGPUTransferBuffer failed: %s\n", SDL_GetError());
		SDL_ReleaseGPUTransferBuffer(device, transfer);
		return false;
	}
	std::memcpy(mapped, data, size);
	SDL_UnmapGPUTransferBuffer(device, transfer);

	SDL_GPUCommandBuffer			*cmd = SDL_AcquireGPUCommandBuffer(device);
	SDL_GPUCopyPass					*copyPass = SDL_BeginGPUCopyPass(cmd);
	SDL_GPUTransferBufferLocation	source{transfer, 0};
	SDL_GPUBufferRegion				destination{buffer, 0, size};

	SDL_UploadToGPUBuffer(copyPass, &source, &destination, false);
	SDL_EndGPUCopyPass(copyPass);

	bool	ok = SDL_SubmitGPUCommandBuffer(cmd);

	SDL_ReleaseGPUTransferBuffer(device, transfer);
	if (!ok)
		std::fprintf(stderr, "rt: SDL_SubmitGPUCommandBuffer failed: %s\n", SDL_GetError());
	return ok;
}

void	SDL_UTILS::releaseBuffer(SDL_GPUDevice *device, SDL_GPUBuffer *buffer)
{
	if (device && buffer)
		SDL_ReleaseGPUBuffer(device, buffer);
}

#endif /* GPU_COMPUTING_COMPATIBILITY */
