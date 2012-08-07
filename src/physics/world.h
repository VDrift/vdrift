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

#ifndef _SIM_WORLD_H
#define _SIM_WORLD_H

#include "BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.h"
#include "BulletCollision/CollisionDispatch/btCollisionDispatcher.h"
#include "BulletCollision/BroadphaseCollision/btDbvtBroadphase.h"
#include "BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h"
#include "BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h"

namespace sim
{

class FractureBody;
struct Ray;

/// simulation configuration
struct Config
{
	btDefaultCollisionConfiguration config;
	btCollisionDispatcher dispatch;
	btDbvtBroadphase broadphase;
	btSequentialImpulseConstraintSolver solver;
	btScalar timeStep;
	int maxSubSteps;

	Config(btScalar timeStep = 1/60.0f, int maxSubSteps = 10);
};

/// discrete dynamics world with fracture body support
class World : public btDiscreteDynamicsWorld
{
public:
	World(Config & config);

	~World();

	/// alternative ray test which supports a ray test result processor
	void rayTest(Ray & ray) const;

	/// optional ray test processor, wraps the ray test callback,
	/// allows additional operations on ray test result
	struct RayProcessor;
	void setRayProcessor(RayProcessor & pr);

	/// set a callback to be executed when a new contact is added
	void setContactAddedCallback(ContactAddedCallback cb);

	void addCollisionObject(btCollisionObject * object);

	void update(btScalar dt);

	void reset();

protected:
	struct ActiveCon
	{
		ActiveCon() : body(0), id(-1) {}
		ActiveCon(FractureBody * body, int id) : body(body), id(id) {}
		FractureBody * body;
		int id;
	};
	btAlignedObjectArray<ActiveCon> m_activeConnections;
	btScalar timeStep;
	int maxSubSteps;

	RayProcessor * rayProc;

	void solveConstraints(btContactSolverInfo& solverInfo);

	void fractureCallback();
};

struct World::RayProcessor : public btCollisionWorld::RayResultCallback
{
	virtual void set(Ray & ray) = 0;
};

}

#endif
