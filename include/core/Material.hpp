#ifndef MATERIAL_HPP
#define MATERIAL_HPP

#include "math/Vec3.hpp"

/* Blinn-Phong material: enough for diffuse color + specular "shine". */
struct Material
{
	Vec3	color        = Vec3(1, 1, 1);
	float	diffuse      = 0.8f;
	float	specular     = 0.4f;
	float	shininess    = 32.0f;
	float	reflectivity = 0.0f; /* 0 = matte, 1 = perfect mirror */
};

#endif
