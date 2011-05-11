#include "collision_world.h"
#include "collision_contact.h"
#include "tobullet.h"
#include "model.h"
#include "track.h"

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

	btVector3	m_rayFromWorld;//used to calculate hitPointWorld from hitFraction
	btVector3	m_rayToWorld;

	btVector3	m_hitNormalWorld;
	btVector3	m_hitPointWorld;

	
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

COLLISION_WORLD::COLLISION_WORLD(btScalar timeStep, int maxSubSteps) :
	collisiondispatcher(&collisionconfig),
	world(&collisiondispatcher, &collisionbroadphase, &constraintsolver, &collisionconfig),
	timeStep(timeStep),
	maxSubSteps(maxSubSteps),
	track(0)
{
	world.setGravity(btVector3(0.0, 0.0, -9.81));
	world.setForceUpdateAllAabbs(false);
}

COLLISION_WORLD::~COLLISION_WORLD()
{
	Clear();
}

void COLLISION_WORLD::AddRigidBody(btRigidBody * body)
{
	world.addRigidBody(body);
}

void COLLISION_WORLD::AddAction(btActionInterface * action)
{
	world.addAction(action);
}

void COLLISION_WORLD::AddCollisionObject(btCollisionObject * object)
{
	world.addCollisionObject(object);
}

void COLLISION_WORLD::RemoveRigidBody(btRigidBody * body)
{
	world.removeRigidBody(body);
}

void COLLISION_WORLD::RemoveAction(btActionInterface * action)
{
	world.removeAction(action);
}

void COLLISION_WORLD::RemoveCollisionObject(btCollisionObject * object)
{
	world.removeCollisionObject(object);
}

void COLLISION_WORLD::Reset(const TRACK & t)
{
	Clear();
	track = &t;
}

bool COLLISION_WORLD::CastRay(
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
	world.rayTest(origin, p, ray);
	
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

void COLLISION_WORLD::Update(btScalar dt)
{
	world.stepSimulation(dt, maxSubSteps, timeStep);
	//CProfileManager::dumpAll();
}

void COLLISION_WORLD::DebugPrint(std::ostream & out) const
{
	out << "Collision objects: " << world.getNumCollisionObjects() << std::endl;
}

void COLLISION_WORLD::Clear()
{
	collisionbroadphase.resetPool(&collisiondispatcher);
	track = 0;
}
