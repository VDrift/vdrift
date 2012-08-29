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

#ifndef _SIM_MOTIONSTATE_H
#define _SIM_MOTIONSTATE_H

#include "LinearMath/btMotionState.h"

namespace sim
{

struct MotionState : public btMotionState
{
	btQuaternion rotation;
	btVector3 position;
	btQuaternion massCenterRotation;
	btVector3 massCenterOffset;

	MotionState();

	/// from user to physics
	virtual void getWorldTransform(btTransform& centerOfMassWorldTrans) const;

	/// from physics to user (for active objects)
	virtual void setWorldTransform(const btTransform& centerOfMassWorldTrans);
};

inline MotionState::MotionState() :
	rotation(0,0,0,1),
	position(0,0,0),
	massCenterRotation(0,0,0,1),
	massCenterOffset(0,0,0)
{
	// ctor
}

inline void MotionState::getWorldTransform(btTransform& centerOfMassWorldTrans) const
{
	//centerOfMassWorldTrans = m_graphicsWorldTrans * m_centerOfMassOffset.inverse();
	btQuaternion rot = rotation * massCenterRotation.inverse();
	btVector3 pos = position - quatRotate(rot, massCenterOffset);
	centerOfMassWorldTrans.setRotation(rot);
	centerOfMassWorldTrans.setOrigin(pos);
}

inline void MotionState::setWorldTransform(const btTransform& centerOfMassWorldTrans)
{
	//m_graphicsWorldTrans = centerOfMassWorldTrans * m_centerOfMassOffset;
	rotation = centerOfMassWorldTrans * massCenterRotation;
	position = centerOfMassWorldTrans * massCenterOffset;
}

}

#endif
