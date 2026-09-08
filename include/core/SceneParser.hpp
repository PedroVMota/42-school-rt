#ifndef SCENE_PARSER_HPP
#define SCENE_PARSER_HPP

#include "core/Scene.hpp"
#include "core/Camera.hpp"
#include <string>

/*
** Parses the miniRT-style ".rt" scene description format: one element per
** line, comments starting with '#' and blank lines ignored.
**
**   A  ratio               R,G,B          ambient light (ratio in [0,1])
**   C  x,y,z  dx,dy,dz     fov            camera (fov in degrees, ]0,180[)
**   L  x,y,z  brightness   [R,G,B]        point light (brightness in [0,1])
**   sp x,y,z  diameter     R,G,B  [reflectivity]         sphere
**   pl x,y,z  dx,dy,dz     R,G,B  [reflectivity]         plane (point + normal)
**   cy x,y,z  dx,dy,dz  diameter  height  R,G,B  [reflectivity]  cylinder (center + axis)
**   co x,y,z  dx,dy,dz  diameter  height  R,G,B  [reflectivity]  cone (apex + axis toward base)
**
** Exactly one A and one C line are required. Vectors are "x,y,z" (no
** spaces), colors are "r,g,b" with each channel in [0,255]. The trailing
** reflectivity is optional and defaults to 0 (matte); 0-1, where 1 is a
** perfect mirror.
*/
class SceneParser
{
	public:
		/* Throws std::runtime_error, with a message naming the offending
		** line, on any malformed input. */
		static Scene	parseFile(const std::string &path, int width, int height, Camera &outCamera);
};

#endif
