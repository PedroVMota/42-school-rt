#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

#include "math/Vec3.hpp"
#include "math/Mat3.hpp"
#include "core/Ray.hpp"

/*
** Every primitive is defined in its own canonical local space (sphere at the
** origin, cylinder/cone along the local Y axis, plane through the origin).
** Translation + rotation move it into world space. Rather than transforming
** every primitive's surface equation, we transform the incoming ray into
** local space (cheap: one matrix-vector product), intersect the canonical
** shape, then transform the resulting point/normal back to world space.
** This is the standard ray tracing trick and is what makes "translate a
** sphere from (0,0,0) to (42,42,42)" trivial and uniform across primitives.
*/
class Transform
{
	public:
		Transform() : _translation(0, 0, 0), _rotation(Mat3::identity()), _rotationInv(Mat3::identity()) {}

		void	setTranslation(const Vec3 &t) { _translation = t; }
		void	setRotationEulerXYZ(float rx, float ry, float rz)
		{
			_rotation = Mat3::fromEulerXYZ(rx, ry, rz);
			_rotationInv = _rotation.transposed();
		}

		const Vec3	&translation() const { return _translation; }

		Ray	toLocal(const Ray &world) const
		{
			Vec3 localOrigin = _rotationInv * (world.origin - _translation);
			Vec3 localDir = _rotationInv * world.dir;
			return Ray(localOrigin, localDir);
		}

		Vec3	pointToWorld(const Vec3 &local) const
		{
			return _rotation * local + _translation;
		}

		Vec3	normalToWorld(const Vec3 &localNormal) const
		{
			return (_rotation * localNormal).normalized();
		}

	private:
		Vec3	_translation;
		Mat3	_rotation;
		Mat3	_rotationInv;
};

#endif
