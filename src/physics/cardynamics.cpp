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

#include "cardynamics.h"
#include "carinput.h"
#include "tracksurface.h"
#include "dynamicsworld.h"
#include "fracturebody.h"
#include "loadcollisionshape.h"
#include "coordinatesystem.h"
#include "content/contentmanager.h"
#include "cfg/ptree.h"
#include "macros.h"

#include "BulletCollision/CollisionShapes/btCompoundShape.h"
#include "BulletCollision/CollisionShapes/btCylinderShape.h"
#include "BulletCollision/CollisionShapes/btTriangleShape.h"

#include <cmath>

static inline std::istream & operator >> (std::istream & lhs, btVector3 & rhs)
{
	std::string str;
	for (int i = 0; i < 3 && !lhs.eof(); ++i)
	{
		std::getline(lhs, str, ',');
		std::istringstream s(str);
		s >> rhs[i];
	}
	return lhs;
}

static bool LoadClutch(
	const PTree & cfg,
	CarClutch & clutch,
	std::ostream & error_output)
{
	btScalar sliding, radius, area, max_pressure;

	const PTree * cfg_clutch;
	if (!cfg.get("clutch", cfg_clutch, error_output)) return false;
	if (!cfg_clutch->get("sliding", sliding, error_output)) return false;
	if (!cfg_clutch->get("radius", radius, error_output)) return false;
	if (!cfg_clutch->get("area", area, error_output)) return false;
	if (!cfg_clutch->get("max-pressure", max_pressure, error_output)) return false;

	clutch.Set(sliding, max_pressure, area, radius);

	return true;
}

static bool LoadTransmission(
	const PTree & cfg,
	CarTransmission & transmission,
	std::ostream & error_output)
{
	btScalar shift_time = 0;
	btScalar ratio;
	int gears;

	const PTree * cfg_trans;
	if (!cfg.get("transmission", cfg_trans, error_output)) return false;
	if (!cfg_trans->get("gears", gears, error_output)) return false;
	for (int i = 0; i < gears; ++i)
	{
		std::ostringstream s;
		s << "gear-ratio-" << i+1;
		if (!cfg_trans->get(s.str(), ratio, error_output)) return false;
		transmission.SetGearRatio(i+1, ratio);
	}
	if (!cfg_trans->get("gear-ratio-r", ratio, error_output)) return false;
	cfg_trans->get("shift-time", shift_time);

	transmission.SetGearRatio(-1, ratio);
	transmission.SetShiftTime(shift_time);

	return true;
}

static bool LoadFuelTank(
	const PTree & cfg,
	CarFuelTank & fuel_tank,
	std::ostream & error_output)
{
	btScalar capacity;
	btScalar volume;
	btScalar fuel_density;
	std::vector<btScalar> pos(3);

	const PTree * cfg_fuel;
	if (!cfg.get("fuel-tank", cfg_fuel, error_output)) return false;
	if (!cfg_fuel->get("capacity", capacity, error_output)) return false;
	if (!cfg_fuel->get("volume", volume, error_output)) return false;
	if (!cfg_fuel->get("fuel-density", fuel_density, error_output)) return false;
	if (!cfg_fuel->get("position", pos, error_output)) return false;

	btVector3 position(pos[0], pos[1], pos[2]);

	fuel_tank.SetCapacity(capacity);
	fuel_tank.SetVolume(volume);
	fuel_tank.SetDensity(fuel_density);
	fuel_tank.SetPosition(position);

	return true;
}

static bool LoadEngine(
	const PTree & cfg,
	CarEngine & engine,
	std::ostream & error_output)
{
	const PTree * cfg_eng;
	CarEngineInfo engine_info;

	if (!cfg.get("engine", cfg_eng, error_output)) return false;
	if (!engine_info.Load(*cfg_eng, error_output)) return false;
	engine.Init(engine_info);

	return true;
}

static bool LoadBrake(
	const PTree & cfg,
	CarBrake & brake,
	std::ostream & error_output)
{
	btScalar friction, max_pressure, area, bias, radius, handbrake(0);

	if (!cfg.get("friction", friction, error_output)) return false;
	if (!cfg.get("area", area, error_output)) return false;
	if (!cfg.get("radius", radius, error_output)) return false;
	if (!cfg.get("bias", bias, error_output)) return false;
	if (!cfg.get("max-pressure", max_pressure, error_output)) return false;
	cfg.get("handbrake", handbrake);

	brake.SetFriction(friction);
	brake.SetArea(area);
	brake.SetRadius(radius);
	brake.SetBias(bias);
	brake.SetMaxPressure(max_pressure*bias);
	brake.SetHandbrake(handbrake);

	return true;
}

#ifdef VDRIFTN
static bool LoadTire(const PTree & cfg_wheel, const PTree & cfg, CarTire & tire, std::ostream & error_output)
{
	CarTireInfo info;

	if (!cfg.get("tread", info.tread, error_output)) return false;

	btVector3 roll_resistance;
	if (!cfg.get("rolling-resistance", roll_resistance, error_output)) return false;
	info.roll_resistance_lin = roll_resistance[0];
	info.roll_resistance_quad = roll_resistance[1];

	if (!cfg.get("FZ0", info.nominal_load, error_output)) return false;
	for (int i = 0; i < CarTireInfo::CNUM; ++i)
	{
		if (!cfg.get(info.coeffname[i], info.coefficients[i], error_output))
			return false;
	}

	// asymmetric tires support (left right facing direction)
	// default facing direction is right
	// symmetric tire has side factor zero
	btScalar side_factor = 0;
	std::string facing;
	if (cfg_wheel.get("tire.facing", facing))
		side_factor = (facing != "left") ? 1 : -1;
	info.coefficients[TireInfo::PEY3] *= side_factor;
	info.coefficients[TireInfo::PEY4] *= side_factor;
	info.coefficients[TireInfo::PVY1] *= side_factor;
	info.coefficients[TireInfo::PVY2] *= side_factor;
	info.coefficients[TireInfo::PHY1] *= side_factor;
	info.coefficients[TireInfo::PHY2] *= side_factor;
	info.coefficients[TireInfo::PHY3] *= side_factor;
	info.coefficients[TireInfo::RBY3] *= side_factor;
	info.coefficients[TireInfo::RHX1] *= side_factor;
	info.coefficients[TireInfo::RHY1] *= side_factor;
	info.coefficients[TireInfo::RVY5] *= side_factor;

	tire.init(info);

	return true;
}
#else
static bool LoadTire(const PTree & cfg_wheel, const PTree & cfg, CarTire & tire, std::ostream & error_output)
{
	CarTireInfo info;

	if (!cfg.get("tread", info.tread, error_output)) return false;

	btVector3 rolling_resistance;
	if (!cfg.get("rolling-resistance", rolling_resistance, error_output)) return false;
	info.rolling_resistance_lin = rolling_resistance[0];
	info.rolling_resistance_quad = rolling_resistance[1];

	// read lateral
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
		std::ostringstream s;
		s << "a" << numinfile;
		if (!cfg.get(s.str(), info.lateral[i], error_output)) return false;
	}

	// read longitudinal
	for (int i = 0; i < 11; i++)
	{
		std::ostringstream s;
		s << "b" << i;
		if (!cfg.get(s.str(), info.longitudinal[i], error_output)) return false;
	}

	// read aligning
	for (int i = 0; i < 18; i++)
	{
		std::ostringstream s;
		s << "c" << i;
		if (!cfg.get(s.str(), info.aligning[i], error_output)) return false;
	}

	// asymmetric tires support (left right facing direction)
	// default facing direction is right
	// fixme: should handle aligning torque too?
	btScalar side_factor = 0;
	std::string facing;
	if (cfg_wheel.get("tire.facing", facing))
		side_factor = (facing != "left") ? 1 : -1;
	info.lateral[13] *= side_factor;
	info.lateral[14] *= side_factor;

	tire.init(info);

	return true;
}
#endif // VDRIFTN

static bool LoadWheel(const PTree & cfg, CarWheel & wheel, std::ostream & error_output)
{
	btVector3 tire_size;
	if (!cfg.get("tire.size", tire_size, error_output)) return false;

	btScalar tire_width = tire_size[0] * 0.001f;
	btScalar tire_aspect_ratio = tire_size[1] * 0.01f;
	btScalar tire_radius = tire_size[2] * 0.5f * 0.0254f + tire_width * tire_aspect_ratio;

	// calculate mass, inertia from dimensions
	btScalar tire_thickness = 0.05;
	btScalar tire_density = 8E3;

	btScalar rim_radius = tire_radius - tire_width * tire_aspect_ratio;
	btScalar rim_width = tire_width;
	btScalar rim_thickness = 0.01;
	btScalar rim_density = 3E5;

	btScalar tire_volume = tire_width * M_PI * tire_thickness * tire_thickness * (2 * tire_radius  - tire_thickness);
	btScalar rim_volume = rim_width * M_PI * rim_thickness * rim_thickness * (2 * rim_radius - rim_thickness);
	btScalar tire_mass = tire_density * tire_volume;
	btScalar rim_mass = rim_density * rim_volume;
	btScalar tire_inertia = tire_mass * tire_radius * tire_radius;
	btScalar rim_inertia = rim_mass * rim_radius * rim_radius;

	btScalar mass = tire_mass + rim_mass;
	btScalar inertia = (tire_inertia + rim_inertia) * 4;	// scale inertia fixme

	// override mass, inertia
	cfg.get("inertia", inertia);
	cfg.get("mass", mass);

	wheel.SetInertia(inertia);
	wheel.SetRadius(tire_radius);
	wheel.SetWidth(tire_width);
	wheel.SetMass(mass);

	return true;
}

static bool LoadDifferential(
	const PTree & cfg,
	CarDifferential & diff,
	std::ostream & error_output)
{
	btScalar final_drive(1), anti_slip(0), anti_slip_torque(0), anti_slip_torque_deceleration_factor(0);

	if (!cfg.get("final-drive", final_drive, error_output)) return false;
	if (!cfg.get("anti-slip", anti_slip, error_output)) return false;
	cfg.get("anti-slip-torque", anti_slip_torque);
	cfg.get("anti-slip-torque-deceleration-factor", anti_slip_torque_deceleration_factor);

	diff.SetFinalDrive(final_drive);
	diff.SetAntiSlip(anti_slip, anti_slip_torque, anti_slip_torque_deceleration_factor);

	return true;
}

struct AeroDeviceFracture : public FractureCallback
{
	btAlignedObjectArray<AeroDevice> & aerodevice;
	int id;

	AeroDeviceFracture(btAlignedObjectArray<AeroDevice> & aerodevice, int id) :
		aerodevice(aerodevice),
		id(id)
	{
		// ctor
	}

	~AeroDeviceFracture()
	{
		// dtor
	}

	void operator()(FractureBody::Connection & /*connection*/)
	{
		int last = aerodevice.size() - 1;
		if (id < last)
		{
			aerodevice.swap(id, last);
			AeroDeviceFracture* ad = static_cast<AeroDeviceFracture*>(aerodevice[id].GetUserPointer());
			btAssert(ad);
			ad->id = id;
		}
		aerodevice.resize(last);
	}
};

struct BodyLoader
{
	btAlignedObjectArray<AeroDevice> & aerodevice;
	FractureBodyInfo & info;
	const bool & damage;

	BodyLoader(
		btAlignedObjectArray<AeroDevice> & aerodevice,
		FractureBodyInfo & info,
		const bool & damage) :
		aerodevice(aerodevice),
		info(info),
		damage(damage)
	{
		// ctor
	}

	// load a body instance, return false on error
	bool operator() (
		const PTree & cfg,
		std::ostream & /*error*/,
		btCollisionShape * shape = 0,
		btScalar mass = 0,
		bool link = false)
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

		LoadCollisionShape(cfg, transform, shape, info.m_shape);

		const PTree * cfg_link = 0;
		cfg.get("link", cfg_link);
		link = (link || cfg_link) && shape;

		if (link)
		{
			btScalar elimit(1E5);
			btScalar plimit(1E5);
			if (damage && cfg_link)
			{
				cfg_link->get("elastic-limit", elimit);
				cfg_link->get("plastic-limit", plimit);
			}

			btVector3 inertia(0, 0, 0);
			if (!cfg.get("inertia", inertia))
			{
				shape->calculateLocalInertia(mass, inertia);
			}

			int shape_id = info.m_shape->getNumChildShapes() - 1;
			info.addBody(shape_id, inertia, mass, elimit, plimit);
			info.m_states.push_back(MotionState());
		}

		loadAeroDevice(cfg, pos, link);

		return true;
	}

	bool loadAeroDevice(const PTree & cfg, const btVector3 & position, bool link)
	{
		btScalar drag_coefficient;
		if (!cfg.get("drag-coefficient", drag_coefficient)) return true;

		AeroDeviceInfo ad;
		ad.position = position;
		ad.drag_coefficient = drag_coefficient;
		cfg.get("frontal-area", ad.drag_frontal_area);
		cfg.get("surface-area", ad.lift_surface_area);
		cfg.get("lift-coefficient", ad.lift_coefficient);
		cfg.get("lift-efficiency", ad.lift_efficiency);
		if (link)
		{
			int id = aerodevice.size();
			AeroDeviceFracture * adf = new AeroDeviceFracture(aerodevice, id);
			info.m_connections[info.m_connections.size() - 1].m_fracture = adf;
			ad.user_ptr = adf;
		}

		aerodevice.push_back(AeroDevice(ad));
		return true;
	}
};

CarDynamics::CarDynamics()
{
	Init();
}

CarDynamics::CarDynamics(const CarDynamics & other)
{
	// we don't really support copying of these suckers
	assert(!other.body);
	Init();
}

CarDynamics & CarDynamics::operator= (const CarDynamics & other)
{
	// we don't really support copying of these suckers
	assert(!other.body && !body);
	return *this;
}

CarDynamics::~CarDynamics()
{
	Clear();
}

bool CarDynamics::Load(
	const PTree & cfg,
	const std::string & cardir,
	const std::string & cartire,
	const btVector3 & position,
	const btQuaternion & rotation,
	const bool damage,
	DynamicsWorld & world,
	ContentManager & content,
	std::ostream & error)
{
	if (!LoadClutch(cfg, clutch, error)) return false;
	if (!LoadTransmission(cfg, transmission, error)) return false;
	if (!LoadFuelTank(cfg, fuel_tank, error)) return false;
	if (!LoadEngine(cfg, engine, error)) return false;

	drive = NONE;
	const PTree * cfg_diff;
	if (cfg.get("differential-front", cfg_diff))
	{
		if (!LoadDifferential(*cfg_diff, differential_front, error)) return false;
		drive = FWD;
	}
	if (cfg.get("differential-rear", cfg_diff))
	{
		if (!LoadDifferential(*cfg_diff, differential_rear, error)) return false;
		drive = (drive == FWD) ? AWD : RWD;
	}
	if (cfg.get("differential-center", cfg_diff) && drive == AWD)
	{
		if (!LoadDifferential(*cfg_diff, differential_center, error)) return false;
	}
	if (drive == NONE)
	{
		error << "No differential declared" << std::endl;
		return false;
	}

	motion_state.push_back(MotionState());
	FractureBodyInfo bodyinfo(motion_state);
	BodyLoader loadBody(aerodevice, bodyinfo, damage);

	// load wheels
	const PTree * cfg_wheels;
	if (!cfg.get("wheel", cfg_wheels, error)) return false;

	int wheel_count = cfg_wheels->size();
	if (wheel_count != WHEEL_POSITION_SIZE)
	{
		error << "Wheels loaded: " << wheel_count << ". Required: " << WHEEL_POSITION_SIZE << std::endl;
		return false;
	}

	int i = 0;
	for (PTree::const_iterator it = cfg_wheels->begin(); it != cfg_wheels->end(); ++it, ++i)
	{
		const PTree & cfg_wheel = it->second;
		if (!LoadWheel(cfg_wheel, wheel[i], error)) return false;

		std::string tirestr(cartire);
		std::shared_ptr<PTree> cfg_tire;
		if ((cartire.empty() || cartire == "default") &&
			!cfg_wheel.get("tire.type", tirestr, error)) return false;
		#ifdef VDRIFTN
		tirestr += "n";
		#endif
		content.load(cfg_tire, cardir, tirestr);
		if (!LoadTire(cfg_wheel, *cfg_tire, tire[i], error)) return false;

		const PTree * cfg_brake;
		if (!cfg_wheel.get("brake", cfg_brake, error)) return false;
		if (!LoadBrake(*cfg_brake, brake[i], error)) return false;

		if (!CarSuspension::Load(cfg_wheel, suspension[i], error)) return false;
		if (suspension[i]->GetMaxSteeringAngle() > maxangle)
			maxangle = suspension[i]->GetMaxSteeringAngle();

		btScalar mass = wheel[i].GetMass();
		btScalar width = wheel[i].GetWidth();
		btScalar radius = wheel[i].GetRadius();
		btVector3 size(width * 0.5f, radius, radius);
		btCollisionShape * shape = new btCylinderShapeX(size);
		loadBody(cfg_wheel, error, shape, mass, true);
	}

	// load children bodies
	for (PTree::const_iterator it = cfg.begin(); it != cfg.end(); ++it)
	{
		if (!loadBody(it->second, error)) return false;
	}

	if (bodyinfo.m_shape->getNumChildShapes() == wheel_count)
	{
		error << "No collision shape defined." << std::endl;
		return false;
	}

	transform.setRotation(rotation);
	transform.setOrigin(position);

	body = new FractureBody(bodyinfo);
	body->setCenterOfMassTransform(transform);
	body->setActivationState(DISABLE_DEACTIVATION);
	body->setContactProcessingThreshold(0.0);
	body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
	world.addRigidBody(body);
	world.addAction(this);
	this->world = &world;

	// position is the center of a 2 x 4 x 1 meter box on track surface
	// move car to fit bounding box front lower edge of the position box
	btVector3 bmin, bmax;
	body->getCollisionShape()->getAabb(btTransform::getIdentity(), bmin, bmax);
	btVector3 fwd = body->getCenterOfMassTransform().getBasis().getColumn(1);
	btVector3 up = body->getCenterOfMassTransform().getBasis().getColumn(2);
	btVector3 fwd_offset = fwd * (2.0 - bmax.y());
	btVector3 up_offset = -up * (0.5 + bmin.z());

	// adjust for suspension rest position
	// a bit hacky here, should use updated aabb
	btScalar dr = GetCenterOfMassOffset()[1];
	btScalar r1 = suspension[0]->GetWheelPosition()[1] + dr;
	btScalar r2 = suspension[3]->GetWheelPosition()[1] + dr;
	btScalar m = 1 / body->getInvMass();
	btScalar m1 = m * r2 / (r2 - r1);
	btScalar m2 = m - m1;
	btScalar d1 = suspension[0]->GetDisplacement(m1 * 9.81 * 0.5);
	btScalar d2 = suspension[3]->GetDisplacement(m2 * 9.81 * 0.5);
	suspension[0]->SetDisplacement(d1);
	suspension[1]->SetDisplacement(d1);
	suspension[2]->SetDisplacement(d2);
	suspension[3]->SetDisplacement(d2);
	up_offset -= up * btMax(d1, d2);

	SetPosition(body->getCenterOfMassPosition() + up_offset + fwd_offset);
	UpdateWheelTransform();

	// update motion state
	body->getMotionState()->setWorldTransform(body->getWorldTransform());

	// init cached state
	linear_velocity = body->getLinearVelocity();
	angular_velocity = body->getAngularVelocity();
	for (int i = 0; i < WHEEL_POSITION_SIZE; ++i)
	{
		wheel_velocity[i].setZero();
		suspension_force[i].setZero();
	}

	maxspeed = CalculateMaxSpeed();

	// calculate steering feedback scale factor
	// use max Mz of the 2 front wheels assuming even weight distribution
	// and a fudge factor of 8 to get feedback into -1, 1 range
	const float max_wheel_load = 0.25 * 9.81 / body->getInvMass() ;
	feedback_scale = 1 / (8 * 2 * tire[0].getMaxMz(max_wheel_load, 0));

	return true;
}

void CarDynamics::SetPosition(const btVector3 & position)
{
	body->translate(position - body->getCenterOfMassPosition());

	transform.setOrigin(position);
	for (int i = 0; i < WHEEL_POSITION_SIZE; ++i)
		wheel_position[i] = LocalToWorld(suspension[i]->GetWheelPosition());
}

void CarDynamics::AlignWithGround()
{
	UpdateWheelContacts();

	btScalar min_height = 0;
	bool no_min_height = true;
	for (int i = 0; i < WHEEL_POSITION_SIZE; ++i)
	{
		btScalar height = wheel_contact[i].GetDepth() - 2 * wheel[i].GetRadius();
		if (height < min_height || no_min_height)
		{
			min_height = height;
			no_min_height = false;
		}
	}

	btVector3 delta = GetDownVector() * min_height;
	btVector3 trimmed_position = transform.getOrigin() + delta;

	SetPosition(trimmed_position);
	UpdateWheelContacts();

	body->setAngularVelocity(btVector3(0, 0, 0));
	body->setLinearVelocity(btVector3(0, 0, 0));
}

void CarDynamics::SetAutoClutch(bool value)
{
	autoclutch = value;
}

void CarDynamics::SetAutoShift(bool value)
{
	autoshift = value;

	// shift into first gear when autoshift enabled
	if (autoshift && GetTransmission().GetGear() == 0)
		ShiftGear(1);
}

void CarDynamics::SetABS(bool value)
{
	abs = value;
}

void CarDynamics::SetTCS(bool value)
{
	tcs = value;
}

void CarDynamics::Update(const std::vector<float> & inputs)
{
	assert(inputs.size() >= CarInput::INVALID);

	SetBrake(inputs[CarInput::BRAKE]);

	SetHandBrake(inputs[CarInput::HANDBRAKE]);

	// do steering
	float steer_value = inputs[CarInput::STEER_RIGHT] - inputs[CarInput::STEER_LEFT];
	SetSteering(steer_value);

	// do shifting
	int gear_change = 0;
	if (inputs[CarInput::SHIFT_UP] == 1.0)
		gear_change = 1;
	if (inputs[CarInput::SHIFT_DOWN] == 1.0)
		gear_change = -1;
	int cur_gear = GetTransmission().GetGear();
	int new_gear = cur_gear + gear_change;

	if (inputs[CarInput::REVERSE])
		new_gear = -1;
	if (inputs[CarInput::NEUTRAL])
		new_gear = 0;
	if (inputs[CarInput::FIRST_GEAR])
		new_gear = 1;
	if (inputs[CarInput::SECOND_GEAR])
		new_gear = 2;
	if (inputs[CarInput::THIRD_GEAR])
		new_gear = 3;
	if (inputs[CarInput::FOURTH_GEAR])
		new_gear = 4;
	if (inputs[CarInput::FIFTH_GEAR])
		new_gear = 5;
	if (inputs[CarInput::SIXTH_GEAR])
		new_gear = 6;

	ShiftGear(new_gear);

	SetThrottle(inputs[CarInput::THROTTLE]);

	SetClutch(1 - inputs[CarInput::CLUTCH]);

	SetNOS(inputs[CarInput::NOS]);

	// start the engine if requested
	if (inputs[CarInput::START_ENGINE])
		StartEngine();

	// do driver aid toggles
	if (inputs[CarInput::ABS_TOGGLE])
		SetABS(!GetABSEnabled());

	if (inputs[CarInput::TCS_TOGGLE])
		SetTCS(!GetTCSEnabled());

	// reset car after a rollover
	if (inputs[CarInput::ROLLOVER])
		RolloverRecover();
}

void CarDynamics::debugDraw(btIDebugDraw*)
{
	// void
}

btVector3 CarDynamics::GetEnginePosition() const
{
	return GetPosition(0) + quatRotate(GetOrientation(0), engine.GetPosition());
}

const btVector3 & CarDynamics::GetPosition() const
{
	return GetPosition(0);
}

const btQuaternion & CarDynamics::GetOrientation() const
{
	return GetOrientation(0);
}

const btVector3 & CarDynamics::GetWheelPosition(WheelPosition wp) const
{
	return motion_state[wp+1].position;
}

const btQuaternion & CarDynamics::GetWheelOrientation(WheelPosition wp) const
{
	return motion_state[wp+1].rotation;
}

btQuaternion CarDynamics::GetUprightOrientation(WheelPosition wp) const
{
	return GetOrientation(0) * suspension[wp]->GetWheelOrientation();
}

unsigned CarDynamics::GetNumBodies() const
{
	return motion_state.size();
}

const btVector3 & CarDynamics::GetPosition(int i) const
{
	btAssert(i < motion_state.size());
	return motion_state[i].position;
}

const btQuaternion & CarDynamics::GetOrientation(int i) const
{
	btAssert(i < motion_state.size());
	return motion_state[i].rotation;
}

/// worldspace wheel center position
const btVector3 & CarDynamics::GetWheelVelocity(WheelPosition wp) const
{
	return wheel_velocity[wp];
}

const CollisionContact & CarDynamics::GetWheelContact(WheelPosition wp) const
{
	return wheel_contact[wp];
}

CollisionContact & CarDynamics::GetWheelContact(WheelPosition wp)
{
	return wheel_contact[wp];
}

const btVector3 & CarDynamics::GetCenterOfMass() const
{
	return body->getCenterOfMassPosition();
}

btScalar CarDynamics::GetInvMass() const
{
	return body->getInvMass();
}

btScalar CarDynamics::GetSpeed() const
{
	return body->getLinearVelocity().length();
}

const btVector3 & CarDynamics::GetVelocity() const
{
	return body->getLinearVelocity();
}

btScalar CarDynamics::GetSpeedMPS() const
{
	return wheel[0].GetRadius() * wheel[0].GetAngularVelocity();
}

btScalar CarDynamics::GetMaxSpeedMPS() const
{
	return maxspeed;
}

btScalar CarDynamics::GetTachoRPM() const
{
	return tacho_rpm;
}

bool CarDynamics::GetABSEnabled() const
{
	return abs;
}

bool CarDynamics::GetABSActive() const
{
	return abs && (abs_active[0]||abs_active[1]||abs_active[2]||abs_active[3]);
}

bool CarDynamics::GetTCSEnabled() const
{
	return tcs;
}

bool CarDynamics::GetTCSActive() const
{
	return tcs && (tcs_active[0]||tcs_active[1]||tcs_active[2]||tcs_active[3]);
}

btScalar CarDynamics::GetMaxSteeringAngle() const
{
	return maxangle;
}

btVector3 CarDynamics::GetTotalAero() const
{
	btVector3 downforce(0, 0, 0);
	for (int i = 0; i != aerodevice.size(); ++i)
	{
		downforce = downforce + aerodevice[i].getLift() +  aerodevice[i].getDrag();
	}
	return downforce;
}

btScalar CarDynamics::GetAerodynamicDownforceCoefficient() const
{
	btScalar coeff = 0.0;
	for (int i = 0; i != aerodevice.size(); ++i)
	{
		coeff += aerodevice[i].getLiftCoefficient();
	}
	return coeff;
}

btScalar CarDynamics::GetAeordynamicDragCoefficient() const
{
	btScalar coeff = 0.0;
	for (int i = 0; i != aerodevice.size(); ++i)
	{
		coeff += aerodevice[i].getDragCoefficient();
	}
	return coeff;
}

btScalar CarDynamics::GetFeedback() const
{
	return feedback;
}

btScalar CarDynamics::GetTireSquealAmount(WheelPosition i) const
{
	const TrackSurface & surface = GetWheelContact(i).GetSurface();
	if (surface.type == TrackSurface::NONE) return 0;

	btQuaternion wheelspace = GetUprightOrientation(i);
	btVector3 groundvel = quatRotate(wheelspace.inverse(), GetWheelVelocity(i));
	btScalar wheelspeed = GetWheel(i).GetAngularVelocity() * GetWheel(i).GetRadius();
	groundvel[0] -= wheelspeed;
	groundvel[1] *= 2.0;
	groundvel[2] = 0;
	btScalar squeal = (groundvel.length() - 3.0) * 0.2;

	btScalar sr = GetTire(i).getSlip() / GetTire(i).getIdealSlip();
	btScalar ar = GetTire(i).getSlipAngle() / GetTire(i).getIdealSlipAngle();
	btScalar maxratio = std::max(std::abs(sr), std::abs(ar));
	btScalar squealfactor = std::max(0.0, maxratio - 1.0);
	squeal *= squealfactor;
	btClamp(squeal, btScalar(0), btScalar(1));

	return squeal;
}

static std::ostream & operator << (std::ostream & os, const btVector3 & v)
{
	os << v[0] << ", " << v[1] << ", " << v[2];
	return os;
}

static std::ostream & operator << (std::ostream & os, const CarTire & tire)
{
	os << "Fx: " << tire.getFx() << "\n";
	os << "Fy: " << tire.getFy() << "\n";
	os << "Slip Ang: " << tire.getSlipAngle() * SIMD_DEGS_PER_RAD << " / ";
	os << tire.getIdealSlipAngle() * SIMD_DEGS_PER_RAD << "\n";
	os << "Slip: " << tire.getSlip() << " / ";
	os << tire.getIdealSlip() << "\n";
	return os;
}

void CarDynamics::DebugPrint(std::ostream & out, bool p1, bool p2, bool p3, bool p4) const
{
	out << std::fixed << std::setprecision(3);

	if (p1)
	{
		out << "---Body---\n";
		out << "Velocity: " << GetVelocity() << "\n";
		out << "Position: " << GetPosition() << "\n";
		out << "Center of mass: " << -GetCenterOfMassOffset() << "\n";
		out << "Total mass: " << 1 / body->getInvMass() << "\n";
		out << "VelocityL: " << body->getCenterOfMassTransform().getBasis().transpose() * GetVelocity() << "\n";
		out << "\n";
		fuel_tank.DebugPrint(out);
		out << "\n";
		engine.DebugPrint(out);
		out << "\n";
		clutch.DebugPrint(out);
		out << "\n";
		transmission.DebugPrint(out);
		out << "\n";
		if (drive == RWD)
		{
			out << "(rear)" << "\n";
			differential_rear.DebugPrint(out);
		}
		else if (drive == FWD)
		{
			out << "(front)" << "\n";
			differential_front.DebugPrint(out);
		}
		else if (drive == AWD)
		{
			out << "(center)" << "\n";
			differential_center.DebugPrint(out);

			out << "(front)" << "\n";
			differential_front.DebugPrint(out);

			out << "(rear)" << "\n";
			differential_rear.DebugPrint(out);
		}
		out << "\n";
	}

	if (p2)
	{
		out << "(front left)" << "\n";
		brake[FRONT_LEFT].DebugPrint(out);
		out << "\n";
		suspension[FRONT_LEFT]->DebugPrint(out);
		out << "\n";
		wheel[FRONT_LEFT].DebugPrint(out);
		out << tire[FRONT_LEFT] << "\n";

		out << "(rear left)" << "\n";
		brake[REAR_LEFT].DebugPrint(out);
		out << "\n";
		suspension[REAR_LEFT]->DebugPrint(out);
		out << "\n";
		wheel[REAR_LEFT].DebugPrint(out);
		out << tire[REAR_LEFT] << "\n";
	}

	if (p3)
	{
		out << "(front right)" << "\n";
		brake[FRONT_RIGHT].DebugPrint(out);
		out << "\n";
		suspension[FRONT_RIGHT]->DebugPrint(out);
		out << "\n";
		wheel[FRONT_RIGHT].DebugPrint(out);
		out << tire[FRONT_RIGHT] << "\n";

		out << "(rear right)" << "\n";
		brake[REAR_RIGHT].DebugPrint(out);
		out << "\n";
		suspension[REAR_RIGHT]->DebugPrint(out);
		out << "\n";
		wheel[REAR_RIGHT].DebugPrint(out);
		out << tire[REAR_RIGHT] << "\n";
	}

	if (p4)
	{
		out << std::fixed << std::setprecision(3);
		for (int i = 0; i != aerodevice.size(); ++i)
		{
			out << "---Aerodynamic Device---" << "\n";
			out << "Drag: " << aerodevice[i].getDrag() << "\n";
			out << "Lift: " << aerodevice[i].getLift() << "\n\n";
		}
	}
}

static bool serialize(joeserialize::Serializer & s, btQuaternion & q)
{
	_SERIALIZE_(s, q[0]);
	_SERIALIZE_(s, q[1]);
	_SERIALIZE_(s, q[2]);
	_SERIALIZE_(s, q[3]);
	return true;
}

static bool serialize(joeserialize::Serializer & s, btVector3 & v)
{
	_SERIALIZE_(s, v[0]);
	_SERIALIZE_(s, v[1]);
	_SERIALIZE_(s, v[2]);
	return true;
}

static bool serialize(joeserialize::Serializer & s, btMatrix3x3 & m)
{
	if (!serialize(s, m[0])) return false;
	if (!serialize(s, m[1])) return false;
	if (!serialize(s, m[2])) return false;
	return true;
}

static bool serialize(joeserialize::Serializer & s, btTransform & t)
{
	if (!serialize(s, t.getBasis())) return false;
	if (!serialize(s, t.getOrigin())) return false;
	return true;
}

static bool serialize(joeserialize::Serializer & s, btRigidBody & b)
{
	btTransform t = b.getCenterOfMassTransform();
	btVector3 v = b.getLinearVelocity();
	btVector3 w = b.getAngularVelocity();
	if (!serialize(s, t)) return false;
	if (!serialize(s, v)) return false;
	if (!serialize(s, w)) return false;
	b.setCenterOfMassTransform(t);
	b.setLinearVelocity(v);
	b.setAngularVelocity(w);
	return true;
}

bool CarDynamics::Serialize(joeserialize::Serializer & s)
{
	_SERIALIZE_(s, engine);
	_SERIALIZE_(s, clutch);
	_SERIALIZE_(s, transmission);
	_SERIALIZE_(s, differential_front);
	_SERIALIZE_(s, differential_rear);
	_SERIALIZE_(s, differential_center);
	_SERIALIZE_(s, fuel_tank);
	_SERIALIZE_(s, abs);
	_SERIALIZE_(s, abs_active);
	_SERIALIZE_(s, tcs);
	_SERIALIZE_(s, tcs_active);
	_SERIALIZE_(s, clutch_value);
	_SERIALIZE_(s, remaining_shift_time);
	_SERIALIZE_(s, shift_gear);
	_SERIALIZE_(s, shifted);
	_SERIALIZE_(s, autoshift);
	if (!serialize(s, *body)) return false;
	if (!serialize(s, transform)) return false;
	if (!serialize(s, linear_velocity)) return false;
	if (!serialize(s, angular_velocity)) return false;
	for (int i = 0; i < WHEEL_POSITION_SIZE; ++i)
	{
		_SERIALIZE_(s, wheel[i]);
		_SERIALIZE_(s, brake[i]);
		//_SERIALIZE_(s, tire[i]);
		_SERIALIZE_(s, *suspension[i]);
		if (!serialize(s, wheel_velocity[i])) return false;
		if (!serialize(s, wheel_position[i])) return false;
		if (!serialize(s, wheel_orientation[i])) return false;
	}
	return true;
}

btVector3 CarDynamics::GetDownVector() const
{
	return -body->getCenterOfMassTransform().getBasis().getColumn(2);
}

const btVector3 & CarDynamics::GetCenterOfMassOffset() const
{
	return motion_state[0].massCenterOffset;
}

btVector3 CarDynamics::LocalToWorld(const btVector3 & local) const
{
	return body->getCenterOfMassTransform() * (GetCenterOfMassOffset() + local);
}

btQuaternion CarDynamics::LocalToWorld(const btQuaternion & local) const
{
	return body->getCenterOfMassTransform() * local;
}

void CarDynamics::UpdateWheelVelocity()
{
	for (int i = 0; i < WHEEL_POSITION_SIZE; ++i)
	{
		btVector3 offset = wheel_position[i] - body->getCenterOfMassPosition();
		wheel_velocity[i] = body->getVelocityInLocalPoint(offset);
	}
}

void CarDynamics::UpdateWheelTransform()
{
	for (int i = 0; i < WHEEL_POSITION_SIZE; ++i)
	{
		if (body->getChildBody(i)->isInWorld()) continue;

		btQuaternion rot = suspension[i]->GetWheelOrientation();
		rot *= btQuaternion(Direction::right, -wheel[i].GetRotation());
		btVector3 pos = suspension[i]->GetWheelPosition() + GetCenterOfMassOffset();
		body->setChildTransform(i, btTransform(rot, pos));

		wheel_position[i] = LocalToWorld(suspension[i]->GetWheelPosition());
		wheel_orientation[i] = LocalToWorld(suspension[i]->GetWheelOrientation());
	}
}

void CarDynamics::ApplyEngineTorqueToBody(btVector3 & torque)
{
	btVector3 engine_torque(-engine.GetTorque(), 0, 0);
	assert(!std::isnan(engine_torque[0]));
	torque = body->getCenterOfMassTransform().getBasis() * engine_torque;
}

void CarDynamics::ApplyAerodynamicsToBody(btVector3 & force, btVector3 & torque)
{
	btMatrix3x3 inv = body->getCenterOfMassTransform().getBasis().inverse();
	btVector3 wind_force(0, 0, 0);
	btVector3 wind_torque(0, 0, 0);
	btVector3 air_velocity = inv * -GetVelocity();
	for (int i = 0; i < aerodevice.size(); ++i)
	{
		btVector3 force = aerodevice[i].getForce(air_velocity);
		wind_force = wind_force + force;
		wind_torque = wind_torque + (aerodevice[i].getPosition() + GetCenterOfMassOffset()).cross(force);
	}
	wind_force = body->getCenterOfMassTransform().getBasis() * wind_force;
	wind_torque = body->getCenterOfMassTransform().getBasis() * wind_torque;
	force = force + wind_force;
	torque = torque + wind_torque;
}

///do traction control system (wheelspin prevention) calculations and modify the throttle position if necessary
void CarDynamics::DoTCS(int i, btScalar /*suspension_force*/)
{
	btScalar sense = 1.0;
	if (transmission.GetGear() < 0)
		sense = -1.0;

	btScalar gas = engine.GetThrottle();

	//only active if throttle commanded past threshold
	btScalar gasthresh = 0.1;
	if (gas > gasthresh)
	{
		//see if we're spinning faster than the rest of the wheels
		btScalar maxspindiff = 0;
		btScalar myrotationalspeed = wheel[i].GetAngularVelocity();
		for (int i2 = 0; i2 < WHEEL_POSITION_SIZE; i2++)
		{
			btScalar spindiff = myrotationalspeed - wheel[WheelPosition(i2)].GetAngularVelocity();
			if (spindiff < 0)
				spindiff = -spindiff;
			if (spindiff > maxspindiff)
				maxspindiff = spindiff;
		}

		//don't engage if all wheels are moving at the same rate
		if (maxspindiff > 1.0)
		{
			btScalar sp = tire[i].getIdealSlip();
			btScalar error = tire[i].getSlip() * sense - sp;
			btScalar thresholdeng = 0.0;
			btScalar thresholddis = -sp / 2.0;

			if (error > thresholdeng && ! tcs_active[i])
				tcs_active[i] = true;

			if (error < thresholddis && tcs_active[i])
				tcs_active[i] = false;

			if (tcs_active[i])
			{
				btScalar curclutch = clutch.GetPosition();
				if (curclutch > 1) curclutch = 1;
				if (curclutch < 0) curclutch = 0;

				gas = gas - error * 10.0  *curclutch;
				if (gas < 0) gas = 0;
				if (gas > 1) gas = 1;
				engine.SetThrottle(gas);
			}
		}
		else
			tcs_active[i] = false;
	}
	else
		tcs_active[i] = false;
}

///do anti-lock brake system calculations and modify the brake force if necessary
void CarDynamics::DoABS(int i, btScalar /*suspension_force*/)
{
	//an ideal ABS algorithm

	//only active if brakes commanded past threshold
	btScalar brakesetting = brake[i].GetBrakeFactor();
	if (brakesetting > 0.1)
	{
		btScalar maxspeed = 0;
		for (int i2 = 0; i2 < WHEEL_POSITION_SIZE; i2++)
		{
			if (wheel[WheelPosition(i2)].GetAngularVelocity() > maxspeed)
				maxspeed = wheel[WheelPosition(i2)].GetAngularVelocity();
		}

		//don't engage ABS if all wheels are moving slowly
		if (maxspeed > 6.0)
		{
			btScalar sp = tire[i].getIdealSlip();
			btScalar error = - tire[i].getSlip() - sp;
			btScalar thresholdeng = 0.0;
			btScalar thresholddis = -sp / 2.0;

			if (error > thresholdeng && ! abs_active[i])
				abs_active[i] = true;

			if (error < thresholddis && abs_active[i])
				abs_active[i] = false;
		}
		else
			abs_active[i] = false;
	}
	else
		abs_active[i] = false;

	if (abs_active[i])
		brake[i].SetBrakeFactor(0.0);
}

void CarDynamics::ComputeSuspensionDisplacement(int i, btScalar /*dt*/)
{
	//compute bump effect
	const TrackSurface & surface = wheel_contact[i].GetSurface();
	btScalar posx = wheel_contact[i].GetPosition()[0];
	btScalar posz = wheel_contact[i].GetPosition()[2];
	btScalar phase = 2 * M_PI * (posx + posz) / surface.bumpWaveLength;
	btScalar shift = 2 * btSin(phase * M_PI_2);
	btScalar amplitude = 0.25 * surface.bumpAmplitude;
	btScalar bumpoffset = amplitude * (btSin(phase + shift) + btSin(M_PI_2 * phase) - 2.0);

	btScalar relative_displacement = wheel_contact[i].GetDepth() - 2 * wheel[i].GetRadius() - bumpoffset;
	assert(!std::isnan(relative_displacement));
	suspension[i]->SetDisplacement(suspension[i]->GetDisplacement() - relative_displacement);
	assert(!std::isnan(suspension[i]->GetDisplacement()));
}

///returns the suspension force (so it can be applied to the tires)
btVector3 CarDynamics::ApplySuspensionForceToBody(int i, btScalar dt, btVector3 & force, btVector3 & torque)
{
	//compute suspension force
	btScalar springdampforce = suspension[i]->GetForce(dt);
	assert(!std::isnan(springdampforce));

	//do anti-roll
	int otheri = i;
	if (i == 0 || i == 2) otheri++;
	else otheri--;
	btScalar antirollforce = suspension[i]->GetAntiRoll() *
		(suspension[i]->GetDisplacement() - suspension[WheelPosition(otheri)]->GetDisplacement());
	assert(!std::isnan(antirollforce));

	//find the vector direction to apply the suspension force
#ifdef SUSPENSION_FORCE_DIRECTION
	const btVector3 & wheelext = wheel[i].GetExtendedPosition();
	const btVector3 & hinge = suspension[i].GetHinge();
	btVector3 relwheelext = wheelext - hinge;
	btVector3 up(0, 0, 1);
	btVector3 rotaxis = up.cross(relwheelext.Normalize());
	btVector3 forcedirection = relwheelext.Normalize().cross(rotaxis);
	//std::cout << i << ". " << forcedirection << std::endl;
#else
	btVector3 forcedirection(Direction::up);
#endif
	forcedirection = body->getCenterOfMassTransform().getBasis() * forcedirection;

	btVector3 suspension_force = forcedirection * (antirollforce + springdampforce);
	btVector3 suspension_force_application_point = wheel_position[i] - body->getCenterOfMassPosition();

	btScalar overtravel = suspension[i]->GetOvertravel();
	if (overtravel > 0)
	{
		btScalar correction_factor = 0.0;
		btScalar dv = body->getVelocityInLocalPoint(suspension_force_application_point).dot(forcedirection);
		dv -= correction_factor * overtravel / dt;
		btScalar effectiveMass = 1.0 / body->computeImpulseDenominator(wheel_position[i], forcedirection);
		btScalar correction = -effectiveMass * dv / dt;
		if (correction > 0 && correction > antirollforce + springdampforce)
		{
			suspension_force = forcedirection * correction;
		}
	}

	force = force + suspension_force;
	torque = torque + suspension_force_application_point.cross(suspension_force);

	for (int n = 0; n < 3; ++n) assert(!std::isnan(force[n]));
	for (int n = 0; n < 3; ++n) assert(!std::isnan(torque[n]));

	return suspension_force;
}

btVector3 CarDynamics::ComputeTireFrictionForce(
	int i, btScalar /*dt*/, btScalar normal_force,
	btScalar rotvel, const btVector3 & linvel,
	const btQuaternion & wheel_orientation)
{
	btMatrix3x3 wheel_mat(wheel_orientation);
	btVector3 xw = wheel_mat.getColumn(0);
	btVector3 yw = wheel_mat.getColumn(1);
	btVector3 z = wheel_contact[i].GetNormal();

	btScalar coszxw = z.dot(xw);
	btScalar coszyw = z.dot(yw);
	btVector3 x = (xw - z * coszxw).normalized();
	btVector3 y = (yw - z * coszyw).normalized();

	btScalar camber = M_PI_2 - btAcos(coszxw);
	btScalar lonvel = y.dot(linvel);
	btScalar latvel = -x.dot(linvel);

	btScalar friction_coeff =
		tire[i].getTread() * wheel_contact[i].GetSurface().frictionTread +
		(1.0 - tire[i].getTread()) * wheel_contact[i].GetSurface().frictionNonTread;

	btVector3 friction_force = tire[i].getForce(
		normal_force, friction_coeff, camber, rotvel, lonvel, latvel);

	for (int n = 0; n < 3; ++n) assert(!std::isnan(friction_force[n]));

	return friction_force;
}

void CarDynamics::ApplyWheelForces(btScalar dt, btScalar wheel_drive_torque, int i, const btVector3 & suspension_force, btVector3 & force, btVector3 & torque)
{
	btScalar normal_force = suspension_force.length();
	btScalar rotvel = wheel[i].GetAngularVelocity() * wheel[i].GetRadius();
	btVector3 friction_force = ComputeTireFrictionForce(i, dt, normal_force, rotvel, wheel_velocity[i], wheel_orientation[i]);

	//calculate friction torque
	btVector3 tire_force = Direction::forward * friction_force[0] - Direction::right * friction_force[1];
	btScalar tire_friction_torque = friction_force[0] * wheel[i].GetRadius();
	assert(!std::isnan(tire_friction_torque));

	//calculate brake torque
	btScalar wheel_lock_torque = -wheel[i].GetAngularVelocity() / dt * wheel[i].GetInertia();
	btScalar wheel_brake_torque = wheel_lock_torque - wheel_drive_torque + tire_friction_torque;
	if (wheel_brake_torque > 0 && wheel_brake_torque > brake[i].GetTorque())
	{
		wheel_brake_torque = brake[i].GetTorque();
	}
	else if (wheel_brake_torque < 0 && wheel_brake_torque < -brake[i].GetTorque())
	{
		wheel_brake_torque = -brake[i].GetTorque();
	}
	assert(!std::isnan(wheel_brake_torque));

	//limit the reaction torque to the applied drive and braking torque
	btScalar reaction_torque = tire_friction_torque;
	btScalar applied_torque = wheel_drive_torque + wheel_brake_torque;
	if ((applied_torque > 0 && reaction_torque > applied_torque) ||
			(applied_torque < 0 && reaction_torque < applied_torque))
		reaction_torque = applied_torque;
	btVector3 tire_torque = Direction::right * reaction_torque;// - direction::up * friction_force[2];

	//set wheel torque due to tire rolling resistance
	btScalar rolling_resistance = -tire[i].getRollingResistance(wheel[i].GetAngularVelocity(), wheel_contact[i].GetSurface().rollResistanceCoefficient);
	btScalar rolling_resistance_torque = rolling_resistance * wheel[i].GetRadius() - tire_friction_torque;
	btScalar wheel_torque = wheel_drive_torque + wheel_brake_torque + rolling_resistance_torque;

	//have the wheels internally apply forces, or just forcibly set the wheel speed if the brakes are locked
	wheel[i].SetTorque(wheel_torque, dt);
	wheel[i].Integrate(dt);

	//viscous tire contact drag (hack)
	btVector3 wheel_drag = -wheel_velocity[i] * wheel_contact[i].GetSurface().rollingDrag;

	//apply forces to body
	btVector3 tire_pos = wheel_position[i] - body->getCenterOfMassPosition();
	btVector3 world_tire_force = quatRotate(wheel_orientation[i], tire_force);
	btVector3 world_tire_torque = quatRotate(wheel_orientation[i],  tire_torque);
	world_tire_force += wheel_drag;
	world_tire_torque += tire_pos.cross(world_tire_force);
	force = force + world_tire_force;
	torque = torque + world_tire_torque;
}

///the core function of the car dynamics simulation:  find and apply all forces on the car and components.
void CarDynamics::ApplyForces(btScalar dt, const btVector3 & ext_force, const btVector3 & ext_torque)
{
	assert(dt > 0);

	//start accumulating forces and torques on the car body
	btVector3 force = ext_force;
	btVector3 torque = ext_torque;

	//do TCS first thing
	if (tcs)
	{
		for (int i = 0; i < WHEEL_POSITION_SIZE; ++i)
		{
			DoTCS(i, suspension_force[i].length());
		}
	}

	//compute wheel torques
	btScalar wheel_drive_torque[4];
	UpdateDriveline(wheel_drive_torque, dt);

	//apply equal and opposite engine torque to the chassis
	//ApplyEngineTorqueToBody(force, torque);

	//apply aerodynamics
	ApplyAerodynamicsToBody(force, torque);

	//compute suspension displacements
	for (int i = 0; i < WHEEL_POSITION_SIZE; ++i)
	{
		ComputeSuspensionDisplacement(i, dt);
	}

	//compute suspension forces
	for (int i = 0; i < WHEEL_POSITION_SIZE; ++i)
	{
		suspension_force[i] = ApplySuspensionForceToBody(i, dt, force, torque);
	}

	//do abs
	if (abs)
	{
		for (int i = 0; i < WHEEL_POSITION_SIZE; ++i)
		{
			DoABS(i, suspension_force[i].length());
		}
	}

	//compute wheel forces
	for (int i = 0; i < WHEEL_POSITION_SIZE; ++i)
	{
		ApplyWheelForces(dt, wheel_drive_torque[i], i, suspension_force[i], force, torque);
	}

	for (int n = 0; n < 3; ++n) assert(!std::isnan(force[n]));
	for (int n = 0; n < 3; ++n) assert(!std::isnan(torque[n]));

	body->applyCentralForce(force);
	body->applyTorque(torque);
}

void CarDynamics::Tick(btScalar dt, const btVector3 & force, const btVector3 & torque)
{
	body->clearForces();

	ApplyForces(dt, force, torque);

	body->integrateVelocities(dt);
	body->predictIntegratedTransform(dt, transform);
	body->proceedToTransform(transform);

	UpdateWheelVelocity();
	UpdateWheelTransform();
	InterpolateWheelContacts();
}

// executed as last function(after integration) in bullet singlestepsimulation
void CarDynamics::updateAction(btCollisionWorld * /*collisionWorld*/, btScalar dt)
{
	// shift/clutch logic
	UpdateTransmission(dt);

	// reset transform, before processing tire/suspension constraints
	// will break bullets collision clamping, tunneling prevention
	body->setCenterOfMassTransform(transform);
	btVector3 dv = body->getLinearVelocity() - linear_velocity;
	btVector3 dw = body->getAngularVelocity() - angular_velocity;
	btVector3 force = 1.0 / body->getInvMass() * dv / dt;
	btVector3 torque = body->getInvInertiaTensorWorld().inverse() * dw / dt;
	body->setLinearVelocity(linear_velocity);
	body->setAngularVelocity(angular_velocity);
	UpdateWheelContacts();

	feedback = 0;
	int repeats = 10;
	for (int i = 0; i < repeats; ++i)
	{
		Tick(dt / repeats, force, torque);

		feedback += tire[FRONT_LEFT].getMz() + tire[FRONT_RIGHT].getMz();
	}
	feedback /= repeats;
	feedback *= feedback_scale;

	//update fuel tank
	fuel_tank.Consume(engine.FuelRate() * dt);
	engine.SetOutOfGas(fuel_tank.Empty());

	//calculate tacho
	const float tacho_factor = 0.1;
	tacho_rpm = engine.GetRPM() * tacho_factor + tacho_rpm * (1.0 - tacho_factor);

	linear_velocity = body->getLinearVelocity();
	angular_velocity = body->getAngularVelocity();
}

void CarDynamics::UpdateWheelContacts()
{
	btVector3 raydir = GetDownVector();
	btScalar raylen = 4;
	for (int i = 0; i < WHEEL_POSITION_SIZE; ++i)
	{
		btVector3 raystart = wheel_position[i] - raydir * wheel[i].GetRadius();
		if (body->getChildBody(i)->isInWorld())
		{
			// wheel separated
			wheel_contact[i] = CollisionContact(raystart, raydir, raylen, -1, 0, TrackSurface::None(), 0);
		}
		else
		{
			world->castRay(raystart, raydir, raylen, body, wheel_contact[i]);
		}
	}
}

void CarDynamics::InterpolateWheelContacts()
{
	btVector3 raydir = GetDownVector();
	btScalar raylen = 4;
	for (int i = 0; i < WHEEL_POSITION_SIZE; ++i)
	{
		btVector3 raystart = wheel_position[i] - raydir * wheel[i].GetRadius();
		GetWheelContact(WheelPosition(i)).CastRay(raystart, raydir, raylen);
	}
}

void CarDynamics::UpdateDriveline(btScalar drive_torque[], btScalar dt)
{
	btScalar driveshaft_speed = CalculateDriveshaftSpeed();
	btScalar clutch_speed = transmission.CalculateClutchSpeed(driveshaft_speed);
	btScalar crankshaft_speed = engine.GetAngularVelocity();
	btScalar clutch_drag = clutch.GetTorque(crankshaft_speed, clutch_speed);
	if (transmission.GetGear() == 0) clutch_drag = 0;

	clutch_drag = engine.Integrate(clutch_drag, clutch_speed, dt);

	CalculateDriveTorque(drive_torque, -clutch_drag);
}

///calculate the drive torque that the engine applies to each wheel, and put the output into the supplied 4-element array
void CarDynamics::CalculateDriveTorque(btScalar wheel_drive_torque[], btScalar clutch_torque)
{
	btScalar driveshaft_torque = transmission.GetTorque(clutch_torque);
	assert(!std::isnan(driveshaft_torque));

	for (int i = 0; i < WHEEL_POSITION_SIZE; ++i)
		wheel_drive_torque[i] = 0;

	if (drive == RWD)
	{
		differential_rear.ComputeWheelTorques(driveshaft_torque);
		wheel_drive_torque[REAR_LEFT] = differential_rear.GetSide1Torque();
		wheel_drive_torque[REAR_RIGHT] = differential_rear.GetSide2Torque();
	}
	else if (drive == FWD)
	{
		differential_front.ComputeWheelTorques(driveshaft_torque);
		wheel_drive_torque[FRONT_LEFT] = differential_front.GetSide1Torque();
		wheel_drive_torque[FRONT_RIGHT] = differential_front.GetSide2Torque();
	}
	else if (drive == AWD)
	{
		differential_center.ComputeWheelTorques(driveshaft_torque);
		differential_front.ComputeWheelTorques(differential_center.GetSide1Torque());
		differential_rear.ComputeWheelTorques(differential_center.GetSide2Torque());
		wheel_drive_torque[FRONT_LEFT] = differential_front.GetSide1Torque();
		wheel_drive_torque[FRONT_RIGHT] = differential_front.GetSide2Torque();
		wheel_drive_torque[REAR_LEFT] = differential_rear.GetSide1Torque();
		wheel_drive_torque[REAR_RIGHT] = differential_rear.GetSide2Torque();
	}

	for (int i = 0; i < WHEEL_POSITION_SIZE; ++i)
		assert(!std::isnan(wheel_drive_torque[WheelPosition(i)]));
}

btScalar CarDynamics::CalculateDriveshaftSpeed()
{
	btScalar driveshaft_speed = 0.0;
	btScalar left_front_wheel_speed = wheel[FRONT_LEFT].GetAngularVelocity();
	btScalar right_front_wheel_speed = wheel[FRONT_RIGHT].GetAngularVelocity();
	btScalar left_rear_wheel_speed = wheel[REAR_LEFT].GetAngularVelocity();
	btScalar right_rear_wheel_speed = wheel[REAR_RIGHT].GetAngularVelocity();

	for (int i = 0; i < 4; ++i)
		assert(!std::isnan(wheel[i].GetAngularVelocity()));

	if (drive == RWD)
	{
		driveshaft_speed = differential_rear.CalculateDriveshaftSpeed(left_rear_wheel_speed, right_rear_wheel_speed);
	}
	else if (drive == FWD)
	{
		driveshaft_speed = differential_front.CalculateDriveshaftSpeed(left_front_wheel_speed, right_front_wheel_speed);
	}
	else if (drive == AWD)
	{
		btScalar front_speed = differential_front.CalculateDriveshaftSpeed(left_front_wheel_speed, right_front_wheel_speed);
		btScalar rear_speed = differential_rear.CalculateDriveshaftSpeed(left_rear_wheel_speed, right_rear_wheel_speed);
		driveshaft_speed = differential_center.CalculateDriveshaftSpeed(front_speed, rear_speed);
	}

	return driveshaft_speed;
}

void CarDynamics::UpdateTransmission(btScalar dt)
{
	btScalar driveshaft_speed = CalculateDriveshaftSpeed();

	driveshaft_rpm = transmission.GetClutchSpeed(driveshaft_speed) * 30.0 / M_PI;

	if (autoshift)
	{
		int gear = NextGear();
		ShiftGear(gear);
	}

	remaining_shift_time -= dt;
	if (remaining_shift_time < 0) remaining_shift_time = 0;

	if (remaining_shift_time <= transmission.GetShiftTime() * 0.5 && !shifted)
	{
		shifted = true;
		transmission.Shift(shift_gear);
	}

	if (autoclutch)
	{
		if (!engine.GetCombustion())
		{
			engine.StartEngine();
		}

		btScalar throttle = engine.GetThrottle();
		throttle = ShiftAutoClutchThrottle(throttle, dt);
		engine.SetThrottle(throttle);

		// allow auto clutch override
		if (clutch.GetPosition() >= clutch_value)
		{
			clutch_value = AutoClutch(dt);
			clutch.SetPosition(clutch_value);
		}
	}
}

bool CarDynamics::WheelDriven(int i) const
{
	return (1 << i) & drive;
}

btScalar CarDynamics::AutoClutch(btScalar dt) const
{
	btScalar clutch_engage_limit = 10.0f * dt; // 0.1 seconds
	btScalar clutch_old = clutch_value;
	btScalar clutch_new = 1.0f;

	// antistall
	btScalar rpm_clutch = driveshaft_rpm;
	btScalar rpm_idle = engine.GetStartRPM();
	if (rpm_clutch < rpm_idle)
	{
		btScalar rpm_engine = engine.GetRPM();
		btScalar rpm_min = 0.5f * (engine.GetStallRPM() + rpm_idle);
		btScalar t = 1.5f - 0.5f * engine.GetThrottle();
		btScalar c = rpm_engine / (rpm_min * (1 - t) + rpm_idle * t) - 1.0f;
		btClamp(c, 0.0f, 1.0f);
		clutch_new = c;
	}

	// shifting
	const btScalar shift_time = transmission.GetShiftTime();
	if (remaining_shift_time > shift_time * 0.5f)
	{
		clutch_new = 0.0f;
	}
	else if (remaining_shift_time > 0.0f)
	{
		clutch_new *= (1.0f - remaining_shift_time / (shift_time * 0.5f));
	}

	// rate limit the autoclutch
	btScalar clutch_delta = clutch_new - clutch_old;
	btClamp(clutch_delta, -clutch_engage_limit * 2.0f, clutch_engage_limit);
	clutch_new = clutch_old + clutch_delta;

	return clutch_new;
}

btScalar CarDynamics::ShiftAutoClutchThrottle(btScalar throttle, btScalar dt)
{
	if (remaining_shift_time > 0.0)
	{
		if (engine.GetRPM() < driveshaft_rpm && engine.GetRPM() < engine.GetRedline())
		{
			remaining_shift_time += dt;
			return 1.0;
		}
		else
		{
			return 0.5 * throttle;
		}
	}
	return throttle;
}

///return the gear change (0 for no change, -1 for shift down, 1 for shift up)
int CarDynamics::NextGear() const
{
	int gear = transmission.GetGear();

	// only autoshift if a shift is not in progress
	if (shifted && clutch.GetPosition() == 1.0)
	{
		// shift up when driveshaft speed exceeds engine redline
		// we do not shift up from neutral/reverse
		if (driveshaft_rpm > engine.GetRedline() && gear > 0)
		{
			return gear + 1;
		}
		// shift down when driveshaft speed below shift_down_point
		// we do not auto shift down from 1st gear to neutral
		if (driveshaft_rpm < DownshiftRPM(gear) && gear > 1)
		{
			return gear - 1;
		}
	}
	return gear;
}

btScalar CarDynamics::DownshiftRPM(int gear) const
{
	btScalar shift_down_point = 0.0;
	if (gear > 1)
	{
		btScalar current_gear_ratio = transmission.GetGearRatio(gear);
		btScalar lower_gear_ratio = transmission.GetGearRatio(gear - 1);
		btScalar peak_engine_speed = engine.GetRedline();
		shift_down_point = 0.7 * peak_engine_speed / lower_gear_ratio * current_gear_ratio;
	}
	return shift_down_point;
}

btScalar CarDynamics::CalculateMaxSpeed() const
{
	// speed limit due to engine and transmission
	btScalar ratio = transmission.GetGearRatio(transmission.GetForwardGears());
	btScalar drive_speed = engine.GetRPMLimit() * M_PI / 30;
	if (drive == RWD)
	{
		ratio *= differential_rear.GetFinalDrive();
		drive_speed *= wheel[REAR_LEFT].GetRadius() / ratio;
	}
	else if (drive == FWD)
	{
		ratio *= differential_front.GetFinalDrive();
		drive_speed *= wheel[FRONT_LEFT].GetRadius() / ratio;
	}
	else if (drive == AWD)
	{
		ratio *= differential_front.GetFinalDrive();
		ratio *= differential_center.GetFinalDrive();
		drive_speed *= wheel[FRONT_LEFT].GetRadius() / ratio;
	}

	// speed limit due to drag
	btScalar aero_speed = btPow(engine.GetMaxPower() / GetAeordynamicDragCoefficient(), 1 / 3.0f);

	return btMin(drive_speed, aero_speed);
}

void CarDynamics::SetSteering(const btScalar value)
{
	for (int i = 0; i < WHEEL_POSITION_SIZE; ++i)
	{
		suspension[i]->SetSteering(value);
	}
}

void CarDynamics::StartEngine()
{
	engine.StartEngine();
}

void CarDynamics::ShiftGear(int value)
{
	if (shifted &&
		value != transmission.GetGear() &&
		value <= transmission.GetForwardGears() &&
		value >= -transmission.GetReverseGears())
	{
		remaining_shift_time = transmission.GetShiftTime();
		shift_gear = value;
		shifted = false;
	}
}

void CarDynamics::SetThrottle(btScalar value)
{
	engine.SetThrottle(value);
}

void CarDynamics::SetNOS(btScalar value)
{
	engine.SetNosBoost(value);
}

void CarDynamics::SetClutch(btScalar value)
{
	clutch.SetPosition(value);
}

void CarDynamics::SetBrake(btScalar value)
{
	brake_value = value;
	for (int i = 0; i < brake.size(); ++i)
	{
		brake[i].SetBrakeFactor(value);
	}
}

void CarDynamics::SetHandBrake(btScalar value)
{
	for (int i = 0; i < brake.size(); ++i)
	{
		brake[i].SetHandbrakeFactor(value);
	}
}

void CarDynamics::RolloverRecover()
{
	btVector3 z(Direction::up);
	btVector3 y_car = transform.getBasis() * Direction::forward;
	y_car = y_car - z * z.dot(y_car);
	y_car.normalize();

	btVector3 z_car = transform.getBasis() * Direction::up;
	z_car = z_car - y_car * y_car.dot(z_car);
	z_car.normalize();

	btScalar angle = z_car.angle(z);
	if (fabs(angle) < M_PI / 4.0) return;

	btQuaternion rot(y_car, angle);
	rot = rot * transform.getRotation();

	transform.setRotation(rot);
	body->setCenterOfMassTransform(transform);

	AlignWithGround();
}

void CarDynamics::Clear()
{
	if (!body) return;

	assert(world);
	for (int i = 0; i < body->getNumChildren(); ++i)
	{
		btRigidBody * child = body->getChildBody(i);
		if (child->isInWorld())
		{
			world->removeRigidBody(child);
			delete child->getCollisionShape();
		}
		delete child;
	}
	world->removeAction(this);
	world->removeRigidBody(body);
	world = 0;

	if (body->getCollisionShape()->isCompound())
	{
		const btCompoundShape * shape = static_cast<btCompoundShape*>(body->getCollisionShape());
		for (int i = 0; i < shape->getNumChildShapes(); ++i)
		{
			delete shape->getChildShape(i);
		}
	}
	delete body->getCollisionShape();
	delete body;
	body = 0;

	for (int i = 0; i < suspension.size(); ++i)
	{
		delete suspension[i];
		suspension[i] = 0;
	}

	for (int i = 0; i < aerodevice.size(); ++i)
	{
		delete static_cast<AeroDeviceFracture*>(aerodevice[i].GetUserPointer());
	}
	aerodevice.clear();
}

void CarDynamics::Init()
{
	world = 0;
	body = 0;
	transform.setIdentity();
	linear_velocity.setZero();
	angular_velocity.setZero();
	drive = NONE;
	driveshaft_rpm = 0;
	tacho_rpm = 0;
	autoclutch = true;
	autoshift = false;
	shifted = true;
	shift_gear = 0;
	remaining_shift_time = 0;
	clutch_value = 1;
	brake_value = 0;
	abs = false;
	tcs = false;
	maxangle = 0;
	maxspeed = 0;
	feedback_scale = 0;
	feedback = 0;

	suspension.resize(WHEEL_POSITION_SIZE, 0);
	wheel.resize(WHEEL_POSITION_SIZE);
	tire.resize(WHEEL_POSITION_SIZE);
	brake.resize(WHEEL_POSITION_SIZE);
	suspension_force.resize(WHEEL_POSITION_SIZE);
	wheel_velocity.resize(WHEEL_POSITION_SIZE);
	wheel_position.resize(WHEEL_POSITION_SIZE);
	wheel_orientation.resize(WHEEL_POSITION_SIZE);
	wheel_contact.resize(WHEEL_POSITION_SIZE);
	abs_active.resize(WHEEL_POSITION_SIZE, false);
	tcs_active.resize(WHEEL_POSITION_SIZE, false);
}

bool CarDynamics::WheelContactCallback(
	btManifoldPoint& cp,
	const btCollisionObjectWrapper* col0,
	int /*partId0*/,
	int /*index0*/,
	const btCollisionObjectWrapper* col1,
	int /*partId1*/,
	int /*index1*/)
{
	// apply to cars only
	const btCollisionObject * obj0 = col0->getCollisionObject();
	if (!(obj0->getInternalType() & CO_FRACTURE_TYPE))
		return false;

	const btCollisionShape * shape1 = col1->getCollisionShape();
	const btCollisionShape * shape0 = col0->getCollisionShape();

	// invalidate wheel shape contact with ground as we are handling it separately
	if (shape0->getShapeType() == CYLINDER_SHAPE_PROXYTYPE)
	{
		const btCollisionShape * root_shape = obj0->getCollisionShape();
		const btCompoundShape * car_shape = static_cast<const btCompoundShape *>(root_shape);
		const btCylinderShapeX * wheel_shape = static_cast<const btCylinderShapeX *>(shape0);
		btVector3 contact_point = cp.m_localPointA - car_shape->getChildTransform(cp.m_index0).getOrigin();
		if (-Direction::up.dot(contact_point) > 0.5 * wheel_shape->getRadius())
		{
			cp.m_normalWorldOnB = btVector3(0, 0, 0);
			cp.m_distance1 = 0;
			cp.m_combinedFriction = 0;
			cp.m_combinedRestitution = 0;
			return true;
		}
	}

#ifndef ENABLE_EDGE_COLLISIONS
	// filter edge collisions
	if (shape1->getShapeType() == TRIANGLE_SHAPE_PROXYTYPE)
	{
		const btTriangleShape * triangle = static_cast<const btTriangleShape *>(shape1);
		const btVector3 * v = &triangle->getVertexPtr(0);
		btVector3 n = (v[1] - v[0]).cross(v[2] - v[0]);
		btScalar d = n.dot(cp.m_normalWorldOnB);
		btScalar n2 = n.dot(n);
		btScalar d2 = d * d;

		// if angle below 80 deg fix up normal
		if (d2 < n2 * 0.97)
		{
			btScalar rlen = 1 / btSqrt(n2);
			if (d < 0) rlen = -rlen;

			cp.m_normalWorldOnB =  n * rlen;
			cp.m_distance1 *= d * rlen;
		}
	}
#endif

	return false;
}

const btCollisionObject & CarDynamics::getCollisionObject() const
{
	return *static_cast<btCollisionObject*>(body);
}
