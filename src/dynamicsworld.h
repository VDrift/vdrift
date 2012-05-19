#ifndef _DYNAMICSWORLD_H
#define _DYNAMICSWORLD_H

#include "btBulletCollisionCommon.h"
#include "btBulletDynamicsCommon.h"

#include <ostream>

class TRACK;
class COLLISION_CONTACT;
class FractureBody;
class BEZIER;

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

	void addCollisionObject(btCollisionObject* object);

	// reset collision world (unloads previous track)
	void reset(const TRACK & t);

	// set custon contact callback
	void setContactAddedCallback(ContactAddedCallback cb);

	const BEZIER* GetLapSequence(int i);

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
	struct ActiveCon
	{
		ActiveCon() : body(0), id(-1) {}
		ActiveCon(FractureBody* body, int id) : body(body), id(id) {}
		FractureBody* body;
		int id;
	};
	btAlignedObjectArray<ActiveCon> m_activeConnections;
	const TRACK * track;
	btScalar timeStep;
	int maxSubSteps;

	void reset();

	void solveConstraints(btContactSolverInfo& solverInfo);

	void fractureCallback();
};

#endif // _DYNAMICSWORLD_H
