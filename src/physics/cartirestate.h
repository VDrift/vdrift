#ifndef _CARTIRESTATE_H
#define _CARTIRESTATE_H

#include "LinearMath/btScalar.h"
#include "fastmath.h"
#include "minmax.h"

struct CarTireState
{
	btScalar camber = 0; ///< tire camber angle relative to track surface
	btScalar slip = 0; ///< ratio of tire contact patch speed to road speed
	btScalar slip_angle = 0; ///< the angle between the wheel heading and the wheel velocity
	btScalar ideal_slip = 0; ///< peak force slip ratio
	btScalar ideal_slip_angle = 0; ///< peak force slip angle
	btScalar fx = 0; ///< positive during traction
	btScalar fy = 0; ///< positive in a right turn
	btScalar mz = 0; ///< positive in a left turn
};

// approximate asin(x) = x + x^3/6 for +-18 deg range
inline btScalar ComputeCamberAngle(btScalar sin_camber)
{
	btScalar sc = Clamp(sin_camber, btScalar(-0.3), btScalar(0.3));
	return (btScalar(1/6.0) * sc) * (sc * sc) + sc;
}

inline void ComputeSlip(
	btScalar vlon, btScalar vlat, btScalar vrot,
	btScalar & slip_ratio, btScalar & slip_angle)
{
	btScalar rvlon = 1 / Max(std::abs(vlon), btScalar(1E-3));
	btScalar vslip = vrot - vlon;
	slip_ratio = vslip * rvlon;
	slip_angle = -Atan(vlat * rvlon);
}

#endif
