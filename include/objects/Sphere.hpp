#ifndef SPHERE_HPP
#define SPHERE_HPP

#include "objects/Object.hpp"

/* Sphere centered on the local origin. */
class Sphere : public Object
{
	public:
		Sphere(float radius, const Material &mat) : Object(mat), _radius(radius) {}

		bool	hit(const Ray &worldRay, float tMin, float tMax, HitRecord &rec) const override;

	private:
		float	_radius;
};

#endif
