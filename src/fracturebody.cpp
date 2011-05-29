#include "BulletCollision/CollisionDispatch/btCollisionWorld.h"
#include "BulletCollision/CollisionShapes/btCompoundShape.h"
#include "BulletDynamics/Dynamics/btDynamicsWorld.h"
#include "fracturebody.h"
#include <iostream>

FractureBody::FractureBody(const btRigidBodyConstructionInfo& info) :
	btRigidBody(info)
{
	btAssert(!info.m_collisionShape->isCompound());
	m_internalType = CUSTOM_FRACTURE_TYPE + CO_RIGID_BODY;
}

void FractureBody::addShape(const btTransform& localTransform, btCollisionShape* shape)
{
	shape->setUserPointer((void*)-1);
	((btCompoundShape*)m_collisionShape)->addChildShape(localTransform, shape);
}

void FractureBody::addConnection(btRigidBody* body, const btTransform& localTransform, btScalar strength, btScalar elasticLimit)
{
	btCompoundShape* compound = (btCompoundShape*)m_collisionShape;
	int shapeId = compound->getNumChildShapes();
	
	// store connection index as user pointer
	body->getCollisionShape()->setUserPointer((void*)m_connections.size());
	
	btConnection c;
	c.m_body = body;
	c.m_strength = strength;
	c.m_elasticLimit = elasticLimit;
	c.m_shapeId = shapeId;
	m_connections.push_back(c);
	
	compound->addChildShape(localTransform, body->getCollisionShape());
}

btRigidBody* FractureBody::breakConnection(int id)
{
	int shapeId = m_connections[id].m_shapeId;
	btRigidBody* child = m_connections[id].m_body;
	btCompoundShape* compound = (btCompoundShape*)getCollisionShape();
	btTransform trans = getWorldTransform() * compound->getChildTransform(shapeId);
	
	child->setWorldTransform(trans);
	child->setLinearVelocity(getLinearVelocity());
	child->setAngularVelocity(getAngularVelocity());
	child->setUserPointer((void*)-1);
	
	//std::cerr << "\nbefore shape removal" << std::endl;
	//debugPrint();
	//std::cerr << "removing shape " << shapeId << std::endl;
	
	// will swap last shape with removed one
	compound->removeChildShapeByIndex(shapeId);
	
	// remove will swap last shape with shapeId need to update connection
	id = (int)compound->getChildShape(shapeId)->getUserPointer();
	if (id >= 0 && shapeId <= compound->getNumChildShapes())
	{
		m_connections[id].m_shapeId = shapeId;
	}
	
	//std::cerr << "\nafter shape removal" << std::endl;
	//debugPrint();
	
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
		int id = (int)shape->getChildShape(i)->getUserPointer();
		if (id >= 0)
		{
			btTransform transform;
			getMotionState()->getWorldTransform(transform);
			transform = transform * shape->getChildTransform(i);
			m_connections[id].m_body->getMotionState()->setWorldTransform(transform);		
		}
	}
}

void FractureBody::debugPrint()
{
	std::cerr << "fracture body " << this << std::endl;
	std::cerr << "connections" << std::endl;
	for (int i = 0; i < m_connections.size(); ++i)
	{
		std::cerr << "body " << m_connections[i].m_body << std::endl;
		std::cerr << "shape " << m_connections[i].m_shapeId << " " << m_connections[i].m_body->getCollisionShape() << std::endl;
	}
	std::cerr << "shapes" << std::endl;
	const btCompoundShape* shape = (btCompoundShape*)getCollisionShape();
	for (int i = 0; i < shape->getNumChildShapes(); ++i)
	{
		std::cerr << "shape "  << (int)shape->getChildShape(i)->getUserPointer() << " " << shape->getChildShape(i) << std::endl;
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
