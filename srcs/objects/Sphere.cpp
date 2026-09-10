#include "objects/Sphere.hpp"
#include <cmath>

bool	Sphere::hit(const Ray &worldRay, float tMin, float tMax, HitRecord &rec) const
{
	Ray local = transform.toLocal(worldRay);

	/* |O + tD|^2 = r^2  =>  quadratic in t. `local.dir` is not necessarily
	** unit length after an arbitrary rotation of a unit vector it still is
	** (rotation preserves length), so `a` below is always ~1 but we keep
	** it general and robust. */
	Vec3 oc = local.origin;
	float a = local.dir.dot(local.dir);
	float halfB = oc.dot(local.dir);
	float c = oc.dot(oc) - _radius * _radius;
	float discriminant = halfB * halfB - a * c;
	if (discriminant < 0.0f)
		return false;

	float sqrtD = std::sqrt(discriminant);
	float t = (-halfB - sqrtD) / a;
	if (t <= tMin || t >= tMax)
	{
		t = (-halfB + sqrtD) / a;
		if (t <= tMin || t >= tMax)
			return false;
	}

	Vec3 localPoint = local.at(t);
	Vec3 localNormal = localPoint / _radius;

	rec.t = t;
	rec.point = transform.pointToWorld(localPoint);
	rec.normal = transform.normalToWorld(localNormal);
	rec.material = material;
	return true;
}

#ifdef GPU_COMPUTING_COMPATIBILITY
/* params[0] = radius, matching the |local point|^2 = radius^2 test hit()
** solves above. */
Object::GPUPrimitive	Sphere::toGPU() const
{
	Object::GPUPrimitive	p = packGPU(GPU_PRIMITIVE_SPHERE);

	p.params[0] = _radius;
	return p;
}
#endif
