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

static const btScalar deg2rad = M_PI / 180;

CarSuspensionInfo::CarSuspensionInfo() :
	spring_constant(50000.0),
	anti_roll(8000),
	bounce(2500),
	rebound(4000),
	travel(0.2),
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
	orientation_ext(btQuaternion::getIdentity()),
	steering_axis(Direction::up),
	tan_ackermann(0),
	orientation_steer(btQuaternion::getIdentity()),
	orientation(btQuaternion::getIdentity()),
	position(0, 0, 0),
	steering_angle(0),
	overtravel(0),
	displacement(0),
	last_displacement(0),
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

	steering_axis = Direction::up * std::cos(-info.caster * deg2rad) +
		Direction::right * std::sin(-info.caster * deg2rad);

	tan_ackermann = std::tan(info.ackermann * deg2rad);

	position = info.position;
	orientation = orientation_ext;
}

void CarSuspension::SetSteering(btScalar value)
{
	btScalar angle = -value * info.steering_angle * deg2rad;
	if (std::abs(angle) > btScalar(1E-9))
	{
		btScalar t = std::tan(btScalar(M_PI/2) - angle) - tan_ackermann;
		steering_angle = std::copysign(btScalar(M_PI/2), t) - std::atan(t);
		orientation_steer = btQuaternion(steering_axis, steering_angle);
	}
	else
	{
		steering_angle = 0;
		orientation_steer = btQuaternion::getIdentity();
	}
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

	btQuaternion orientation_upright;
	info.GetWheelTransform(displacement, orientation_upright, position);
	orientation = orientation_steer * orientation_ext * orientation_upright;
}

void CarSuspension::UpdateDisplacement(btScalar displacement_delta, btScalar dt)
{/*
	wheel_contact = 1;
	if (displacement_delta < 0)
	{
		// simulate wheel rebound (ignoring relative chassis acceleration)
		btScalar spring_force = displacement * info.spring_constant;
		btScalar damping_force = displacement_delta / dt * info.rebound;
		btScalar force = spring_force + damping_force;
		btScalar new_delta = -btScalar(0.5) * info.inv_mass * force * dt * dt;
		btScalar old_delta = displacement - last_displacement;
		if (old_delta < 0)
			new_delta += old_delta;
		if (new_delta > displacement_delta)
		{
			displacement_delta = new_delta;
			wheel_contact = 0;
		}
	}*/
	SetDisplacement(displacement + displacement_delta);
}

struct Hinge
{
	btVector3 anchor;	///< the point that the wheels are rotated around as the suspension compresses
	btVector3 arm;		///< anchor to wheel vector
	btScalar length_2;	///< arm length squared
	btScalar norm_xy;	///< 1 / arm length in hinge axis normal plane

	Hinge() {}

	Hinge(const btVector3 & hinge_anchor, const btVector3 & hinge_arm) :
		anchor(hinge_anchor),
		arm(hinge_arm),
		length_2(hinge_arm.dot(hinge_arm)),
		norm_xy(1 / std::sqrt(hinge_arm[0] * hinge_arm[0] + hinge_arm[1] * hinge_arm[1]))
	{}

	btVector3 Rotate(btScalar travel) const
	{
		btScalar z = arm[2] + travel;
		btScalar nxy = norm_xy * std::sqrt(Max(length_2 - z * z, btScalar(0)));
		btVector3 arm_new(arm[0] * nxy, arm[1] * nxy, z);

		return anchor + arm_new;
	}
};

// Wishbone suspension with parralel upper and lower hinges
struct BasicSuspension
{
	Hinge hinge;

	BasicSuspension(const btVector3 & wheel, const btVector3 & hinge_body, const btVector3 & hinge_wheel) :
		hinge(wheel - (hinge_wheel - hinge_body), hinge_wheel - hinge_body)
	{}

	void GetWheelTransform(btScalar travel, btQuaternion & rotation, btVector3 & position) const
	{
		rotation = btQuaternion::getIdentity();
		position = hinge.Rotate(travel);
	}
};

struct MacPhersonSuspension
{
	btVector3 wheel_offset;		///< wheel - strut_wheel
	btVector3 upright_top;		///< strut_body attachment point
	btVector3 upright_axis;		///< (strut_body - strut_wheel).normalized()
	Hinge hinge;

	MacPhersonSuspension(
		const btVector3 & wheel,
		const btVector3 & strut_body,
		const btVector3 & strut_wheel,
		const btVector3 & hinge_body) :
		wheel_offset(wheel - strut_wheel),
		upright_top(strut_body),
		upright_axis((strut_body - strut_wheel).normalized()),
		hinge(hinge_body, strut_wheel - hinge_body)
	{}

	void GetWheelTransform(btScalar travel, btQuaternion & rotation, btVector3 & position) const
	{
		btVector3 hinge_end = hinge.Rotate(travel);

		// rotate upright
		btVector3 upright_axis_new = (upright_top - hinge_end).normalized();

		rotation = shortestArcQuat(upright_axis, upright_axis_new);
		position = hinge_end + quatRotate(rotation, wheel_offset);
	}
};

struct WishboneSuspension
{
	btVector3 wheel_offset;
	btVector3 upper_hinge_anchor;
	btVector3 upper_hinge_axis;
	btVector3 upright_axis;
	btScalar upright_length_2;
	btScalar upper_hinge_length_2;
	Hinge lower_hinge;

	// hinges[upper, lower][chassis front, chassis rear, hub]
	WishboneSuspension(const btVector3 & wheel, const btVector3 (&hinges)[2][3])
	{
		btVector3 upright = hinges[0][2] - hinges[1][2];
		btVector3 axis[2], arm[2], anchor[2];
		for (int i = 0; i < 2; ++i)
		{
			auto & hinge = hinges[i];
			axis[i] = (hinge[0] - hinge[1]).normalized();
			btVector3 rear_arm = hinge[2] - hinge[1];
			btVector3 arm_offset = axis[i] * axis[i].dot(rear_arm);
			arm[i] = rear_arm - arm_offset;
			anchor[i] = hinge[1] + arm_offset;
		}

		wheel_offset = wheel - hinges[1][2];
		upper_hinge_anchor = anchor[0];
		upper_hinge_axis = axis[0];
		upright_axis = upright.normalized();
		upright_length_2 = upright.dot(upright);
		upper_hinge_length_2 = arm[0].dot(arm[0]);
		lower_hinge = Hinge(anchor[1], arm[1]);
	}

	void GetWheelTransform(btScalar travel, btQuaternion & rotation, btVector3 & position) const
	{
		btVector3 lower_hinge_end = lower_hinge.Rotate(travel);

		// rotate upright
		// circle - circle intersection in upper hinge axis plane
		btVector3 x = upper_hinge_axis;
		btVector3 v = lower_hinge_end - upper_hinge_anchor;
		btScalar xv = x.dot(v);
		btVector3 u = v - x * xv;
		btScalar u2 = u.dot(u);
		assert(u2 > 0);
		btScalar ru = 1 / std::sqrt(u2);
		btScalar r2 = upright_length_2 - xv * xv;
		btScalar y1 = (u2 - r2 + upper_hinge_length_2) * ru * btScalar(0.5);
		assert(upper_hinge_length_2 > y1 * y1);
		btScalar z1 = std::sqrt(upper_hinge_length_2 - y1 * y1);
		btVector3 y = u * ru;
		btVector3 z = y.cross(x);
		btVector3 upper_hinge_arm = y * y1 + z * std::copysign(z1, z[2]);
		btVector3 upright_axis_new = (upper_hinge_arm - v).normalized();

		rotation = shortestArcQuat(upright_axis, upright_axis_new);
		position = lower_hinge_end + quatRotate(rotation, wheel_offset);
	}
};

template <class Suspension>
void ComputeTransforms(
	const Suspension & s, btScalar travel_min, btScalar travel_max,
	btQuaternion (&orientations)[5], btVector3 (&positions)[5])
{
	btScalar travel_delta = (travel_max - travel_min) / (5 - 1);
	for (int i = 0; i < 5; ++i)
		s.GetWheelTransform(travel_min + travel_delta * i, orientations[i], positions[i]);
}

std::istream & operator>>(std::istream & s, btVector3 & v)
{
	int i = 0;
	std::string token;
	while (std::getline(s, token, ',') && i < 3)
	{
		std::stringstream ss(token);
		ss >> v[i++];
	}
	while (i < 3)
		v[i++] = 0;
	return s;
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
	return true;
}

bool CarSuspension::Load(
	const PTree & cfg_wheel,
	btScalar wheel_mass,
	btScalar wheel_load,
	CarSuspension & suspension,
	std::ostream & error_output)
{
	CarSuspensionInfo info;
	if (!cfg_wheel.get("position", info.position, error_output)) return false;
	if (!cfg_wheel.get("camber", info.camber, error_output)) return false;
	if (!cfg_wheel.get("caster", info.caster, error_output)) return false;
	if (!cfg_wheel.get("toe", info.toe, error_output)) return false;
	cfg_wheel.get("steering", info.steering_angle);
	cfg_wheel.get("ackermann", info.ackermann);
	info.inv_mass = 1 / wheel_mass;

	const PTree * cfg_coil;
	if (!cfg_wheel.get("coilover", cfg_coil, error_output)) return false;
	if (!LoadCoilover(*cfg_coil, info, error_output)) return false;

	// the loaded info.travel is max travel from rest postion under load
	// calculate the extended suspension travel
	btScalar travel_max = info.travel;
	btScalar travel_min = -wheel_load / info.spring_constant;
	info.travel = travel_max - travel_min;

	const PTree * cfg_susp;
	if (cfg_wheel.get("double-wishbone", cfg_susp))
	{
		btVector3 hinges[2][3] ;
		if (!cfg_susp->get("upper-chassis-front", hinges[0][0], error_output)) return false;
		if (!cfg_susp->get("upper-chassis-rear", hinges[0][1], error_output)) return false;
		if (!cfg_susp->get("upper-hub", hinges[0][2], error_output)) return false;
		if (!cfg_susp->get("lower-chassis-front", hinges[1][0], error_output)) return false;
		if (!cfg_susp->get("lower-chassis-rear", hinges[1][1], error_output)) return false;
		if (!cfg_susp->get("lower-hub", hinges[1][2], error_output)) return false;

		WishboneSuspension s(info.position, hinges);
		ComputeTransforms(s, travel_min, travel_max, info.orientations, info.positions);
	}
	else if (cfg_wheel.get("macpherson-strut", cfg_susp))
	{
		btVector3 hinge, strut_top, strut_end;
		if (!cfg_susp->get("hinge", hinge, error_output)) return false;
		if (!cfg_susp->get("strut-top", strut_top, error_output)) return false;
		if (!cfg_susp->get("strut-end", strut_end, error_output)) return false;

		MacPhersonSuspension s(info.position, strut_top, strut_end, hinge);
		ComputeTransforms(s, travel_min, travel_max, info.orientations, info.positions);
	}
	else
	{
		btVector3 chassis, wheel;
		if (!cfg_wheel.get("hinge", cfg_susp, error_output)) return false;
		if (!cfg_susp->get("chassis", chassis, error_output)) return false;
		if (!cfg_susp->get("wheel", wheel, error_output)) return false;

		BasicSuspension s(info.position, chassis, wheel);
		ComputeTransforms(s, travel_min, travel_max, info.orientations, info.positions);
	}

	suspension.Init(info);
	suspension.SetDisplacement(-travel_min);
/*
	const btScalar rad2deg = 180/M_PI;
	btScalar z, y, x;
	suspension.orientation_ext.getEulerZYX(z, y, x);
	error_output << "0 " << y * rad2deg << "\n";
	for (int i = 0; i < 5; ++i) {
		(suspension.orientation_ext * info.orientations[i]).getEulerZYX(z, y, x);
		error_output << travel_min + ((travel_max - travel_min)/4)*i << " " << y * rad2deg << "\n";
	}
	error_output << std::endl;
*/
	return true;
}
