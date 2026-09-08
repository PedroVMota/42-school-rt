#ifndef MAT3_HPP
#define MAT3_HPP

#include "math/Vec3.hpp"
#include <cmath>

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
