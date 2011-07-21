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
	static_cast<btCompoundShape*>(m_collisionShape)->addChildShape(localTransform, shape);
}

void FractureBody::addConnection(btRigidBody* body, const btTransform& localTransform, btScalar strength, btScalar elasticLimit)
{
	// Store connection index as user pointer.
	body->getCollisionShape()->setUserPointer(cast<void*>(m_connections.size()));

	btCompoundShape* compound = (btCompoundShape*)m_collisionShape;
	compound->addChildShape(localTransform, body->getCollisionShape());

	Connection connection;
	connection.m_body = body;
	connection.m_strength = strength;
	connection.m_elasticLimit = elasticLimit;
	connection.m_accImpulse = 0;
	m_connections.push_back(connection);
}

btRigidBody* FractureBody::addBody(const btRigidBodyConstructionInfo& info, btScalar strength, btScalar elasticLimit)
{
	btRigidBody* rb = new btRigidBody(info);
	btTransform transform;
	rb->getMotionState()->getWorldTransform(transform);
	addConnection(rb, transform, strength, elasticLimit);
	return rb;
}

btRigidBody* FractureBody::updateConnection(int shape_id)
{
	btCompoundShape* compound = (btCompoundShape*)getCollisionShape();
	btAssert(shape_id >= 0);
	if (shape_id >= compound->getNumChildShapes())
	{
#ifdef DEBUG
		std::cerr << "Trying to fracture child shape " << shape_id << std::endl;
		std::cerr << "Number of child shapes " << compound->getNumChildShapes() << std::endl;
#endif
		return 0;
	}

	btCollisionShape* child_shape = compound->getChildShape(shape_id);
	int con_id = cast<int>(child_shape->getUserPointer());
	btAssert(con_id >= 0 && con_id < m_connections.size());

	Connection& connection = m_connections[con_id];
	btScalar damage = connection.m_accImpulse - connection.m_elasticLimit;
	connection.m_accImpulse = 0;

	if (damage > 0)
	{
		connection.m_elasticLimit -= damage * 0.5;
		connection.m_strength -= damage;

		if (connection.m_strength < 0)
		{
			// Init child body.
			btRigidBody* child = m_connections[con_id].m_body;
			btTransform trans = getWorldTransform() * compound->getChildTransform(shape_id);
			child->setWorldTransform(trans);
			child->setLinearVelocity(getVelocityInLocalPoint(trans.getOrigin()-getCenterOfMassPosition()));
			child->setAngularVelocity(btVector3(0, 0, 0));
			child->getCollisionShape()->setUserPointer(cast<void*>(-1));

			// Remove child shape.
			compound->removeChildShapeByIndex(shape_id);

			return child;
		}
	}

	return 0;
}

void FractureBody::updateMassCenter()
{
	for (int i = 0; i < m_connections.size(); ++i)
	{

	}
}

void FractureBody::updateState()
{
	btAssert(m_internalType & CUSTOM_FRACTURE_TYPE);
	const btCompoundShape* shape = (btCompoundShape*)getCollisionShape();
	for (int i = 0; i < shape->getNumChildShapes(); ++i)
	{
		int con_id = cast<int>(shape->getChildShape(i)->getUserPointer());
		if (con_id >= 0)
		{
			btTransform transform;
			getMotionState()->getWorldTransform(transform);
			transform = transform * shape->getChildTransform(i);

			btAssert(con_id < numConnections());
			m_connections[con_id].m_body->getMotionState()->setWorldTransform(transform);
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
