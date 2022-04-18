#ifndef _CARTIREBASE_H
#define _CARTIREBASE_H

#include "LinearMath/btScalar.h"
#include "fastmath.h"
#include "minmax.h"

/// approximate asin(x) = x + x^3/6 for +-18 deg range
inline btScalar ComputeCamberAngle(btScalar sin_camber)
{
	btScalar sc = Clamp(sin_camber, btScalar(-0.3), btScalar(0.3));
	return (btScalar(1/6.0) * (sc * sc) + 1 ) * sc;
}

/// induced lateral slip velocity from camber induced slip angle sa and lon velocity vx
inline btScalar ComputeCamberVelocity(btScalar sa, btScalar vx)
{
	btScalar tansa = (btScalar(1/3.0) * (sa * sa) + 1) * sa; // small angle tan approximation
	return tansa * vx;
}

/// slip and slip angle from longitudinal (vx), lateral (vy), rotational (wr) velocity
inline void ComputeSlip(
	btScalar vlon, btScalar vlat, btScalar vrot,
	btScalar & slip_ratio, btScalar & slip_angle)
{
	btScalar rvlon = 1 / Max(std::abs(vlon), btScalar(1E-3));
	btScalar vslip = vrot - vlon;
	slip_ratio = vslip * rvlon;
	slip_angle = -Atan(vlat * rvlon);
}

struct CarTireSlipLUT
{
	/// slip and slip_angle at peak force for given fz
	void get(btScalar fz, btScalar & s, btScalar & a) const
	{
		btScalar n = Clamp(fz * rdelta() - 1, btScalar(0), btScalar(size() - 1 - 1E-6));
		int i = n;
		btScalar blend = n - i;
		auto sn = ideal_slip_lut[i];
		auto sm = ideal_slip_lut[i+1];
		s = sn[0] + (sm[0] - sn[0]) * blend;
		a = sn[1] + (sm[1] - sn[1]) * blend;
	}

	constexpr int size() const
	{
		return sizeof(ideal_slip_lut) / sizeof(ideal_slip_lut[0]);
	}

	constexpr btScalar delta() const { return btScalar(500); }

	constexpr btScalar rdelta() const { return btScalar(1/500.0); }

	btScalar ideal_slip_lut[20][2];  ///< peak force slip ratio and angle [rad]
};

struct CarTireState
{
	btScalar friction = 0; ///< surface friction coefficient
	btScalar camber = 0; ///< tire camber angle relative to track surface
	btScalar vcam = 0; ///< camber thrust induced lateral slip velocity
	btScalar slip = 0; ///< ratio of tire contact patch speed to road speed
	btScalar slip_angle = 0; ///< the angle between the wheel heading and the wheel velocity
	btScalar ideal_slip = 0; ///< peak force slip ratio
	btScalar ideal_slip_angle = 0; ///< peak force slip angle
	btScalar fx = 0; ///< positive during traction
	btScalar fy = 0; ///< positive in a right turn
	btScalar mz = 0; ///< positive in a left turn
};

#endif
