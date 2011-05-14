#include "BulletCollision/CollisionDispatch/btCollisionWorld.h"
#include "BulletCollision/CollisionShapes/btCompoundShape.h"
#include "BulletDynamics/Dynamics/btDynamicsWorld.h"
#include "fracturebody.h"

FractureBody::FractureBody(const btRigidBodyConstructionInfo& info) :
	btRigidBody(info)
{
	btAssert(info.m_collisionShape->isCompound());
	btCompoundShape* compound = (btCompoundShape*)info.m_collisionShape;
	m_shapeOffset = compound->getNumChildShapes();
	m_internalType = CUSTOM_FRACTURE_TYPE+CO_RIGID_BODY;
}

void FractureBody::addConnection(btRigidBody* body, const btTransform& offset, btScalar strength, btScalar elasticLimit)
{
	btConnection c;
	c.m_body = body;
	c.m_strength = strength;
	c.m_elasticLimit = elasticLimit;
	m_connections.push_back(c);
	
	btCompoundShape* compound = (btCompoundShape*)getCollisionShape();
	compound->addChildShape(offset, body->getCollisionShape());
}

btRigidBody* FractureBody::breakConnection(int i)
{
	int ishape = i + m_shapeOffset;
	btRigidBody* child = m_connections[i].m_body;
	btCompoundShape* compound = (btCompoundShape*)getCollisionShape();
	btTransform trans = getWorldTransform() * compound->getChildTransform(ishape);
	
	child->setWorldTransform(trans);
	child->setLinearVelocity(getLinearVelocity());
	child->setAngularVelocity(getAngularVelocity());
	
	compound->removeChildShapeByIndex(ishape);
	
	m_connections.swap(i, m_connections.size() - 1);
	m_connections.pop_back();
	
	return child;
}

void FractureBody::updateMassCenter()
{
	for (int i = 0; i < m_connections.size(); ++i)
	{
		
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
