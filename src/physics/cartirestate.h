#ifndef _CARTIRESTATE_H
#define _CARTIRESTATE_H

#include "LinearMath/btScalar.h"

class CarTireState
{
public:
	btScalar getCamber() const { return camber; }
	btScalar getSlip() const { return slip; }
	btScalar getSlipAngle() const { return slip_angle; }
	btScalar getIdealSlip() const { return ideal_slip; }
	btScalar getIdealSlipAngle() const { return ideal_slip_angle; }
	btScalar getFx() const { return fx; }
	btScalar getFy() const { return fy; }
	btScalar getMz() const { return mz; }

protected:
	btScalar camber = 0; ///< tire camber angle relative to track surface
	btScalar slip = 0; ///< ratio of tire contact patch speed to road speed
	btScalar slip_angle = 0; ///< the angle between the wheel heading and the wheel velocity
	btScalar ideal_slip = 0; ///< peak force slip ratio
	btScalar ideal_slip_angle = 0; ///< peak force slip angle
	btScalar fx = 0, fy = 0, mz = 0;
};

#endif
