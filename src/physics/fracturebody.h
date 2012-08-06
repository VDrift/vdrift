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

#ifndef _SIM_FRACTURE_BODY
#define _SIM_FRACTURE_BODY

#include "BulletDynamics/Dynamics/btRigidBody.h"
#include "LinearMath/btAlignedObjectArray.h"

#define CO_FRACTURE_TYPE (btRigidBody::CO_USER_TYPE)

class btDynamicsWorld;
class btCompoundShape;

namespace sim
{

struct MotionState;
struct FractureBodyInfo;

class FractureBody : public btRigidBody
{
public:
	FractureBody(const FractureBodyInfo & info);

	/// returns >= 0 if shape is connected
	int getConnectionId(int shape_id) const;

	/// true if child connected
	bool isChildConnected(int i) const;

	/// only applied if child is connected to body
	void setChildTransform(int i, const btTransform& transform);

	/// apply impulse to connection, return true if connection is activated
	bool applyImpulse(int con_id, btScalar impulse);

	/// if accumulated impulse breaks connection return child else null
	btRigidBody* updateConnection(int con_id);

	/// remove connections, remove child shapes from world
	void clear(btDynamicsWorld& world);

	/// center of mass offset from original shape coordinate system
	const btVector3 & getCenterOfMassOffset() const;

	struct Connection;

private:
	btAlignedObjectArray<Connection> m_connections;
	btVector3 m_centerOfMassOffset;

	/// motion state wrapper, updates children motion states
	class FrMotionState : public btMotionState
	{
	public:
		FrMotionState(FractureBody & body, btMotionState * state = 0);
		~FrMotionState();
		void getWorldTransform(btTransform & worldTrans) const;
		void setWorldTransform(const btTransform & worldTrans);

	private:
		FractureBody & m_body;
		btMotionState * m_state;
	};
	FrMotionState m_motionState;

	/// separate shape, return child body
	btRigidBody* breakConnection(int con_id);
};

struct FractureBodyInfo
{
	btCompoundShape* m_shape;
    btAlignedObjectArray<MotionState*>& m_states;
	btAlignedObjectArray<FractureBody::Connection> m_connections;
	btVector3 m_inertia;
	btVector3 m_massCenter;
	btScalar m_mass;

    FractureBodyInfo(btAlignedObjectArray<MotionState*>& m_states);	

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

inline const btVector3 & FractureBody::getCenterOfMassOffset() const
{
	return m_centerOfMassOffset;
}

}

#endif
