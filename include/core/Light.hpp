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
};

#endif
