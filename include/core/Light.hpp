#ifndef LIGHT_HPP
#define LIGHT_HPP

#include "math/Vec3.hpp"

/* A point light ("spot" in the subject's wording): position + color +
** brightness. Multiple instances in Scene give the "multi-spot" behaviour. */
struct Light
{
	Vec3	position;
	Vec3	color      = Vec3(1, 1, 1);
	float	brightness = 1.0f; /* variable brightness requirement */

#ifdef GPU_COMPUTING_COMPATIBILITY
	/* Lights are uploaded as an array (one storage buffer element per
	** light), and std430 requires an array's element stride to be a
	** multiple of the element's own 16-byte base alignment. Unpadded this
	** struct is 36 bytes (two vec4s + one float), so 12 bytes of explicit
	** tail padding round it up to the required 48. */
	struct GPULight
	{
		Vec3	position;
		Vec3	color;
		float	brightness;
		float	_pad[3];
	};

	GPULight	toGPU() const
	{
		return GPULight{ position, color, brightness, { 0.0f, 0.0f, 0.0f } };
	}
#endif
};

#endif
