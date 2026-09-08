#ifndef HITRECORD_HPP
#define HITRECORD_HPP

#include "math/Vec3.hpp"
#include "core/Material.hpp"

struct HitRecord
{
	float		t = 0.0f;
	Vec3		point;
	Vec3		normal;   /* world space, unit length, faces the ray origin */
	Material	material;
};

#endif
