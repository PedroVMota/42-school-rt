#ifndef VEC3_HPP
#define VEC3_HPP

#include <cmath>

/*
** 16-byte aligned so the compiler can lower dot/cross/add/scale to a single
** SSE (or NEON) instruction instead of 3 scalar ones. The 4th lane (w) is
** unused padding, always kept at 0 so it never pollutes a dot product.
** With -O3 -ffast-math (release build) gcc/clang auto-vectorize these
** operators; no hand-written intrinsics needed to get the SIMD win, which
** keeps the code portable across x86/ARM.
**
** This layout (four floats, 16-byte aligned) is also exactly a GLSL/HLSL
** vec4 under std430 rules, so under GPU_COMPUTING_COMPATIBILITY Vec3 is
** already GPU-storage-buffer-ready as-is - no separate GPUVec4 wrapper or
** toGPU() needed, unlike Mat3/Transform/Material/Light/Camera below, whose
** CPU layouts don't already match std430 and so get their own GPU mirrors.
*/
struct alignas(16) Vec3
{
	float x;
	float y;
	float z;
	float w;

	constexpr Vec3() : x(0), y(0), z(0), w(0) {}
	constexpr Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_), w(0) {}

	Vec3	operator+(const Vec3 &o) const { return Vec3(x + o.x, y + o.y, z + o.z); }
	Vec3	operator-(const Vec3 &o) const { return Vec3(x - o.x, y - o.y, z - o.z); }
	Vec3	operator-() const { return Vec3(-x, -y, -z); }
	Vec3	operator*(float s) const { return Vec3(x * s, y * s, z * s); }
	Vec3	operator/(float s) const { return Vec3(x / s, y / s, z / s); }
	/* component-wise product, used to modulate light color by object color */
	Vec3	operator*(const Vec3 &o) const { return Vec3(x * o.x, y * o.y, z * o.z); }

	Vec3	&operator+=(const Vec3 &o) { x += o.x; y += o.y; z += o.z; return *this; }

	float	dot(const Vec3 &o) const { return x * o.x + y * o.y + z * o.z; }
	Vec3	cross(const Vec3 &o) const
	{
		return Vec3(y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x);
	}

	float	lengthSquared() const { return dot(*this); }
	float	length() const { return std::sqrt(lengthSquared()); }

	Vec3	normalized() const
	{
		float len = length();
		if (len <= 1e-8f)
			return Vec3(0, 0, 0);
		return *this / len;
	}

	Vec3	reflect(const Vec3 &normal) const
	{
		return *this - normal * (2.0f * dot(normal));
	}

	static Vec3	clamp01(const Vec3 &v)
	{
		auto c = [](float f) { return f < 0.0f ? 0.0f : (f > 1.0f ? 1.0f : f); };
		return Vec3(c(v.x), c(v.y), c(v.z));
	}
};

inline Vec3	operator*(float s, const Vec3 &v) { return v * s; }

#endif
