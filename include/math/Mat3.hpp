#ifndef MAT3_HPP
#define MAT3_HPP

#include "math/Vec3.hpp"
#include <cmath>

#ifndef M_PI
# define M_PI 3.14159265358979323846
#endif

/*
** Plain 3x3 rotation matrix, row-major (m[row][col]).
** Objects only ever need translation + rotation (subject requires no
** scaling), so a 3x3 orthonormal matrix plus a translation Vec3 is enough:
** no homogeneous 4x4 / projective machinery required.
*/
struct Mat3
{
	float	m[3][3];

	static Mat3	identity()
	{
		Mat3 r{};
		r.m[0][0] = 1; r.m[1][1] = 1; r.m[2][2] = 1;
		return r;
	}

	static Mat3	rotationX(float rad)
	{
		Mat3 r = identity();
		float c = std::cos(rad), s = std::sin(rad);
		r.m[1][1] = c; r.m[1][2] = -s;
		r.m[2][1] = s; r.m[2][2] = c;
		return r;
	}

	static Mat3	rotationY(float rad)
	{
		Mat3 r = identity();
		float c = std::cos(rad), s = std::sin(rad);
		r.m[0][0] = c; r.m[0][2] = s;
		r.m[2][0] = -s; r.m[2][2] = c;
		return r;
	}

	static Mat3	rotationZ(float rad)
	{
		Mat3 r = identity();
		float c = std::cos(rad), s = std::sin(rad);
		r.m[0][0] = c; r.m[0][1] = -s;
		r.m[1][0] = s; r.m[1][1] = c;
		return r;
	}

	/* Combined intrinsic rotation, applied Z then Y then X (yaw/pitch/roll
	** style), matching how the scene-description Euler angles are read. */
	static Mat3	fromEulerXYZ(float rx, float ry, float rz)
	{
		return rotationX(rx) * rotationY(ry) * rotationZ(rz);
	}

	/* Rodrigues' rotation formula: rotate by `angleRad` around a unit axis. */
	static Mat3	rotationAxisAngle(const Vec3 &axisUnit, float angleRad)
	{
		float c = std::cos(angleRad), s = std::sin(angleRad), t = 1.0f - c;
		float x = axisUnit.x, y = axisUnit.y, z = axisUnit.z;
		Mat3 r{};
		r.m[0][0] = t * x * x + c;     r.m[0][1] = t * x * y - s * z; r.m[0][2] = t * x * z + s * y;
		r.m[1][0] = t * x * y + s * z; r.m[1][1] = t * y * y + c;     r.m[1][2] = t * y * z - s * x;
		r.m[2][0] = t * x * z - s * y; r.m[2][1] = t * y * z + s * x; r.m[2][2] = t * z * z + c;
		return r;
	}

	/* Rotation that takes unit vector `from` onto unit vector `to`; used to
	** orient a primitive whose scene-file declaration gives an axis/normal
	** vector instead of Euler angles (e.g. cylinder axis, plane normal). */
	static Mat3	fromToRotation(const Vec3 &from, const Vec3 &to)
	{
		Vec3 f = from.normalized();
		Vec3 t = to.normalized();
		float c = f.dot(t);
		if (c > 1.0f - 1e-6f)
			return identity();
		if (c < -1.0f + 1e-6f)
		{
			/* 180 degrees: cross product is zero, so pick any axis
			** perpendicular to `f` instead. */
			Vec3 axis = (std::fabs(f.x) < 0.9f) ? Vec3(1, 0, 0) : Vec3(0, 1, 0);
			axis = (axis - f * f.dot(axis)).normalized();
			return rotationAxisAngle(axis, (float)M_PI);
		}
		Vec3 axis = f.cross(t).normalized();
		float angle = std::acos(c < -1.0f ? -1.0f : (c > 1.0f ? 1.0f : c));
		return rotationAxisAngle(axis, angle);
	}

	Mat3	operator*(const Mat3 &o) const
	{
		Mat3 r{};
		for (int i = 0; i < 3; ++i)
			for (int j = 0; j < 3; ++j)
				r.m[i][j] = m[i][0] * o.m[0][j] + m[i][1] * o.m[1][j] + m[i][2] * o.m[2][j];
		return r;
	}

	Vec3	operator*(const Vec3 &v) const
	{
		return Vec3(
			m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z,
			m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z,
			m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z);
	}

	/* For an orthonormal rotation matrix, inverse == transpose. */
	Mat3	transposed() const
	{
		Mat3 r{};
		for (int i = 0; i < 3; ++i)
			for (int j = 0; j < 3; ++j)
				r.m[i][j] = m[j][i];
		return r;
	}
};

#endif
