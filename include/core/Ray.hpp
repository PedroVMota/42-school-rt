#ifndef RAY_HPP
#define RAY_HPP

#include "math/Vec3.hpp"

struct Ray
{
	Vec3	origin;
	Vec3	dir; /* must be kept normalized */

	Ray() = default;
	Ray(const Vec3 &o, const Vec3 &d) : origin(o), dir(d) {}

	Vec3	at(float t) const { return origin + dir * t; }
};

#endif
