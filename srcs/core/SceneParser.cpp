#include "core/SceneParser.hpp"
#include "objects/Sphere.hpp"
#include "objects/Plane.hpp"
#include "objects/Cylinder.hpp"
#include "objects/Cone.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <cmath>

namespace
{
	std::vector<std::string>	splitOn(const std::string &s, char sep)
	{
		std::vector<std::string>	parts;
		std::stringstream			ss(s);
		std::string					item;

		while (std::getline(ss, item, sep))
			if (!item.empty())
				parts.push_back(item);
		return parts;
	}

	std::vector<std::string>	splitWhitespace(const std::string &s)
	{
		std::vector<std::string>	parts;
		std::istringstream			iss(s);
		std::string					item;

		while (iss >> item)
			parts.push_back(item);
		return parts;
	}

	[[noreturn]] void	fail(int lineNo, const std::string &msg)
	{
		throw std::runtime_error("scene file, line " + std::to_string(lineNo) + ": " + msg);
	}

	float	toFloat(const std::string &tok, int lineNo, const std::string &field)
	{
		try
		{
			size_t	consumed;
			float	v = std::stof(tok, &consumed);
			if (consumed != tok.size())
				throw std::invalid_argument("trailing characters");
			return v;
		}
		catch (const std::exception &)
		{
			fail(lineNo, "invalid number for " + field + ": '" + tok + "'");
		}
	}

	int	toChannel(const std::string &tok, int lineNo)
	{
		try
		{
			size_t	consumed;
			int		v = std::stoi(tok, &consumed);
			if (consumed != tok.size() || v < 0 || v > 255)
				throw std::invalid_argument("out of range");
			return v;
		}
		catch (const std::exception &)
		{
			fail(lineNo, "invalid colour channel: '" + tok + "' (expected 0-255)");
		}
	}

	Vec3	parseVec3(const std::string &tok, int lineNo, const std::string &field)
	{
		std::vector<std::string>	parts = splitOn(tok, ',');
		if (parts.size() != 3)
			fail(lineNo, field + " must be 3 comma-separated numbers, got '" + tok + "'");
		return Vec3(toFloat(parts[0], lineNo, field),
					toFloat(parts[1], lineNo, field),
					toFloat(parts[2], lineNo, field));
	}

	Vec3	parseColor(const std::string &tok, int lineNo)
	{
		std::vector<std::string>	parts = splitOn(tok, ',');
		if (parts.size() != 3)
			fail(lineNo, "colour must be 3 comma-separated values 0-255, got '" + tok + "'");
		return Vec3(toChannel(parts[0], lineNo) / 255.0f,
					toChannel(parts[1], lineNo) / 255.0f,
					toChannel(parts[2], lineNo) / 255.0f);
	}

	Vec3	parseUnitish(const std::string &tok, int lineNo, const std::string &field)
	{
		Vec3	v = parseVec3(tok, lineNo, field);
		auto	inRange = [](float f) { return f >= -1.0f - 1e-3f && f <= 1.0f + 1e-3f; };

		if (!inRange(v.x) || !inRange(v.y) || !inRange(v.z))
			fail(lineNo, field + " components must be in [-1, 1]");
		if (v.lengthSquared() < 1e-8f)
			fail(lineNo, field + " cannot be the zero vector");
		return v;
	}

	float	toRatio(const std::string &tok, int lineNo, const std::string &field)
	{
		float	v = toFloat(tok, lineNo, field);
		if (v < 0.0f || v > 1.0f)
			fail(lineNo, field + " must be in [0, 1], got " + tok);
		return v;
	}

	void	requireFields(const std::vector<std::string> &tokens, size_t expected, int lineNo, const std::string &id)
	{
		if (tokens.size() != expected)
			fail(lineNo, "'" + id + "' expects " + std::to_string(expected - 1) +
				" fields, got " + std::to_string(tokens.size() - 1));
	}
}

Scene	SceneParser::parseFile(const std::string &path, int width, int height, Camera &outCamera)
{
	std::ifstream	file(path);
	if (!file)
		throw std::runtime_error("could not open scene file: " + path);

	Scene		scene;
	std::string	line;
	int			lineNo = 0;
	bool		haveCamera = false;
	bool		haveAmbient = false;
	Vec3		camPos, camDir;
	float		camFov = 60.0f;

	while (std::getline(file, line))
	{
		++lineNo;

		size_t	hash = line.find('#');
		if (hash != std::string::npos)
			line = line.substr(0, hash);

		std::vector<std::string>	tokens = splitWhitespace(line);
		if (tokens.empty())
			continue;

		const std::string	&id = tokens[0];

		if (id == "A")
		{
			if (haveAmbient)
				fail(lineNo, "duplicate ambient (A) declaration");
			requireFields(tokens, 3, lineNo, id);
			scene.ambientBrightness = toRatio(tokens[1], lineNo, "ambient ratio");
			scene.ambientColor = parseColor(tokens[2], lineNo);
			haveAmbient = true;
		}
		else if (id == "C")
		{
			if (haveCamera)
				fail(lineNo, "duplicate camera (C) declaration");
			requireFields(tokens, 4, lineNo, id);
			camPos = parseVec3(tokens[1], lineNo, "camera position");
			camDir = parseUnitish(tokens[2], lineNo, "camera direction");
			camFov = toFloat(tokens[3], lineNo, "field of view");
			if (camFov <= 0.0f || camFov >= 180.0f)
				fail(lineNo, "field of view must be in ]0, 180[, got " + tokens[3]);
			haveCamera = true;
		}
		else if (id == "L")
		{
			if (tokens.size() != 3 && tokens.size() != 4)
				fail(lineNo, "'L' expects 2 or 3 fields, got " + std::to_string(tokens.size() - 1));
			Light	light;
			light.position = parseVec3(tokens[1], lineNo, "light position");
			light.brightness = toRatio(tokens[2], lineNo, "light brightness");
			light.color = (tokens.size() == 4) ? parseColor(tokens[3], lineNo) : Vec3(1, 1, 1);
			scene.lights.push_back(light);
		}
		else if (id == "sp")
		{
			requireFields(tokens, 4, lineNo, id);
			Vec3	pos = parseVec3(tokens[1], lineNo, "sphere position");
			float	diameter = toFloat(tokens[2], lineNo, "sphere diameter");
			if (diameter <= 0.0f)
				fail(lineNo, "sphere diameter must be > 0");
			Material	mat;
			mat.color = parseColor(tokens[3], lineNo);
			auto	sphere = std::make_unique<Sphere>(diameter * 0.5f, mat);
			sphere->transform.setTranslation(pos);
			scene.objects.push_back(std::move(sphere));
		}
		else if (id == "pl")
		{
			requireFields(tokens, 4, lineNo, id);
			Vec3	pos = parseVec3(tokens[1], lineNo, "plane position");
			Vec3	normal = parseUnitish(tokens[2], lineNo, "plane normal");
			Material	mat;
			mat.color = parseColor(tokens[3], lineNo);
			auto	plane = std::make_unique<Plane>(mat);
			plane->transform.setTranslation(pos);
			plane->transform.setRotation(Mat3::fromToRotation(Vec3(0, 1, 0), normal));
			scene.objects.push_back(std::move(plane));
		}
		else if (id == "cy" || id == "co")
		{
			requireFields(tokens, 6, lineNo, id);
			Vec3	pos = parseVec3(tokens[1], lineNo, id == "cy" ? "cylinder position" : "cone apex");
			Vec3	axis = parseUnitish(tokens[2], lineNo, "axis vector");
			float	diameter = toFloat(tokens[3], lineNo, "diameter");
			float	height = toFloat(tokens[4], lineNo, "height");
			if (diameter <= 0.0f)
				fail(lineNo, "diameter must be > 0");
			if (height <= 0.0f)
				fail(lineNo, "height must be > 0");
			Material	mat;
			mat.color = parseColor(tokens[5], lineNo);

			std::unique_ptr<Object>	obj;
			if (id == "cy")
				obj = std::make_unique<Cylinder>(diameter * 0.5f, height, mat);
			else
				obj = std::make_unique<Cone>(diameter * 0.5f, height, mat);
			obj->transform.setTranslation(pos);
			obj->transform.setRotation(Mat3::fromToRotation(Vec3(0, 1, 0), axis));
			scene.objects.push_back(std::move(obj));
		}
		else
			fail(lineNo, "unknown element identifier '" + id + "'");
	}

	if (!haveCamera)
		throw std::runtime_error("scene file missing required camera (C) declaration");
	if (!haveAmbient)
		throw std::runtime_error("scene file missing required ambient light (A) declaration");

	outCamera.setup(camPos, camDir, camFov, width, height);
	return scene;
}
