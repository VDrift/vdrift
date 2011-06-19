#ifndef BT_FRACTURE_BODY
#define BT_FRACTURE_BODY

class btDynamicsWorld;
class btCollisionWorld;
class btCompoundShape;
class btManifoldPoint;

#include "LinearMath/btAlignedObjectArray.h"
#include "BulletDynamics/Dynamics/btRigidBody.h"

#define CUSTOM_FRACTURE_TYPE (btRigidBody::CO_USER_TYPE*2)

template <typename T0, typename T1> T0 cast(T1 t)
{
	union {T0 t0; T1 t1;} cast;
	cast.t1 = t;
	return cast.t0;
}

class FractureBody : public btRigidBody
{
public:
	struct Connection
	{
		btRigidBody* m_body;
		btScalar m_strength;
		btScalar m_elasticLimit;
		btScalar m_accImpulse;
		int m_shapeId;

		Connection() :
			m_body(0), m_strength(0), m_elasticLimit(0), m_accImpulse(0), m_shapeId(-1)
		{
			// ctor
		}
	};
	btAlignedObjectArray<Connection> m_connections;

	// the collisionshape has to be a compound shape containing parent body children shapes
	FractureBody(const btRigidBodyConstructionInfo& info);

	void addShape(const btTransform& localTransform, btCollisionShape* shape);

	// body collision shape user pointer will used to store the connection index
	void addConnection(btRigidBody* body, const btTransform& localTransform, btScalar strength, btScalar elasticLimit);

	btRigidBody* breakConnection(int i);

	// to be called after adding/breaking connections
	void updateMassCenter();

	// sync child body motionstates
	void updateMotionState();

	static btCompoundShape* shiftTransform(btCompoundShape* compound, btScalar* masses, btTransform& shift, btVector3& principalInertia);
};

#endif //BT_FRACTURE_BODY
