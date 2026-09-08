#ifndef FRAMEBUFFER_HPP
#define FRAMEBUFFER_HPP

#include "mlx_wrapper.hpp"
#include "math/Vec3.hpp"
#include <cstdint>
#include <algorithm>

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

	private:
		void	*_img;
		char	*_data;
		int		_bitsPerPixel;
		int		_lineLength;
		int		_endian;
		int		_width;
		int		_height;
};

#endif
