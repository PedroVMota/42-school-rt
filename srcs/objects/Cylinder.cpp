#include "objects/Cylinder.hpp"
#include <cmath>

bool	Cylinder::hit(const Ray &worldRay, float tMin, float tMax, HitRecord &rec) const
{
	Ray local = transform.toLocal(worldRay);

	bool	found = false;
	float	bestT = tMax;
	Vec3	bestLocalPoint, bestLocalNormal;

	/* Side surface: x^2 + z^2 = r^2, restricted to |y| <= halfHeight. */
	float a = local.dir.x * local.dir.x + local.dir.z * local.dir.z;
	if (a > 1e-8f)
	{
		float b = 2.0f * (local.origin.x * local.dir.x + local.origin.z * local.dir.z);
		float c = local.origin.x * local.origin.x + local.origin.z * local.origin.z - _radius * _radius;
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
				if (y < -_halfHeight || y > _halfHeight)
					continue;
				bestT = t;
				bestLocalPoint = local.at(t);
				bestLocalNormal = Vec3(bestLocalPoint.x, 0.0f, bestLocalPoint.z) / _radius;
				found = true;
			}
		}
	}

	/* Top/bottom caps: y = +-halfHeight disks of radius r. */
	if (std::fabs(local.dir.y) > 1e-8f)
	{
		float capsY[2] = { _halfHeight, -_halfHeight };
		for (float capY : capsY)
		{
			float t = (capY - local.origin.y) / local.dir.y;
			if (t <= tMin || t >= bestT)
				continue;
			Vec3 p = local.at(t);
			if (p.x * p.x + p.z * p.z > _radius * _radius)
				continue;
			bestT = t;
			bestLocalPoint = p;
			bestLocalNormal = Vec3(0.0f, capY > 0.0f ? 1.0f : -1.0f, 0.0f);
			found = true;
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
