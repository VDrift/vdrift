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

#include "dynamicsworld.h"
#include "fracturebody.h"
#include "collision_contact.h"
#include "tobullet.h"
#include "track.h"

#include "BulletCollision/CollisionShapes/btCollisionShape.h"

#define EXTBULLET

struct MyRayResultCallback : public btCollisionWorld::RayResultCallback
{
	MyRayResultCallback(
		const btVector3 & rayFromWorld,
		const btVector3 & rayToWorld,
		const btCollisionObject * exclude) :
		m_rayFromWorld(rayFromWorld),
		m_rayToWorld(rayToWorld),
		m_shapePart(-1),
		m_triangleId(-1),
		m_shape(0),
		m_exclude(exclude)
	{
		// ctor
	}

	btVector3 m_rayFromWorld;//used to calculate hitPointWorld from hitFraction
	btVector3 m_rayToWorld;
	btVector3 m_hitNormalWorld;
	btVector3 m_hitPointWorld;

	int m_shapePart;
	int m_triangleId;
	const btCollisionShape * m_shape;
	const btCollisionObject * m_exclude;

	virtual	btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace)
	{
		if (rayResult.m_collisionObject == m_exclude) return 1.0;

		//caller already does the filter on the m_closestHitFraction
		btAssert(rayResult.m_hitFraction <= m_closestHitFraction);

		m_closestHitFraction = rayResult.m_hitFraction;
		m_collisionObject = rayResult.m_collisionObject;

		if (rayResult.m_localShapeInfo)
		{
#ifndef EXTBULLET
			m_shape = rayResult.m_localShapeInfo->m_shape;
#endif
			m_shapePart = rayResult.m_localShapeInfo->m_shapePart;
			m_triangleId = rayResult.m_localShapeInfo->m_triangleIndex;
		}

		if (normalInWorldSpace)
		{
			m_hitNormalWorld = rayResult.m_hitNormalLocal;
		}
		else
		{
			///need to transform normal into worldspace
			m_hitNormalWorld = m_collisionObject->getWorldTransform().getBasis() * rayResult.m_hitNormalLocal;
		}

		m_hitPointWorld.setInterpolate3(m_rayFromWorld,m_rayToWorld,rayResult.m_hitFraction);
		return rayResult.m_hitFraction;
	}
};

DynamicsWorld::DynamicsWorld(
	btDispatcher* dispatcher,
	btBroadphaseInterface* broadphase,
	btConstraintSolver* constraintSolver,
	btCollisionConfiguration* collisionConfig,
	btScalar timeStep,
	int maxSubSteps) :
	btDiscreteDynamicsWorld(dispatcher, broadphase, constraintSolver, collisionConfig),
	track(0),
	timeStep(timeStep),
	maxSubSteps(maxSubSteps)
{
	setGravity(btVector3(0.0, 0.0, -9.81));
	setForceUpdateAllAabbs(false);
}

DynamicsWorld::~DynamicsWorld()
{
	reset();
}

const Bezier* DynamicsWorld::GetSectorPatch(int i){
	return track->GetSectorPatch(i);
}

bool DynamicsWorld::castRay(
	const btVector3 & origin,
	const btVector3 & direction,
	const btScalar length,
	const btCollisionObject * caster,
	CollisionContact & contact) const
{
	btVector3 p = origin + direction * length;
	btVector3 n = -direction;
	btScalar d = length;
	int patch_id = -1;
	const Bezier * b = 0;
	const TrackSurface * s = TrackSurface::None();
	const btCollisionObject * c = 0;

	MyRayResultCallback ray(origin, p, caster);
	rayTest(origin, p, ray);

	// track geometry collision
	if (ray.hasHit())
	{
		p = ray.m_hitPointWorld;
		n = ray.m_hitNormalWorld;
		d = ray.m_closestHitFraction * length;
		c = ray.m_collisionObject;
		if (c->isStaticObject())
		{
			TrackSurface * ts = static_cast<TrackSurface*>(c->getUserPointer());
			if (c->getCollisionShape()->isCompound())
				ts = static_cast<TrackSurface*>(ray.m_shape->getUserPointer());

			// verify surface pointer
			if (track)
			{
				const std::vector<TrackSurface> & surfaces = track->GetSurfaces();
				if (ts < &surfaces[0] || ts > &surfaces[surfaces.size() - 1])
					ts = NULL;
				assert(ts);
			}

			if (ts)
				s = ts;
		}

		// track bezierpatch collision
		if (track)
		{
			Vec3 org = ToMathVector<float>(origin);
			Vec3 dir = ToMathVector<float>(direction);
			Vec3 colpoint;
			Vec3 colnormal;
			patch_id = contact.GetPatchId();
			if (track->CastRay(org, dir, length, patch_id, colpoint, b, colnormal))
			{
				p = ToBulletVector(colpoint);
				n = ToBulletVector(colnormal);
				d = (colpoint - org).Magnitude();
			}
		}

		contact = CollisionContact(p, n, d, patch_id, b, s, c);
		return true;
	}

	// should only happen on vehicle rollover
	contact = CollisionContact(p, n, d, patch_id, b, s, c);
	return false;
}

void DynamicsWorld::update(btScalar dt)
{
	stepSimulation(dt, maxSubSteps, timeStep);
	//CProfileManager::dumpAll();
}

void DynamicsWorld::debugPrint(std::ostream & out) const
{
	out << "Collision objects: " << getNumCollisionObjects() << std::endl;
}

void DynamicsWorld::solveConstraints(btContactSolverInfo& solverInfo)
{
	// todo: after fracture we should run the solver again for better realism
	// for example
	//	save all velocities and if one or more objects fracture:
	//	1) revert all velocties
	//	2) apply impulses for the fracture bodies at the contact locations
	//	3) and run the constaint solver again
	btDiscreteDynamicsWorld::solveConstraints(solverInfo);
	fractureCallback();
}

void DynamicsWorld::addCollisionObject(btCollisionObject* object)
{
	// disable shape drawing for meshes
	if (object->getCollisionShape()->getShapeType() == TRIANGLE_MESH_SHAPE_PROXYTYPE)
	{
		int flags = object->getCollisionFlags();
		//flags |= btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK;
		flags |= btCollisionObject::CF_DISABLE_VISUALIZE_OBJECT;
		object->setCollisionFlags(flags);
	}
	btDiscreteDynamicsWorld::addCollisionObject(object);
}

void DynamicsWorld::reset(const Track & t)
{
	reset();
	track = &t;
}

void DynamicsWorld::reset()
{
	getBroadphase()->resetPool(getDispatcher());
	m_nonStaticRigidBodies.resize(0);
	m_collisionObjects.resize(0);
	track = 0;
}

void DynamicsWorld::setContactAddedCallback(ContactAddedCallback cb)
{
	gContactAddedCallback = cb;
}

void DynamicsWorld::fractureCallback()
{
#if (BT_BULLET_VERSION < 281)
	m_activeConnections.resize(0);

	int numManifolds = getDispatcher()->getNumManifolds();
	for (int i = 0; i < numManifolds; ++i)
	{
		btPersistentManifold* manifold = getDispatcher()->getManifoldByIndexInternal(i);
		if (!manifold->getNumContacts()) continue;

		FractureBody* body = static_cast<FractureBody*>(manifold->getBody0());
		if (body->getInternalType() & CO_FRACTURE_TYPE)
		{
			for (int k = 0; k < manifold->getNumContacts(); ++k)
			{
				btManifoldPoint& point = manifold->getContactPoint(k);
				int con_id = body->getConnectionId(point.m_index0);
				if (point.m_appliedImpulse > 1E-3 &&
					body->applyImpulse(con_id, point.m_appliedImpulse))
				{
					m_activeConnections.push_back(ActiveCon(body, con_id));
				}
			}
		}

		body = static_cast<FractureBody*>(manifold->getBody1());
		if (body->getInternalType() & CO_FRACTURE_TYPE)
		{
			for (int k = 0; k < manifold->getNumContacts(); ++k)
			{
				btManifoldPoint& point = manifold->getContactPoint(k);
				int con_id = body->getConnectionId(point.m_index1);
				if (point.m_appliedImpulse > 1E-3 &&
					body->applyImpulse(con_id, point.m_appliedImpulse))
				{
					m_activeConnections.push_back(ActiveCon(body, con_id));
				}
			}
		}
	}

	// Update active connections.
	for (int i = 0; i < m_activeConnections.size(); ++i)
	{
		int con_id = m_activeConnections[i].id;
		FractureBody* body = m_activeConnections[i].body;
		btRigidBody* child = body->updateConnection(con_id);
		if (child) addRigidBody(child);
	}
#endif
}
