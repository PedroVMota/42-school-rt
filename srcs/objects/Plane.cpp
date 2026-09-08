#include "objects/Plane.hpp"
#include <cmath>

bool	Plane::hit(const Ray &worldRay, float tMin, float tMax, HitRecord &rec) const
{
	Ray local = transform.toLocal(worldRay);

	/* Plane: y = 0, normal = (0,1,0). Ray misses if it runs parallel to it. */
	if (std::fabs(local.dir.y) < 1e-6f)
		return false;

	float t = -local.origin.y / local.dir.y;
	if (t <= tMin || t >= tMax)
		return false;

	Vec3 localPoint = local.at(t);
	Vec3 localNormal(0, 1, 0);
	if (local.dir.dot(localNormal) > 0.0f)
		localNormal = -localNormal;

	rec.t = t;
	rec.point = transform.pointToWorld(localPoint);
	rec.normal = transform.normalToWorld(localNormal);
	rec.material = material;
	return true;
}
