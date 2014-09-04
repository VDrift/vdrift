/************************************************************************/
/*                                                                      */
/* This file is part of VDrift.                                         */
/*                                                                      */
/* VDrift is free software: you can redistribute it and/or modify       */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation, either version 3 of the License, or    */
/* (at your option) any later version.                                  */
/*                                                                      */
/* VDrift is distributed in the hope that it will be useful,            */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */
/*                                                                      */
/* You should have received a copy of the GNU General Public License    */
/* along with VDrift.  If not, see <http://www.gnu.org/licenses/>.      */
/*                                                                      */
/************************************************************************/

#include "BulletCollision/CollisionShapes/btCompoundShape.h"
#include "BulletCollision/CollisionShapes/btMultiSphereShape.h"
#include "BulletCollision/CollisionShapes/btConvexHullShape.h"
#include "BulletCollision/CollisionShapes/btCapsuleShape.h"
#include "BulletCollision/CollisionShapes/btBoxShape.h"
#include "cfg/ptree.h"

static inline std::istream & operator >> (std::istream & lhs, btVector3 & rhs)
{
	std::string str;
	for (int i = 0; i < 3 && !lhs.eof(); ++i)
	{
		std::getline(lhs, str, ',');
		std::istringstream s(str);
		s >> rhs[i];
	}
	return lhs;
}

void LoadCapsuleShape(
	const PTree & cfg,
	const btTransform & transform,
	btCollisionShape *& shape,
	btCompoundShape *& compound)
{
	btVector3 center(0, 0, 0);
	cfg.get("center", center);
	btTransform shape_transform = transform;
	shape_transform.getOrigin() += center;

	btVector3 size(0.5, 1, 0.5);
	cfg.get("size", size);

	int axis = size.maxAxis();
	int radius = size.minAxis();
	if (axis == 0)
	{
		shape = new btCapsuleShapeX(size[radius] * 0.5, size[axis]);
	}
	else if (axis == 1)
	{
		shape = new btCapsuleShape(size[radius] * 0.5, size[axis]);
	}
	else
	{
		shape = new btCapsuleShapeZ(size[radius] * 0.5, size[axis]);
	}

	if (!shape_transform.getOrigin().isZero() && !compound)
	{
		compound = new btCompoundShape(false);
	}

	if (compound)
	{
		compound->addChildShape(shape_transform, shape);
	}
}

void LoadBoxShape(
	const PTree & cfg,
	const btTransform & transform,
	btCollisionShape *& shape,
	btCompoundShape *& compound)
{
	btVector3 center(0, 0, 0);
	cfg.get("center", center);
	btTransform shape_transform = transform;
	shape_transform.getOrigin() += center;

	btVector3 size(1, 1, 1);
	cfg.get("size", size);
	btBoxShape * box = new btBoxShape(size * 0.5);

	if (!shape_transform.getOrigin().isZero() && !compound)
	{
		compound = new btCompoundShape(false);
	}

	shape = box;
	if (compound)
	{
		compound->addChildShape(shape_transform, shape);
	}
}

void LoadHullShape(
	const PTree & cfg,
	const btTransform & transform,
	btCollisionShape *& shape,
	btCompoundShape *& compound)
{
	btAlignedObjectArray<btVector3> points;
	btAlignedObjectArray<btScalar> radii;
	btScalar has_radius = 0;
	for (PTree::const_iterator i = cfg.begin(); i != cfg.end(); ++i)
	{
		btVector3 point(0, 0, 0);
		btScalar radius = 0;

		std::istringstream str(i->second.value());
		str >> point;
		str >> radius;
		has_radius += radius;

		points.push_back(transform * point);
		radii.push_back(radius);
	}

	if (has_radius > 0)
	{
		shape = new btMultiSphereShape(&points[0], &radii[0], points.size());
	}
	else
	{
		shape = new btConvexHullShape(&points[0][0], points.size());
	}

	if (compound)
	{
		compound->addChildShape(btTransform::getIdentity(), shape);
	}
}

void LoadCollisionShape(
	const PTree & cfg,
	const btTransform & transform,
	btCollisionShape *& shape,
	btCompoundShape *& compound)
{
	if (shape && compound)
	{
		compound->addChildShape(transform, shape);
		return;
	}

	const PTree * cfg_shape = 0;
	if (cfg.get("hull", cfg_shape))
	{
		LoadHullShape(*cfg_shape, transform, shape, compound);
	}
	else if (cfg.get("box", cfg_shape))
	{
		LoadBoxShape(*cfg_shape, transform, shape, compound);
	}
	else if (cfg.get("capsule", cfg_shape))
	{
		LoadCapsuleShape(*cfg_shape, transform, shape, compound);
	}
}
