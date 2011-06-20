#include "dynamicsworld.h"
#include "fracturebody.h"
#include "collision_contact.h"
#include "tobullet.h"
#include "model.h"
#include "track.h"

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

		if(rayResult.m_localShapeInfo)
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

bool DynamicsWorld::castRay(
	const btVector3 & origin,
	const btVector3 & direction,
	const btScalar length,
	const btCollisionObject * caster,
	COLLISION_CONTACT & contact) const
{
	btVector3 p = origin + direction * length;
	btVector3 n = -direction;
	btScalar d = length;
	int patch_id = -1;
	const BEZIER * b = 0;
	const TRACKSURFACE * s = TRACKSURFACE::None();
	btCollisionObject * c = 0;

	MyRayResultCallback ray(origin, p, caster);
	rayTest(origin, p, ray);

	// track geometry collision
	bool geometryHit = ray.hasHit();
	if (geometryHit)
	{
		p = ray.m_hitPointWorld;
		n = ray.m_hitNormalWorld;
		d = ray.m_closestHitFraction * length;
		c = ray.m_collisionObject;
		if (c->isStaticObject())
		{
			TRACKSURFACE* tsc = (TRACKSURFACE*)c->getUserPointer();
			const std::vector<TRACKSURFACE> & surfaces = track->GetSurfaces();
			if (tsc >= &surfaces[0] && tsc <= &surfaces[surfaces.size()-1])
			{
				s = tsc;
			}
#ifndef EXTBULLET
			else if (c->getCollisionShape()->isCompound())
			{
				TRACKSURFACE* tss = (TRACKSURFACE*)ray.m_shape->getUserPointer();
				if (tss >= &surfaces[0] && tss <= &surfaces[surfaces.size()-1])
				{
					s = tss;
				}
			}
#endif
			//std::cerr << "static object without surface" << std::endl;
		}

		// track bezierpatch collision
		if (track)
		{
			MATHVECTOR<float, 3> bezierspace_raystart(origin[1], origin[2], origin[0]);
			MATHVECTOR<float, 3> bezierspace_dir(direction[1], direction[2], direction[0]);
			MATHVECTOR<float, 3> colpoint;
			MATHVECTOR<float, 3> colnormal;
			patch_id = contact.GetPatchId();

			if(track->CastRay(bezierspace_raystart, bezierspace_dir, length,
				patch_id, colpoint, b, colnormal))
			{
				p = btVector3(colpoint[2], colpoint[0], colpoint[1]);
				n = btVector3(colnormal[2], colnormal[0], colnormal[1]);
				d = (colpoint - bezierspace_raystart).Magnitude();
			}
		}

		contact = COLLISION_CONTACT(p, n, d, patch_id, b, s, c);
		return true;
	}

	// should only happen on vehicle rollover
	contact = COLLISION_CONTACT(p, n, d, patch_id, b, s, c);
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

void DynamicsWorld::addRigidBody(btRigidBody* body)
{
	if (body->getInternalType() & CUSTOM_FRACTURE_TYPE)
	{
		FractureBody* fbody = (FractureBody*)body;
		m_fractureBodies.push_back(fbody);
	}
	btDiscreteDynamicsWorld::addRigidBody(body);
}

void DynamicsWorld::removeRigidBody(btRigidBody* body)
{
	if (body->getInternalType() & CUSTOM_FRACTURE_TYPE)
	{
		FractureBody* fbody = (FractureBody*)body;
		m_fractureBodies.remove(fbody);
	}
	btDiscreteDynamicsWorld::removeRigidBody(body);
}

void DynamicsWorld::reset(const TRACK & t)
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

void DynamicsWorld::fractureCallback()
{
	m_activeConnections.resize(0);
	int numManifolds = getDispatcher()->getNumManifolds();
	for (int i = 0; i < numManifolds; ++i)
	{
		btPersistentManifold* manifold = getDispatcher()->getManifoldByIndexInternal(i);
		if (!manifold->getNumContacts()) continue;

		if (((btCollisionObject*)manifold->getBody0())->getInternalType() & CUSTOM_FRACTURE_TYPE)
		{
			FractureBody* body = (FractureBody*)manifold->getBody0();
			btCompoundShape* shape = (btCompoundShape*)body->getCollisionShape();
			for (int k = 0; k < manifold->getNumContacts(); ++k)
			{
				btManifoldPoint& pt = manifold->getContactPoint(k);
				int shape_id = pt.m_index0;
				int con_id = cast<int>(shape->getChildShape(shape_id)->getUserPointer());
				if (con_id >= 0 && pt.m_appliedImpulse > 1E-3)
				{
					FractureBody::Connection & connection = body->m_connections[con_id];
					btAssert(connection.m_shapeId == shape_id);
					if (connection.m_accImpulse < 1E-3)
					{
						// activate connection
						m_activeConnections.push_back(ActiveCon(body, con_id));
					}
					connection.m_accImpulse += pt.m_appliedImpulse;
				}
			}
		}

		if (((btCollisionObject*)manifold->getBody1())->getInternalType() & CUSTOM_FRACTURE_TYPE)
		{
			FractureBody* body = (FractureBody*)manifold->getBody1();
			btCompoundShape* shape = (btCompoundShape*)body->getCollisionShape();
			for (int k = 0; k < manifold->getNumContacts(); ++k)
			{
				btManifoldPoint& pt = manifold->getContactPoint(k);
				int shape_id = pt.m_index1;
				int con_id = cast<int>(shape->getChildShape(shape_id)->getUserPointer());
				if (con_id >= 0 && pt.m_appliedImpulse > 1E-3)
				{
					FractureBody::Connection & connection = body->m_connections[con_id];
					btAssert(connection.m_shapeId == shape_id);
					if (connection.m_accImpulse < 1E-3)
					{
						// activate connection
						m_activeConnections.push_back(ActiveCon(body, con_id));
					}
					connection.m_accImpulse += pt.m_appliedImpulse;
				}
			}
		}
	}

	for (int i = 0; i < m_activeConnections.size(); ++i)
	{
		// Check connection.
		FractureBody* body = m_activeConnections[i].body;
		int con_id = m_activeConnections[i].id;
		FractureBody::Connection& connection = body->m_connections[con_id];
		btScalar damage = connection.m_accImpulse - connection.m_elasticLimit;
		connection.m_accImpulse = 0;

		if (damage > 0)
		{
			// weaken connection
			connection.m_elasticLimit -= damage * 0.5;
			connection.m_strength -= damage;

			if (connection.m_strength < 0)
			{
				// break connection
				btRigidBody * child = body->breakConnection(con_id);
				addRigidBody(child);
			}
		}
	}
}
