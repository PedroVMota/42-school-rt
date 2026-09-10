#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "math/Vec3.hpp"
#include "core/Ray.hpp"
#include <cmath>

#ifndef M_PI
# define M_PI 3.14159265358979323846
#endif

/*
** Pinhole camera. Position and direction are plain public-facing setters so
** they can be "easily changed" (subject requirement) either from code, a
** scene file, or a live keyboard hook.
*/
class Camera
{
	public:
		Camera(const Vec3 &position, const Vec3 &lookDir, float fovDegrees, int width, int height)
		{
			setup(position, lookDir, fovDegrees, width, height);
		}

		void	setup(const Vec3 &position, const Vec3 &lookDir, float fovDegrees, int width, int height)
		{
			_position = position;
			_forward = lookDir.normalized();
			Vec3 worldUp(0, 1, 0);
			if (std::fabs(_forward.dot(worldUp)) > 0.999f)
				worldUp = Vec3(0, 0, 1);
			_right = _forward.cross(worldUp).normalized();
			_up = _right.cross(_forward).normalized();
			_fovScale = std::tan((fovDegrees * 0.5f) * (float)M_PI / 180.0f);
			_aspect = (float)width / (float)height;
		}

		void	setPosition(const Vec3 &p) { _position = p; }
		void	setDirection(const Vec3 &dir)
		{
			_forward = dir.normalized();
			Vec3 worldUp(0, 1, 0);
			if (std::fabs(_forward.dot(worldUp)) > 0.999f)
				worldUp = Vec3(0, 0, 1);
			_right = _forward.cross(worldUp).normalized();
			_up = _right.cross(_forward).normalized();
		}

		const Vec3	&position() const { return _position; }
		const Vec3	&forward() const { return _forward; }
		const Vec3	&right() const { return _right; }
		const Vec3	&up() const { return _up; }

		/* Builds the primary ray through pixel (px, py) of a `width`x`height` image. */
		Ray	rayForPixel(int px, int py, int width, int height) const
		{
			float ndcX = (2.0f * ((px + 0.5f) / (float)width) - 1.0f) * _aspect * _fovScale;
			float ndcY = (1.0f - 2.0f * ((py + 0.5f) / (float)height)) * _fovScale;
			Vec3 dir = (_forward + _right * ndcX + _up * ndcY).normalized();
			return Ray(_position, dir);
		}

#ifdef GPU_COMPUTING_COMPATIBILITY
		/* Lets the compute shader rebuild rayForPixel()'s exact primary-ray
		** math per invocation instead of re-deriving it from raw
		** position/lookDir/fov. Four vec4s (64 bytes) plus 2 trailing
		** floats need 8 bytes of padding to round the struct up to the
		** required 16-byte-multiple std430 size (80 bytes). */
		struct GPUCamera
		{
			Vec3	position;
			Vec3	forward;
			Vec3	right;
			Vec3	up;
			float	fovScale;
			float	aspect;
			float	_pad[2];
		};

		GPUCamera	toGPU() const
		{
			return GPUCamera{ _position, _forward, _right, _up, _fovScale, _aspect, { 0.0f, 0.0f } };
		}
#endif

	private:
		Vec3	_position;
		Vec3	_forward;
		Vec3	_right;
		Vec3	_up;
		float	_fovScale;
		float	_aspect;
};

#endif
