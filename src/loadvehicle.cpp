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

#include "loadvehicle.h"
#include "loadcollisionshape.h"
#include "physics/vehicleinfo.h"
#include "cfg/ptree.h"
#include "spline.h"

#include "BulletCollision/CollisionShapes/btMultiSphereShape.h"
#include "BulletCollision/CollisionShapes/btCompoundShape.h"
#include "BulletCollision/CollisionShapes/btCylinderShape.h"

static inline std::istream & operator >> (std::istream & lhs, btVector3 & rhs)
{
	char sep;
	return lhs >> rhs[0] >> sep >> rhs[1] >> sep >> rhs[2];
}

static bool LoadClutch(
	const PTree & cfg,
	sim::ClutchInfo & info,
	std::ostream & error)
{
	const PTree * cfg_clutch;
	if (!cfg.get("clutch", cfg_clutch, error)) return false;
	if (!cfg_clutch->get("sliding", info.friction, error)) return false;
	if (!cfg_clutch->get("radius", info.radius, error)) return false;
	if (!cfg_clutch->get("area", info.area, error)) return false;
	if (!cfg_clutch->get("max-pressure", info.max_pressure, error)) return false;
	return true;
}

static bool LoadTransmission(
	const PTree & cfg,
	sim::TransmissionInfo & info,
	std::ostream & error)
{
	const PTree * cfg_trans;
	if (!cfg.get("transmission", cfg_trans, error)) return false;

	if (!cfg_trans->get("gears", info.forward_gears, error)) return false;

	info.gear_ratios.resize(info.forward_gears + 2);
	info.reverse_gears = 1;
	if (!cfg_trans->get("gear-ratio-r", info.gear_ratios[0], error)) return false;

	info.gear_ratios[1] = 0;
	for (size_t i = 2; i < info.gear_ratios.size(); ++i)
	{
		std::stringstream s;
		s << "gear-ratio-" << i-1;
		if (!cfg_trans->get(s.str(), info.gear_ratios[i], error)) return false;
	}
	cfg_trans->get("shift-time", info.shift_time);

	return true;
}

static void SetTorqueCurve(
	const std::vector<std::pair<btScalar, btScalar> > & torque,
	SPLINE<btScalar> & torque_curve)
{
	torque_curve.Clear();

	//ensure we have a smooth curve down to 0 RPM
	if (torque[0].first != 0)
	{
		torque_curve.AddPoint(0, 0);
	}

	for (std::vector<std::pair<btScalar, btScalar> >::const_iterator i = torque.begin(); i != torque.end(); ++i)
	{
		torque_curve.AddPoint(i->first, i->second);
	}

	// ensure we have a smooth curve for over-revs
	torque_curve.AddPoint(torque[torque.size()-1].first + 10000, 0);
}

static bool LoadEngine(
	const PTree & cfg,
	sim::EngineInfo & info,
	std::ostream & error)
{
	const PTree * cfg_eng;
	if (!cfg.get("engine", cfg_eng, error)) return false;
	if (!cfg_eng->get("peak-engine-rpm", info.redline, error)) return false;
	if (!cfg_eng->get("rpm-limit", info.rpm_limit, error)) return false;
	if (!cfg_eng->get("inertia", info.inertia, error)) return false;
	if (!cfg_eng->get("start-rpm", info.rpm_start, error)) return false;
	if (!cfg_eng->get("stall-rpm", info.rpm_stall, error)) return false;
	if (!cfg_eng->get("position", info.position, error)) return false;
	if (!cfg_eng->get("mass", info.mass, error)) return false;

	// fuel
	btScalar capacity;
	btScalar volume;
	btScalar density;
	btVector3 position;
	const PTree * cfg_fuel;
	if (!cfg.get("fuel-tank", cfg_fuel, error)) return false;
	if (!cfg_fuel->get("capacity", capacity, error)) return false;
	if (!cfg_fuel->get("volume", volume, error)) return false;
	if (!cfg_fuel->get("fuel-density", density, error)) return false;
	if (!cfg_fuel->get("position", position, error)) return false;

	btScalar fuel_heating_value = 4.5E7; // Ws/kg
	btScalar engine_efficiency = 0.35;
	cfg_eng->get("fuel-heating-value", fuel_heating_value);
	cfg_eng->get("efficiency", engine_efficiency);
	info.fuel_rate = 1 / (engine_efficiency * fuel_heating_value);
	info.fuel_capacity = capacity * density;
	info.fuel_mass = volume * density;

	// nos
	cfg_eng->get("nos-mass", info.nos_mass);
	cfg_eng->get("nos-boost", info.nos_boost);
	cfg_eng->get("nos-ratio", info.nos_fuel_ratio);

	// torque
	int curve_num = 0;
	std::vector<btScalar> torque_point(2);
	std::string torque_str("torque-curve-00");
	std::vector<std::pair<btScalar, btScalar> > torque;
	while (cfg_eng->get(torque_str, torque_point))
	{
		torque.push_back(std::pair<btScalar, btScalar>(torque_point[0], torque_point[1]));

		curve_num++;
		std::stringstream str;
		str << "torque-curve-";
		str.width(2);
		str.fill('0');
		str << curve_num;
		torque_str = str.str();
	}

	if (curve_num < 2)
	{
		error << "You must define at least 2 torque curve points." << std::endl;
		return false;
	}

	// resample torque
	int samples = curve_num * 2;
	btScalar rpm_delta = (info.rpm_limit - info.rpm_stall) / samples;
	SPLINE<btScalar> torque_curve;
	SetTorqueCurve(torque, torque_curve);
	info.torque.resize(samples);
	for (int i = 0; i < samples; ++i)
	{
		btScalar rpm = info.rpm_stall + rpm_delta * i;
		info.torque[i] = torque_curve.Interpolate(rpm);
	}

	return true;
}

static bool LoadBrake(
	const PTree & cfg,
	sim::BrakeInfo & info,
	std::ostream & error)
{
	if (!cfg.get("friction", info.friction, error)) return false;
	if (!cfg.get("area", info.area, error)) return false;
	if (!cfg.get("radius", info.radius, error)) return false;
	if (!cfg.get("bias", info.bias, error)) return false;
	if (!cfg.get("max-pressure", info.max_pressure, error)) return false;
	cfg.get("handbrake", info.handbrake);
	return true;
}

static bool LoadTire(const PTree & cfg, sim::TireInfo & info, std::ostream & error)
{
	//read lateral
	int numinfile;
	for (int i = 0; i < 15; i++)
	{
		numinfile = i;
		if (i == 11)
			numinfile = 111;
		else if (i == 12)
			numinfile = 112;
		else if (i > 12)
			numinfile -= 1;
		std::stringstream st;
		st << "a" << numinfile;
		if (!cfg.get(st.str(), info.lateral[i], error)) return false;
	}

	//read longitudinal, error)) return false;
	for (int i = 0; i < 11; i++)
	{
		std::stringstream st;
		st << "b" << i;
		if (!cfg.get(st.str(), info.longitudinal[i], error)) return false;
	}

	//read aligning, error)) return false;
	for (int i = 0; i < 18; i++)
	{
		std::stringstream st;
		st << "c" << i;
		if (!cfg.get(st.str(), info.aligning[i], error)) return false;
	}

	if (!cfg.get("tread", info.tread, error)) return false;

	return true;
}
/*
static void LoadPoints(
	const PTree & cfg,
	const std::string & name,
	LINEARINTERP<btScalar> & points)
{
	int i = 1;
	std::stringstream s;
	s << std::setw(1) << i;
	btVector3 point(2);
	while (cfg.get(name+s.str(), point) && i < 10)
	{
		s.clear();
		s << std::setw(1) << ++i;
		points.AddPoint(point[0], point[1]);
	}
}
*/
static bool LoadCoilover(
	const PTree & cfg,
	sim::SuspensionInfo & info,
	std::ostream & error)
{
	if (!cfg.get("spring-constant", info.stiffness, error)) return false;
	if (!cfg.get("bounce", info.bounce, error)) return false;
	if (!cfg.get("rebound", info.rebound, error)) return false;
	if (!cfg.get("travel", info.travel, error)) return false;
	//LoadPoints(cfg, "damper-factor-", info.damper_factors);
	//LoadPoints(cfg, "spring-factor-", info.spring_factors);
	return true;
}

static bool LoadArm(
	const PTree & cfg,
	sim::SuspensionArm & arm,
	std::ostream & error)
{
	btVector3 hub;
	if (!cfg.get("hub", hub, error)) return false;

	btVector3 front, rear;
	if (cfg.get("chassis-front", front) && cfg.get("chassis-rear", rear))
	{
		btVector3 axis = front - rear;
		btScalar axis_length = axis.length();
		axis = axis / axis_length;

		btVector3 chassis = rear + axis * axis_length * 0.5f;
		btVector3 dir = hub - chassis;
		btScalar dir_length = dir.length();

		arm.length = dir_length;
		arm.dir = dir / dir_length;
		arm.axis = axis;
		arm.anchor = chassis;

		return true;
	}

	btVector3 chassis;
	if (!cfg.get("chassis", chassis, error)) return false;

	btVector3 dir = hub - chassis;
	btScalar dir_length = dir.length();

	arm.length = dir_length;
	arm.dir = dir / dir_length;
	arm.axis = direction::forward;
	arm.anchor = chassis;

	return true;
}

static bool LoadSuspension(
	const PTree & cfg_wheel,
	sim::SuspensionInfo & info,
	std::ostream & error)
{
	if (!cfg_wheel.get("position", info.position0, error)) return false;
	cfg_wheel.get("steering", info.max_steering_angle);
	cfg_wheel.get("ackermann", info.ackermann);

	btScalar camber(0), caster(0), toe(0);
	cfg_wheel.get("camber", camber);
	cfg_wheel.get("caster", caster);
	cfg_wheel.get("toe", toe);

	info.steering_axis = direction::up * cos(-caster * M_PI / 180.0) +
		direction::right * sin(-caster * M_PI / 180.0);

	btQuaternion toe_rot(direction::up, toe * M_PI / 180.0);
	btQuaternion cam_rot(direction::forward, -camber * M_PI / 180.0);
	info.orientation0 = toe_rot * cam_rot;

	const PTree * cfg_coil;
	if (!cfg_wheel.get("coilover", cfg_coil, error)) return false;
	if (!LoadCoilover(*cfg_coil, info, error)) return false;

	const PTree * cfg_arm;
	if (!cfg_wheel.get("lower-arm", cfg_arm)) return false;
	if (!LoadArm(*cfg_arm, info.lower_arm, error)) return false;
	//info.type = sim::SuspensionInfo::HINGE;

	if (cfg_wheel.get("strut", cfg_arm))
	{
		if (!LoadArm(*cfg_arm, info.upper_arm, error)) return false;
		//info.type = sim::SuspensionInfo::MACPHERSON;
	}
	else if (cfg_wheel.get("upper-arm", cfg_arm))
	{
		if (!LoadArm(*cfg_arm, info.upper_arm, error)) return false;
		//info.type = sim::SuspensionInfo::DWISHBONE;
	}

	return true;
}

static bool LoadAntiRollBar(
	const PTree & cfg,
	const std::map<std::string, int> wheel_map,
	sim::AntiRollBar & info,
	std::ostream & error)
{
	if (!cfg.get("stiffness", info.stiffness, error)) return false;

	std::string link0, link1;
	if (!cfg.get("link-a", link0, error)) return false;
	if (!cfg.get("link-b", link1, error)) return false;

	std::map<std::string, int>::const_iterator it0 = wheel_map.find(link0);
	if (it0 == wheel_map.end())
	{
		error << cfg.fullname() << " link: " << link0 << " not found." << std::endl;
		return false;
	}
	std::map<std::string, int>::const_iterator it1 = wheel_map.find(link1);
	if (it1 == wheel_map.end())
	{
		error << cfg.fullname() << " link: " << link1 << " not found." << std::endl;
		return false;
	}

	info.wheel0 = it0->second;
	info.wheel1 = it1->second;
	return true;
}

static void CalculateWheelMass(
	btScalar tire_radius,
	btScalar tire_width,
	btScalar tire_ar,
	btScalar & mass,
	btScalar & inertia)
{
	btScalar tire_thickness = 0.05;
	btScalar tire_density = 8E3;

	btScalar rim_radius = tire_radius - tire_width * tire_ar;
	btScalar rim_width = tire_width;
	btScalar rim_thickness = 0.01;
	btScalar rim_density = 3E5;

	btScalar tire_volume = tire_width * M_PI * tire_thickness * tire_thickness * (2 * tire_radius  - tire_thickness);
	btScalar rim_volume = rim_width * M_PI * rim_thickness * rim_thickness * (2 * rim_radius - rim_thickness);
	btScalar tire_mass = tire_density * tire_volume;
	btScalar rim_mass = rim_density * rim_volume;
	btScalar tire_inertia = tire_mass * tire_radius * tire_radius;
	btScalar rim_inertia = rim_mass * rim_radius * rim_radius;

	mass = tire_mass + rim_mass;
	inertia = (tire_inertia + rim_inertia);
}

static bool LoadWheel(
	const PTree & cfg_wheel,
	sim::WheelInfo & info,
	std::ostream & error)
{
	const PTree * cfg_tire, * cfg_brake;
	if (!cfg_wheel.get("tire", cfg_tire, error)) return false;
	if (!cfg_wheel.get("brake", cfg_brake, error)) return false;
	if (!LoadTire(*cfg_tire, info.tire, error)) return false;
	if (!LoadBrake(*cfg_brake, info.brake, error)) return false;
	if (!LoadSuspension(cfg_wheel, info.suspension, error)) return false;

	btVector3 size;
	if (!cfg_wheel.get("size", size, error)) return false;

	btScalar width = size[0] * 0.001f;
	btScalar ratio = size[1] * 0.01f;
	btScalar radius = size[2] * 0.5f * 0.0254f + width * ratio;
	btScalar mass, inertia;
	CalculateWheelMass(radius, width, ratio, mass, inertia);
	cfg_wheel.get("mass", mass);
	cfg_wheel.get("inertia", inertia);
	info.inertia = inertia;
	info.radius = radius;
	info.width = width;
	info.mass = mass;

	return true;
}

// use btMultiSphereShape(4 spheres) to approximate bounding box
static btMultiSphereShape * CreateCollisionShape(
	const btVector3 & center,
	const btVector3 & size)
{
	btVector3 hsize = 0.5 * size;
	int min = hsize.minAxis();
	int max = hsize.maxAxis();
	btVector3 maxAxis(0, 0, 0);
	maxAxis[max] = 1;
	int numSpheres = 4;

	btScalar radius = hsize[min];
	btScalar radii[4] = {radius, radius, radius, radius};
	btVector3 positions[4];
	btVector3 offset0 = hsize - btVector3(radius, radius, radius);
	btVector3 offset1 = offset0 - 2 * offset0[max] * maxAxis;
	positions[0] = center + offset0;
	positions[1] = center + offset1;
	positions[2] = center - offset0;
	positions[3] = center - offset1;

	return new btMultiSphereShape(positions, radii, numSpheres);
}

static bool LinkShaft(
	const PTree & cfg,
	const std::string & link_name,
	std::map<std::string, int> & shaft_map,
	int & link_id,
	std::ostream & error)
{
	std::string link_to;
	if (!cfg.get(link_name, link_to, error)) return false;

	std::map<std::string, int>::iterator it = shaft_map.find(link_to);
	if (it == shaft_map.end())
	{
		error << cfg.fullname() << " link: " << link_to << " not found." << std::endl;
		return false;
	}
	if (it->second == -1)
	{
		error << cfg.fullname() << " link: " << it->first << " already linked." << std::endl;
		return false;
	}
	link_id = it->second;
	it->second = -1;
	return true;
}

static bool LoadDifferential(
	const PTree & cfg,
	sim::DifferentialInfo & info,
	std::ostream & error)
{
	if (!cfg.get("final-drive", info.final_drive, error)) return false;
	if (!cfg.get("anti-slip", info.anti_slip, error)) return false;
	cfg.get("anti-slip-torque", info.anti_slip_factor);
	cfg.get("anti-slip-torque-deceleration-factor", info.deceleration_factor);
	cfg.get("inertia", info.inertia);
	return true;
}

struct BodyLoader
{
	sim::FractureBodyInfo & info;
	btAlignedObjectArray<sim::AeroDeviceInfo> & aero;
	const bool & damage;
	BodyLoader(
		sim::FractureBodyInfo & info,
		btAlignedObjectArray<sim::AeroDeviceInfo> & aero,
		const bool & damage) :
		info(info),
		aero(aero),
		damage(damage)
	{
		// ctor
	}

	// load a body instance, return false on error
	bool operator() (
		const PTree & cfg,
		std::ostream & error,
		btCollisionShape * shape = 0,
		btScalar mass = 0,
		bool mount = false)
	{
		btVector3 pos(0, 0, 0);
		btVector3 rot(0, 0, 0);
		cfg.get("mass", mass);
		cfg.get("position", pos);
		cfg.get("rotation", rot);

		btQuaternion qrot(rot[1] * M_PI/180, rot[0] * M_PI/180, rot[2] * M_PI/180);
		btTransform transform;
		transform.setOrigin(pos);
		transform.setRotation(qrot);

		info.addMass(pos, mass);

		loadAero(cfg, pos);

		LoadCollisionShape(cfg, transform, shape, info.m_shape);

		const PTree * cfg_mount = 0;
		mount |= cfg.get("link", cfg_mount);

		if (shape && mount)
		{
			btScalar elimit(10E5);
			btScalar plimit(10E5);
			if (damage && cfg_mount)
			{
				cfg_mount->get("elastic-limit", elimit);
				cfg_mount->get("plastic-limit", plimit);
			}

			btVector3 inertia(0, 0, 0);
			if (!cfg.get("inertia", inertia))
			{
				shape->calculateLocalInertia(mass, inertia);
			}

			int shape_id = info.m_shape->getNumChildShapes() - 1;
			info.addBody(shape_id, inertia, mass, elimit, plimit);
		}

		return true;
	}

	bool loadAero(const PTree & cfg, const btVector3 & position)
	{
		sim::AeroDeviceInfo info;
		if (!cfg.get("drag-coefficient", info.drag_coefficient)) return true;
		cfg.get("frontal-area", info.drag_frontal_area);
		cfg.get("surface-area", info.lift_surface_area);
		cfg.get("lift-coefficient", info.lift_coefficient);
		cfg.get("lift-efficiency", info.lift_efficiency);
		info.position = position;
		aero.push_back(info);
		return true;
	}
};

bool LoadVehicle(
	const PTree & cfg,
	const bool damage,
	const btVector3 & modelcenter,
	const btVector3 & modelsize,
	sim::VehicleInfo & info,
	std::ostream & error)
{
	BodyLoader loadBody(info.body, info.aerodevice, damage);

	// load wheels
	const PTree * cfg_wheels;
	if (!cfg.get("wheel", cfg_wheels, error)) return false;

	int wheel_count = cfg_wheels->size();
	if (wheel_count < 2)
	{
		error << "Found " << wheel_count << " wheels. ";
		error << "Two wheels are required at minimum." << std::endl;
		return false;
	}

	int i = 0;
	info.wheel.resize(wheel_count);
	for (PTree::const_iterator it = cfg_wheels->begin(); it != cfg_wheels->end(); ++it, ++i)
	{
		const PTree & cfg_wheel = it->second;
		if (!LoadWheel(cfg_wheel, info.wheel[i], error)) return false;
		btScalar mass = info.wheel[i].mass;
		btScalar width = info.wheel[i].width;
		btScalar radius = info.wheel[i].radius;
		btVector3 size(width * 0.5f, radius, radius);
		btCollisionShape * shape = new btCylinderShapeX(size);
		loadBody(cfg_wheel, error, shape, mass, true);
	}

	// load children bodies
	for (PTree::const_iterator it = cfg.begin(); it != cfg.end(); ++it)
	{
		if (!loadBody(it->second, error)) return false;
	}

	// create default shape if no shapes loaded
	if (info.body.m_shape->getNumChildShapes() == wheel_count)
	{
		info.body.m_shape->addChildShape(
			btTransform::getIdentity(),
			CreateCollisionShape(modelcenter, modelsize));
	}

	// get wheel shafts for linkage
	int shaft_id = 0;
	std::map<std::string, int> shaft_map;
	for (PTree::const_iterator it = cfg_wheels->begin(); it != cfg_wheels->end(); ++it)
	{
		shaft_map["wheel." + it->first] = shaft_id++;
	}

	// load anti-roll bars
	const PTree * cfg_bar;
	if (cfg.get("antiroll", cfg_bar))
	{
		for (PTree::const_iterator it = cfg_bar->begin(); it != cfg_bar->end(); ++it)
		{
			sim::AntiRollBar bar;
			LoadAntiRollBar(it->second, shaft_map, bar, error);
			info.antiroll.push_back(bar);
		}
	}

	// load differentials
	const PTree * cfg_diff;
	if (cfg.get("differential", cfg_diff))
	{
		info.differential.resize(cfg_diff->size());
		info.differential_link_a.resize(cfg_diff->size());
		info.differential_link_b.resize(cfg_diff->size());
		for (PTree::const_iterator it = cfg_diff->begin(); it != cfg_diff->end(); ++it)
		{
			shaft_map["differential." + it->first] = shaft_id++;
		}

		int i = 0;
		for (PTree::const_iterator it = cfg_diff->begin(); it != cfg_diff->end(); ++it, ++i)
		{
			if (!LoadDifferential(it->second, info.differential[i], error)) return false;
			if (!LinkShaft(it->second, "link-a", shaft_map, info.differential_link_a[i], error)) return false;
			if (!LinkShaft(it->second, "link-b", shaft_map, info.differential_link_b[i], error)) return false;
		}
	}

	const PTree * cfg_trans;
	if (!cfg.get("transmission", cfg_trans, error)) return false;
	if (!LinkShaft(*cfg_trans, "link", shaft_map, info.transmission_link, error)) return false;

	if (!LoadTransmission(cfg, info.transmission, error)) return false;
	if (!LoadClutch(cfg, info.clutch, error)) return false;
	if (!LoadEngine(cfg, info.engine, error)) return false;

	return true;
}
