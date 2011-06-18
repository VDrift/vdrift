#include "BulletCollision/CollisionDispatch/btCollisionWorld.h"
#include "BulletCollision/CollisionShapes/btCompoundShape.h"
#include "BulletDynamics/Dynamics/btDynamicsWorld.h"
#include "fracturebody.h"
#include <iostream>

FractureBody::FractureBody(const btRigidBodyConstructionInfo& info) :
	btRigidBody(info)
{
	btAssert(info.m_collisionShape->isCompound());
	m_internalType = CUSTOM_FRACTURE_TYPE | CO_RIGID_BODY;
}

void FractureBody::addShape(const btTransform& localTransform, btCollisionShape* shape)
{
	shape->setUserPointer(cast<void*>(-1));
	((btCompoundShape*)m_collisionShape)->addChildShape(localTransform, shape);
}

void FractureBody::addConnection(btRigidBody* body, const btTransform& localTransform, btScalar strength, btScalar elasticLimit)
{
	// store connection index as user pointer
	body->getCollisionShape()->setUserPointer(cast<void*>(m_connections.size()));
	
	btCompoundShape* compound = (btCompoundShape*)m_collisionShape;
	compound->addChildShape(localTransform, body->getCollisionShape());
	
	Connection connection;
	connection.m_body = body;
	connection.m_strength = strength;
	connection.m_elasticLimit = elasticLimit;
	connection.m_shapeId = compound->getNumChildShapes() - 1;
	m_connections.push_back(connection);
}

btRigidBody* FractureBody::breakConnection(int con_id)
{
	btCompoundShape* compound = (btCompoundShape*)getCollisionShape();
	btAssert(con_id >= 0);
	btAssert(con_id < m_connections.size());
	
	int shape_id = m_connections[con_id].m_shapeId;
	btAssert(shape_id >= 0);
	btAssert(shape_id < compound->getNumChildShapes());
	
	// init child body
	btRigidBody* child = m_connections[con_id].m_body;
	btTransform trans = getWorldTransform() * compound->getChildTransform(shape_id);
	child->setWorldTransform(trans);
	child->setLinearVelocity(getLinearVelocity());
	child->setAngularVelocity(getAngularVelocity());
	child->getCollisionShape()->setUserPointer(cast<void*>(-1));
	
	// invalidate connection
	m_connections[con_id].m_shapeId = -1;
	
	// remove child shape
	compound->removeChildShapeByIndex(shape_id);
	if (shape_id < compound->getNumChildShapes() - 1)
	{
		int id = cast<int>(compound->getChildShape(shape_id)->getUserPointer());
		btAssert(id >= 0);
		m_connections[id].m_shapeId = shape_id;
	}
	
	return child;
}

void FractureBody::updateMassCenter()
{
	for (int i = 0; i < m_connections.size(); ++i)
	{
		
	}
}

void FractureBody::updateMotionState()
{
	btAssert(m_internalType & CUSTOM_FRACTURE_TYPE);
	const btCompoundShape* shape = (btCompoundShape*)getCollisionShape();
	for (int i = 0; i < shape->getNumChildShapes(); ++i)
	{
		int id = cast<int>(shape->getChildShape(i)->getUserPointer());
		if (id >= 0)
		{
			btTransform transform;
			getMotionState()->getWorldTransform(transform);
			transform = transform * shape->getChildTransform(i);
			m_connections[id].m_body->getMotionState()->setWorldTransform(transform);		
		}
	}
}

btCompoundShape* FractureBody::shiftTransform(btCompoundShape* compound, btScalar* masses, btTransform& shift, btVector3& principalInertia)
{
	btTransform principal;
	compound->calculatePrincipalAxisTransform(masses, principal, principalInertia);
	
	///create a new compound with world transform/center of mass properly aligned with the principal axis
	///creation is faster using a new compound to store the shifted children
	btCompoundShape* newCompound = new btCompoundShape();
	btTransform principalInv = principal.inverse();
	for (int i = 0; i < compound->getNumChildShapes(); ++i)
	{
		btTransform newChildTransform = principalInv * compound->getChildTransform(i);
		///updateChildTransform is really slow, because it re-calculates the AABB each time. todo: add option to disable this update
		newCompound->addChildShape(newChildTransform, compound->getChildShape(i));
	}
	
	shift = principal;
	return newCompound;
}
