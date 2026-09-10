#ifndef FRAMEBUFFER_HPP
#define FRAMEBUFFER_HPP

#ifdef GPU_COMPUTING_COMPATIBILITY
	#include "sdl_wrapper.hpp"
	#include <vector>
	#include <cstring>
#else
	#include "mlx_wrapper.hpp"
#endif
#include "math/Vec3.hpp"
#include <cstdint>
#include <algorithm>

#ifdef GPU_COMPUTING_COMPATIBILITY

/*
** Thin wrapper around an SDL streaming texture. setPixel writes into a
** CPU-side scratch buffer (cheap, called once per pixel per re-trace);
** present() uploads the whole buffer to the GPU texture in one call. The
** App calls present() once per re-trace (see App::rerender), then blits the
** already-uploaded texture on every frame tick via SDL_RenderTexture with
** zero further CPU work — same "redraw without recalculating" contract the
** MLX backend gets for free from mlx_put_image_to_window.
*/
class FrameBuffer
{
	public:
		FrameBuffer(void *renderer, int width, int height)
			: _texture(nullptr), _width(width), _height(height), _pixels(width * height, 0)
		{
			_texture = SDL_CreateTexture(static_cast<SDL_Renderer *>(renderer),
				SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
		}

		~FrameBuffer()
		{
			if (_texture)
				SDL_DestroyTexture(static_cast<SDL_Texture *>(_texture));
		}

		int		width() const { return _width; }
		int		height() const { return _height; }
		void	*handle() const { return _texture; }

		void	setPixel(int x, int y, const Vec3 &color)
		{
			uint8_t r = (uint8_t)std::clamp(color.x * 255.0f, 0.0f, 255.0f);
			uint8_t g = (uint8_t)std::clamp(color.y * 255.0f, 0.0f, 255.0f);
			uint8_t b = (uint8_t)std::clamp(color.z * 255.0f, 0.0f, 255.0f);

			_pixels[y * _width + x] = ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
		}

		/* Bulk equivalent of setPixel, used by Renderer's GPU path: the
		** compute shader already packs pixels into this exact 0x00RRGGBB
		** layout (see shaders/raytrace.msl), so its readback can be copied
		** straight in instead of round-tripping every pixel through a Vec3. */
		void	setPixelsRaw(const uint32_t *packed)
		{
			std::memcpy(_pixels.data(), packed, _pixels.size() * sizeof(uint32_t));
		}

		void	present()
		{
			SDL_UpdateTexture(static_cast<SDL_Texture *>(_texture), nullptr,
				_pixels.data(), _width * (int)sizeof(uint32_t));
		}

		/* Read-only access to the packed 0x00RRGGBB CPU-side buffer, e.g.
		** for dumping a rendered frame to disk for verification/tooling. */
		const uint32_t	*rawPixels() const { return _pixels.data(); }

	private:
		void					*_texture;
		int						_width;
		int						_height;
		std::vector<uint32_t>	_pixels;
};

#else

/*
** Thin wrapper around an MLX image. We render into this backing buffer once,
** then `mlx_put_image_to_window` blits the whole thing on the expose event
** with zero ray tracing work — that's what satisfies the "redraw without
** recalculating the entire image" requirement.
*/
class FrameBuffer
{
	public:
		FrameBuffer(void *mlx, int width, int height) : _width(width), _height(height)
		{
			_img = mlx_new_image(mlx, width, height);
			_data = mlx_get_data_addr(_img, &_bitsPerPixel, &_lineLength, &_endian);
		}

		~FrameBuffer()
		{
			/* mlx_destroy_image intentionally not called here: the App owns
			** the mlx connection lifetime and tears everything down at exit
			** via mlx_destroy_window / process exit, which is sufficient
			** for this single long-lived framebuffer. */
		}

		int		width() const { return _width; }
		int		height() const { return _height; }
		void	*handle() const { return _img; }

		void	setPixel(int x, int y, const Vec3 &color)
		{
			uint8_t r = (uint8_t)std::clamp(color.x * 255.0f, 0.0f, 255.0f);
			uint8_t g = (uint8_t)std::clamp(color.y * 255.0f, 0.0f, 255.0f);
			uint8_t b = (uint8_t)std::clamp(color.z * 255.0f, 0.0f, 255.0f);
			uint32_t pixel = ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;

			char *dst = _data + (y * _lineLength) + (x * (_bitsPerPixel / 8));
			*reinterpret_cast<uint32_t *>(dst) = pixel;
		}

		/* No-op: setPixel already writes straight into MLX's own backing
		** memory, so there is no separate upload step (unlike the SDL
		** backend's streaming-texture path above). */
		void	present() {}

	private:
		void	*_img;
		char	*_data;
		int		_bitsPerPixel;
		int		_lineLength;
		int		_endian;
		int		_width;
		int		_height;
};

#endif /* GPU_COMPUTING_COMPATIBILITY */

#endif
