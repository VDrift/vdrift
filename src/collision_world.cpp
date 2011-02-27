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
			m_shape = rayResult.m_localShapeInfo->m_shape;
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

static btIndexedMesh GetIndexedMesh(const MODEL & model)
{
	const float * vertices;
	int vcount;
	const int * faces;
	int fcount;
	model.GetVertexArray().GetVertices(vertices, vcount);
	model.GetVertexArray().GetFaces(faces, fcount);

	assert(fcount % 3 == 0); //Face count is not a multiple of 3

	btIndexedMesh mesh;
	mesh.m_numTriangles = fcount / 3;
	mesh.m_triangleIndexBase = (const unsigned char *)faces;
	mesh.m_triangleIndexStride = sizeof(int) * 3;
	mesh.m_numVertices = vcount;
	mesh.m_vertexBase = (const unsigned char *)vertices;
	mesh.m_vertexStride = sizeof(float) * 3;
	mesh.m_vertexType = PHY_FLOAT;
	return mesh;
}

COLLISION_WORLD::COLLISION_WORLD(btScalar timeStep, int maxSubSteps) :
	collisiondispatcher(&collisionconfig),
	//collisionbroadphase(btVector3(-5000, -5000, -5000), btVector3(5000, 5000, 5000)),
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

void COLLISION_WORLD::RemoveRigidBody(btRigidBody * body)
{
	world.removeRigidBody(body);
}

void COLLISION_WORLD::RemoveAction(btActionInterface * action)
{
	world.removeAction(action);
}

void COLLISION_WORLD::Reset(const TRACK & t)
{
	Clear();
	
	track = &t;
	const std::vector<TRACKOBJECT> & ob = track->GetTrackObjects();
#ifndef EXTBULLET
	meshes.reserve(ob.size());
	btCompoundShape * trackShape = new btCompoundShape(true);
	for (int i = 0, n = ob.size(); i < n; ++i)
	{
		btTransform transform;
		transform.setOrigin(ToBulletVector(ob[i].position));
		transform.setRotation(ToBulletQuaternion(ob[i].rotation));
		
		btTriangleIndexVertexArray * mesh = new btTriangleIndexVertexArray();
		mesh->addIndexedMesh(GetIndexedMesh(*ob[i].model));
		meshes.push_back(mesh);
		
		btBvhTriangleMeshShape * shape = new btBvhTriangleMeshShape(mesh, true);
		shape->setUserPointer((void*)(i+1));
		trackShape->addChildShape(transform, shape);
	}
	trackShape->createAabbTreeFromChildren();
	
	btCollisionObject * trackObject = new btCollisionObject();
	trackObject->setCollisionShape(trackShape);
	world.addCollisionObject(trackObject);
	objects.push_back(trackObject);
#else
	shapes.reserve(ob.size());
	objects.reserve(ob.size());
	for (int i = 0, n = trackob.size(); i < n; ++i)
	{
		btTriangleIndexVertexArray * mesh = new btTriangleIndexVertexArray();
		mesh->addIndexedMesh(GetIndexedMesh(*ob[i].model));
		meshes.push_back(mesh);
		
		btBvhTriangleMeshShape * shape = new btBvhTriangleMeshShape(mesh, true);
		shapes.push_back(shape);
		
		btCollisionObject * object = new btCollisionObject();
		object->setCollisionShape(shape);
		object->setUserPointer((void*)(i+1));
		objects.push_back(object);
		
		btTransform transform;
		transform.setOrigin(ToBulletVector(ob[i].position));
		transform.setRotation(ToBulletQuaternion(ob[i].rotation));
		object->setWorldTransform(transform);
		
		world.addCollisionObject(object);
	}
#endif
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
			int ic = (int)c->getUserPointer();
			int is = (int)ray.m_shape->getUserPointer();
			int n = (int)track->GetTrackObjects().size();
			if (ic > 0 && ic <= n)
			{
				s = track->GetTrackObjects()[ic-1].surface;
			}
			else if (is > 0 && is <= n)
			{
				s = track->GetTrackObjects()[is-1].surface;
			}
			//std::cerr << "static object without surface" << std::endl;
		}
		
		// track bezierpatch collision
		if (track)
		{
			MATHVECTOR <float, 3> bezierspace_raystart(origin[1], origin[2], origin[0]);
			MATHVECTOR <float, 3> bezierspace_dir(direction[1], direction[2], direction[0]);
			MATHVECTOR <float, 3> colpoint;
			MATHVECTOR <float, 3> colnormal;
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
	track = 0;
	
	for(int i = 0; i < objects.size(); ++i)
	{
		world.removeCollisionObject(objects[i]);
		delete objects[i];
	}
	objects.resize(0);
/*	
	int wnum = world.getCollisionObjectArray().size();
	std::cerr << "world collision objects leaking: " << wnum << std::endl;
	for (int i = 0; i < wnum; ++i)
	{
		btCollisionObject * ob = world.getCollisionObjectArray()[i];
		std::cerr << "collision object leaking: " << ob << std::endl;
		world.removeCollisionObject(ob);
	}
*/
	for(int i = 0; i < shapes.size(); ++i)
	{
		btCollisionShape * shape = shapes[i];
		if (shape->isCompound())
		{
			btCompoundShape * cs = (btCompoundShape *)shape;
			for (int i = 0; i < cs->getNumChildShapes(); ++i)
			{
				delete cs->getChildShape(i);
			}
		}
		delete shape;
	}
	shapes.resize(0);
	
	for(int i = 0; i < meshes.size(); ++i)
	{
		delete meshes[i];
	}
	meshes.resize(0);
	
	collisionbroadphase.resetPool(&collisiondispatcher);
}
