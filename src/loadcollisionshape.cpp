#include "BulletCollision/CollisionShapes/btCompoundShape.h"
#include "BulletCollision/CollisionShapes/btConvexHullShape.h"
#include "BulletCollision/CollisionShapes/btBoxShape.h"
#include "cfg/ptree.h"

inline void operator >> (std::istream & lhs, btVector3 & rhs)
{
	for (size_t i = 0; i < 3; ++i)
	{
		std::string str;
		std::getline(lhs, str, ',');
		
		std::stringstream s(str);
		btScalar val(0);
		s >> val;
		rhs[i] = val;
	}
}

void LoadBoxShape(
	const PTree & cfg,
	const btVector3 & center,
	btCollisionShape *& shape)
{
	btVector3 box_size(1, 1, 1);
	cfg.get("size", box_size);
	btBoxShape * box = new btBoxShape(box_size * 0.5);
	
	btVector3 box_center(0, 0, 0);
	cfg.get("center", box_center);
	btTransform transform = btTransform::getIdentity();
	transform.setOrigin(box_center - center);
	
	if (!shape)
	{
		if (center.isZero() && box_center.isZero())
		{
			shape = box;
		}
		else
		{
			btCompoundShape * compound = new btCompoundShape(false);
			compound->addChildShape(transform, box);
			shape = compound;
		}
	}
	else
	{
		if (!shape->isCompound())
		{
			// create compound, remap shape pointer
			btCollisionShape * temp = shape;
			shape = new btCompoundShape(false);
			static_cast<btCompoundShape*>(shape)->addChildShape(btTransform::getIdentity(), temp);
		}
		static_cast<btCompoundShape*>(shape)->addChildShape(transform, box);
	}
}

void LoadHullShape(
	const PTree & cfg,
	const btVector3 & center,
	btCollisionShape *& shape)
{
	btConvexHullShape * hull = new btConvexHullShape();
	for (PTree::const_iterator i = cfg.begin(); i != cfg.end(); ++i)
	{
		btVector3 point;
		std::istringstream str(i->second.value());
		str >> point;
		hull->addPoint(point - center);
	}
	
	if (!shape)
	{
		shape = hull;
	}
	else
	{
		if (!shape->isCompound())
		{
			// create compound, remap shape pointer
			btCollisionShape * temp = shape;
			shape = new btCompoundShape(false);
			static_cast<btCompoundShape*>(shape)->addChildShape(btTransform::getIdentity(), temp);
		}
		static_cast<btCompoundShape*>(shape)->addChildShape(btTransform::getIdentity(), hull);
	}
}

void LoadCollisionShape(
	const PTree & cfg,
	const btVector3 & center,
	btCollisionShape *& shape)
{
	const PTree * cfg_shape = 0;
	if (cfg.get("shape.hull", cfg_shape))
	{
		// load hulls
		for (PTree::const_iterator i = cfg_shape->begin(); i != cfg_shape->end(); ++i)
		{
			LoadHullShape(i->second, center, shape);
		}
	}
	if (cfg.get("shape.box", cfg_shape))
	{
		// load boxes
		for (PTree::const_iterator i = cfg_shape->begin(); i != cfg_shape->end(); ++i)
		{
			LoadBoxShape(i->second, center, shape);
		}
	}
}
