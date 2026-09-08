#ifndef SCENE_HPP
#define SCENE_HPP

#include <vector>
#include <memory>
#include "objects/Object.hpp"
#include "core/Light.hpp"
#include "core/Camera.hpp"

class Scene
{
	public:
		std::vector<std::unique_ptr<Object>>	objects;
		std::vector<Light>						lights;
		float									ambientBrightness = 0.1f;
		Vec3									ambientColor = Vec3(1, 1, 1);

		/* Closest-hit query used both for primary rays and shadow rays. */
		bool	trace(const Ray &ray, float tMin, float tMax, HitRecord &rec) const
		{
			bool	hitAnything = false;
			float	closest = tMax;
			HitRecord	tmp;

			for (const auto &obj : objects)
			{
				if (obj->hit(ray, tMin, closest, tmp))
				{
					hitAnything = true;
					closest = tmp.t;
					rec = tmp;
				}
			}
			return hitAnything;
		}

		/* True if anything blocks the [point, light] segment (shadow test). */
		bool	isOccluded(const Vec3 &point, const Vec3 &lightPos) const
		{
			Vec3 toLight = lightPos - point;
			float distance = toLight.length();
			if (distance < 1e-6f)
				return false;
			Ray shadowRay(point, toLight / distance);
			HitRecord tmp;
			/* small epsilon offset avoids self-shadowing ("shadow acne") */
			return trace(shadowRay, 1e-3f, distance - 1e-3f, tmp);
		}

		/* Builds the reference scene from the subject: 4 primitives, 2 lights. */
		static Scene	buildDefault(int width, int height, Camera &outCamera);
};

#endif
