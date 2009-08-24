#ifndef _COLLISION_DETECTION_H
#define _COLLISION_DETECTION_H

#include <ostream>
#include <list>
#include <set>
#include <map>

#include "mathvector.h"
#include "quaternion.h"
#include "vertexarray.h"
#include "aabb.h"
#include "aabb_space_partitioning.h"

#include "btBulletCollisionCommon.h"
//#include "BulletCollision/Gimpact/btGImpactShape.h"
//#include "BulletCollision/Gimpact/btGImpactCollisionAlgorithm.h"

namespace COLLISION_DETECTION
{
btVector3 ToBulletVector(const MATHVECTOR <float, 3> & v);
MATHVECTOR <float, 3> ToMathVector(const btVector3 & v);
btQuaternion ToBulletQuaternion(const QUATERNION <float> & q);
QUATERNION <float> ToMathQuaternion(const btQuaternion & q);
};

struct	MultipleRayResultCallback : public btCollisionWorld::RayResultCallback
{
	MultipleRayResultCallback(const btVector3& rayFromWorld,const btVector3& rayToWorld) : m_rayFromWorld(rayFromWorld), m_rayToWorld(rayToWorld) {}
	btVector3	m_rayFromWorld;//used to calculate hitPointWorld from hitFraction
	btVector3	m_rayToWorld;
	
	std::list <btCollisionWorld::LocalRayResult> results;
	
	virtual	btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult,bool normalInWorldSpace)
	{
		m_closestHitFraction = rayResult.m_hitFraction;
		results.push_back(rayResult);
		
		if (!normalInWorldSpace)
		{
			///need to transform normal into worldspace
			results.back().m_hitNormalLocal = rayResult.m_collisionObject->getWorldTransform().getBasis()*rayResult.m_hitNormalLocal;
		}
		
		return 1.0;
	}
};

struct COLLISION_OBJECT_VERTEX
{
	float vertex[3];
};

struct COLLISION_OBJECT_FACE
{
	int index[3];
};

class COLLISION_OBJECT_SETTINGS
{
	public:
		enum OBJECT_TYPE
		{
			STATIC,
   			DYNAMIC
		};
	private:
		OBJECT_TYPE type;
		void * objid;
		unsigned short int mask;
		unsigned short int group;
	
	public:
		COLLISION_OBJECT_SETTINGS() : type(STATIC), objid(NULL), mask(1), group(1) {}
		void SetStaticObject() {type = STATIC;}
		void SetDynamicObject() {type = DYNAMIC;}
		void SetObjectID(void * newobjid) {objid = newobjid;}
	
		OBJECT_TYPE GetType() const {return type;}
		const void * ObjID() const {return objid;}
		
		///groupid is 0 through 15.  by default objects are members of group 0. objects can be members of multiple groups.
		void SetDynamicObjectGroup(unsigned int groupid, bool isamember)
		{
			unsigned int bits = 1 << (groupid);
			if (isamember)
				group = group | bits;
			else
				group = group & (~bits);
		}
		
		///maskid is 0 through 15.  by default objects have mask 0 enabled. objects can have multiple masks enabled.
		///objects that have a mask enabled will collide with objects of that group.
		void SetDynamicObjectMask(unsigned int maskid, bool enable)
		{
			unsigned int bits = 1 << (maskid);
			if (enable)
				mask = mask | bits;
			else
				mask = mask & (~bits);
		}

		unsigned short int GetMask() const
		{
			return mask;
		}
	
		unsigned short int GetGroup() const
		{
			return group;
		}
};

class COLLISION_OBJECT
{
	private:
		btCollisionObject id;
		btCollisionShape * shape;
		btTriangleIndexVertexArray * trimesh_varray;
	
		bool loaded;
		
		AABB <float> bbox;
	
		COLLISION_OBJECT_SETTINGS settings;

	public:
		COLLISION_OBJECT() : shape(NULL),trimesh_varray(NULL),loaded(false) {}
		~COLLISION_OBJECT() {DeInit();}
	
		AABB <float> GetBBOX() const;
		btCollisionObject & GetBulletObject() {return id;}
	
		void InitTrimesh(const float * vertices, int vstride, int vcount, const int * faces, int fcount, int istride, const float * normals, const COLLISION_OBJECT_SETTINGS & objsettings);
		void InitTrimesh(const VERTEXARRAY & varray, const COLLISION_OBJECT_SETTINGS & objsettings);
		void InitConvexHull(const std::vector <float> & varray, const COLLISION_OBJECT_SETTINGS & objsettings);
		void InitBox(const MATHVECTOR <float, 3> & halfextents, const COLLISION_OBJECT_SETTINGS & objsettings);
		
		///the cylinder is pointing along the Z axis and the half dimensions are provided in the halfextents vector
		void InitCylinderZ(const MATHVECTOR <float, 3> & halfextents, const COLLISION_OBJECT_SETTINGS & objsettings);
		
		void DeInit();
		void SetPosition(const MATHVECTOR <float, 3> & newpos);
		void SetQuaternion(const QUATERNION <float> & newquat);
		MATHVECTOR <float, 3> GetPosition() const;
	
		COLLISION_OBJECT_SETTINGS & GetSettings() {return settings;}
		const COLLISION_OBJECT_SETTINGS & GetSettings() const {return settings;}
		
		const void * ObjID() const {return settings.ObjID();}
};

class COLLISION_CONTACT
{
	private:
		MATHVECTOR <float, 3> position;
		MATHVECTOR <float, 3> normal;
		float depth;
		COLLISION_OBJECT * col1, * col2;

	public:
		COLLISION_CONTACT() : depth(0), col1(NULL), col2(NULL) {}
		//void SetFromODEContactGeom(dContactGeom odecontactgeom, const PHYSICSCOLLISIONSETTINGS & colsettings);
		//void SetFromBulletContact(btCollisionWorld::LocalRayResult * contact, const PHYSICSCOLLISIONSETTINGS & colsettings);
		const MATHVECTOR <float, 3> & GetContactPosition() const {return position;}
		const MATHVECTOR <float, 3> & GetContactNormal() const {return normal;}
		float GetContactDepth() const {return depth;}
		bool operator<(const COLLISION_CONTACT& other) {return depth < other.depth;}
		COLLISION_OBJECT * GetCollidingObject1() {return col1;}
		COLLISION_OBJECT * GetCollidingObject2() {return col2;}
		void Set(const MATHVECTOR <float, 3> & pos, const MATHVECTOR <float, 3> & norm, float d, COLLISION_OBJECT * c1, COLLISION_OBJECT * c2)
		{
			position = pos;
			normal = norm;
			depth = d;
			col1 = c1;
			col2 = c2;
		}
		bool CollideRay(const MATHVECTOR <float, 3> & origin, const MATHVECTOR <float, 3> & direction, const float length, COLLISION_CONTACT & output_contact) const;
};

class COLLISION_SETTINGS
{
	private:
		bool staticcollide;
		bool dynamiccollide;
		std::list <void *> exception_objectids;
	
	public:
		COLLISION_SETTINGS() : staticcollide(true),dynamiccollide(false) {}
		
		void SetExceptionObjectID(void * except_id) {exception_objectids.push_back(except_id);}
		const std::list <void *> & GetExceptionObjectIDs() const {return exception_objectids;}

		void SetStaticCollide ( bool value )
		{
			staticcollide = value;
		}
	
		bool GetStaticCollide() const
		{
			return staticcollide;
		}
	
		void SetDynamicCollide ( bool value )
		{
			dynamiccollide = value;
		}
	
		bool GetDynamicCollide() const
		{
			return dynamiccollide;
		}
		
		bool CanCollide(const COLLISION_OBJECT & obj) const
		{
			return (staticcollide && obj.GetSettings().GetType() == COLLISION_OBJECT_SETTINGS::STATIC) ||
				   (dynamiccollide && obj.GetSettings().GetType() == COLLISION_OBJECT_SETTINGS::DYNAMIC);
		}
};

class COLLISION_WORLD
{
	private:
		btDefaultCollisionConfiguration collisionconfig;
		mutable btCollisionDispatcher collisiondispatcher; //the dispatcher unfortunately likes to allocate memory when we do queries, so we have to make it mutable to keep our collision query interface const
		bt32BitAxisSweep3 collisionbroadphase;
		//btSimpleBroadphase collisionbroadphase;
		mutable btCollisionWorld id; //we want to keep collision queries const
		
		AABB_SPACE_PARTITIONING_NODE <COLLISION_OBJECT *> colspeedup;
		std::set <COLLISION_OBJECT *> dynamic_objects;

		bool PassesFilter(const COLLISION_SETTINGS & settings, void * checkme) const;
		
		struct BulletBroadphaseFilterCallback : public btOverlapFilterCallback
		{
        	// return true when pairs need collision
			virtual bool needBroadphaseCollision(btBroadphaseProxy* proxy0,btBroadphaseProxy* proxy1) const
			{
				bool collides = (proxy0->m_collisionFilterGroup & proxy1->m_collisionFilterMask) != 0;
				collides = collides && (proxy1->m_collisionFilterGroup & proxy0->m_collisionFilterMask);
				btCollisionObject * obj0 = reinterpret_cast<btCollisionObject*>(proxy0->m_clientObject);
				btCollisionObject * obj1 = reinterpret_cast<btCollisionObject*>(proxy1->m_clientObject);
				if (obj0 && obj1)
				{
					COLLISION_OBJECT * col0 = reinterpret_cast<COLLISION_OBJECT*>(obj0->getUserPointer());
					COLLISION_OBJECT * col1 = reinterpret_cast<COLLISION_OBJECT*>(obj1->getUserPointer());
					if (col0 && col1)
					{
						collides = collides && (col0->ObjID() != col1->ObjID());
					}
				}
				return collides;
			}
		} bulletbroadphasefiltercallback;

	public:
		COLLISION_WORLD() : collisionconfig(), collisiondispatcher(&collisionconfig), collisionbroadphase(btVector3(-10000, -10000, -10000), btVector3(10000, 10000, 10000)), id(&collisiondispatcher, &collisionbroadphase, &collisionconfig)
		{
			//btGImpactCollisionAlgorithm::registerAlgorithm(&collisiondispatcher);
		}
		//COLLISION_WORLD() : collisionconfig(), collisiondispatcher(&collisionconfig), collisionbroadphase(), id(&collisiondispatcher, &collisionbroadphase, &collisionconfig) {}
		~COLLISION_WORLD() {Clear();}
		
		void DebugPrint(std::ostream & out)
		{
			std::stringstream junk;
			int numobj(0);
			colspeedup.DebugPrint(0, numobj, true, junk);
			out << "Collision objects: " << numobj << " (internal), " << id.getNumCollisionObjects() << " (external)" << std::endl;
		}
		
		void CollideRay(const MATHVECTOR <float, 3> & position, const MATHVECTOR <float, 3> & direction, const float length, std::list <COLLISION_CONTACT> & outputcontactlist, const COLLISION_SETTINGS & settings) const;
		void CollideObject(COLLISION_OBJECT & object, std::list <COLLISION_CONTACT> & outputcontactlist, const COLLISION_SETTINGS & settings) const;
		
		///slow delete
		void DeletePhysicsObject(COLLISION_OBJECT * objtodel)
		{
			assert(objtodel);
			if (objtodel->GetSettings().GetType() == COLLISION_OBJECT_SETTINGS::DYNAMIC)
			{
				id.removeCollisionObject(&objtodel->GetBulletObject());
				dynamic_objects.erase(objtodel);
			}
			else
				colspeedup.Delete(objtodel);
		}
		
		///fast delete
		void DeletePhysicsObject(COLLISION_OBJECT * objtodel, AABB <float> & bbox)
		{
			assert(objtodel);
			if (objtodel->GetSettings().GetType() == COLLISION_OBJECT_SETTINGS::DYNAMIC)
			{
				id.removeCollisionObject(&objtodel->GetBulletObject());
				dynamic_objects.erase(objtodel);
			}
			else
				colspeedup.Delete(objtodel, bbox);
		}
		
		void Clear()
		{
			for (std::set <COLLISION_OBJECT *>::iterator i = dynamic_objects.begin();
				i != dynamic_objects.end(); ++i)
			{
				id.removeCollisionObject(&(*i)->GetBulletObject());
			}
			dynamic_objects.clear();
			colspeedup.Clear();
		}
		
		void OptimizeObjects() {colspeedup.Optimize();}
		
		void AddPhysicsObject(COLLISION_OBJECT & object)
		{
			COLLISION_OBJECT * obj = &object;
			
			if (object.GetSettings().GetType() == COLLISION_OBJECT_SETTINGS::DYNAMIC)
			{
				short int group = 1;
				short int mask = 1;
				btCollisionObject * colobj = &object.GetBulletObject();
				assert(colobj);
				id.addCollisionObject(colobj, group, mask);
				dynamic_objects.insert(&object);
				//std::cout << "Adding dynamic collision object." << std::endl;
			}
			else
			{
				AABB <float> bbox = object.GetBBOX();
				colspeedup.Add(obj, bbox);
			}
		}
		
		void CollideDynamicObjects(std::map <COLLISION_OBJECT *, std::list <COLLISION_CONTACT> > & outputcontactlist) const;
		
		void CollideBox(const MATHVECTOR <float, 3> & position, const QUATERNION <float> & orientation, const MATHVECTOR <float, 3> & dimensions, std::list <COLLISION_CONTACT> & outputcontactlist, const COLLISION_SETTINGS & settings) const;
		
		void CollideMovingBox(const MATHVECTOR <float, 3> & position, const MATHVECTOR <float, 3> & velocity, const QUATERNION <float> & orientation, const MATHVECTOR <float, 3> & half_dimensions, std::list <COLLISION_CONTACT> & outputcontactlist, const COLLISION_SETTINGS & settings, float dt) const;
		
		///prevent dynamic objects that have ObjIDs pointing to the same place from colliding
		void FilterOutDynamicCollisionsFromEqualObjectIDs()
		{
			id.getPairCache()->setOverlapFilterCallback(&bulletbroadphasefiltercallback);
		}
};

#endif
