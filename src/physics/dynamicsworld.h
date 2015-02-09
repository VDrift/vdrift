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

#ifndef _DYNAMICSWORLD_H
#define _DYNAMICSWORLD_H

#include "BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h"

#include <iosfwd>

class Track;
class CollisionContact;
class FractureBody;
class Bezier;

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
	void reset(const Track & t);

	// set custon contact callback
	void setContactAddedCallback(ContactAddedCallback cb);

	const Bezier* GetSectorPatch(int i);

	// cast ray into collision world, returns first hit, caster is excluded fom hits
	bool castRay(
		const btVector3 & position,
		const btVector3 & direction,
		const btScalar length,
		const btCollisionObject * caster,
		CollisionContact & contact) const;

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
	const Track * track;
	btScalar timeStep;
	int maxSubSteps;

	void reset();

	void solveConstraints(btContactSolverInfo& solverInfo);

	void fractureCallback();
};

#endif // _DYNAMICSWORLD_H
