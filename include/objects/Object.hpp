#ifndef OBJECT_HPP
#define OBJECT_HPP

#include "core/Ray.hpp"
#include "core/HitRecord.hpp"
#include "core/Material.hpp"
#include "core/Transform.hpp"

#ifdef GPU_COMPUTING_COMPATIBILITY
	#include <cstdint>
#endif

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

#ifdef GPU_COMPUTING_COMPATIBILITY
		/* Tags which local-space intersection routine the compute shader
		** should run for a given GPUPrimitive - mirrors the dynamic dispatch
		** hit() already gets for free from the vtable on the CPU side. */
		enum : uint32_t
		{
			GPU_PRIMITIVE_SPHERE   = 0,
			GPU_PRIMITIVE_PLANE    = 1,
			GPU_PRIMITIVE_CYLINDER = 2,
			GPU_PRIMITIVE_CONE     = 3,
		};

		/*
		** One of these per primitive instance, uploaded as an array to the
		** primitive storage buffer the compute shader loops over (see
		** Scene::buildGPUBuffers, Phase 7). `params` holds whatever scalars
		** a given primitive kind needs on top of the shared transform and
		** material - see each subclass's toGPU() for what it puts there.
		** `type`'s 16-byte pad exists because `transform`'s GPUTransform
		** (see Transform.hpp) has a 16-byte base alignment under std430 and
		** so must start on a 16-byte boundary.
		*/
		struct GPUPrimitive
		{
			uint32_t					type;
			uint32_t					_pad0[3];
			Transform::GPUTransform	transform;
			Material::GPUMaterial		material;
			float						params[4];
		};

		virtual GPUPrimitive	toGPU() const = 0;

	protected:
		/* Shared by every subclass's toGPU(): fills the type tag plus the
		** transform/material every primitive carries regardless of kind,
		** leaving `params` zero-initialized for the caller to fill in. */
		GPUPrimitive	packGPU(uint32_t type) const
		{
			GPUPrimitive	p{};

			p.type = type;
			p.transform = transform.toGPU();
			p.material = material.toGPU();
			return p;
		}
#endif
};

#endif
