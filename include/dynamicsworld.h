#ifndef _DYNAMICSWORLD_H
#define _DYNAMICSWORLD_H

#include "btBulletDynamicsCommon.h"

#include <ostream>

class TRACK;
class COLLISION_CONTACT;
class FractureBody;

class DynamicsWorld  : public btDiscreteDynamicsWorld
{
public:
	DynamicsWorld(
		btDispatcher* dispatcher,
		btBroadphaseInterface* broadphase,
		btConstraintSolver* constraintSolver,
		btCollisionConfiguration* collisionConfig,
		btScalar timeStep = 1/60.0,
		int maxSubSteps = 10);

	~DynamicsWorld();

	void addRigidBody(btRigidBody * body);

	void removeRigidBody(btRigidBody * body);

	// reset collision world (unloads previous track)
	void reset(const TRACK & t);

	// cast ray into collision world, returns first hit, caster is excluded fom hits
	bool castRay(
		const btVector3 & position,
		const btVector3 & direction,
		const btScalar length,
		const btCollisionObject * caster,
		COLLISION_CONTACT & contact) const;

	void update(btScalar dt);

	void draw();

	void debugPrint(std::ostream & out) const;

protected:
	btScalar timeStep;
	int maxSubSteps;
	
	const TRACK * track;	
	btAlignedObjectArray<FractureBody*> m_fractureBodies;
	
	void reset();
	
	void solveConstraints(btContactSolverInfo& solverInfo);
	
	void fractureCallback();
};

#endif // _DYNAMICSWORLD_H
