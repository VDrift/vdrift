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
#include "minmax.h"

#include "BulletCollision/CollisionShapes/btCompoundShape.h"
#include "BulletCollision/CollisionShapes/btCylinderShape.h"
#include "BulletCollision/CollisionShapes/btTriangleShape.h"

#include <cmath>

static const btScalar gravity = 9.81;

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

static btScalar ComputeFrictionCoeff(btScalar r, btScalar w, btScalar ar, btScalar pt, btScalar fz)
{
	btScalar wt = (1.03f - 0.4f * ar) * w;
	btScalar cf = 0.28f * btSqrt(wt * r * 2);
	btScalar kz = 9.81f * (1E5f * pt * cf + 3450);
	btScalar dz = fz / kz;
	btScalar a = 0.3f * (dz + 2.25f * btSqrt(r * dz));
	btScalar p = fz / (2 * a * wt);
	btScalar mup = 100 * btPow(p, -1/3.0);
	return mup;
}

static btScalar ComputeFrictionFactor(const PTree & cfg, const btVector3 & size)
{
	btScalar r0, w0, ar0, pt0, fz0;
	if (!cfg.get("R0", r0)) return 1;
	if (!cfg.get("W0", w0)) return 1;
	if (!cfg.get("AR0", ar0)) return 1;
	if (!cfg.get("PT0", pt0)) return 1;
	if (!cfg.get("FZ0", fz0)) return 1;

	btScalar w = size[0] * 0.001f;
	btScalar ar = size[1] * 0.01f;
	btScalar r = size[2] * 0.5f * 0.0254f + w * ar;

	btScalar mu0 = ComputeFrictionCoeff(r0, w0, ar0, pt0, fz0);
	btScalar mu1 = ComputeFrictionCoeff(r, w, ar, pt0, fz0);
	btScalar cf = mu1 / mu0;
	return cf;
}

#if defined(VDRIFTP)
static bool LoadTire(const PTree & cfg_wheel, const PTree & cfg, CarTire & tire, std::ostream & error_output)
{
	btVector3 tire_size;
	if (!cfg_wheel.get("tire.size", tire_size, error_output)) return false;
	btScalar width = tire_size[0] * 0.001f;
	btScalar aspect_ratio = tire_size[1] * 0.01f;
	btScalar radius = tire_size[2] * 0.5f * 0.0254f + width * aspect_ratio;

	CarTireInfo info;
	info.radius = radius;
	info.width = width;
	info.ar = aspect_ratio;

	if (!cfg.get("pt", info.pt, error_output)) return false;
	if (!cfg.get("ktx", info.ktx, error_output)) return false;
	if (!cfg.get("kty", info.kty, error_output)) return false;
	if (!cfg.get("kcb", info.kcb, error_output)) return false;
	if (!cfg.get("ccb", info.ccb, error_output)) return false;
	if (!cfg.get("cfy", info.cfy, error_output)) return false;
	if (!cfg.get("dz0", info.dz0, error_output)) return false;
	if (!cfg.get("p0", info.p0, error_output)) return false;
	if (!cfg.get("mus", info.mus, error_output)) return false;
	if (!cfg.get("muc", info.muc, error_output)) return false;
	if (!cfg.get("vs", info.vs, error_output)) return false;
	if (!cfg.get("cr0", info.cr0, error_output)) return false;
	if (!cfg.get("cr2", info.cr2, error_output)) return false;
	if (!cfg.get("tread", info.tread, error_output)) return false;

	tire.init(info);
	return true;
}
#elif defined(VDRIFTN)
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
	info.coefficients[CarTireInfo::PEY3] *= side_factor;
	info.coefficients[CarTireInfo::PEY4] *= side_factor;
	info.coefficients[CarTireInfo::PVY1] *= side_factor;
	info.coefficients[CarTireInfo::PVY2] *= side_factor;
	info.coefficients[CarTireInfo::PHY1] *= side_factor;
	info.coefficients[CarTireInfo::PHY2] *= side_factor;
	info.coefficients[CarTireInfo::PHY3] *= side_factor;
	info.coefficients[CarTireInfo::RBY3] *= side_factor;
	info.coefficients[CarTireInfo::RHX1] *= side_factor;
	info.coefficients[CarTireInfo::RHY1] *= side_factor;
	info.coefficients[CarTireInfo::RVY5] *= side_factor;

	btScalar size_factor = 1;
	btVector3 size;
	if (cfg_wheel.get("tire.size", size))
		size_factor = ComputeFrictionFactor(cfg, size);
	info.coefficients[CarTireInfo::PDX1] *= size_factor;
	info.coefficients[CarTireInfo::PDY1] *= size_factor;

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

	// read combining
	if (!cfg.get("gy1", info.combining[0], error_output)) return false;
	if (!cfg.get("gy2", info.combining[1], error_output)) return false;
	if (!cfg.get("gx1", info.combining[2], error_output)) return false;
	if (!cfg.get("gx2", info.combining[3], error_output)) return false;

	// asymmetric tires support (left right facing direction)
	// default facing direction is right
	// fixme: should handle aligning torque too?
	btScalar side_factor = 0;
	std::string facing;
	if (cfg_wheel.get("tire.facing", facing))
		side_factor = (facing != "left") ? 1 : -1;
	info.lateral[13] *= side_factor;
	info.lateral[14] *= side_factor;

	btScalar size_factor = 1;
	btVector3 size;
	if (cfg_wheel.get("tire.size", size))
		size_factor = ComputeFrictionFactor(cfg, size);
	info.longitudinal[2] *= size_factor;
	info.lateral[2] *= size_factor;

	tire.init(info);

	return true;
}
#endif

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

	btScalar tire_volume = btScalar(M_PI) * tire_width * tire_thickness * tire_thickness * (2 * tire_radius  - tire_thickness);
	btScalar rim_volume = btScalar(M_PI) * rim_width * rim_thickness * rim_thickness * (2 * rim_radius - rim_thickness);
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
	CarDifferentialInfo info;
	if (!cfg.get("final-drive", info.final_drive, error_output)) return false;
	if (!cfg.get("anti-slip", info.anti_slip, error_output)) return false;
	cfg.get("anti-slip-torque", info.anti_slip_torque);
	cfg.get("anti-slip-torque-deceleration-factor", info.anti_slip_torque_deceleration_factor);
	cfg.get("torque-split", info.torque_split);

	diff.Init(info);
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

		const btScalar deg2rad = M_PI / 180;
		btQuaternion qrot(rot[1] * deg2rad, rot[0] * deg2rad, rot[2] * deg2rad);
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
	for (const auto & node : *cfg_wheels)
	{
		const PTree & cfg_wheel = node.second;
		if (!LoadWheel(cfg_wheel, wheel[i], error)) return false;

		std::string tirestr(cartire);
		std::shared_ptr<PTree> cfg_tire;
		if ((cartire.empty() || cartire == "default") &&
			!cfg_wheel.get("tire.type", tirestr, error)) return false;
		#if defined(VDRIFTP)
		tirestr += "p";
		#elif defined(VDRIFTN)
		tirestr += "n";
		#endif
		content.load(cfg_tire, cardir, tirestr);
		if (!LoadTire(cfg_wheel, *cfg_tire, tire[i], error)) return false;

		const PTree * cfg_brake;
		if (!cfg_wheel.get("brake", cfg_brake, error)) return false;
		if (!LoadBrake(*cfg_brake, brake[i], error)) return false;

		if (!CarSuspension::Load(cfg_wheel, wheel[i].GetMass(), suspension[i], error)) return false;
		if (suspension[i]->GetMaxSteeringAngle() > maxangle)
			maxangle = suspension[i]->GetMaxSteeringAngle();

		btScalar mass = wheel[i].GetMass();
		btScalar width = wheel[i].GetWidth();
		btScalar radius = wheel[i].GetRadius();
		btVector3 size(width * 0.5f, radius, radius);
		btCollisionShape * shape = new btCylinderShapeX(size);
		loadBody(cfg_wheel, error, shape, mass, true);
		i++;
	}

	// load children bodies
	for (const auto & node : cfg)
	{
		if (!loadBody(node.second, error)) return false;
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
	btVector3 fwd_offset = fwd * (2 - bmax.y());
	btVector3 up_offset = -up * (btScalar(0.5) + bmin.z());

	// adjust for suspension rest position
	// a bit hacky here, should use updated aabb
	btScalar m = 1 / body->getInvMass();
	btScalar m1 = m * CalculateFrontMassRatio();
	btScalar m2 = m - m1;
	btScalar d1 = suspension[0]->GetDisplacement(m1 * gravity / 2);
	btScalar d2 = suspension[3]->GetDisplacement(m2 * gravity / 2);
	suspension[0]->SetDisplacement(d1);
	suspension[1]->SetDisplacement(d1);
	suspension[2]->SetDisplacement(d2);
	suspension[3]->SetDisplacement(d2);
	up_offset -= up * Max(d1, d2);

	SetPosition(body->getCenterOfMassPosition() + up_offset + fwd_offset);
	UpdateWheelTransform();

	// update motion state
	body->getMotionState()->setWorldTransform(body->getWorldTransform());

	// init cached state
	linear_velocity = body->getLinearVelocity();
	angular_velocity = body->getAngularVelocity();
	for (int i = 0; i < WHEEL_POSITION_SIZE; ++i)
		wheel_velocity[i].setZero();

	CalculateAerodynamicCoeffs(aero_lift_coeff, aero_drag_coeff);
	CalculateFrictionCoeffs(lon_friction_coeff, lat_friction_coeff);
	maxspeed = CalculateMaxSpeed();

	// calculate steering feedback scale factor
	// use max Mz of the 2 front wheels assuming even weight distribution
	// and a fudge factor of 8 to get feedback into -1, 1 range
	const float max_wheel_load = gravity / (4 * body->getInvMass());
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

void CarDynamics::SetSteeringAssist(bool value)
{
	steering_assist = value;
}

void CarDynamics::SetAutoReverse(bool value)
{
	autoreverse = value;
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

	// do shifting
	int gear_change = 0;
	if (inputs[CarInput::SHIFT_UP] == 1)
		gear_change = 1;
	if (inputs[CarInput::SHIFT_DOWN] == 1)
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

	float brake_input = inputs[CarInput::BRAKE];
	float throttle_input = inputs[CarInput::THROTTLE];
	if (autoreverse && driveshaft_rpm < 1)
	{
		if (new_gear > 0 &&
			brake_input > 1E-3f &&
			throttle_input < 1E-3f &&
			inputs[CarInput::CLUTCH] < 1)
		{
			new_gear = -1;
		}
		else if (new_gear < 0)
		{
			if (driveshaft_rpm > -1 &&
				brake_input < 1E-3f &&
				throttle_input > 1E-3f &&
				inputs[CarInput::CLUTCH] < 1)
			{
				new_gear = 1;
			}
			//else
			{
				throttle_input = inputs[CarInput::BRAKE];
				brake_input = inputs[CarInput::THROTTLE];
			}
		}
	}

	SetSteering(inputs[CarInput::STEER_RIGHT] - inputs[CarInput::STEER_LEFT]);

	SetHandBrake(inputs[CarInput::HANDBRAKE]);

	SetBrake(brake_input);

	SetThrottle(throttle_input);

	SetClutch(1 - inputs[CarInput::CLUTCH]);

	ShiftGear(new_gear);

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
	groundvel[1] *= 2;
	groundvel[2] = 0;
	btScalar squeal = (groundvel.length() - 3) * btScalar(0.2);

	btScalar sr = GetTire(i).getSlip() / GetTire(i).getIdealSlip();
	btScalar ar = GetTire(i).getSlipAngle() / GetTire(i).getIdealSlipAngle();
	btScalar maxratio = Max(std::abs(sr), std::abs(ar));
	btScalar squealfactor = Max(btScalar(0), maxratio - 1);
	squeal = Clamp(squeal * squealfactor, btScalar(0), btScalar(1));

	return squeal;
}

btScalar CarDynamics::GetMaxSpeed(btScalar radius, btScalar friction) const
{
	// m*v^2 / r = mu * (m*g + cl*v^2)
	btScalar fr = friction * lat_friction_coeff * radius;
	btScalar d = 1 + fr * aero_lift_coeff * GetInvMass();
	btScalar v2 = fr * gravity  / Max(d, btScalar(1E-9));
	btScalar v = std::sqrt(v2);
	return v;
}

btScalar CarDynamics::GetBrakeDistance(btScalar initial_speed, btScalar final_speed, btScalar friction) const
{
	if (initial_speed <= final_speed)
		return 0;

	// m/2 * (v2^2 - v1^2) = integrate(mu * (m*g + cl*v^2) + cd*v^2)
	btScalar mu = friction * lon_friction_coeff;
	btScalar v1s = initial_speed * initial_speed;
	btScalar v2s = final_speed * final_speed;
	btScalar d = (-aero_lift_coeff * mu + aero_drag_coeff) * GetInvMass();
	btScalar e = 1 / d;
	btScalar f = e * mu * gravity;
	btScalar g = (f + v1s) / (f + v2s);
	btScalar distance = 0.5f * e * std::log(g);
	//btScalar distance = (v1s - v2s) / (mu * gravity * 2);
	return distance;
}

std::vector<float> CarDynamics::GetSpecs() const
{
	std::vector<float> specs;
	specs.reserve(7);
	specs.push_back(maxspeed);
	specs.push_back(float(int(drive)));
	specs.push_back(engine.GetDisplacement());
	specs.push_back(engine.GetMaxPower());
	specs.push_back(engine.GetMaxTorque());
	specs.push_back(1 / body->getInvMass());
	specs.push_back(CalculateFrontMassRatio());
	return specs;
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

void CarDynamics::DoTCS(int i)
{
	btScalar sense = 1;
	if (transmission.GetGear() < 0)
		sense = -1;

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
		if (maxspindiff > 1)
		{
			btScalar sp = tire[i].getIdealSlip();
			btScalar error = tire[i].getSlip() * sense - sp;
			btScalar thresholdeng = 0;
			btScalar thresholddis = -sp * btScalar(0.5);

			if (error > thresholdeng && ! tcs_active[i])
				tcs_active[i] = true;

			if (error < thresholddis && tcs_active[i])
				tcs_active[i] = false;

			if (tcs_active[i])
			{
				btScalar curclutch = clutch.GetPosition();
				if (curclutch > 1) curclutch = 1;
				if (curclutch < 0) curclutch = 0;

				gas = gas - error * curclutch * 10;
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

void CarDynamics::DoABS(int i)
{
	//only active if brakes commanded past threshold
	btScalar brakesetting = brake[i].GetBrakeFactor();
	if (brakesetting > btScalar(0.1))
	{
		btScalar maxspeed = 0;
		for (int i2 = 0; i2 < WHEEL_POSITION_SIZE; i2++)
		{
			if (wheel[WheelPosition(i2)].GetAngularVelocity() > maxspeed)
				maxspeed = wheel[WheelPosition(i2)].GetAngularVelocity();
		}

		//don't engage ABS if all wheels are moving slowly
		if (maxspeed > 6)
		{
			btScalar sp = tire[i].getIdealSlip();
			btScalar error = - tire[i].getSlip() - sp;
			btScalar thresholdeng = 0;
			btScalar thresholddis = -sp * btScalar(0.5);

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
		brake[i].SetBrakeFactor(0);
}

void CarDynamics::ComputeSuspensionDisplacement(int i, btScalar dt)
{
	//compute bump effect
	const TrackSurface & surface = wheel_contact[i].GetSurface();
	btScalar posx = wheel_contact[i].GetPosition()[0];
	btScalar posz = wheel_contact[i].GetPosition()[2];
	btScalar phase = btScalar(2 * M_PI) * (posx + posz) / surface.bumpWaveLength;
	btScalar shift = 2 * btSin(phase * btScalar(M_PI_2));
	btScalar amplitude = btScalar(0.25) * surface.bumpAmplitude;
	btScalar bumpoffset = amplitude * (btSin(phase + shift) + btSin(btScalar(M_PI_2) * phase) - 2);

	btScalar displacement_delta = 2 * wheel[i].GetRadius() - wheel_contact[i].GetDepth() + bumpoffset;
	suspension[i]->UpdateDisplacement(displacement_delta, dt);
}

void CarDynamics::ApplySuspensionForceToBody(int i, btScalar dt, btVector3 & force, btVector3 & torque)
{
	int j = (i == 0 || i == 2) ? i + 1: i - 1;
	btScalar roll_delta = suspension[i]->GetDisplacement() - suspension[j]->GetDisplacement();
	suspension[i]->UpdateForces(roll_delta, dt);
	btScalar suspension_force_magnitude = suspension[i]->GetForce();

	//find the vector direction to apply the suspension force
#ifdef SUSPENSION_FORCE_DIRECTION
	const btVector3 & wheelext = wheel[i].GetExtendedPosition();
	const btVector3 & hinge = suspension[i].GetHinge();
	btVector3 relwheelext = wheelext - hinge;
	btVector3 up(0, 0, 1);
	btVector3 rotaxis = up.cross(relwheelext.Normalize());
	btVector3 force_direction = relwheelext.Normalize().cross(rotaxis);
	//std::cout << i << ". " << forcedirection << std::endl;
#else
	btVector3 force_direction(Direction::up);
#endif
	force_direction = body->getCenterOfMassTransform().getBasis() * force_direction;
	btVector3 force_application_point = wheel_position[i] - body->getCenterOfMassPosition();

	btScalar overtravel = suspension[i]->GetOvertravel();
	if (overtravel > 0)
	{
		btScalar correction_factor = 0;
		btScalar dv = body->getVelocityInLocalPoint(force_application_point).dot(force_direction);
		dv -= correction_factor * overtravel / dt;
		btScalar effective_mass = 1 / body->computeImpulseDenominator(wheel_position[i], force_direction);
		btScalar overtravel_force = -effective_mass * dv / dt;
		if (overtravel_force > 0 && overtravel_force > suspension_force_magnitude)
			suspension_force_magnitude = overtravel_force;
	}

	btVector3 suspension_force = force_direction * suspension_force_magnitude;
	force = force + suspension_force;
	torque = torque + force_application_point.cross(suspension_force);
}

btVector3 CarDynamics::ComputeTireFrictionForce(
	int i, btScalar normal_force,
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
	btScalar lonvel = y.dot(linvel);
	btScalar latvel = -x.dot(linvel);

	btScalar friction_coeff =
		tire[i].getTread() * wheel_contact[i].GetSurface().frictionTread +
		(1 - tire[i].getTread()) * wheel_contact[i].GetSurface().frictionNonTread;

	btVector3 friction_force = tire[i].getForce(
		normal_force, friction_coeff, coszxw, rotvel, lonvel, latvel);

	return friction_force;
}

void CarDynamics::ApplyWheelForces(int i, btScalar dt, btScalar wheel_drive_torque, btVector3 & force, btVector3 & torque)
{
	btScalar wheel_force = suspension[i]->GetWheelForce();
	btScalar rotvel = wheel[i].GetAngularVelocity() * wheel[i].GetRadius();
	btVector3 friction_force = ComputeTireFrictionForce(i, wheel_force, rotvel, wheel_velocity[i], wheel_orientation[i]);

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

	// start accumulating forces and torques on the car body
	btVector3 force = ext_force;
	btVector3 torque = ext_torque;

	if (tcs)
	{
		for (int i = 0; i < WHEEL_POSITION_SIZE; ++i)
		{
			DoTCS(i);
		}
	}

	// compute wheel torques
	btScalar wheel_drive_torque[WHEEL_POSITION_SIZE];
	UpdateDriveline(wheel_drive_torque, dt);

	//ApplyEngineTorqueToBody(force, torque);

	ApplyAerodynamicsToBody(force, torque);

	for (int i = 0; i < WHEEL_POSITION_SIZE; ++i)
	{
		ComputeSuspensionDisplacement(i, dt);
	}

	for (int i = 0; i < WHEEL_POSITION_SIZE; ++i)
	{
		ApplySuspensionForceToBody(i, dt, force, torque);
	}

	if (abs)
	{
		for (int i = 0; i < WHEEL_POSITION_SIZE; ++i)
		{
			DoABS(i);
		}
	}

	for (int i = 0; i < WHEEL_POSITION_SIZE; ++i)
	{
		ApplyWheelForces(i, dt, wheel_drive_torque[i], force, torque);
	}

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
	btVector3 force = 1 / body->getInvMass() * dv / dt;
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
	const float tacho_factor = 0.1f;
	tacho_rpm = engine.GetRPM() * tacho_factor + tacho_rpm * (1 - tacho_factor);

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
	driveshaft_rpm = driveshaft_speed * btScalar(30 / M_PI);

	btScalar clutch_rpm = transmission.GetClutchSpeed(driveshaft_speed) * btScalar(30 / M_PI);

	if (autoshift)
	{
		int gear = NextGear(clutch_rpm);
		ShiftGear(gear);
	}

	remaining_shift_time = Max(remaining_shift_time - dt, 0.0f);
	if (remaining_shift_time <= transmission.GetShiftTime() *  btScalar(0.5) && !shifted)
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
		throttle = ShiftAutoClutchThrottle(throttle, clutch_rpm, dt);
		engine.SetThrottle(throttle);

		// allow auto clutch override
		if (clutch.GetPosition() >= clutch_value)
		{
			clutch_value = AutoClutch(clutch_rpm, dt);
			clutch.SetPosition(clutch_value);
		}
	}
}

bool CarDynamics::WheelDriven(int i) const
{
	return (1 << i) & drive;
}

btScalar CarDynamics::AutoClutch(btScalar clutch_rpm, btScalar dt) const
{
	btScalar clutch_engage_limit = 10 * dt; // 0.1 seconds
	btScalar clutch_old = clutch_value;
	btScalar clutch_new = 1;

	// antistall
	btScalar rpm_idle = engine.GetStartRPM();
	if (clutch_rpm < rpm_idle)
	{
		btScalar rpm_engine = engine.GetRPM();
		btScalar rpm_min = 0.5f * (engine.GetStallRPM() + rpm_idle);
		btScalar t = 1.5f - 0.5f * engine.GetThrottle();
		btScalar c = rpm_engine / (rpm_min * (1 - t) + rpm_idle * t) - 1;
		clutch_new = Clamp(c, btScalar(0), btScalar(1));
	}

	// shifting
	const btScalar shift_time = transmission.GetShiftTime();
	if (remaining_shift_time > shift_time * 0.5f)
	{
		clutch_new = 0;
	}
	else if (remaining_shift_time > 0)
	{
		clutch_new *= (1 - 2 * remaining_shift_time / shift_time);
	}

	// rate limit the autoclutch
	btScalar clutch_delta = clutch_new - clutch_old;
	clutch_delta = Clamp(clutch_delta, -clutch_engage_limit * 2, clutch_engage_limit);
	clutch_new = clutch_old + clutch_delta;

	return clutch_new;
}

btScalar CarDynamics::ShiftAutoClutchThrottle(btScalar throttle, btScalar clutch_rpm, btScalar dt)
{
	if (remaining_shift_time > 0)
	{
		if (engine.GetRPM() < clutch_rpm && engine.GetRPM() < engine.GetRedline())
		{
			remaining_shift_time += dt;
			return 1;
		}
		else
		{
			return throttle * btScalar(0.5);
		}
	}
	return throttle;
}

///return the gear change (0 for no change, -1 for shift down, 1 for shift up)
int CarDynamics::NextGear(btScalar clutch_rpm) const
{
	int gear = transmission.GetGear();

	// only autoshift if a shift is not in progress
	if (shifted && clutch.GetPosition() == 1)
	{
		// shift up when clutch speed exceeds engine redline
		// we do not shift up from neutral/reverse
		if (clutch_rpm > engine.GetRedline() && gear > 0)
		{
			return gear + 1;
		}
		// shift down when clutch speed below shift_down_point
		// we do not auto shift down from 1st gear to neutral
		if (clutch_rpm < DownshiftRPM(gear) && gear > 1)
		{
			return gear - 1;
		}
	}
	return gear;
}

btScalar CarDynamics::DownshiftRPM(int gear) const
{
	btScalar shift_down_point = 0;
	if (gear > 1)
	{
		btScalar current_gear_ratio = transmission.GetGearRatio(gear);
		btScalar lower_gear_ratio = transmission.GetGearRatio(gear - 1);
		btScalar peak_engine_speed = engine.GetRedline();
		shift_down_point = btScalar(0.7) * peak_engine_speed / lower_gear_ratio * current_gear_ratio;
	}
	return shift_down_point;
}

btScalar CarDynamics::CalculateMaxSpeed() const
{
	// speed limit due to engine and transmission
	btScalar ratio = transmission.GetGearRatio(transmission.GetForwardGears());
	btScalar drive_speed = engine.GetRPMLimit() * btScalar(M_PI / 30);
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
	btScalar aero_speed = btPow(engine.GetMaxPower() / aero_drag_coeff, 1 / 3.0f);

	return Min(drive_speed, aero_speed);
}

btScalar CarDynamics::CalculateFrontMassRatio() const
{
	btScalar dr = GetCenterOfMassOffset()[1];
	btScalar r1 = suspension[0]->GetWheelPosition()[1] + dr;
	btScalar r2 = suspension[3]->GetWheelPosition()[1] + dr;
	return r2 / (r2 - r1);
}

void CarDynamics::CalculateAerodynamicCoeffs(btScalar & cl, btScalar & cd) const
{
	btScalar lift = 0;
	btScalar drag = 0;
	for (int i = 0; i < aerodevice.size(); i++)
	{
		lift += aerodevice[i].getLiftCoefficient();
		drag += aerodevice[i].getDragCoefficient();
	}
	cl = lift;
	cd = drag;
}

void CarDynamics::CalculateFrictionCoeffs(btScalar & mulon, btScalar & mulat) const
{
	btScalar mg = gravity / GetInvMass();
	btScalar tire_load = 0.25f * mg;
	btScalar lon_friction = 0;
	btScalar lat_friction = 0;
	for (int i = 0; i < 4; i++)
	{
		lon_friction += tire[i].getMaxFx(tire_load);
		lat_friction += tire[i].getMaxFy(tire_load, 0);
	}
	mulon = lon_friction / mg;
	mulat = lat_friction / mg;
}

void CarDynamics::SetSteering(btScalar value)
{
	if (steering_assist)
	{
		btScalar ideal_angle = tire[0].getIdealSlipAngle();
		btScalar max_angle = suspension[0]->GetMaxSteeringAngle();
		btScalar ideal_value = btScalar(180/M_PI) * ideal_angle / max_angle;

		// transit to ideal value at 10 m/s (36km/h)
		btScalar transition = Min(linear_velocity.length2() * 0.01f, 1.0f);
		btScalar abs_value = std::abs(value);
		btScalar max_value = abs_value + (ideal_value - abs_value) * transition;

		value = Clamp(value, -max_value, max_value);
	}

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
	if (std::abs(angle) < btScalar(M_PI / 4)) return;

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
	aero_lift_coeff = 0.1;
	aero_drag_coeff = 0.3;
	lon_friction_coeff = 1;
	lat_friction_coeff = 1;
	maxangle = 0;
	maxspeed = 0;
	feedback_scale = 0;
	feedback = 0;
	brake_value = 0;
	clutch_value = 1;
	remaining_shift_time = 0;
	shift_gear = 0;
	shifted = true;
	steering_assist = false;
	autoreverse = false;
	autoclutch = true;
	autoshift = false;
	abs = false;
	tcs = false;

	suspension.resize(WHEEL_POSITION_SIZE, 0);
	wheel.resize(WHEEL_POSITION_SIZE);
	tire.resize(WHEEL_POSITION_SIZE);
	brake.resize(WHEEL_POSITION_SIZE);
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
		if (-Direction::up.dot(contact_point) > btScalar(0.5) * wheel_shape->getRadius())
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
		if (d2 < n2 * btScalar(0.97))
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
