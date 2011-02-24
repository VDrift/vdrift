#ifndef _COLLISION_WORLD_H
#define _COLLISION_WORLD_H

#include "btBulletDynamicsCommon.h"

#include <ostream>

class MODEL;
class TRACK;
class TRACKSURFACE;
class COLLISION_CONTACT;

// manages bodies / collision objects / collision shapes
class COLLISION_WORLD
{
public:
	COLLISION_WORLD(btScalar timeStep = 1/60.0, int maxSubSteps = 10);

	~COLLISION_WORLD();

	void AddRigidBody(btRigidBody * body);

	void AddAction(btActionInterface * action);

	void RemoveRigidBody(btRigidBody * body);

	void RemoveAction(btActionInterface * action);

	// reset collision world (unloads previous track)
	void Reset(const TRACK & t);

	// cast ray into collision world, returns first hit, caster is excluded fom hits
	bool CastRay(
		const btVector3 & position,
		const btVector3 & direction,
		const btScalar length,
		const btCollisionObject * caster,
		COLLISION_CONTACT & contact) const;

	// update world physics
	void Update(btScalar dt);

	void Draw();

	void DebugPrint(std::ostream & out) const;

	void Clear();

protected:
	btDefaultCollisionConfiguration collisionconfig;
	btCollisionDispatcher collisiondispatcher;
	//bt32BitAxisSweep3 collisionbroadphase;
	btDbvtBroadphase collisionbroadphase;
	btSequentialImpulseConstraintSolver constraintsolver;
	btDiscreteDynamicsWorld world;
	btScalar timeStep;
	int maxSubSteps;

	const TRACK * track;
	btAlignedObjectArray<btCollisionShape *> shapes;
	btAlignedObjectArray<btTriangleIndexVertexArray *> meshes;
	btAlignedObjectArray<btCollisionObject *> objects;
};

#endif // _COLLISION_WORLD_H
