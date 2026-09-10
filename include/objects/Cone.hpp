#ifndef CONE_HPP
#define CONE_HPP

#include "objects/Object.hpp"

/* Finite, capped cone: local apex at the origin, axis = +Y, base radius
** `radius` at y = height (a disk cap closes it off there). */
class Cone : public Object
{
	public:
		Cone(float radius, float height, const Material &mat)
			: Object(mat), _radius(radius), _height(height) {}

		bool	hit(const Ray &worldRay, float tMin, float tMax, HitRecord &rec) const override;

#ifdef GPU_COMPUTING_COMPATIBILITY
		Object::GPUPrimitive	toGPU() const override;
#endif

	private:
		float	_radius;
		float	_height;
};

#endif
