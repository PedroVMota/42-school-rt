#include "core/Scene.hpp"
#include "objects/Plane.hpp"
#include "objects/Sphere.hpp"
#include "objects/Cylinder.hpp"
#include "objects/Cone.hpp"

Scene	Scene::buildDefault(int width, int height, Camera &outCamera)
{
	Scene scene;

	scene.ambientBrightness = 0.12f;
	scene.ambientColor = Vec3(1, 1, 1);

	/* Ground plane. */
	{
		Material mat;
		mat.color = Vec3(0.55f, 0.55f, 0.6f);
		mat.specular = 0.1f;
		mat.shininess = 8.0f;
		auto plane = std::make_unique<Plane>(mat);
		plane->transform.setTranslation(Vec3(0, -1.0f, 0));
		scene.objects.push_back(std::move(plane));
	}

	/* Sphere, slightly translated off the origin (translation requirement). */
	{
		Material mat;
		mat.color = Vec3(0.9f, 0.2f, 0.2f);
		mat.specular = 0.6f;
		mat.shininess = 64.0f;
		auto sphere = std::make_unique<Sphere>(1.0f, mat);
		sphere->transform.setTranslation(Vec3(-2.2f, 0.0f, 0.0f));
		scene.objects.push_back(std::move(sphere));
	}

	/* Cylinder, rotated on its side to show the rotation requirement. */
	{
		Material mat;
		mat.color = Vec3(0.2f, 0.7f, 0.3f);
		mat.specular = 0.4f;
		mat.shininess = 32.0f;
		auto cyl = std::make_unique<Cylinder>(0.7f, 2.0f, mat);
		cyl->transform.setTranslation(Vec3(0.3f, -0.2f, 0.0f));
		cyl->transform.setRotationEulerXYZ(0.0f, 0.0f, (float)M_PI / 2.4f);
		scene.objects.push_back(std::move(cyl));
	}

	/* Cone. */
	{
		Material mat;
		mat.color = Vec3(0.25f, 0.4f, 0.9f);
		mat.specular = 0.5f;
		mat.shininess = 48.0f;
		auto cone = std::make_unique<Cone>(0.8f, 1.8f, mat);
		cone->transform.setTranslation(Vec3(2.4f, -1.0f, -0.5f));
		scene.objects.push_back(std::move(cone));
	}

	/* Two spot lights so shadows overlap/mix (Fig VI.3 style scene). */
	Light key;
	key.position = Vec3(-4.0f, 5.0f, -3.0f);
	key.color = Vec3(1.0f, 1.0f, 0.95f);
	key.brightness = 1.0f;
	scene.lights.push_back(key);

	Light fill;
	fill.position = Vec3(5.0f, 3.0f, 2.0f);
	fill.color = Vec3(0.6f, 0.7f, 1.0f);
	fill.brightness = 0.6f;
	scene.lights.push_back(fill);

	outCamera.setup(Vec3(0.0f, 1.5f, 8.0f), Vec3(0.0f, -0.15f, -1.0f), 60.0f, width, height);

	return scene;
}
