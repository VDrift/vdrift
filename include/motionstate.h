#ifndef _MOTIONSTATE_H
#define _MOTIONSTATE_H

#include "LinearMath/btMotionState.h"

struct MotionState : public btMotionState {
	btTransform m_graphicsWorldTrans;
	btTransform	m_centerOfMassOffset;

	MotionState(const btTransform& startTrans = btTransform::getIdentity(), const btTransform& centerOfMassOffset = btTransform::getIdentity())
	: m_graphicsWorldTrans(startTrans), m_centerOfMassOffset(centerOfMassOffset)
	{
	}

	/// from user to physics
	virtual void getWorldTransform(btTransform& centerOfMassWorldTrans) const 
	{
		centerOfMassWorldTrans = m_graphicsWorldTrans * m_centerOfMassOffset.inverse();
	}

	/// from physics to user (for active objects)
	virtual void setWorldTransform(const btTransform& centerOfMassWorldTrans)
	{
		m_graphicsWorldTrans = centerOfMassWorldTrans * m_centerOfMassOffset ;
	}
};

#endif // _MOTIONSTATE_H
