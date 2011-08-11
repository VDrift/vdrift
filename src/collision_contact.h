#ifndef _COLLISION_CONTACT_H
#define _COLLISION_CONTACT_H

#include "LinearMath/btVector3.h"
#include "tracksurface.h"

class BEZIER;
class btCollisionObject;

class COLLISION_CONTACT
{
public:
	COLLISION_CONTACT() :
		depth(0),
		patchid(-1),
		patch(0),
		surface(TRACKSURFACE::None()),
		col(0)
	{
		// ctor
	}

	COLLISION_CONTACT(
		const btVector3 & p,
		const btVector3 & n,
		const btScalar d,
		const int i,
		const BEZIER * b,
		const TRACKSURFACE * s,
		const btCollisionObject * c) :
		position(p),
		normal(n),
		depth(d),
		patchid(i),
		patch(b),
		surface(s),
		col(c)
	{
		assert(s != NULL);
	}

	const btVector3 & GetPosition() const
	{
		return position;
	}

	const btVector3 & GetNormal() const
	{
		return normal;
	}

	btScalar GetDepth() const
	{
		return depth;
	}

	const TRACKSURFACE & GetSurface() const
	{
		return *surface;
	}

	int GetPatchId() const
	{
		return patchid;
	}

	const BEZIER * GetPatch() const
	{
		return patch;
	}

	const btCollisionObject * GetObject() const
	{
		return col;
	}

	// update/interpolate contact
	bool CastRay(
		const btVector3 & origin,
		const btVector3 & direction,
		const btScalar length)
	{
		// plane-based approximation
		btScalar nd = normal.dot(direction);
		if (nd < 0)
		{
			depth = normal.dot(position - origin) / nd;
			position = origin + direction * depth;
			return true;
		}
		position = origin + direction * length;
		depth = length;
		return false;
	}

private:
	btVector3 position;
	btVector3 normal;
	btScalar depth;
	int patchid;
	const BEZIER * patch;
	const TRACKSURFACE * surface;
	const btCollisionObject * col;
};

#endif // _COLLISION_CONTACT_H
