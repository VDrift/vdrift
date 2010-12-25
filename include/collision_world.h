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

	btCollisionObject * AddCollisionObject(const MODEL & model);

	void AddRigidBody(btRigidBody * body);

	void AddAction(btActionInterface * action);

	void RemoveRigidBody(btRigidBody * body);

	void RemoveAction(btActionInterface * action);

	// add track to collision world (unloads previous track)
	void SetTrack(const TRACK * t);

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
	bt32BitAxisSweep3 collisionbroadphase;
	btSequentialImpulseConstraintSolver constraintsolver;
	btDiscreteDynamicsWorld world;
	btScalar timeStep;
	int maxSubSteps;

	//todo: cleanup here
	const TRACK * track;
	btCollisionObject * trackObject;
	btTriangleIndexVertexArray * trackMesh;
	btAlignedObjectArray<btCollisionShape *> shapes;
	btAlignedObjectArray<btTriangleIndexVertexArray *> meshes;

	btCollisionShape * AddMeshShape(const MODEL & model);
	btIndexedMesh GetIndexedMesh(const MODEL & model);
};

#endif // _COLLISION_WORLD_H
