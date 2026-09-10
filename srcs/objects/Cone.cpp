#include "objects/Cone.hpp"
#include <cmath>

bool	Cone::hit(const Ray &worldRay, float tMin, float tMax, HitRecord &rec) const
{
	Ray local = transform.toLocal(worldRay);

	bool	found = false;
	float	bestT = tMax;
	Vec3	bestLocalPoint, bestLocalNormal;

	/* Side surface: x^2 + z^2 = k^2 * y^2, k = radius / height, y in [0, height]. */
	float k = _radius / _height;
	float k2 = k * k;
	float a = local.dir.x * local.dir.x + local.dir.z * local.dir.z - k2 * local.dir.y * local.dir.y;
	float b = 2.0f * (local.origin.x * local.dir.x + local.origin.z * local.dir.z
			- k2 * local.origin.y * local.dir.y);
	float c = local.origin.x * local.origin.x + local.origin.z * local.origin.z
			- k2 * local.origin.y * local.origin.y;

	if (std::fabs(a) > 1e-8f)
	{
		float discriminant = b * b - 4.0f * a * c;
		if (discriminant >= 0.0f)
		{
			float sqrtD = std::sqrt(discriminant);
			float roots[2] = { (-b - sqrtD) / (2.0f * a), (-b + sqrtD) / (2.0f * a) };
			for (float t : roots)
			{
				if (t <= tMin || t >= bestT)
					continue;
				float y = local.origin.y + t * local.dir.y;
				if (y < 0.0f || y > _height)
					continue;
				bestT = t;
				bestLocalPoint = local.at(t);
				bestLocalNormal = Vec3(bestLocalPoint.x, -k2 * bestLocalPoint.y, bestLocalPoint.z).normalized();
				found = true;
			}
		}
	}
	else if (std::fabs(b) > 1e-8f)
	{
		float t = -c / b;
		if (t > tMin && t < bestT)
		{
			float y = local.origin.y + t * local.dir.y;
			if (y >= 0.0f && y <= _height)
			{
				bestT = t;
				bestLocalPoint = local.at(t);
				bestLocalNormal = Vec3(bestLocalPoint.x, -k2 * bestLocalPoint.y, bestLocalPoint.z).normalized();
				found = true;
			}
		}
	}

	/* Base cap: disk of radius `radius` at y = height. */
	if (std::fabs(local.dir.y) > 1e-8f)
	{
		float t = (_height - local.origin.y) / local.dir.y;
		if (t > tMin && t < bestT)
		{
			Vec3 p = local.at(t);
			if (p.x * p.x + p.z * p.z <= _radius * _radius)
			{
				bestT = t;
				bestLocalPoint = p;
				bestLocalNormal = Vec3(0.0f, 1.0f, 0.0f);
				found = true;
			}
		}
	}

	if (!found)
		return false;

	if (local.dir.dot(bestLocalNormal) > 0.0f)
		bestLocalNormal = -bestLocalNormal;

	rec.t = bestT;
	rec.point = transform.pointToWorld(bestLocalPoint);
	rec.normal = transform.normalToWorld(bestLocalNormal);
	rec.material = material;
	return true;
}

#ifdef GPU_COMPUTING_COMPATIBILITY
/* params[0] = radius, params[1] = height, matching the k = radius/height
** side-surface equation and the [0, height] / base-cap tests hit() runs
** above. */
Object::GPUPrimitive	Cone::toGPU() const
{
	Object::GPUPrimitive	p = packGPU(GPU_PRIMITIVE_CONE);

	p.params[0] = _radius;
	p.params[1] = _height;
	return p;
}
#endif
