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
#include "fastmath.h"
#include "minmax.h"

#include "BulletCollision/CollisionShapes/btCompoundShape.h"
#include "BulletCollision/CollisionShapes/btCylinderShape.h"
#include "BulletCollision/CollisionShapes/btTriangleShape.h"

#include <cmath>

static const btScalar gravity = 9.81;
static const int substeps = 10;
static const btScalar rsubsteps = 1.0/substeps;

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
	const PTree * cfg_clutch;
	if (!cfg.get("clutch", cfg_clutch, error_output)) return false;

	CarClutchInfo info;
	cfg_clutch->get("sliding", info.friction, error_output);
	cfg_clutch->get("max-pressure", info.max_pressure, error_output);
	cfg_clutch->get("radius", info.radius, error_output);
	cfg_clutch->get("area", info.area, error_output);

	clutch.Init(info);
	return true;
}

static bool LoadTransmission(
	const PTree & cfg,
	CarTransmission & transmission,
	std::ostream & error_output)
{
	const PTree * cfg_trans = 0;
	int gears = 0;
	if (!cfg.get("transmission", cfg_trans, error_output)) return false;
	if (!cfg_trans->get("gears", gears, error_output)) return false;

	std::vector<btScalar> ratios(2 + gears, 0);
	if (!cfg_trans->get("gear-ratio-r", ratios[0], error_output)) return false;
	for (int i = 0; i < gears; ++i)
	{
		std::ostringstream s;
		s << "gear-ratio-" << i + 1;
		if (!cfg_trans->get(s.str(), ratios[i + 2], error_output)) return false;
	}
	transmission.SetGears(ratios, gears, 1);

	btScalar shift_time = 0;
	cfg_trans->get("shift-time", shift_time);
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
	btVector3 position;

	const PTree * cfg_fuel;
	if (!cfg.get("fuel-tank", cfg_fuel, error_output)) return false;
	if (!cfg_fuel->get("capacity", capacity, error_output)) return false;
	if (!cfg_fuel->get("volume", volume, error_output)) return false;
	if (!cfg_fuel->get("fuel-density", fuel_density, error_output)) return false;
	if (!cfg_fuel->get("position", position, error_output)) return false;

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
	CarBrakeInfo info;
	cfg.get("friction", info.friction, error_output);
	cfg.get("max-pressure", info.max_pressure, error_output);
	cfg.get("radius", info.radius, error_output);
	cfg.get("area", info.area, error_output);
	cfg.get("bias", info.brake_bias, error_output);
	cfg.get("handbrake", info.handbrake_bias);

	brake.Init(info);
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

	tire.radius = radius;
	tire.width = width;
	tire.ar = aspect_ratio;

	if (!cfg.get("pt", tire.pt, error_output)) return false;
	if (!cfg.get("ktx", tire.ktx, error_output)) return false;
	if (!cfg.get("kty", tire.kty, error_output)) return false;
	if (!cfg.get("kcb", tire.kcb, error_output)) return false;
	if (!cfg.get("ccb", tire.ccb, error_output)) return false;
	if (!cfg.get("cfy", tire.cfy, error_output)) return false;
	if (!cfg.get("dz0", tire.dz0, error_output)) return false;
	if (!cfg.get("p0", tire.p0, error_output)) return false;
	if (!cfg.get("mus", tire.mus, error_output)) return false;
	if (!cfg.get("muc", tire.muc, error_output)) return false;
	if (!cfg.get("vs", tire.vs, error_output)) return false;
	if (!cfg.get("cr0", tire.cr0, error_output)) return false;
	if (!cfg.get("cr2", tire.cr2, error_output)) return false;
	if (!cfg.get("tread", tire.tread, error_output)) return false;

	tire.init();
	return true;
}
#elif defined(VDRIFTN)
static bool LoadTire(const PTree & cfg_wheel, const PTree & cfg, CarTire & tire, std::ostream & error_output)
{
	if (!cfg.get("tread", tire.tread, error_output)) return false;

	btVector3 roll_resistance;
	if (!cfg.get("rolling-resistance", roll_resistance, error_output)) return false;
	tire.roll_resistance_lin = roll_resistance[0];
	tire.roll_resistance_quad = roll_resistance[1];

	if (!cfg.get("FZ0", tire.nominal_load, error_output)) return false;
	for (int i = 0; i < CarTire::CNUM; ++i)
	{
		if (!cfg.get(tire.coeffname[i], tire.coefficients[i], error_output))
			return false;
	}

	// asymmetric tires support (left right facing direction)
	// default facing direction is right
	// symmetric tire has side factor zero
	btScalar side_factor = 0;
	std::string facing;
	if (cfg_wheel.get("tire.facing", facing))
		side_factor = (facing != "left") ? 1 : -1;
	tire.coefficients[CarTire::PEY3] *= side_factor;
	tire.coefficients[CarTire::PEY4] *= side_factor;
	tire.coefficients[CarTire::PVY1] *= side_factor;
	tire.coefficients[CarTire::PVY2] *= side_factor;
	tire.coefficients[CarTire::PHY1] *= side_factor;
	tire.coefficients[CarTire::PHY2] *= side_factor;
	tire.coefficients[CarTire::PHY3] *= side_factor;
	tire.coefficients[CarTire::RBY3] *= side_factor;
	tire.coefficients[CarTire::RHX1] *= side_factor;
	tire.coefficients[CarTire::RHY1] *= side_factor;
	tire.coefficients[CarTire::RVY5] *= side_factor;

	btScalar size_factor = 1;
	btVector3 size;
	if (cfg_wheel.get("tire.size", size))
		size_factor = ComputeFrictionFactor(cfg, size);
	tire.coefficients[CarTire::PDX1] *= size_factor;
	tire.coefficients[CarTire::PDY1] *= size_factor;

	return true;
}
#else
static bool LoadTire(const PTree & cfg_wheel, const PTree & cfg, CarTire & tire, std::ostream & error_output)
{
	if (!cfg.get("tread", tire.tread, error_output)) return false;

	btVector3 rolling_resistance;
	if (!cfg.get("rolling-resistance", rolling_resistance, error_output)) return false;
	tire.rolling_resistance_lin = rolling_resistance[0];
	tire.rolling_resistance_quad = rolling_resistance[1];

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
		if (!cfg.get(s.str(), tire.lateral[i], error_output)) return false;
	}

	// read longitudinal
	for (int i = 0; i < 11; i++)
	{
		std::ostringstream s;
		s << "b" << i;
		if (!cfg.get(s.str(), tire.longitudinal[i], error_output)) return false;
	}

	// read aligning
	for (int i = 0; i < 18; i++)
	{
		std::ostringstream s;
		s << "c" << i;
		if (!cfg.get(s.str(), tire.aligning[i], error_output)) return false;
	}

	// read combining
	if (!cfg.get("gy1", tire.combining[0], error_output)) return false;
	if (!cfg.get("gy2", tire.combining[1], error_output)) return false;
	if (!cfg.get("gx1", tire.combining[2], error_output)) return false;
	if (!cfg.get("gx2", tire.combining[3], error_output)) return false;

	// asymmetric tires support (left right facing direction)
	// default facing direction is right
	// fixme: should handle aligning torque too?
	btScalar side_factor = 0;
	std::string facing;
	if (cfg_wheel.get("tire.facing", facing))
		side_factor = (facing != "left") ? 1 : -1;
	tire.lateral[13] *= side_factor;
	tire.lateral[14] *= side_factor;

	btScalar size_factor = 1;
	btVector3 size;
	if (cfg_wheel.get("tire.size", size))
		size_factor = ComputeFrictionFactor(cfg, size);
	tire.longitudinal[2] *= size_factor;
	tire.lateral[2] *= size_factor;

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
	btScalar tire_thickness = 0.02;
	btScalar tire_density = 1000; // rubber

	btScalar rim_radius = tire_radius - tire_width * tire_aspect_ratio;
	btScalar rim_width = tire_width;
	btScalar rim_thickness = 0.01;
	btScalar rim_density = 2700; // aluminium

	btScalar tire_volume = btScalar(M_PI) * tire_width * tire_thickness * (2 * tire_radius  - tire_thickness);
	btScalar rim_volume = btScalar(M_PI) * rim_width * rim_thickness * (2 * rim_radius - rim_thickness);
	btScalar tire_mass = tire_density * tire_volume;
	btScalar rim_mass = rim_density * rim_volume;
	btScalar tire_inertia = tire_mass * tire_radius * tire_radius;
	btScalar rim_inertia = rim_mass * rim_radius * rim_radius;

	btScalar mass = tire_mass + rim_mass;
	btScalar inertia = tire_inertia + rim_inertia;

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
	CarDifferential & d,
	std::ostream & error_output)
{
	cfg.get("final-drive", d.final_drive, error_output);
	cfg.get("anti-slip", d.anti_slip, error_output);
	cfg.get("anti-slip-torque", d.anti_slip_torque);
	cfg.get("anti-slip-torque-deceleration-factor", d.anti_slip_torque_deceleration_factor);
	cfg.get("torque-split", d.torque_split);
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

	void operator()(FractureBody::Connection & /*connection*/) override
	{
		int last = aerodevice.size() - 1;
		if (id < last)
		{
			aerodevice.swap(id, last);
			auto ad = static_cast<AeroDeviceFracture*>(aerodevice[id].GetUserPointer());
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
		if (!cfg.get("drag-coefficient", drag_coefficient))
			return true;

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
			auto adf = new AeroDeviceFracture(aerodevice, id);
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
		if (!LoadDifferential(*cfg_diff, differential[DIFF_FRONT], error)) return false;
		drive = FWD;
	}
	if (cfg.get("differential-rear", cfg_diff))
	{
		if (!LoadDifferential(*cfg_diff, differential[DIFF_REAR], error)) return false;
		drive = (drive == FWD) ? AWD : RWD;
	}
	if (cfg.get("differential-center", cfg_diff) && drive == AWD)
	{
		if (!LoadDifferential(*cfg_diff, differential[DIFF_CENTER], error)) return false;
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
	if (wheel_count != WHEEL_COUNT)
	{
		error << "Wheels loaded: " << wheel_count << ". Required: " << WHEEL_COUNT << std::endl;
		return false;
	}

	int i = 0;
	for (const auto & node : *cfg_wheels)
	{
		const auto & cfg_wheel = node.second;
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
		tire[i].initSlipLUT(tire_slip_lut[i]);

		const PTree * cfg_brake;
		if (!cfg_wheel.get("brake", cfg_brake, error)) return false;
		if (!LoadBrake(*cfg_brake, brake[i], error)) return false;

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

	// get suspension load at rest
	btVector3 f(0, 0, 0), r(0, 0, 0);
	(cfg_wheels->begin())->second.get("position", f, error);
	(--cfg_wheels->end())->second.get("position", r, error);
	btScalar half_car_load = loadBody.info.m_mass * (gravity * 0.5f);
	btScalar dr = -loadBody.info.m_massCenter[1] / loadBody.info.m_mass;
	btScalar front_mass_ratio = (r[1] + dr) / (r[1] - f[1]);
	btScalar fwl = front_mass_ratio * half_car_load;
	btScalar rwl = half_car_load - fwl;
	btScalar wheel_load[WHEEL_COUNT] = {fwl, fwl, rwl, rwl};

	// load suspension
	i = 0;
	for (const auto & node : *cfg_wheels)
	{
		if (!CarSuspension::Load( node.second, wheel[i].GetMass(), wheel_load[i], suspension[i], error))
			return false;
		if (suspension[i].GetMaxSteeringAngle() > maxangle)
			maxangle = suspension[i].GetMaxSteeringAngle();
		i++;
	}

	if (drive == AWD)
		InitDriveline4(world.getTimeStep() * rsubsteps);
	else
		InitDriveline2(world.getTimeStep() * rsubsteps);

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
	btVector3 fwd = body->getCenterOfMassTransform().getBasis().getColumn(Direction::FORWARD);
	btVector3 up = body->getCenterOfMassTransform().getBasis().getColumn(Direction::UP);
	btVector3 fwd_offset = fwd * (2 - bmax.y());
	btVector3 up_offset = -up * (btScalar(0.5) + bmin.z());
	SetPosition(body->getCenterOfMassPosition() + up_offset + fwd_offset);
	UpdateWheelTransform();

	// update motion state
	body->getMotionState()->setWorldTransform(body->getWorldTransform());

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
	for (int i = 0; i < WHEEL_COUNT; ++i)
		wheel_position[i] = transform.getBasis() * (suspension[i].GetWheelPosition() + GetCenterOfMassOffset());
}

void CarDynamics::AlignWithGround()
{
	UpdateWheelContacts();

	btScalar min_height = 0;
	bool no_min_height = true;
	for (int i = 0; i < WHEEL_COUNT; ++i)
	{
		btScalar height = wheel_contact[i].GetDepth() - 2 * wheel[i].GetRadius();
		if (height < min_height || no_min_height)
		{
			min_height = height;
			no_min_height = false;
		}
	}

	btVector3 delta = GetDownVector() * min_height;
	btVector3 position = transform.getOrigin() + delta;
	SetPosition(position);

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
	return GetOrientation(0) * suspension[wp].GetWheelOrientation();
}

unsigned CarDynamics::GetNumBodies() const
{
	return motion_state.size();
}

const btVector3 & CarDynamics::GetPosition(int i) const
{
	assert(i < motion_state.size());
	return motion_state[i].position;
}

const btQuaternion & CarDynamics::GetOrientation(int i) const
{
	assert(i < motion_state.size());
	return motion_state[i].rotation;
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

btVector3 CarDynamics::GetVelocity(const btVector3 & pos) const
{
	return body->getVelocityInLocalPoint(pos - body->getCenterOfMassPosition());
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

btScalar CarDynamics::GetTireSqueal(WheelPosition i) const
{
	btScalar v0 = wheel_velocity[i][0] - wheel_velocity[i][2];
	btScalar v1 = wheel_velocity[i][1];
	btScalar vq = Min(v0 * v0 + v1 * v1, btScalar(1));

	auto & t = tire_state[i];
	btScalar sr = t.slip / t.ideal_slip;
	btScalar ar = t.slip_angle / t.ideal_slip_angle;
	btScalar sq = sr * sr + ar * ar;

	return Clamp(sq * vq - btScalar(0.4), btScalar(0), btScalar(1));
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
	btScalar vi2 = initial_speed * initial_speed;
	btScalar vf2 = final_speed * final_speed;
	btScalar d = (-aero_lift_coeff * mu + aero_drag_coeff) * GetInvMass();
	btScalar e = 1 / d;
	btScalar f = e * mu * gravity;
	btScalar g = (f + vi2) / (f + vf2);
	btScalar distance = 0.5f * e * std::log(g);
	//btScalar distance = (v1s - v2s) / (mu * gravity * 2);
	return distance;
}

std::vector<float> CarDynamics::GetSpecs() const
{
	return std::vector<float>{
		maxspeed,
		float(int(drive)),
		engine.GetDisplacement(),
		engine.GetMaxPower(),
		engine.GetMaxTorque(),
		1 / body->getInvMass(),
		CalculateFrontMassRatio(),
		CalculateGripBalance()
	};
}

btVector3 CarDynamics::GetDownVector() const
{
	return -body->getCenterOfMassTransform().getBasis().getColumn(Direction::UP);
}

const btVector3 & CarDynamics::GetCenterOfMassOffset() const
{
	return motion_state[0].massCenterOffset;
}

btVector3 CarDynamics::LocalToWorld(const btVector3 & local) const
{
	return body->getCenterOfMassTransform() * (GetCenterOfMassOffset() + local);
}

void CarDynamics::UpdateWheelTransform()
{
	for (int i = 0; i < WHEEL_COUNT; ++i)
	{
		if (body->getChildBody(i)->isInWorld()) continue;

		btQuaternion r = suspension[i].GetWheelOrientation();
		r *= btQuaternion(Direction::right, -wheel[i].GetRotation());
		btVector3 p = suspension[i].GetWheelPosition() + GetCenterOfMassOffset();
		body->setChildTransform(i, btTransform(r, p));
	}
}

void CarDynamics::ApplyAerodynamics(btScalar dt)
{
	auto & m = body->getCenterOfMassTransform().getBasis();
	btVector3 air_velocity = -body->getLinearVelocity() * m;
	btVector3 wind_force(0, 0, 0);
	btVector3 wind_torque(0, 0, 0);
	for (int i = 0; i < aerodevice.size(); ++i)
	{
		btVector3 force = aerodevice[i].getForce(air_velocity);
		wind_force = wind_force + force;
		wind_torque = wind_torque + (aerodevice[i].getPosition() + GetCenterOfMassOffset()).cross(force);
	}
	wind_force = m * wind_force;
	wind_torque = m * wind_torque;
	body->applyCentralImpulse(wind_force * dt);
	body->applyTorqueImpulse(wind_torque * dt);
}

void CarDynamics::ApplyTCS(int i)
{
	// only active if throttle commanded past threshold
	btScalar gas = engine.GetThrottle();
	if (gas > btScalar(0.1))
	{
		auto & t = tire_state[i];
		btScalar slip_angle = t.slip_angle;
		btScalar slip_ideal = t.ideal_slip_angle * CosPi2(slip_angle);
		btScalar slip = std::abs(t.slip);
		btScalar slip_engage = slip_ideal * btScalar(1.2);
		btScalar slip_disengage = slip_ideal * btScalar(0.8);

		if (slip > slip_engage)
			tcs_active[i] = true;
		else if (slip < slip_disengage)
			tcs_active[i] = false;

		if (tcs_active[i])
		{
			btScalar error = slip - slip_disengage;
			gas = gas - error * 2 * clutch.GetPosition();
			gas = Clamp(gas, btScalar(0), btScalar(1));
			engine.SetThrottle(gas);
		}
	}
	else
		tcs_active[i] = false;
}

void CarDynamics::ApplyABS(int i)
{
	// only active if speed exceeds 4 m/s
	btScalar slip = tire_state[i].slip;
	if (brake_value > 0.01f && body->getLinearVelocity().length2() > 16)
	{
		// limit brake value based on normalized predicted slip
		btScalar slip_delta = slip - wheel_slip[i];
		btScalar slip_predicted = slip + slip_delta;
		btScalar slip_ideal = tire_state[i].ideal_slip;
		btScalar sr = std::abs(slip_predicted) / slip_ideal;
		btScalar brake_delta_limit = (1 - sr) * 2;

		btScalar brake_old = brake[i].GetBrakeFactor();
		btScalar brake_delta = Min(brake_delta_limit, brake_value - brake_old);
		btScalar brake_new = Clamp(brake_old + brake_delta, btScalar(0), btScalar(1));
		brake[i].SetBrakeFactor(brake_new);
		abs_active[i] = brake_new < brake_value ? true : false;
	}
	else
	{
		brake[i].SetBrakeFactor(brake_value);
		abs_active[i] = false;
	}
	wheel_slip[i] = slip;
}

// even triangle wave with a period and amplitude of 1
inline btScalar TriangleWave(btScalar x)
{
	return 4 * std::abs(std::abs(x - (int)x) - btScalar(0.5)) - 1;
}

// simple cosine cubic approximation with a period of 1
inline btScalar SmoothWave(btScalar x)
{
	x = TriangleWave(x);
	return (btScalar(1.5) - btScalar(0.5) * (x * x)) * x;
}

void CarDynamics::UpdateSuspension(int i, btScalar dt)
{
	auto surface = wheel_contact[i].GetSurface();
	btScalar bump = 0;
	if (surface.bumpAmplitude > 0)
	{
		btScalar frequency = 1 / surface.bumpWaveLength;
		btScalar nx = wheel_contact[i].GetPosition()[0] * frequency;
		btScalar ny = wheel_contact[i].GetPosition()[1] * frequency;
		btScalar sx = SmoothWave(nx);
		btScalar sy = SmoothWave(ny);
		bump = btScalar(0.5) * surface.bumpAmplitude * sx * sy;
	}
	btScalar displacement_delta = 2 * wheel[i].GetRadius() - wheel_contact[i].GetDepth() + bump;
	suspension[i].UpdateDisplacement(displacement_delta, dt);
}

// executed as last function(after integration) in bullet singlestepsimulation
void CarDynamics::updateAction(btCollisionWorld * /*collisionWorld*/, btScalar dt)
{
	// reset body transform
	body->setCenterOfMassTransform(transform);

	if (tcs)
	{
		for (int i = 0; i < WHEEL_COUNT; ++i)
			ApplyTCS(i);
	}

	if (abs)
	{
		for (int i = 0; i < WHEEL_COUNT; ++i)
			ApplyABS(i);
	}
	else
	{
		for (int i = 0; i < WHEEL_COUNT; ++i)
			brake[i].SetBrakeFactor(brake_value);
	}

	UpdateTransmission(dt);

	engine.Update(dt);

	ApplyAerodynamics(dt);

	UpdateDriveline(dt);

	fuel_tank.Consume(engine.FuelRate() * dt);
	engine.SetOutOfGas(fuel_tank.Empty());

	const btScalar tacho_factor = 0.1;
	tacho_rpm += (engine.GetRPM() - tacho_rpm) * tacho_factor;

	// update steering feedback
	feedback = feedback_scale * (tire_state[FRONT_LEFT].mz + tire_state[FRONT_RIGHT].mz);

	// update body transform
	body->predictIntegratedTransform(dt, transform);
	body->setCenterOfMassTransform(transform);
	UpdateWheelTransform();
}

void CarDynamics::UpdateWheelContacts()
{
	btVector3 raydir = GetDownVector();
	btScalar raylen = 4;
	for (int i = 0; i < WHEEL_COUNT; ++i)
	{
		btVector3 raystart = body->getCenterOfMassPosition() + wheel_position[i] - raydir * wheel[i].GetRadius();
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

void CarDynamics::InitDriveline2(btScalar dt)
{
	driveline.shaft[0] = &engine.GetShaft();
	driveline.shaft[1] = &wheel[(drive == FWD) ? FRONT_LEFT : REAR_LEFT].GetShaft();
	driveline.shaft[2] = &wheel[(drive == FWD) ? FRONT_RIGHT : REAR_RIGHT].GetShaft();

	auto & diff = differential[(drive == FWD) ? DIFF_FRONT : DIFF_REAR];
	driveline.clutch[1].softness = 12 / (diff.anti_slip * dt); // max torque at 12 rad/s (~2 rps) slip
	driveline.clutch[1].load_coeff = diff.anti_slip_torque;
	driveline.clutch[1].decel_factor = diff.anti_slip_torque_deceleration_factor;
	driveline.clutch[1].impulse_limit = diff.anti_slip * dt;

	driveline.clutch_count = 2;
	driveline.computeDiffInertia2();
}

void CarDynamics::InitDriveline4(btScalar dt)
{
	driveline.shaft[0] = &engine.GetShaft();
	driveline.shaft[1] = &wheel[FRONT_LEFT].GetShaft();
	driveline.shaft[2] = &wheel[FRONT_RIGHT].GetShaft();
	driveline.shaft[3] = &wheel[REAR_LEFT].GetShaft();
	driveline.shaft[4] = &wheel[REAR_RIGHT].GetShaft();

	const DiffEnum n[] = { DIFF_COUNT, DIFF_CENTER, DIFF_FRONT, DIFF_REAR };
	for (unsigned i = 1; i <= DIFF_COUNT; ++i)
	{
		auto & diff = differential[n[i]];
		driveline.clutch[i].softness = 12 / (diff.anti_slip * dt); // max torque at 12 rad/s (~2 rps) slip
		driveline.clutch[i].load_coeff = diff.anti_slip_torque;
		driveline.clutch[i].decel_factor = diff.anti_slip_torque_deceleration_factor;
		driveline.clutch[i].impulse_limit = diff.anti_slip * dt;
	}

	driveline.clutch_count = 4;
	driveline.torque_split = differential[DIFF_CENTER].torque_split;
	driveline.computeDiffInertia4();
}

void CarDynamics::UpdateDrivelineGearRatio()
{
	if (drive != AWD)
	{
		driveline.gear_ratio = transmission.GetCurrentGearRatio() *
			differential[(drive == FWD) ? DIFF_FRONT : DIFF_REAR].final_drive;
		driveline.computeDriveInertia2();
	}
	else
	{
		// assume rear final drive is equal to front final dirve
		driveline.gear_ratio = transmission.GetCurrentGearRatio() *
			differential[DIFF_CENTER].final_drive *
			differential[DIFF_REAR].final_drive;
		driveline.computeDriveInertia4();
	}
}

void CarDynamics::SetupDriveline(const btMatrix3x3 wheel_orientation[WHEEL_COUNT], btScalar dt)
{
	auto & c = driveline.clutch[0];
	c.impulse_limit_delta = (clutch.GetTorque() * dt - c.impulse_limit) * rsubsteps;

	auto & m = driveline.motor[0];
	m.shaft = &engine.GetShaft();
	m.target_velocity = engine.GetTorque() > 0 ? 100000 : 0;
	m.impulse_limit_delta = (std::abs(engine.GetTorque()) * dt - m.impulse_limit) * rsubsteps;
	m.computeInertia(*body, body->getWorldTransform().getBasis().getColumn(Direction::RIGHT));

	driveline.motor_count = 1;
	for (int i = 0; i < WHEEL_COUNT; ++i)
	{
		auto & m = driveline.motor[driveline.motor_count++];
		m.shaft = &wheel[i].GetShaft();
		m.target_velocity = 0;
		m.impulse_limit_delta = (brake[i].GetTorque() * dt - m.impulse_limit) * rsubsteps;
		m.computeInertia(*body, wheel_orientation[i].getColumn(Direction::RIGHT));
	}
}

void CarDynamics::SetupWheelConstraints(const btMatrix3x3 wheel_orientation[WHEEL_COUNT], btScalar dt)
{
	btScalar antiroll[WHEEL_COUNT];
	for (int i = 0; i < WHEEL_COUNT; i+=2)
	{
		// calculate anti-roll contribution to stiffness
		auto & s0 = suspension[i];
		auto & s1 = suspension[i+1];
		btScalar d0 = s0.GetDisplacement();
		btScalar d1 = s1.GetDisplacement();
		btScalar d = d0 - d1;
		btScalar k = s0.GetAntiRoll();
		btScalar k0 = (d0 > 0) ? k * d / d0 : 0;
		btScalar k1 = (d1 > 0) ? -k * d / d1 : 0;

		// avoid negative stiffness
		k0 = Max(k0, 1E-3f - s0.GetStiffness());
		k1 = Max(k1, 1E-3f - s1.GetStiffness());
		antiroll[i] = k0;
		antiroll[i+1] = k1;
	}

	for (int i = 0; i < WHEEL_COUNT; ++i)
	{
		auto & s = suspension[i];
		btScalar displacement = s.GetDisplacement();
		btScalar overtravel = s.GetOvertravel();
		btScalar stiffness = s.GetStiffness() + antiroll[i];
		btScalar damping = s.GetDamping();
		if (overtravel > 0)
		{
			// combine spring and bump stiffness
			btScalar bump_stiffness = 5E5;
			displacement += overtravel;
			stiffness += bump_stiffness * overtravel / displacement;
		}

		btVector3 xw = wheel_orientation[i].getColumn(Direction::RIGHT);
		btVector3 yw = wheel_orientation[i].getColumn(Direction::FORWARD);
		btVector3 z = wheel_contact[i].GetNormal();
		btScalar coszxw = z.dot(xw);
		btScalar coszyw = z.dot(yw);
		btVector3 x = (xw - z * coszxw).normalized();
		btVector3 y = (yw - z * coszyw).normalized();
		btScalar friction = tire[i].getTread() * wheel_contact[i].GetSurface().frictionTread +
			(1 - tire[i].getTread()) * wheel_contact[i].GetSurface().frictionNonTread;

		auto & w = wheel_constraint[i];
		w.body = body;
		w.shaft = &wheel[i].GetShaft();
		w.position = wheel_position[i];
		w.radius = wheel[i].GetRadius();
		w.constraint[0].axis = y;
		w.constraint[1].axis = x;
		w.constraint[2].axis = z;
		w.init(stiffness, damping, displacement, dt);

		auto & t = tire_state[i];
		t.friction = friction;
#if defined(VDRIFTP)
		t.camber = coszxw;
#else
		t.camber = ComputeCamberAngle(coszxw);
#endif
	}
}

void CarDynamics::ApplyWheelContactDrag(btScalar dt)
{
	btVector3 total_impulse(0, 0, 0);
	btVector3 total_torque_impulse(0, 0, 0);
	for (int i = 0; i < WHEEL_COUNT; ++i)
	{
		if (suspension[i].GetDisplacement() > 0)
		{
			btVector3 velocity = body->getVelocityInLocalPoint(wheel_position[i]);
			velocity -= velocity.dot(wheel_contact[i].GetNormal()) * wheel_contact[i].GetNormal();
			btVector3 impulse = -velocity * (wheel_contact[i].GetSurface().rollingDrag * dt);
			total_impulse = total_impulse + impulse;
			total_torque_impulse = total_torque_impulse + wheel_position[i].cross(impulse);
		}
	}

	btScalar vmag = total_impulse.length2() * (body->getInvMass() * body->getInvMass());
	btScalar vlimit = body->getLinearVelocity().length2();
	if (vmag > vlimit)
	{
		btScalar limit = btSqrt(vlimit / vmag);
		total_impulse = total_impulse * limit;
		total_torque_impulse = total_torque_impulse * limit;
	}

	body->applyCentralImpulse(total_impulse);
	body->applyTorqueImpulse(total_torque_impulse);
}

void CarDynamics::ApplyRollingResistance(int i)
{
	auto & shaft = wheel[i].GetShaft();
	btScalar vr = wheel[i].GetAngularVelocity() * wheel[i].GetRadius();
	btScalar cr = tire[i].getRollingResistance(vr, wheel_contact[i].GetSurface().rollResistanceCoefficient);
	btScalar suspension_impulse = wheel_constraint[i].constraint[2].impulse;
	btScalar impulse_mag = cr * suspension_impulse * wheel[i].GetRadius();
	btScalar impulse_limit = std::abs(shaft.ang_velocity) * shaft.inertia;
	btScalar impulse = std::copysign(Min(impulse_mag, impulse_limit), -shaft.ang_velocity);
	shaft.applyImpulse(impulse);
}

void CarDynamics::UpdateWheelConstraints(btScalar rdt, btScalar sdt)
{
	for (int i = 0; i < WHEEL_COUNT; ++i)
	{
		auto & c = wheel_constraint[i];
		auto & t = tire_state[i];
		btScalar v[3];
		c.getContactVelocity(v);
		btScalar suspension_force = c.constraint[2].impulse * rdt;
		tire[i].ComputeState(suspension_force, v[2], v[0], v[1], t);
		c.vcam = t.vcam;
		c.constraint[0].upper_impulse_limit = Max(t.fx * sdt, btScalar(0));
		c.constraint[0].lower_impulse_limit = Min(t.fx * sdt, btScalar(0));
		c.constraint[0].impulse = 0;
		c.constraint[1].upper_impulse_limit = Max(t.fy * sdt, btScalar(0));
		c.constraint[1].lower_impulse_limit = Min(t.fy * sdt, btScalar(0));
		c.constraint[1].impulse = 0;
	}
}

void CarDynamics::UpdateDriveline(btScalar dt)
{
	const int solver_iterations = 4;
	const btScalar rdt = 1 / dt;
	const btScalar sdt = dt * rsubsteps;

	UpdateWheelContacts();
	btMatrix3x3 wheel_orientation[WHEEL_COUNT];
	for (int i = 0; i < WHEEL_COUNT; ++i)
	{
		UpdateSuspension(i, dt);
		wheel_orientation[i] = transform.getBasis() * btMatrix3x3(suspension[i].GetWheelOrientation());
		wheel_position[i] = transform.getBasis() * (suspension[i].GetWheelPosition() + GetCenterOfMassOffset());
	}

	SetupWheelConstraints(wheel_orientation, dt);

	// presolve suspension
	for (int n = 0; n < solver_iterations; ++n)
	{
		for (int i = 0; i < WHEEL_COUNT; ++i)
			wheel_constraint[i].solveSuspension();
	}

	ApplyWheelContactDrag(dt);
	for (int i = 0; i < WHEEL_COUNT; ++i)
		ApplyRollingResistance(i);

	SetupDriveline(wheel_orientation, sdt);

	// solve driveline
	for (int n = 0; n < substeps; ++n)
	{
		UpdateWheelConstraints(rdt, sdt);

		driveline.clearImpulses();
		driveline.updateImpulseLimits();
		for (int m = 0; m < solver_iterations; ++m)
		{
			if (drive != AWD)
				driveline.solve2(*body);
			else
				driveline.solve4(*body);

			for (int i = 0; i < WHEEL_COUNT; ++i)
				wheel_constraint[i].solveFriction();
		}

		for (int i = 0; i < WHEEL_COUNT; ++i)
			wheel_constraint[i].solveSuspension();
	}

	// update wheel and tire state
	for (int i = 0; i < WHEEL_COUNT; ++i)
	{
		auto & c = wheel_constraint[i];
		auto & t = tire_state[i];
		c.getContactVelocity(wheel_velocity[i]);
		btScalar fz = c.constraint[2].impulse * rdt;
		tire_slip_lut[i].get(fz, t.ideal_slip, t.ideal_slip_angle);
		tire[i].ComputeAligningTorque(fz, t);
		wheel[i].Integrate(dt);
	}
}

void CarDynamics::UpdateTransmission(btScalar dt)
{
	btScalar driveshaft_speed = 0;
	if (drive & FWD)
	{
		driveshaft_speed += 0.5f * differential[DIFF_FRONT].final_drive *
			(wheel[FRONT_LEFT].GetAngularVelocity() + wheel[FRONT_RIGHT].GetAngularVelocity());
	}
	if (drive & RWD)
	{
		driveshaft_speed += 0.5f * differential[DIFF_REAR].final_drive *
			(wheel[REAR_LEFT].GetAngularVelocity() + wheel[REAR_RIGHT].GetAngularVelocity());
	}
	if (drive == AWD)
	{
		driveshaft_speed *= 0.5f * differential[DIFF_CENTER].final_drive;
	}

	driveshaft_rpm = driveshaft_speed * btScalar(30 / M_PI);

	btScalar clutch_rpm = transmission.GetClutchSpeed(driveshaft_speed) * btScalar(30 / M_PI);

	if (autoshift)
	{
		int gear = NextGear(clutch_rpm);
		ShiftGear(gear);
	}

	remaining_shift_time = Max(remaining_shift_time - dt, btScalar(0));
	if (remaining_shift_time <= transmission.GetShiftTime() *  btScalar(0.5) && !shifted)
	{
		shifted = true;
		transmission.Shift(shift_gear);
		UpdateDrivelineGearRatio();
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
		return throttle * btScalar(0.5);
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
		ratio *= differential[DIFF_REAR].final_drive;
		drive_speed *= wheel[REAR_LEFT].GetRadius() / ratio;
	}
	else if (drive == FWD)
	{
		ratio *= differential[DIFF_FRONT].final_drive;
		drive_speed *= wheel[FRONT_LEFT].GetRadius() / ratio;
	}
	else if (drive == AWD)
	{
		ratio *= differential[DIFF_FRONT].final_drive;
		ratio *= differential[DIFF_CENTER].final_drive;
		drive_speed *= wheel[FRONT_LEFT].GetRadius() / ratio;
	}

	// speed limit due to drag
	btScalar aero_speed = btPow(engine.GetMaxPower() / aero_drag_coeff, 1 / 3.0f);

	return Min(drive_speed, aero_speed);
}

btScalar CarDynamics::CalculateFrontMassRatio() const
{
	btScalar dr = GetCenterOfMassOffset()[1];
	btScalar r1 = suspension[0].GetWheelPosition()[1];
	btScalar r2 = suspension[2].GetWheelPosition()[1];
	return (r2 + dr) / (r2 - r1);
}

btScalar CarDynamics::CalculateGripBalance() const
{
	btScalar dr = GetCenterOfMassOffset()[1];
	btScalar r1 = suspension[0].GetWheelPosition()[1] + dr;
	btScalar r2 = suspension[2].GetWheelPosition()[1] + dr;
	btScalar m = 1 / body->getInvMass();
	btScalar m1 = m * r2 / (r2 - r1);
	btScalar m2 = m - m1;
	btScalar f1 = tire[0].getMaxFy(m1 * (gravity * 0.5f), 0);
	btScalar f2 = tire[2].getMaxFy(m2 * (gravity * 0.5f), 0);
	btScalar t1 = f1 * r1;
	btScalar t2 = f2 * r2;
	return -t1 / (t2 - t1);
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
	for (int i = 0; i < WHEEL_COUNT; i++)
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
		btScalar ideal_angle = tire_state[0].ideal_slip_angle;
		btScalar max_angle = suspension[0].GetMaxSteeringAngle();
		btScalar ideal_value = btScalar(180/M_PI) * ideal_angle / max_angle;

		// transit to ideal value at 10 m/s (36km/h)
		btScalar transition = Min(body->getLinearVelocity().length2() * 0.01f, btScalar(1));
		btScalar abs_value = std::abs(value);
		btScalar max_value = abs_value + (ideal_value - abs_value) * transition;

		value = Clamp(value, -max_value, max_value);
	}

	for (int i = 0; i < WHEEL_COUNT; ++i)
	{
		suspension[i].SetSteering(value);
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
}

void CarDynamics::SetHandBrake(btScalar value)
{
	for (int i = 0; i < WHEEL_COUNT; ++i)
	{
		brake[i].SetHandbrakeFactor(value);
	}
}

void CarDynamics::RolloverRecover()
{
	btVector3 z = Direction::up;
	btVector3 y_car = transform.getBasis().getColumn(Direction::FORWARD);
	y_car = y_car - z * z.dot(y_car);
	y_car.normalize();

	btVector3 z_car = transform.getBasis().getColumn(Direction::UP);
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
	for (int i = 0; i < WHEEL_COUNT; ++i)
	{
		wheel_velocity[i][0] = 0;
		wheel_velocity[i][1] = 0;
		wheel_velocity[i][2] = 0;
		wheel_slip[i] = 0;
		abs_active[i] = 0;
		tcs_active[i] = 0;
	}
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
		btVector3 up_dir = obj0->getWorldTransform ().getBasis().getColumn(Direction::UP);
		btVector3 wheel_center = car_shape->getChildTransform(cp.m_index0).getOrigin();
		btVector3 contact_vec = wheel_center - cp.m_localPointA;
		// only invalidate if contact point in the lower quarter of the wheel
		if (contact_vec[Direction::UP] > btScalar(0.5) * wheel_shape->getRadius())
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
