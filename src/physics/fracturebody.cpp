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

#include "fracturebody.h"
#include "motionstate.h"
#include "BulletCollision/CollisionShapes/btCompoundShape.h"
#include "BulletCollision/CollisionDispatch/btCollisionWorld.h"
#include "BulletDynamics/Dynamics/btDynamicsWorld.h"

template <typename T0, typename T1>
static inline T0 cast(const T1 & t)
{
	union {T0 t0; T1 t1;} cast;
	cast.t1 = t;
	return cast.t0;
}

static inline int getConId(const btCollisionShape & shape)
{
	return cast<int>(shape.getUserPointer()) - 1;
}

static inline void setConId(btCollisionShape & shape, int id)
{
	shape.setUserPointer(cast<void*>(id + 1));
}

static inline btVector3 getPrincipalInertia(const btVector3 & p, const btScalar & m)
{
	return m * btVector3(
		p.y() * p.y() + p.z() * p.z(),
		p.x() * p.x() + p.z() * p.z(),
		p.x() * p.x() + p.y() * p.y());
}

FractureBody::FractureBody(const FractureBodyInfo & info) :
	btRigidBody(btRigidBodyConstructionInfo(1, 0, info.m_shape, btVector3(1, 1, 1))),
	m_connections(info.m_connections),
	m_motionState(*this)
{
	m_internalType = CO_FRACTURE_TYPE | CO_RIGID_BODY;

	// calculate center of mass and inertia
	m_centerOfMassOffset = -info.m_massCenter / info.m_mass;
	btVector3 inertia = info.m_inertia - getPrincipalInertia(m_centerOfMassOffset, info.m_mass);
	setMassProps(info.m_mass, inertia);

	// shift children shapes due to new center of mass
	for (int i = 0; i < info.m_shape->getNumChildShapes(); ++i)
	{
		info.m_shape->getChildTransform(i).getOrigin() += m_centerOfMassOffset;
	}
	info.m_shape->recalculateLocalAabb();

	// set motion states
	if (info.m_states.size() == m_connections.size() + 1)
	{
		info.m_states[0].massCenterOffset = m_centerOfMassOffset;
		new (&m_motionState) MotionState(*this, &info.m_states[0]);
		setMotionState(&m_motionState);

		for (int i = 0; i < m_connections.size(); ++i)
		{
			m_connections[i].m_body->setMotionState(&info.m_states[i + 1]);
		}
	}
}

int FractureBody::getConnectionId(int shape_id) const
{
	btCompoundShape* compound = static_cast<btCompoundShape*>(m_collisionShape);
	btAssert(shape_id >= 0 && shape_id < compound->getNumChildShapes());
	btCollisionShape* child_shape = compound->getChildShape(shape_id);
	return getConId(*child_shape);
}

bool FractureBody::isChildConnected(int i) const
{
	btAssert(i >= 0 && i < m_connections.size());
	return m_connections[i].m_shapeId >= 0;
}

int FractureBody::getNumChildren() const
{
	return m_connections.size();
}

btRigidBody* FractureBody::getChildBody(int i) const
{
	btAssert(i >= 0 && i < m_connections.size());
	return m_connections[i].m_body;
}

void FractureBody::setChildTransform(int i, const btTransform& transform)
{
	if (isChildConnected(i))
	{
		btCompoundShape* compound = static_cast<btCompoundShape*>(m_collisionShape);
		int shape_id = m_connections[i].m_shapeId;
		compound->updateChildTransform(shape_id, transform, false);
	}
}

bool FractureBody::applyImpulse(int con_id, btScalar impulse)
{
	if (con_id < 0) return false;
	btAssert(con_id < m_connections.size());
	bool activate = m_connections[con_id].m_accImpulse < 1E-3;
	m_connections[con_id].m_accImpulse += impulse;
	return activate;
}

btRigidBody* FractureBody::updateConnection(int con_id)
{
	btAssert(con_id >= 0 && con_id < m_connections.size());
	Connection& connection = m_connections[con_id];
	if (m_connections[con_id].m_shapeId < 0)
	{
		return 0;
	}
	if (connection.m_accImpulse > connection.m_elasticLimit)
	{
		if (connection.m_accImpulse > connection.m_plasticLimit)
		{
			return breakConnection(con_id);
		}
		btScalar damage = connection.m_accImpulse - connection.m_elasticLimit;
		connection.m_elasticLimit -= damage * 0.5;
		connection.m_plasticLimit -= damage * 0.5;
	}
	connection.m_accImpulse = 0;
	return 0;
}

btRigidBody* FractureBody::breakConnection(int con_id)
{
	btAssert(con_id >= 0 && con_id < m_connections.size());
	int shape_id = m_connections[con_id].m_shapeId;
	btCompoundShape* compound = static_cast<btCompoundShape*>(m_collisionShape);

	// Init child body.
	btRigidBody* child = m_connections[con_id].m_body;
	btTransform trans = getWorldTransform() * compound->getChildTransform(shape_id);
	child->setWorldTransform(trans);
	child->setLinearVelocity(getVelocityInLocalPoint(trans.getOrigin()-getCenterOfMassPosition()));
	child->setAngularVelocity(btVector3(0, 0, 0));
	setConId(*child->getCollisionShape(), -1);

	// Remove child shape.
	btAssert(child->getCollisionShape() == compound->getChildShape(shape_id));
	compound->removeChildShapeByIndex(shape_id);

	// Execute fracture callback.
	if (m_connections[con_id].m_fracture)
	{
		(*m_connections[con_id].m_fracture)(m_connections[con_id]);
	}

	// Invalidate connection.
	m_connections[con_id].m_shapeId = -1;

	// Update shape id due to shape swapping.
	if (shape_id < compound->getNumChildShapes())
	{
		btCollisionShape* child_shape = compound->getChildShape(shape_id);
		con_id = getConId(*child_shape);
		if (con_id >= 0 && con_id < m_connections.size())
		{
			m_connections[con_id].m_shapeId = shape_id;
		}
	}

	return child;
}

FractureBodyInfo::FractureBodyInfo(btAlignedObjectArray<MotionState>& states) :
	m_shape(new btCompoundShape(false)),
	m_states(states),
	m_inertia(0, 0, 0),
	m_massCenter(0, 0, 0),
	m_mass(0)
{
	// ctor
}

void FractureBodyInfo::addMass(
	const btVector3& position,
	btScalar mass)
{
	m_inertia += getPrincipalInertia(position, mass);
	m_massCenter += position * mass;
	m_mass += mass;
}

void FractureBodyInfo::addBody(
	int shape_id,
	const btVector3& inertia,
	btScalar mass,
	btScalar elasticLimit,
	btScalar plasticLimit)
{
	btCollisionShape* shape = m_shape->getChildShape(shape_id);
	btAssert(!shape->isCompound()); // compound children not suported

	setConId(*shape, m_connections.size());

	btVector3 shape_inertia(inertia);
	if (inertia.isZero())
	{
		shape->calculateLocalInertia(mass, shape_inertia);
	}

	btRigidBody::btRigidBodyConstructionInfo info(mass, 0, shape, shape_inertia);
	btRigidBody* body = new btRigidBody(info);

	FractureBody::Connection connection;
	connection.m_body = body;
	connection.m_elasticLimit = elasticLimit;
	connection.m_plasticLimit = plasticLimit;
	connection.m_accImpulse = 0;
	connection.m_shapeId = shape_id;
	m_connections.push_back(connection);
}

void FractureBodyInfo::addBody(
	const btTransform& localTransform,
	btCollisionShape* shape,
	const btVector3& inertia,
	btScalar mass,
	btScalar elasticLimit,
	btScalar plasticLimit)
{
	m_shape->addChildShape(localTransform, shape);
	addBody(m_shape->getNumChildShapes(), inertia, mass, elasticLimit, plasticLimit);
}

FractureBody::Connection::Connection() :
	m_body(0),
	m_fracture(0),
	m_elasticLimit(0),
	m_plasticLimit(0),
	m_accImpulse(0),
	m_shapeId(-1)
{
	// ctor
}

FractureBody::MotionState::MotionState(FractureBody & body, btMotionState * state) :
	m_body(body),
	m_state(state)
{
	// ctor
}

void FractureBody::MotionState::getWorldTransform(btTransform & worldTrans) const
{
	btAssert(m_state);
	m_state->getWorldTransform(worldTrans);
}

void FractureBody::MotionState::setWorldTransform(const btTransform & worldTrans)
{
	btAssert(m_state);
	m_state->setWorldTransform(worldTrans);

	// children motion states
	const btCompoundShape* compound = static_cast<btCompoundShape*>(m_body.m_collisionShape);
	for (int i = 0; i < compound->getNumChildShapes(); ++i)
	{
		int con_id = getConId(*compound->getChildShape(i));
		if (con_id == -1) continue;

		btAssert(con_id < m_body.m_connections.size());
		btTransform transform = worldTrans * compound->getChildTransform(i);
		m_body.m_connections[con_id].m_body->getMotionState()->setWorldTransform(transform);
	}
}
