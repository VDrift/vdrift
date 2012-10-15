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

#ifndef _FRACTUREBODY_H
#define _FRACTUREBODY_H

#include "BulletDynamics/Dynamics/btRigidBody.h"
#include "LinearMath/btAlignedObjectArray.h"

#define CO_FRACTURE_TYPE (btRigidBody::CO_USER_TYPE)

class btDynamicsWorld;
class btCompoundShape;
struct MotionState;
struct FractureBodyInfo;

class FractureBody : public btRigidBody
{
public:
	FractureBody(const FractureBodyInfo & info);

	// returns >= 0 if shape is connected
	int getConnectionId(int shape_id) const;

	// true if chil is connected
	bool isChildConnected(int i) const;

	// number of connected children
	int getNumChildren() const;

	// true if child connected
	btRigidBody* getChildBody(int i) const;

	// only applied if child is connected to body
	void setChildTransform(int i, const btTransform& transform);

	// apply impulse to connection, return true if connection is activated
	bool applyImpulse(int con_id, btScalar impulse);

	// if accumulated impulse breaks connection return child else null
	btRigidBody* updateConnection(int con_id);

	// center of mass offset from original shape coordinate system
	const btVector3 & getCenterOfMassOffset() const
	{
		return m_centerOfMassOffset;
	}

	struct Connection;

private:
	btAlignedObjectArray<Connection> m_connections;
	btVector3 m_centerOfMassOffset;

	// motion state wrapper, updates children motion states
	class MotionState : public btMotionState
	{
	public:
		MotionState(FractureBody & body, btMotionState * state = 0);
		void getWorldTransform(btTransform & worldTrans) const;
		void setWorldTransform(const btTransform & worldTrans);

	private:
		FractureBody & m_body;
		btMotionState * m_state;
	};
	MotionState m_motionState;

	// separate shape, return child body
	btRigidBody* breakConnection(int con_id);
};

struct FractureBodyInfo
{
	void addMass(
		const btVector3& position,
		btScalar mass);

	void addBody(
		const btTransform& localTransform,
		btCollisionShape* shape,
		const btVector3& inertia,
		btScalar mass,
		btScalar elasticLimit,
		btScalar plasticLimit);

	void addBody(
		int shape_id,
		const btVector3& inertia,
		btScalar mass,
		btScalar elasticLimit,
		btScalar plasticLimit);

	FractureBodyInfo(btAlignedObjectArray<MotionState>& m_states);

	btCompoundShape* m_shape;
	btAlignedObjectArray<MotionState>& m_states;
	btAlignedObjectArray<FractureBody::Connection> m_connections;
	btVector3 m_inertia;
	btVector3 m_massCenter;
	btScalar m_mass;
};

struct FractureCallback
{
	virtual void operator()(FractureBody::Connection & connection) = 0;
	virtual ~FractureCallback() {};
};

struct FractureBody::Connection
{
	btRigidBody* m_body;
	FractureCallback* m_fracture;
	btScalar m_elasticLimit;
	btScalar m_plasticLimit;
	btScalar m_accImpulse;
	int m_shapeId;
	Connection();
};

#endif
