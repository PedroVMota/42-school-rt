#ifndef PLANE_HPP
#define PLANE_HPP

#include "objects/Object.hpp"

/* Infinite plane through the local origin, local normal = (0, 1, 0). */
class Plane : public Object
{
	public:
		explicit Plane(const Material &mat) : Object(mat) {}

		bool	hit(const Ray &worldRay, float tMin, float tMax, HitRecord &rec) const override;

#ifdef GPU_COMPUTING_COMPATIBILITY
		Object::GPUPrimitive	toGPU() const override;
#endif
};

#endif
