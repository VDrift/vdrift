#ifndef BT_FRACTURE_BODY
#define BT_FRACTURE_BODY

class btDynamicsWorld;
class btCollisionWorld;
class btCompoundShape;
class btManifoldPoint;

#include "LinearMath/btAlignedObjectArray.h"
#include "BulletDynamics/Dynamics/btRigidBody.h"

#define CUSTOM_FRACTURE_TYPE (btRigidBody::CO_USER_TYPE*2)

struct btConnection
{
	btRigidBody* m_body;
	btScalar m_strength;
	btScalar m_elasticLimit;
	btScalar m_accImpulse;
	btConnection() : m_body(0), m_strength(0), m_elasticLimit(0), m_accImpulse(0) {}
};

class FractureBody : public btRigidBody
{
public:
	btAlignedObjectArray<btConnection> m_connections;
	int m_shapeOffset; // fracture body shape offset in btCompoundShape
	
	// the collisionshape has to be a compound shape containing parent body children shapes
	FractureBody(const btRigidBodyConstructionInfo& info);
	
	void addConnection(btRigidBody* body, const btTransform& offset, btScalar strength, btScalar elasticLimit);
	
	btRigidBody* breakConnection(int i);
	
	// to be called after adding/breaking connections
	void updateMassCenter();
	
	static btCompoundShape* shiftTransform(btCompoundShape* compound, btScalar* masses, btTransform& shift, btVector3& principalInertia);
};

#endif //BT_FRACTURE_BODY
