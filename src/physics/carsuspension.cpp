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

#include "carsuspension.h"
#include "coordinatesystem.h"
#include "cfg/ptree.h"

#include <iomanip> // std::setw

static const btScalar deg2rad = M_PI / 180;

CarSuspensionInfo::CarSuspensionInfo() :
	spring_constant(50000.0),
	anti_roll(8000),
	bounce(2500),
	rebound(4000),
	travel(0.2),
	damper_factors(1),
	spring_factors(1),
	steering_angle(0),
	ackermann(0),
	camber(0),
	caster(0),
	toe(0),
	inv_mass(0.05)
{
	// ctor
}

CarSuspension::CarSuspension() :
	orientation_ext(Direction::up, 0),
	steering_axis(Direction::up),
	orientation(orientation_ext),
	position(0, 0, 0),
	steering_angle(0),
	spring_force(0),
	damp_force(0),
	force(0),
	overtravel(0),
	displacement(0),
	last_displacement(0),
	wheel_force(0),
	wheel_contact(0)
{
	// ctor
}

void CarSuspension::Init(const CarSuspensionInfo & info)
{
	this->info = info;

	btQuaternion toe(Direction::up, info.toe * deg2rad);
	btQuaternion cam(Direction::forward, -info.camber * deg2rad);
	orientation_ext = toe * cam;

	steering_axis = Direction::up * btCos(-info.caster * deg2rad) +
		Direction::right * btSin(-info.caster * deg2rad);

	position = info.position;

	SetSteering(0.0);
}

btScalar CarSuspension::GetDisplacement(btScalar force) const
{
	// ignoring spring factors here, duh
	return force / info.spring_constant;
}

void CarSuspension::SetSteering(btScalar value)
{
	btScalar alpha = -value * info.steering_angle * deg2rad;
	steering_angle = 0;
	if (alpha != 0)
	{
		steering_angle = std::atan(1 / (1 / std::tan(alpha) - std::tan(info.ackermann * deg2rad)));
	}
	btQuaternion steer(steering_axis, steering_angle);
	orientation = steer * orientation_ext;
}

void CarSuspension::SetDisplacement(btScalar value)
{
	last_displacement = displacement;
	displacement = value;
	overtravel = 0;
	if (displacement > info.travel)
	{
		overtravel = displacement - info.travel;
		displacement = info.travel;
	}
	else if (displacement < 0)
	{
		displacement = 0;
		wheel_contact = 0;
	}

	// update wheel position
	position = GetWheelPosition(displacement / info.travel);
}

void CarSuspension::UpdateDisplacement(btScalar displacement_delta, btScalar dt)
{
	wheel_contact = 1;
	if (displacement_delta < 0)
	{
		// simulate wheel rebound (ignoring relative chassis acceleration)
		btScalar new_delta = -btScalar(0.5) * info.inv_mass * force * dt * dt;
		btScalar old_delta = displacement - last_displacement;
		if (old_delta < 0)
			new_delta += old_delta;
		if (new_delta > displacement_delta)
		{
			displacement_delta = new_delta;
			wheel_contact = 0;
		}
	}
	SetDisplacement(displacement + displacement_delta);
}

void CarSuspension::UpdateForces(btScalar roll_delta, btScalar dt)
{
	btScalar displacement_delta = displacement - last_displacement;
	btScalar velocity = displacement_delta / dt;
	btScalar damp_factor = info.damper_factors.Interpolate(btFabs(velocity));
	btScalar damping_constant = (displacement_delta < 0) ? info.rebound : info.bounce;
	damp_force = velocity * damping_constant * damp_factor;

	btScalar spring_factor = info.spring_factors.Interpolate(displacement);
	spring_force = displacement * info.spring_constant * spring_factor;

	btScalar anti_roll_force = info.anti_roll * roll_delta;
	force = spring_force + damp_force + anti_roll_force;

	wheel_force = (force * wheel_contact > 0) ? force : 0;
}

class BasicSuspension : public CarSuspension
{
public:
	btVector3 GetWheelPosition(btScalar displacement_fraction)
	{
		btVector3 up(0, 0, 1);
		btVector3 hinge = hinge_arm + up * displacement_fraction * info.travel;
		hinge = hinge.normalized() * hinge_radius;
		return hinge_anchor + hinge;
	}

	void Init(
		const CarSuspensionInfo & info,
		const std::vector<btScalar> & ch,
		const std::vector<btScalar> & wh)
	{
		CarSuspension::Init(info);
		hinge_anchor.setValue(ch[0], ch[1], ch[2]);
		btVector3 wheel_hub(wh[0], wh[1], wh[2]);
		hinge_arm = wheel_hub - hinge_anchor;
		hinge_radius = hinge_arm.length();

		// take hinge arm offset into account
		hinge_anchor += (info.position - wheel_hub);
	}

private:
	btVector3 hinge_anchor;	///< the point that the wheels are rotated around as the suspension compresses
	btVector3 hinge_arm;	///< vector from anchor towards extended position
	btScalar hinge_radius;	///< length of the hinge arm
};

inline btScalar angle_from_sides(btScalar a, btScalar h, btScalar o)
{
	return acos((a * a + h * h - o * o) / (2 * a * h));
}

inline btScalar side_from_angle(btScalar theta, btScalar a, btScalar h)
{
	return sqrt(a * a + h * h - 2 * a * h * cos(theta));
}

class WishboneSuspension : public CarSuspension
{
public:
	WishboneSuspension() : mountrot(btQuaternion::getIdentity()) {}

	btVector3 GetWheelPosition(btScalar displacement_fraction)
	{
		btVector3 rel_uc_uh = (hinge[UPPER_HUB] - hinge[UPPER_CHASSIS]),
			rel_lc_lh = (hinge[LOWER_HUB] - hinge[LOWER_CHASSIS]),
			rel_uc_lh = (hinge[LOWER_HUB] - hinge[UPPER_CHASSIS]),
			rel_lc_uh = (hinge[UPPER_HUB] - hinge[LOWER_CHASSIS]),
			rel_uc_lc = (hinge[LOWER_CHASSIS] - hinge[UPPER_CHASSIS]),
			rel_lh_uh = (hinge[UPPER_HUB] - hinge[LOWER_HUB]),
			localwheelpos = (info.position - hinge[LOWER_HUB]);

		btScalar radlc = angle_from_sides(rel_lc_lh.length(), rel_uc_lc.length(), rel_uc_lh.length());
		btScalar lrotrad = -displacement_fraction * info.travel / rel_lc_lh.length();
		btScalar radlcd = radlc + lrotrad;
		btScalar d_uc_lh = side_from_angle(radlcd, rel_lc_lh.length(), rel_uc_lc.length());
		btScalar radlh = angle_from_sides(rel_lc_lh.length(), rel_lh_uh.length(), rel_lc_uh.length());
		btScalar dradlh = (angle_from_sides(rel_lc_lh.length(), d_uc_lh, rel_uc_lc.length()) +
								angle_from_sides(d_uc_lh, rel_lh_uh.length(), rel_uc_uh.length()));

		btVector3 axiswd = -rel_lc_lh.cross(rel_lh_uh);

		btQuaternion hingerot(axis[LOWER_CHASSIS], lrotrad);
		mountrot.setRotation(axiswd, radlh - dradlh);
		rel_lc_lh = quatRotate(hingerot, rel_lc_lh);
		localwheelpos = quatRotate(mountrot, localwheelpos);

		return hinge[LOWER_CHASSIS] + rel_lc_lh + localwheelpos;
	}

	void Init(
		const CarSuspensionInfo & info,
		const std::vector<btScalar> & up_ch0, const std::vector<btScalar> & up_ch1,
		const std::vector<btScalar> & lo_ch0, const std::vector<btScalar> & lo_ch1,
		const std::vector<btScalar> & up_hub, const std::vector<btScalar> & lo_hub)
	{
		btVector3 hingev[3];
		btScalar innerangle[2];

		CarSuspension::Init(info);

		hingev[0].setValue(up_ch0[0], up_ch0[1], up_ch0[2]);
		hingev[1].setValue(up_ch1[0], up_ch1[1], up_ch1[2]);
		hingev[2].setValue(up_hub[0], up_hub[1], up_hub[2]);

		axis[UPPER_CHASSIS] = (hingev[1] - hingev[0]).normalized();

		innerangle[0] = angle_from_sides(
								(hingev[2] - hingev[0]).length(),
								(hingev[1] - hingev[0]).length(),
								(hingev[2] - hingev[1]).length());

		hinge[UPPER_CHASSIS] = hingev[0] + (axis[UPPER_CHASSIS]	*
				((hingev[2] - hingev[0]).length() * cos(innerangle[0])));

		if (up_hub[1] < hinge[UPPER_CHASSIS][1])
			axis[UPPER_CHASSIS] = -axis[UPPER_CHASSIS];

		hingev[0].setValue(lo_ch0[0], lo_ch0[1], lo_ch0[2]);
		hingev[1].setValue(lo_ch1[0], lo_ch1[1], lo_ch1[2]);
		hingev[2].setValue(lo_hub[0], lo_hub[1], lo_hub[2]);

		axis[LOWER_CHASSIS] = (hingev[1] - hingev[0]).normalized();

		innerangle[1] = angle_from_sides(
								(hingev[2] - hingev[0]).length(),
								(hingev[1] - hingev[0]).length(),
								(hingev[2] - hingev[1]).length());

		hinge[LOWER_CHASSIS] = hingev[0] + (axis[LOWER_CHASSIS]	*
				((hingev[2] - hingev[0]).length() * cos(innerangle[1])));

		if (lo_hub[1] < hinge[LOWER_CHASSIS][1])
			axis[LOWER_CHASSIS] = -axis[LOWER_CHASSIS];

		hinge[UPPER_HUB].setValue(up_hub[0], up_hub[1], up_hub[2]);
		hinge[LOWER_HUB].setValue(lo_hub[0], lo_hub[1], lo_hub[2]);

	}

	virtual void SetSteering(btScalar value)
	{
		CarSuspension::SetSteering(value);
		orientation = orientation * mountrot;
	}

private:
	enum {
		UPPER_CHASSIS = 0,
		LOWER_CHASSIS,
		UPPER_HUB,
		LOWER_HUB
	};

	btVector3 hinge[4], axis[2];
	btQuaternion mountrot;
};

class MacPhersonSuspension : public CarSuspension
{
public:
	btVector3 GetWheelPosition(btScalar displacement_fraction)
	{
		btVector3 up (0, 0, 1);
		btVector3 hinge_end = strut.end - strut.hinge;
		btVector3 end_top = strut.top - strut.end;
		btVector3 hinge_top = strut.top - strut.hinge;

		btVector3 rotaxis = up.cross(hinge_end).normalized();
		btVector3 localwheelpos = info.position - strut.end;

		btScalar hingeradius = hinge_end.length();
		btScalar disp_rad = asin(displacement_fraction * info.travel / hingeradius);

		btQuaternion hingerotate(rotaxis, -disp_rad);

		btScalar e_angle = angle_from_sides(end_top.length(), hinge_end.length(), hinge_top.length());

		hinge_end = quatRotate(hingerotate, hinge_end);

		btScalar e_angle_disp = angle_from_sides(end_top.length(), hinge_end.length(), hinge_top.length());

		rotaxis = up.cross(end_top).normalized();

		mountrot.setRotation(rotaxis, e_angle_disp - e_angle);
		localwheelpos = quatRotate(mountrot, localwheelpos);

		return localwheelpos + strut.hinge + hinge_end;
	}

	void Init(
		const CarSuspensionInfo & info,
		const std::vector<btScalar> & top,
		const std::vector<btScalar> & end,
		const std::vector<btScalar> & hinge)
	{
		CarSuspension::Init(info);
		strut.top.setValue(top[0], top[1], top[2]);
		strut.end.setValue(end[0], end[1], end[2]);
		strut.hinge.setValue(hinge[0], hinge[1], hinge[2]);
	}

	virtual void SetSteering(btScalar value)
	{
		CarSuspension::SetSteering(value);
		orientation = orientation * mountrot;
	}

private:
	enum {
		UPPER_CHASSIS = 0,
		LOWER_CHASSIS,
		UPPER_HUB,
		LOWER_HUB
	};

	struct {
		btVector3 top;
		btVector3 end;
		btVector3 hinge;
	} strut;

	btQuaternion mountrot;
};

// 1-9 points
static void LoadPoints(const PTree & cfg, const std::string & name, LinearInterp<btScalar> & points)
{
	int i = 1;
	std::ostringstream s;
	s << std::setw(1) << i;
	std::vector<btScalar> point(2);
	while (cfg.get(name+s.str(), point) && i < 10)
	{
		s.clear();
		s << std::setw(1) << ++i;
		points.AddPoint(point[0], point[1]);
	}
}

static bool LoadCoilover(
	const PTree & cfg,
	CarSuspensionInfo & info,
	std::ostream & error_output)
{
	if (!cfg.get("spring-constant", info.spring_constant, error_output)) return false;
	if (!cfg.get("bounce", info.bounce, error_output)) return false;
	if (!cfg.get("rebound", info.rebound, error_output)) return false;
	if (!cfg.get("travel", info.travel, error_output)) return false;
	if (!cfg.get("anti-roll", info.anti_roll, error_output)) return false;
	LoadPoints(cfg, "damper-factor-", info.damper_factors);
	LoadPoints(cfg, "spring-factor-", info.spring_factors);
	return true;
}

bool CarSuspension::Load(
	const PTree & cfg_wheel,
	btScalar wheel_mass,
	CarSuspension *& suspension,
	std::ostream & error_output)
{
	CarSuspensionInfo info;
	std::vector<btScalar> p(3);
	if (!cfg_wheel.get("position", p, error_output)) return false;
	if (!cfg_wheel.get("camber", info.camber, error_output)) return false;
	if (!cfg_wheel.get("caster", info.caster, error_output)) return false;
	if (!cfg_wheel.get("toe", info.toe, error_output)) return false;
	cfg_wheel.get("steering", info.steering_angle);
	cfg_wheel.get("ackermann", info.ackermann);
	info.position.setValue(p[0], p[1], p[2]);
	info.inv_mass = 1 / wheel_mass;

	const PTree * cfg_coil;
	if (!cfg_wheel.get("coilover", cfg_coil, error_output)) return false;
	if (!LoadCoilover(*cfg_coil, info, error_output)) return false;

	const PTree * cfg_susp;
	if (cfg_wheel.get("macpherson-strut", cfg_susp))
	{
		std::vector<btScalar> strut_top(3), strut_end(3), hinge(3);

		if (!cfg_susp->get("hinge", hinge, error_output)) return false;
		if (!cfg_susp->get("strut-top", strut_top, error_output)) return false;
		if (!cfg_susp->get("strut-end", strut_end, error_output)) return false;

		MacPhersonSuspension * mps = new MacPhersonSuspension();
		mps->Init(info, strut_top, strut_end, hinge);
		suspension = mps;
	}
/*	else if (cfg_wheel.get("double-wishbone", cfg_susp))
	{
		std::vector<btScalar> up_ch0(3), up_ch1(3), lo_ch0(3), lo_ch1(3), up_hub(3), lo_hub(3);

		if (!cfg_susp->get("upper-chassis-front", up_ch0, error_output)) return false;
		if (!cfg_susp->get("upper-chassis-rear", up_ch1, error_output)) return false;
		if (!cfg_susp->get("lower-chassis-front", lo_ch0, error_output)) return false;
		if (!cfg_susp->get("lower-chassis-rear", lo_ch1, error_output)) return false;
		if (!cfg_susp->get("upper-hub", up_hub, error_output)) return false;
		if (!cfg_susp->get("lower-hub", lo_hub, error_output)) return false;

		WishboneSuspension * wbs = new WishboneSuspension();
		wbs->Init(info, up_ch0, up_ch1, lo_ch0, lo_ch1, up_hub, lo_hub);
		suspension = wbs;
	}*/
	else
	{
		std::vector<btScalar> ch(3, 0), wh(3, 0);

		if (!cfg_wheel.get("hinge", cfg_susp, error_output)) return false;
		if (!cfg_susp->get("chassis", ch, error_output)) return false;
		if (!cfg_susp->get("wheel", wh, error_output)) return false;

		BasicSuspension * bs = new BasicSuspension();
		bs->Init(info, ch, wh);
		suspension = bs;
	}

	return true;
}
