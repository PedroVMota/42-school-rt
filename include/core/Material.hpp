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

#ifdef GPU_COMPUTING_COMPATIBILITY
	/* color's 16-byte vec4 base alignment already puts the 4 trailing
	** scalars on a clean 4-byte boundary with no gap, so this needs no
	** extra padding to be std430-safe (32 bytes, a multiple of color's
	** 16-byte alignment). */
	struct GPUMaterial
	{
		Vec3	color;
		float	diffuse;
		float	specular;
		float	shininess;
		float	reflectivity;
	};

	GPUMaterial	toGPU() const
	{
		return GPUMaterial{ color, diffuse, specular, shininess, reflectivity };
	}
#endif
};

#endif
