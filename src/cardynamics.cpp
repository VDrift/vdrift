#include "cardynamics.h"
#include "tracksurface.h"
#include "dynamicsworld.h"
#include "fracturebody.h"
#include "loadcollisionshape.h"
#include "coordinatesystem.h"
#include "cfg/ptree.h"
#include "macros.h"

template<class T>
static inline bool isnan(const T & x)
{
	return x != x;
}

static inline std::istream & operator >> (std::istream & lhs, btVector3 & rhs)
{
	std::string str;
	for (int i = 0; i < 3 && !lhs.eof(); ++i)
	{
		std::getline(lhs, str, ',');
		std::stringstream s(str);
		s >> rhs[i];
	}
	return lhs;
}

static bool LoadClutch(
	const PTree & cfg,
	CARCLUTCH & clutch,
	std::ostream & error_output)
{
	btScalar sliding, radius, area, max_pressure;

	const PTree * cfg_clutch;
	if (!cfg.get("clutch", cfg_clutch, error_output)) return false;
	if (!cfg_clutch->get("sliding", sliding, error_output)) return false;
	if (!cfg_clutch->get("radius", radius, error_output)) return false;
	if (!cfg_clutch->get("area", area, error_output)) return false;
	if (!cfg_clutch->get("max-pressure", max_pressure, error_output)) return false;

	clutch.SetSlidingFriction(sliding);
	clutch.SetRadius(radius);
	clutch.SetArea(area);
	clutch.SetMaxPressure(max_pressure);

	return true;
}

static bool LoadTransmission(
	const PTree & cfg,
	CARTRANSMISSION & transmission,
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
		std::stringstream s;
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
	CARFUELTANK & fuel_tank,
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
	CARENGINE & engine,
	std::ostream & error_output)
{
	const PTree * cfg_eng;
	CARENGINEINFO engine_info;

	if (!cfg.get("engine", cfg_eng, error_output)) return false;
	if (!engine_info.Load(*cfg_eng, error_output)) return false;
	engine.Init(engine_info);

	return true;
}


static bool LoadBrake(
	const PTree & cfg,
	CARBRAKE & brake,
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

static bool LoadWheel(
	const PTree & cfg,
	const CARTIRE & tire,
	CARWHEEL & wheel)
{
	btScalar mass, inertia;

	// calculate mass, inertia from dimensions
	btScalar tire_radius = tire.GetRadius();
	btScalar tire_width = tire.GetWidth();
	btScalar tire_thickness = 0.05;
	btScalar tire_density = 8E3;

	btScalar rim_radius = tire_radius - tire_width * tire.GetAspectRatio();
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
	inertia = (tire_inertia + rim_inertia) * 4;	// scale inertia fixme

	// override mass, inertia
	cfg.get("mass", mass);
	cfg.get("inertia", inertia);

	wheel.SetMass(mass);
	wheel.SetInertia(inertia);

	return true;
}

static bool LoadDifferential(
	const PTree & cfg,
	CARDIFFERENTIAL & diff,
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

// use btMultiSphereShape(4 spheres) to approximate bounding box
static btMultiSphereShape * CreateCollisionShape(const btVector3 & center, const btVector3 & size)
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

	void operator()(FractureBody::Connection & connection)
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
		std::ostream & error,
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

CARDYNAMICS::CARDYNAMICS() :
	world(0),
	body(0),
	transform(btTransform::getIdentity()),
	linear_velocity(0,0,0),
	angular_velocity(0,0,0),
	drive(NONE),
	driveshaft_rpm(0),
	tacho_rpm(0),
	autoclutch(true),
	autoshift(false),
	shifted(true),
	shift_gear(0),
	last_auto_clutch(1),
	remaining_shift_time(0),
	abs(false),
	tcs(false),
	maxangle(0),
	maxspeed(0),
	feedback(0)
{
	suspension.resize(WHEEL_POSITION_SIZE);
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

CARDYNAMICS::~CARDYNAMICS()
{
	if (!body) return;

	// delete children
	for (int i = 0; i < body->getNumChildren(); ++i)
	{
		btRigidBody* child = body->getChildBody(i);
		if (child->isInWorld())
		{
			world->removeRigidBody(child);
			delete child->getCollisionShape();
		}
		delete child;
	}

	// delete body
	if (world)
	{
		world->removeAction(this);
		world->removeRigidBody(body);
	}
	if (body->getCollisionShape()->isCompound())
	{
		const btCompoundShape* shape = static_cast<btCompoundShape*>(body->getCollisionShape());
		for (int i = 0; i < shape->getNumChildShapes(); ++i)
		{
			delete shape->getChildShape(i);
		}
	}
	delete body->getCollisionShape();
	delete body;

	for (int i = 0; i < suspension.size(); ++i)
	{
		delete suspension[i];
	}

	for (int i = 0; i < aerodevice.size(); ++i)
	{
		delete static_cast<AeroDeviceFracture*>(aerodevice[i].GetUserPointer());
	}
}

bool CARDYNAMICS::Load(
	const PTree & cfg,
	const btVector3 & meshsize,
	const btVector3 & meshcenter,
	const btVector3 & position,
	const btQuaternion & rotation,
	const bool damage,
	DynamicsWorld & world,
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
		error << WHEEL_POSITION_SIZE << " are required." << std::endl;
		return false;
	}

	int i = 0;
	for (PTree::const_iterator it = cfg_wheels->begin(); it != cfg_wheels->end(); ++it, ++i)
	{
		const PTree & cfg_wheel = it->second;
		const PTree * cfg_tire, * cfg_brake;
		if (!cfg_wheel.get("tire", cfg_tire, error)) return false;
		if (!cfg_wheel.get("brake", cfg_brake, error)) return false;
		if (!tire[i].Load(*cfg_tire, error)) return false;
		if (!LoadBrake(*cfg_brake, brake[i], error)) return false;
		if (!LoadWheel(cfg_wheel, tire[i], wheel[i])) return false;
		if (!CARSUSPENSION::Load(cfg_wheel, suspension[i], error)) return false;
		if (suspension[i]->GetMaxSteeringAngle() > maxangle)
		{
			maxangle = suspension[i]->GetMaxSteeringAngle();
		}

		btScalar mass = wheel[i].GetMass();
		btScalar width = tire[i].GetWidth();
		btScalar radius = tire[i].GetRadius();
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
	if (bodyinfo.m_shape->getNumChildShapes() == wheel_count)
	{
		bodyinfo.m_shape->addChildShape(
			btTransform::getIdentity(),
			CreateCollisionShape(meshcenter, meshsize));
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
	SetPosition(body->getCenterOfMassPosition() + up_offset + fwd_offset);

	// realign with ground
	//AlignWithGround();

	// init cached state
	linear_velocity = body->getLinearVelocity();
	angular_velocity = body->getAngularVelocity();
	for (int i = 0; i < WHEEL_POSITION_SIZE; ++i)
	{
		wheel_orientation[i] = LocalToWorld(suspension[i]->GetWheelOrientation());
		wheel_velocity[i].setValue(0, 0, 0);
		suspension_force[i].setValue(0, 0, 0);
	}

	// initialize telemetry
	telemetry.clear();
	//telemetry.push_back(CARTELEMETRY("brakes"));
	//telemetry.push_back(CARTELEMETRY("suspension"));
	//etc

	maxspeed = CalculateMaxSpeed();

	// init steering force feedback value
	feedback = 0;

	return true;
}

void CARDYNAMICS::debugDraw(btIDebugDraw*)
{

}

btVector3 CARDYNAMICS::GetEnginePosition() const
{
	return GetPosition(0) + quatRotate(GetOrientation(0), engine.GetPosition());
}

const btVector3 & CARDYNAMICS::GetPosition() const
{
	return GetPosition(0);
}

const btQuaternion & CARDYNAMICS::GetOrientation() const
{
	return GetOrientation(0);
}

const btVector3 & CARDYNAMICS::GetWheelPosition(WHEEL_POSITION wp) const
{
	return motion_state[wp+1].position;
}

const btQuaternion & CARDYNAMICS::GetWheelOrientation(WHEEL_POSITION wp) const
{
	return motion_state[wp+1].rotation;
}

btQuaternion CARDYNAMICS::GetUprightOrientation(WHEEL_POSITION wp) const
{
	return GetOrientation(0) * suspension[wp]->GetWheelOrientation();
}

unsigned CARDYNAMICS::GetNumBodies() const
{
	return motion_state.size();
}

const btVector3 & CARDYNAMICS::GetPosition(int i) const
{
	btAssert(i < motion_state.size());
	return motion_state[i].position;
}

const btQuaternion & CARDYNAMICS::GetOrientation(int i) const
{
	btAssert(i < motion_state.size());
	return motion_state[i].rotation;
}

/// worldspace wheel center position
const btVector3 & CARDYNAMICS::GetWheelVelocity(WHEEL_POSITION wp) const
{
	return wheel_velocity[wp];
}

const COLLISION_CONTACT & CARDYNAMICS::GetWheelContact(WHEEL_POSITION wp) const
{
	return wheel_contact[wp];
}

COLLISION_CONTACT & CARDYNAMICS::GetWheelContact(WHEEL_POSITION wp)
{
	return wheel_contact[wp];
}

const btVector3 & CARDYNAMICS::GetCenterOfMass() const
{
	return body->getCenterOfMassPosition();
}

btScalar CARDYNAMICS::GetInvMass() const
{
	return body->getInvMass();
}

btScalar CARDYNAMICS::GetSpeed() const
{
	return body->getLinearVelocity().length();
}

const btVector3 & CARDYNAMICS::GetVelocity() const
{
	return body->getLinearVelocity();
}

void CARDYNAMICS::StartEngine()
{
	engine.StartEngine();
}

void CARDYNAMICS::ShiftGear(int value)
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

void CARDYNAMICS::SetThrottle(btScalar value)
{
	engine.SetThrottle(value);
}

void CARDYNAMICS::SetNOS(btScalar value)
{
	engine.SetNosBoost(value);
}

void CARDYNAMICS::SetClutch(btScalar value)
{
	clutch.SetClutch(value);
}

void CARDYNAMICS::SetBrake(btScalar value)
{
	for (int i = 0; i < brake.size(); ++i)
	{
		brake[i].SetBrakeFactor(value);
	}
}

void CARDYNAMICS::SetHandBrake(btScalar value)
{
	for (int i = 0; i < brake.size(); ++i)
	{
		brake[i].SetHandbrakeFactor(value);
	}
}

void CARDYNAMICS::SetAutoClutch(bool value)
{
	autoclutch = value;
}

void CARDYNAMICS::SetAutoShift(bool value)
{
	autoshift = value;
}

btScalar CARDYNAMICS::GetSpeedMPS() const
{
	return tire[0].GetRadius() * wheel[0].GetAngularVelocity();
}

btScalar CARDYNAMICS::GetMaxSpeedMPS() const
{
	return maxspeed;
}

btScalar CARDYNAMICS::GetTachoRPM() const
{
	return tacho_rpm;
}

void CARDYNAMICS::SetABS(const bool newabs)
{
	abs = newabs;
}

bool CARDYNAMICS::GetABSEnabled() const
{
	return abs;
}

bool CARDYNAMICS::GetABSActive() const
{
	return abs && ( abs_active[0]||abs_active[1]||abs_active[2]||abs_active[3] );
}

void CARDYNAMICS::SetTCS ( const bool newtcs )
{
	tcs = newtcs;
}

bool CARDYNAMICS::GetTCSEnabled() const
{
	return tcs;
}

bool CARDYNAMICS::GetTCSActive() const
{
	return tcs && ( tcs_active[0]||tcs_active[1]||tcs_active[2]||tcs_active[3] );
}

void CARDYNAMICS::SetPosition(const btVector3 & position)
{
	body->translate(position - body->getCenterOfMassPosition());

	transform.setOrigin(position);
	for (int i = 0; i < WHEEL_POSITION_SIZE; ++i)
		wheel_position[i] = LocalToWorld(suspension[i]->GetWheelPosition());
}

void CARDYNAMICS::AlignWithGround()
{
	UpdateWheelContacts();

	btScalar min_height = 0;
	bool no_min_height = true;
	for (int i = 0; i < WHEEL_POSITION_SIZE; ++i)
	{
		btScalar height = wheel_contact[i].GetDepth() - 2 * tire[i].GetRadius();
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

// ugh, ugly code
void CARDYNAMICS::RolloverRecover()
{
	btVector3 z(direction::up);
	btVector3 y_car = transform.getBasis() * direction::forward;
	y_car = y_car - z * z.dot(y_car);
	y_car.normalize();

	btVector3 z_car = transform.getBasis() * direction::up;
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

void CARDYNAMICS::SetSteering(const btScalar value)
{
	for (int i = 0; i < WHEEL_POSITION_SIZE; ++i)
	{
		suspension[i]->SetSteering(value);
	}
}

btScalar CARDYNAMICS::GetMaxSteeringAngle() const
{
	return maxangle;
}

btVector3 CARDYNAMICS::GetTotalAero() const
{
	btVector3 downforce(0, 0, 0);
	for (int i = 0; i != aerodevice.size(); ++i)
	{
		downforce = downforce + aerodevice[i].getLift() +  aerodevice[i].getDrag();
	}
	return downforce;
}

btScalar CARDYNAMICS::GetAerodynamicDownforceCoefficient() const
{
	btScalar coeff = 0.0;
	for (int i = 0; i != aerodevice.size(); ++i)
	{
		coeff += aerodevice[i].getLiftCoefficient();
	}
	return coeff;
}

btScalar CARDYNAMICS::GetAeordynamicDragCoefficient() const
{
	btScalar coeff = 0.0;
	for (int i = 0; i != aerodevice.size(); ++i)
	{
		coeff += aerodevice[i].getDragCoefficient();
	}
	return coeff;
}

btScalar CARDYNAMICS::GetFeedback() const
{
	return feedback;
}

void CARDYNAMICS::UpdateTelemetry(btScalar dt)
{
	for (std::list<CARTELEMETRY>::iterator i = telemetry.begin(); i != telemetry.end(); ++i)
	{
		i->Update(dt);
	}
}

std::ostream & operator << (std::ostream & os, const btVector3 & v)
{
	os << v[0] << ", " << v[1] << ", " << v[2];
	return os;
}

/// print debug info to the given ostream.  set p1, p2, etc if debug info part 1, and/or part 2, etc is desired
void CARDYNAMICS::DebugPrint ( std::ostream & out, bool p1, bool p2, bool p3, bool p4 ) const
{
	if ( p1 )
	{
		out << std::fixed << std::setprecision(3);
		out << "---Body---\n";
		out << "Velocity: " << GetVelocity() << "\n";
		out << "Position: " << GetPosition() << "\n";
		out << "Center of mass: " << -GetCenterOfMassOffset() << "\n";
		out << "Total mass: " << 1 / body->getInvMass() << "\n";
		out << "\n";
		fuel_tank.DebugPrint ( out );
		out << "\n";
		engine.DebugPrint ( out );
		out << "\n";
		clutch.DebugPrint ( out );
		out << "\n";
		transmission.DebugPrint ( out );
		out << "\n";
		if ( drive == RWD )
		{
			out << "(rear)" << "\n";
			differential_rear.DebugPrint ( out );
		}
		else if ( drive == FWD )
		{
			out << "(front)" << "\n";
			differential_front.DebugPrint ( out );
		}
		else if ( drive == AWD )
		{
			out << "(center)" << "\n";
			differential_center.DebugPrint ( out );

			out << "(front)" << "\n";
			differential_front.DebugPrint ( out );

			out << "(rear)" << "\n";
			differential_rear.DebugPrint ( out );
		}
		out << "\n";
	}

	if ( p2 )
	{
		out << std::fixed << std::setprecision(3);
		out << "(front left)" << "\n";
		suspension[FRONT_LEFT]->DebugPrint ( out );
		out << "\n";
		out << "(front right)" << "\n";
		suspension[FRONT_RIGHT]->DebugPrint ( out );
		out << "\n";
		out << "(rear left)" << "\n";
		suspension[REAR_LEFT]->DebugPrint ( out );
		out << "\n";
		out << "(rear right)" << "\n";
		suspension[REAR_RIGHT]->DebugPrint ( out );
		out << "\n";

		out << "(front left)" << "\n";
		brake[FRONT_LEFT].DebugPrint ( out );
		out << "\n";
		out << "(front right)" << "\n";
		brake[FRONT_RIGHT].DebugPrint ( out );
		out << "\n";
		out << "(rear left)" << "\n";
		brake[REAR_LEFT].DebugPrint ( out );
		out << "\n";
		out << "(rear right)" << "\n";
		brake[REAR_RIGHT].DebugPrint ( out );
	}

	if ( p3 )
	{
		out << std::fixed << std::setprecision(3);
		out << "\n";
		out << "(front left)" << "\n";
		wheel[FRONT_LEFT].DebugPrint ( out );
		out << "\n";
		out << "(front right)" << "\n";
		wheel[FRONT_RIGHT].DebugPrint ( out );
		out << "\n";
		out << "(rear left)" << "\n";
		wheel[REAR_LEFT].DebugPrint ( out );
		out << "\n";
		out << "(rear right)" << "\n";
		wheel[REAR_RIGHT].DebugPrint ( out );

		out << "\n";
		out << "(front left)" << "\n";
		tire[FRONT_LEFT].DebugPrint ( out );
		out << "\n";
		out << "(front right)" << "\n";
		tire[FRONT_RIGHT].DebugPrint ( out );
		out << "\n";
		out << "(rear left)" << "\n";
		tire[REAR_LEFT].DebugPrint ( out );
		out << "\n";
		out << "(rear right)" << "\n";
		tire[REAR_RIGHT].DebugPrint ( out );
	}

	if ( p4 )
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

bool CARDYNAMICS::Serialize ( joeserialize::Serializer & s )
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
	_SERIALIZE_(s, last_auto_clutch);
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
		_SERIALIZE_(s, tire[i]);
		_SERIALIZE_(s, *suspension[i]);
		if (!serialize(s, wheel_velocity[i])) return false;
		if (!serialize(s, wheel_position[i])) return false;
		if (!serialize(s, wheel_orientation[i])) return false;
	}
	return true;
}

btVector3 CARDYNAMICS::GetDownVector() const
{
	return -body->getCenterOfMassTransform().getBasis().getColumn(2);
}

const btVector3 & CARDYNAMICS::GetCenterOfMassOffset() const
{
	return motion_state[0].massCenterOffset;
}

btVector3 CARDYNAMICS::LocalToWorld(const btVector3 & local) const
{
	return body->getCenterOfMassTransform() * (GetCenterOfMassOffset() + local);
}

btQuaternion CARDYNAMICS::LocalToWorld(const btQuaternion & local) const
{
	return body->getCenterOfMassTransform() * local;
}

void CARDYNAMICS::UpdateWheelVelocity()
{
	for (int i = 0; i < WHEEL_POSITION_SIZE; ++i)
	{
		btVector3 offset = wheel_position[i] - body->getCenterOfMassPosition();
		wheel_velocity[i] = body->getVelocityInLocalPoint(offset);
	}
}

void CARDYNAMICS::UpdateWheelTransform()
{
	for (int i = 0; i < WHEEL_POSITION_SIZE; ++i)
	{
		if (body->getChildBody(i)->isInWorld()) continue;

		btQuaternion rot = suspension[i]->GetWheelOrientation();
		rot *= btQuaternion(direction::right, -wheel[i].GetRotation());
		btVector3 pos = suspension[i]->GetWheelPosition() + GetCenterOfMassOffset();
		body->setChildTransform(i, btTransform(rot, pos));

		wheel_position[i] = LocalToWorld(suspension[i]->GetWheelPosition());
		wheel_orientation[i] = LocalToWorld(suspension[i]->GetWheelOrientation());
	}
}

void CARDYNAMICS::ApplyEngineTorqueToBody ( btVector3 & torque )
{
	btVector3 engine_torque ( -engine.GetTorque(), 0, 0 );
	assert ( !isnan ( engine_torque[0] ) );
	torque = body->getCenterOfMassTransform().getBasis() * engine_torque;
}

void CARDYNAMICS::ApplyAerodynamicsToBody ( btVector3 & force, btVector3 & torque )
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
void CARDYNAMICS::DoTCS ( int i, btScalar suspension_force )
{
	btScalar sp ( 0 ), ah ( 0 );
	tire[i].GetSigmaHatAlphaHat ( suspension_force, sp, ah );
	//sp is the ideal slip ratio given tire loading

	btScalar gasthresh = 0.1;

	btScalar sense = 1.0;
	if ( transmission.GetGear() < 0 )
		sense = -1.0;

	btScalar gas = engine.GetThrottle();

	//only active if throttle commanded past threshold
	if ( gas > gasthresh )
	{
		//see if we're spinning faster than the rest of the wheels
		btScalar maxspindiff = 0;
		btScalar myrotationalspeed = wheel[i].GetAngularVelocity();
		for ( int i2 = 0; i2 < WHEEL_POSITION_SIZE; i2++ )
		{
			btScalar spindiff = myrotationalspeed - wheel[WHEEL_POSITION ( i2 ) ].GetAngularVelocity();
			if ( spindiff < 0 )
				spindiff = -spindiff;
			if ( spindiff > maxspindiff )
				maxspindiff = spindiff;
		}

		//don't engage if all wheels are moving at the same rate
		if ( maxspindiff > 1.0 )
		{
			btScalar error = tire[i].GetSlide() *sense - sp;
			btScalar thresholdeng = 0.0;
			btScalar thresholddis = -sp/2.0;

			if ( error > thresholdeng && ! tcs_active[i] )
				tcs_active[i] = true;

			if ( error < thresholddis && tcs_active[i] )
				tcs_active[i] = false;

			if ( tcs_active[i] )
			{
				btScalar curclutch = clutch.GetClutch();
				if ( curclutch > 1 ) curclutch = 1;
				if ( curclutch < 0 ) curclutch = 0;

				gas = gas - error*10.0*curclutch;
				if ( gas < 0 ) gas = 0;
				if ( gas > 1 ) gas = 1;
				engine.SetThrottle ( gas );
			}
		}
		else
			tcs_active[i] = false;
	}
	else
		tcs_active[i] = false;
}

///do anti-lock brake system calculations and modify the brake force if necessary
void CARDYNAMICS::DoABS ( int i, btScalar suspension_force )
{
	//an ideal ABS algorithm
	btScalar sp ( 0 ), ah ( 0 );
	tire[i].GetSigmaHatAlphaHat ( suspension_force, sp, ah );
	//sp is the ideal slip ratio given tire loading

	//only active if brakes commanded past threshold
	btScalar brakesetting = brake[i].GetBrakeFactor();
	if ( brakesetting > 0.1 )
	{
		btScalar maxspeed = 0;
		for ( int i2 = 0; i2 < WHEEL_POSITION_SIZE; i2++ )
		{
			if ( wheel[WHEEL_POSITION ( i2 ) ].GetAngularVelocity() > maxspeed )
				maxspeed = wheel[WHEEL_POSITION ( i2 ) ].GetAngularVelocity();
		}

		//don't engage ABS if all wheels are moving slowly
		if ( maxspeed > 6.0 )
		{
			btScalar error = - tire[i].GetSlide() - sp;
			btScalar thresholdeng = 0.0;
			btScalar thresholddis = -sp/2.0;

			if ( error > thresholdeng && ! abs_active[i] )
				abs_active[i] = true;

			if ( error < thresholddis && abs_active[i] )
				abs_active[i] = false;
		}
		else
			abs_active[i] = false;
	}
	else
		abs_active[i] = false;

	if ( abs_active[i] )
		brake[i].SetBrakeFactor ( 0.0 );
}

void CARDYNAMICS::ComputeSuspensionDisplacement ( int i, btScalar dt )
{
	//compute bump effect
	const TRACKSURFACE & surface = wheel_contact[i].GetSurface();
	btScalar posx = wheel_contact[i].GetPosition()[0];
	btScalar posz = wheel_contact[i].GetPosition()[2];
	btScalar phase = 2 * M_PI * ( posx + posz ) / surface.bumpWaveLength;
	btScalar shift = 2 * sin ( phase * M_PI_2 );
	btScalar amplitude = 0.25 * surface.bumpAmplitude;
	btScalar bumpoffset = amplitude * ( sin ( phase + shift ) + sin ( M_PI_2*phase ) - 2.0 );

	btScalar relative_displacement = wheel_contact[i].GetDepth() - 2 * tire[i].GetRadius() - bumpoffset;
	assert ( !isnan ( relative_displacement ) );
	suspension[i]->SetDisplacement ( suspension[i]->GetDisplacement()-relative_displacement );
	assert ( !isnan ( suspension[i]->GetDisplacement() ) );
}

///returns the suspension force (so it can be applied to the tires)
btVector3 CARDYNAMICS::ApplySuspensionForceToBody ( int i, btScalar dt, btVector3 & force, btVector3 & torque )
{
	//compute suspension force
	btScalar springdampforce = suspension[i]->GetForce ( dt );
	assert ( !isnan ( springdampforce ) );

	//do anti-roll
	int otheri = i;
	if ( i == 0 || i == 2 ) otheri++;
	else otheri--;
	btScalar antirollforce = suspension[i]->GetAntiRoll() *
		( suspension[i]->GetDisplacement() - suspension[WHEEL_POSITION ( otheri ) ]->GetDisplacement() );
	assert ( !isnan ( antirollforce ) );

	//find the vector direction to apply the suspension force
#ifdef SUSPENSION_FORCE_DIRECTION
	const btVector3 & wheelext = wheel[i].GetExtendedPosition();
	const btVector3 & hinge = suspension[i].GetHinge();
	btVector3 relwheelext = wheelext - hinge;
	btVector3 up ( 0,0,1 );
	btVector3 rotaxis = up.cross ( relwheelext.Normalize() );
	btVector3 forcedirection = relwheelext.Normalize().cross ( rotaxis );
	//std::cout << i << ". " << forcedirection << std::endl;
#else
	btVector3 forcedirection(direction::up);
#endif
	forcedirection = body->getCenterOfMassTransform().getBasis() * forcedirection;

	btVector3 suspension_force = forcedirection * ( antirollforce + springdampforce );
	btVector3 suspension_force_application_point = wheel_position[i] - body->getCenterOfMassPosition();

	btScalar overtravel = suspension[i]->GetOvertravel();
	if ( overtravel > 0 )
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

	for ( int n = 0; n < 3; ++n ) assert ( !isnan ( force[n] ) );
	for ( int n = 0; n < 3; ++n ) assert ( !isnan ( torque[n] ) );

	return suspension_force;
}

btVector3 CARDYNAMICS::ComputeTireFrictionForce (int i, btScalar dt, btScalar normal_force,
        btScalar angvel, btVector3 & groundvel, const btQuaternion & wheel_orientation)
{
	//determine camber relative to the road
	//the component of vector A projected onto plane B = A || B = B × (A×B / |B|) / |B|
	//plane B is the plane defined by using the tire's forward-facing vector as the plane's normal, in wheelspace
	//vector A is the normal of the driving surface, in wheelspace
	btVector3 B = direction::forward; //forward facing normal vector
	btVector3 A = quatRotate(wheel_orientation.inverse(), wheel_contact[ WHEEL_POSITION ( i ) ].GetNormal() ) ; //driving surface normal in wheelspace
	btVector3 Aproj = B.cross(A.cross(B)); //project the ground normal onto our forward facing plane
	assert(Aproj.length() > 0.001); //ensure the wheel isn't in an odd orientation
	Aproj = Aproj.normalize();
	btScalar camber_rads = btAcos(Aproj.dot(direction::up)); //find the angular difference in the camber axis between up and the projected ground normal
	assert(!isnan(camber_rads));
	//btVector3 crosscheck = Aproj.cross(up); //find axis of rotation between Aproj and up
	//camber_rads = (crosscheck[0] < 0) ? -camber_rads : camber_rads; //correct sign of angular distance
	camber_rads = -camber_rads;

	btScalar camber = camber_rads * SIMD_DEGS_PER_RAD;
	btScalar lonvel = direction::forward.dot(groundvel);
	btScalar latvel = -direction::right.dot(groundvel);
	btScalar friction_coeff =
		tire[i].GetTread() * wheel_contact[i].GetSurface().frictionTread +
		(1.0 - tire[i].GetTread()) * wheel_contact[i].GetSurface().frictionNonTread;

	btVector3 friction_force = tire[i].GetForce(
		normal_force, friction_coeff, camber, angvel, lonvel, latvel);

	for (int n = 0; n < 3; ++n) assert(!isnan(friction_force[n]));

	return friction_force;
}

void CARDYNAMICS::ApplyWheelForces ( btScalar dt, btScalar wheel_drive_torque, int i, const btVector3 & suspension_force, btVector3 & force, btVector3 & torque )
{
	btVector3 groundvel = quatRotate ( wheel_orientation[i].inverse(), wheel_velocity[i] );

#ifdef SUSPENSION_FORCE_DIRECTION
	btVector3 wheel_normal = quatRotate ( wheel_orientation[i], up );
	//btScalar normal_force = suspension_force.dot(wheel_normal);
	btScalar normal_force = suspension_force.length();
#else
	btScalar normal_force = suspension_force.length();
#endif
	assert(!isnan(normal_force));

	btVector3 friction_force = ComputeTireFrictionForce ( i, dt, normal_force, wheel[i].GetAngularVelocity(), groundvel, wheel_orientation[i] );

	//calculate friction torque
	btVector3 tire_force = direction::forward * friction_force[0] - direction::right * friction_force[1];
	btScalar tire_friction_torque = friction_force[0] * tire[i].GetRadius();
	assert ( !isnan ( tire_friction_torque ) );

	//calculate brake torque
	btScalar wheel_brake_torque = (0 - wheel[i].GetAngularVelocity()) / dt * wheel[i].GetInertia() - wheel_drive_torque + tire_friction_torque;
	if (wheel_brake_torque > 0 && wheel_brake_torque > brake[i].GetTorque())
	{
		wheel_brake_torque = brake[i].GetTorque();
	}
	else if (wheel_brake_torque < 0 && wheel_brake_torque < -brake[i].GetTorque())
	{
		wheel_brake_torque = -brake[i].GetTorque();
	}
	assert ( !isnan ( wheel_brake_torque ) );

	//limit the reaction torque to the applied drive and braking torque
	btScalar reaction_torque = tire_friction_torque;
	btScalar applied_torque = wheel_drive_torque + wheel_brake_torque;
	if ( ( applied_torque > 0 && reaction_torque > applied_torque ) ||
			( applied_torque < 0 && reaction_torque < applied_torque ) )
		reaction_torque = applied_torque;
	btVector3 tire_torque = direction::right * reaction_torque - direction::up * friction_force[2];

	//set wheel torque due to tire rolling resistance
	btScalar rolling_resistance = -tire[i].GetRollingResistance(wheel[i].GetAngularVelocity(), wheel_contact[i].GetSurface().rollResistanceCoefficient);
	btScalar tire_rolling_resistance_torque = rolling_resistance * tire[i].GetRadius() - tire_friction_torque;
	assert(!isnan(tire_rolling_resistance_torque));

	tire_force -= groundvel * wheel_contact[i].GetSurface().rollingDrag;

	//have the wheels internally apply forces, or just forcibly set the wheel speed if the brakes are locked
	wheel[i].SetTorque ( wheel_drive_torque+wheel_brake_torque+tire_rolling_resistance_torque, dt );
	wheel[i].Integrate ( dt );

	//apply forces to body
	btVector3 world_tire_force = quatRotate ( wheel_orientation[i], tire_force );
	btVector3 world_tire_torque = quatRotate ( wheel_orientation[i],  tire_torque);
	btVector3 tirepos = wheel_position[i] - body->getCenterOfMassPosition();
	force = force + world_tire_force;
	torque = torque + world_tire_torque + tirepos.cross(world_tire_force);
}

///the core function of the car dynamics simulation:  find and apply all forces on the car and components.
void CARDYNAMICS::ApplyForces ( btScalar dt, const btVector3 & ext_force, const btVector3 & ext_torque)
{
	assert ( dt > 0 );

	//start accumulating forces and torques on the car body
	btVector3 force (ext_force);
	btVector3 torque (ext_torque);

	// call before UpdateDriveline, overrides clutch, throttle
	UpdateTransmission(dt);

	//do TCS first thing
	if ( tcs )
	{
		for ( int i = 0; i < WHEEL_POSITION_SIZE; ++i )
		{
			DoTCS ( i, suspension_force[i].length() );
		}
	}

	//compute wheel torques
	btScalar wheel_drive_torque[4];
	UpdateDriveline(wheel_drive_torque, dt);

	//apply equal and opposite engine torque to the chassis
	//ApplyEngineTorqueToBody ( force, torque );

	//apply aerodynamics
	ApplyAerodynamicsToBody ( force, torque );

	//compute suspension displacements
	for ( int i = 0; i < WHEEL_POSITION_SIZE; ++i )
	{
		ComputeSuspensionDisplacement ( i, dt );
	}

	//compute suspension forces
	for ( int i = 0; i < WHEEL_POSITION_SIZE; ++i )
	{
		suspension_force[i] = ApplySuspensionForceToBody ( i, dt, force, torque );
	}

	//do abs
	if ( abs )
	{
		for ( int i = 0; i < WHEEL_POSITION_SIZE; ++i )
		{
			DoABS ( i, suspension_force[i].length() );
		}
	}

	//compute wheel forces
	for ( int i = 0; i < WHEEL_POSITION_SIZE; ++i )
	{
		ApplyWheelForces ( dt, wheel_drive_torque[i], i, suspension_force[i], force, torque );
	}

	for ( int n = 0; n < 3; ++n ) assert ( !isnan ( force[n] ) );
	for ( int n = 0; n < 3; ++n ) assert ( !isnan ( torque[n] ) );
	body->applyCentralForce( force );
	body->applyTorque( torque );
}

void CARDYNAMICS::Tick(btScalar dt, const btVector3 & force, const btVector3 & torque)
{
	body->clearForces();

	ApplyForces(dt, force, torque );

	body->integrateVelocities(dt);
	body->predictIntegratedTransform (dt, transform);
	body->proceedToTransform (transform);

	UpdateWheelVelocity();
	UpdateWheelTransform();
	InterpolateWheelContacts();
}

// executed as last function(after integration) in bullet singlestepsimulation
void CARDYNAMICS::updateAction(btCollisionWorld * collisionWorld, btScalar dt)
{
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

	int repeats = 10;
	btScalar dt_internal = dt / repeats;
	for (int i = 0; i < repeats; ++i)
	{
		Tick(dt_internal, force, torque);

		feedback += 0.5 * (tire[FRONT_LEFT].GetFeedback() + tire[FRONT_RIGHT].GetFeedback());
	}
	feedback /= (repeats + 1);

	//update fuel tank
	fuel_tank.Consume ( engine.FuelRate() * dt );
	engine.SetOutOfGas ( fuel_tank.Empty() );

	//calculate tacho
	const float tacho_factor = 0.1;
	tacho_rpm = engine.GetRPM() * tacho_factor + tacho_rpm * (1.0 - tacho_factor);

	UpdateTelemetry(dt);

	linear_velocity = body->getLinearVelocity();
	angular_velocity = body->getAngularVelocity();
}

void CARDYNAMICS::UpdateWheelContacts()
{
	btVector3 raydir = GetDownVector();
	btScalar raylen = 4;
	for (int i = 0; i < WHEEL_POSITION_SIZE; ++i)
	{
		btVector3 raystart = wheel_position[i] - raydir * tire[i].GetRadius();
		if (body->getChildBody(i)->isInWorld())
		{
			// wheel separated
			wheel_contact[i] = COLLISION_CONTACT(raystart, raydir, raylen, -1, 0, TRACKSURFACE::None(), 0);
		}
		else
		{
			world->castRay(raystart, raydir, raylen, body, wheel_contact[i]);
		}
	}
}

void CARDYNAMICS::InterpolateWheelContacts()
{
	btVector3 raydir = GetDownVector();
	btScalar raylen = 4;
	for (int i = 0; i < WHEEL_POSITION_SIZE; ++i)
	{
		btVector3 raystart = wheel_position[i] - raydir * tire[i].GetRadius();
		GetWheelContact(WHEEL_POSITION(i)).CastRay(raystart, raydir, raylen);
	}
}

void CARDYNAMICS::UpdateDriveline(btScalar drive_torque[], btScalar dt)
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
void CARDYNAMICS::CalculateDriveTorque(btScalar wheel_drive_torque[], btScalar clutch_torque)
{
	btScalar driveshaft_torque = transmission.GetTorque(clutch_torque);
	assert(!isnan(driveshaft_torque));

	for ( int i = 0; i < WHEEL_POSITION_SIZE; ++i )
		wheel_drive_torque[i] = 0;

	if ( drive == RWD )
	{
		differential_rear.ComputeWheelTorques(driveshaft_torque);
		wheel_drive_torque[REAR_LEFT] = differential_rear.GetSide1Torque();
		wheel_drive_torque[REAR_RIGHT] = differential_rear.GetSide2Torque();
	}
	else if ( drive == FWD )
	{
		differential_front.ComputeWheelTorques(driveshaft_torque);
		wheel_drive_torque[FRONT_LEFT] = differential_front.GetSide1Torque();
		wheel_drive_torque[FRONT_RIGHT] = differential_front.GetSide2Torque();
	}
	else if ( drive == AWD )
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
		assert(!isnan(wheel_drive_torque[WHEEL_POSITION(i)]));
}

btScalar CARDYNAMICS::CalculateDriveshaftSpeed()
{
	btScalar driveshaft_speed = 0.0;
	btScalar left_front_wheel_speed = wheel[FRONT_LEFT].GetAngularVelocity();
	btScalar right_front_wheel_speed = wheel[FRONT_RIGHT].GetAngularVelocity();
	btScalar left_rear_wheel_speed = wheel[REAR_LEFT].GetAngularVelocity();
	btScalar right_rear_wheel_speed = wheel[REAR_RIGHT].GetAngularVelocity();

	for ( int i = 0; i < 4; ++i )
		assert ( !isnan ( wheel[i].GetAngularVelocity() ) );

	if (drive == RWD)
	{
		driveshaft_speed = differential_rear.CalculateDriveshaftSpeed ( left_rear_wheel_speed, right_rear_wheel_speed );
	}
	else if (drive == FWD)
	{
		driveshaft_speed = differential_front.CalculateDriveshaftSpeed ( left_front_wheel_speed, right_front_wheel_speed );
	}
	else if (drive == AWD)
	{
		btScalar front_speed = differential_front.CalculateDriveshaftSpeed ( left_front_wheel_speed, right_front_wheel_speed );
		btScalar rear_speed = differential_rear.CalculateDriveshaftSpeed ( left_rear_wheel_speed, right_rear_wheel_speed );
		driveshaft_speed = differential_center.CalculateDriveshaftSpeed ( front_speed, rear_speed );
	}

	return driveshaft_speed;
}

void CARDYNAMICS::UpdateTransmission(btScalar dt)
{
	btScalar driveshaft_speed = CalculateDriveshaftSpeed();

	driveshaft_rpm = transmission.GetClutchSpeed(driveshaft_speed) * 30.0 / 3.141593;

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
		    //std::cout << "start engine" << std::endl;
		}

		btScalar throttle = engine.GetThrottle();
		throttle = ShiftAutoClutchThrottle(throttle, dt);
		engine.SetThrottle(throttle);

		btScalar new_clutch = AutoClutch(last_auto_clutch, dt);
		clutch.SetClutch(new_clutch);
		last_auto_clutch = new_clutch;
	}
}

bool CARDYNAMICS::WheelDriven(int i) const
{
	return (1 << i) & drive;
}

btScalar CARDYNAMICS::AutoClutch(btScalar last_clutch, btScalar dt) const
{
	btScalar rpm = engine.GetRPM();
	btScalar stallrpm = engine.GetStallRPM();
	btScalar clutchrpm = driveshaft_rpm; //clutch rpm on driveshaft/transmission side

	// clutch slip
	btScalar clutch = (5.0 * rpm + clutchrpm) / (9.0 * stallrpm) - 1.5;
	if (clutch < 0.0) clutch = 0.0;
	else if (clutch > 1.0) clutch = 1.0;

	// shift time
	clutch *= ShiftAutoClutch();

	// rate limit the autoclutch
	btScalar min_engage_time = 0.05;
	btScalar engage_limit = dt / min_engage_time;
	if (last_clutch - clutch > engage_limit)
	{
		clutch = last_clutch - engage_limit;
	}

	return clutch;
}

btScalar CARDYNAMICS::ShiftAutoClutch() const
{
	const btScalar shift_time = transmission.GetShiftTime();
	btScalar shift_clutch = 1.0;
	if (remaining_shift_time > shift_time * 0.5)
	    shift_clutch = 0.0;
	else if (remaining_shift_time > 0.0)
	    shift_clutch = 1.0 - remaining_shift_time / (shift_time * 0.5);
	return shift_clutch;
}

btScalar CARDYNAMICS::ShiftAutoClutchThrottle(btScalar throttle, btScalar dt)
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
int CARDYNAMICS::NextGear() const
{
	int gear = transmission.GetGear();

	// only autoshift if a shift is not in progress
	if (shifted && clutch.GetClutch() == 1.0)
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

btScalar CARDYNAMICS::DownshiftRPM(int gear) const
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

btScalar CARDYNAMICS::CalculateMaxSpeed() const
{
	btScalar maxspeed = 0;
	btScalar ratio = transmission.GetGearRatio(transmission.GetForwardGears());
	if (drive == RWD)
	{
		ratio *= differential_rear.GetFinalDrive();
		maxspeed = tire[REAR_LEFT].GetRadius() * engine.GetRPMLimit() * M_PI / 30 / ratio;
	}
	else if (drive == FWD)
	{
		ratio *= differential_front.GetFinalDrive();
		maxspeed = tire[FRONT_LEFT].GetRadius() * engine.GetRPMLimit() * M_PI / 30 / ratio;
	}
	else if (drive == AWD)
	{
		ratio *= differential_front.GetFinalDrive();
		ratio *= differential_center.GetFinalDrive();
		maxspeed = tire[FRONT_LEFT].GetRadius() * engine.GetRPMLimit() * M_PI / 30 / ratio;
	}
	return maxspeed;
}

bool CARDYNAMICS::WheelContactCallback(
	btManifoldPoint& cp,
	const btCollisionObject* colObj0,
	int partId0,
	int index0,
	const btCollisionObject* colObj1,
	int partId1,
	int index1)
{
	// cars are fracture bodies, wheel is a cylinder shape
	const btCollisionShape* shape = colObj0->getCollisionShape();
	if ((colObj0->getInternalType() & CO_FRACTURE_TYPE) &&
		(shape->getShapeType() == CYLINDER_SHAPE_PROXYTYPE))
	{
		// is contact within contact patch?
		const btCompoundShape* car = static_cast<const btCompoundShape*>(colObj0->getRootCollisionShape());
		const btCylinderShapeX* wheel = static_cast<const btCylinderShapeX*>(shape);
		btVector3 contactPoint = cp.m_localPointA - car->getChildTransform(cp.m_index0).getOrigin();
		if (-direction::up.dot(contactPoint) > 0.5 * wheel->getRadius())
		{
			// break contact (hack)
			cp.m_normalWorldOnB = btVector3(0, 0, 0);
			cp.m_distance1 = 0;
			cp.m_combinedFriction = 0;
			cp.m_combinedRestitution = 0;
			return true;
		}
	}
	return false;
}
const btCollisionObject& CARDYNAMICS::getCollisionObject() const {
	return *static_cast<btCollisionObject*>(body);
}
