#ifndef _MOTIONSTATE_H
#define _MOTIONSTATE_H

#include "LinearMath/btMotionState.h"

struct MotionState : public btMotionState {
	btQuaternion rotation;
	btVector3 position;
	btQuaternion massCenterRotation;
	btVector3 massCenterOffset;

	MotionState() : rotation(0,0,0,1), position(0,0,0),
		massCenterRotation(0,0,0,1), massCenterOffset(0,0,0)
	{
		// ctor
	}

	/// from user to physics
	virtual void getWorldTransform(btTransform& centerOfMassWorldTrans) const
	{
		//centerOfMassWorldTrans = m_graphicsWorldTrans * m_centerOfMassOffset.inverse();
		btQuaternion rot = rotation * massCenterRotation.inverse();
		btVector3 pos = position - quatRotate(rot, massCenterOffset);
		centerOfMassWorldTrans.setRotation(rot);
		centerOfMassWorldTrans.setOrigin(pos);
	}

	/// from physics to user (for active objects)
	virtual void setWorldTransform(const btTransform& centerOfMassWorldTrans)
	{
		//m_graphicsWorldTrans = centerOfMassWorldTrans * m_centerOfMassOffset;
		rotation = centerOfMassWorldTrans * massCenterRotation;
		position = centerOfMassWorldTrans * massCenterOffset;
	}
};

#endif
