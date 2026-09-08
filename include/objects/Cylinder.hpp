#ifndef CYLINDER_HPP
#define CYLINDER_HPP

#include "objects/Object.hpp"

/* Finite, capped cylinder: local axis = Y, centered on the origin,
** extends from y = -height/2 to y = +height/2. */
class Cylinder : public Object
{
	public:
		Cylinder(float radius, float height, const Material &mat)
			: Object(mat), _radius(radius), _halfHeight(height * 0.5f) {}

		bool	hit(const Ray &worldRay, float tMin, float tMax, HitRecord &rec) const override;

	private:
		float	_radius;
		float	_halfHeight;
};

#endif
