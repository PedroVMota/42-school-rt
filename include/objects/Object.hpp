#ifndef OBJECT_HPP
#define OBJECT_HPP

#include "core/Ray.hpp"
#include "core/HitRecord.hpp"
#include "core/Material.hpp"
#include "core/Transform.hpp"

class Object
{
	public:
		explicit Object(const Material &mat) : material(mat) {}
		virtual ~Object() = default;

		/*
		** Returns true and fills `rec` if the ray hits the object with
		** t in (tMin, tMax). Implementations work in local space (via
		** transform.toLocal) and convert the result back to world space
		** before returning, so every primitive shares the exact same
		** transform pipeline.
		*/
		virtual bool	hit(const Ray &worldRay, float tMin, float tMax, HitRecord &rec) const = 0;

		Transform	transform;
		Material	material;
};

#endif
