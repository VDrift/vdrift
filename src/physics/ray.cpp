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

#include "ray.h"
#include "BulletCollision/CollisionShapes/btCollisionShape.h"

namespace sim
{

void Ray::set(const btVector3 & rayFrom, const btVector3 & rayDir, btScalar rayLen)
{
	m_closestHitFraction = 1;
	m_rayFrom = rayFrom;
	m_rayTo = rayFrom + rayDir * rayLen;
	m_rayLen = rayLen;
	m_hitNormal = -rayDir;
	m_hitPoint = m_rayTo;
	m_depth = rayLen;
	m_surface = 0;
	m_patch = 0;
	m_patchid = 0;
}

btScalar Ray::addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace)
{
	if (rayResult.m_collisionObject == m_exclude) return 1.0;

	btCollisionShape * s = rayResult.m_collisionObject->getCollisionShape();
	m_closestHitFraction = rayResult.m_hitFraction;
	if (normalInWorldSpace)
	{
		m_hitNormal = rayResult.m_hitNormalLocal;
	}
	else
	{
		m_hitNormal = m_collisionObject->getWorldTransform().getBasis() * rayResult.m_hitNormalLocal;
	}
	m_hitPoint.setInterpolate3(m_rayFrom, m_rayTo, rayResult.m_hitFraction);
	m_collisionObject = rayResult.m_collisionObject;
	m_surface = static_cast<const Surface *>(s->getUserPointer());
	m_depth = m_rayLen * m_closestHitFraction;

	return rayResult.m_hitFraction;
}

}
