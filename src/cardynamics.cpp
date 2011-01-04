#include "cardynamics.h"
#include "config.h"
#include "tracksurface.h"
#include "coordinatesystems.h"
#include "collision_world.h"
#include "tobullet.h"
#include "model.h"

#if defined(_WIN32) || defined(__APPLE__)
bool isnan(float number);
bool isnan(double number);
#endif

CARDYNAMICS::CARDYNAMICS() :
	world(0),
	shape(0),
	body(0),
	center_of_mass(0, 0, 0),
	transform(btTransform::getIdentity()),
	bodyPosition(0, 0, 0),
	bodyRotation(btQuaternion::getIdentity()),
	drive(NONE),
	tacho_rpm(0),
	autoclutch(true),
	autoshift(false),
	shifted(true),
	shift_gear(0),
	last_auto_clutch(1),
	remaining_shift_time(0),
	abs(false),
	tcs(false),
	maxangle(0)
{
	suspension.reserve(WHEEL_POSITION_SIZE);
	wheel.reserve(WHEEL_POSITION_SIZE);
	tire.reserve(WHEEL_POSITION_SIZE);
	brake.reserve(WHEEL_POSITION_SIZE);
	
	wheel_velocity.resize(WHEEL_POSITION_SIZE);
	wheel_position.resize(WHEEL_POSITION_SIZE);
	wheel_orientation.resize(WHEEL_POSITION_SIZE);
	wheel_contact.resize(WHEEL_POSITION_SIZE);
	abs_active.resize(WHEEL_POSITION_SIZE, false);
	tcs_active.resize(WHEEL_POSITION_SIZE, false);
}

CARDYNAMICS::~CARDYNAMICS()
{
	if (body)
	{
		world->RemoveAction(this);
		world->RemoveRigidBody(body);
		delete body;
		delete shape;
	}
}

static bool LoadClutch(
	const CONFIG & c,
	CARCLUTCH & clutch,
	std::ostream & error_output)
{
	btScalar sliding, radius, area, max_pressure;

	CONFIG::const_iterator it;
	if (!c.GetSection("clutch", it, error_output)) return false;
	if (!c.GetParam(it, "sliding", sliding, error_output)) return false;
	if (!c.GetParam(it, "radius", radius, error_output)) return false;
	if (!c.GetParam(it, "area", area, error_output)) return false;
	if (!c.GetParam(it, "max-pressure", max_pressure, error_output)) return false;

	clutch.SetSlidingFriction(sliding);
	clutch.SetRadius(radius);
	clutch.SetArea(area);
	clutch.SetMaxPressure(max_pressure);

	return true;
}

static bool LoadTransmission(
	const CONFIG & c,
	CARTRANSMISSION & transmission,
	std::ostream & error_output)
{
	btScalar shift_time = 0;
	btScalar ratio;
	int gears;

	CONFIG::const_iterator it;
	if (!c.GetSection("transmission", it, error_output)) return false;
	if (!c.GetParam(it, "gears", gears, error_output)) return false;
	for (int i = 0; i < gears; i++)
	{
		std::stringstream s;
		s << "gear-ratio-" << i+1;
		if (!c.GetParam(it, s.str(), ratio, error_output)) return false;
		transmission.SetGearRatio(i+1, ratio);
	}
	if (!c.GetParam(it, "gear-ratio-r", ratio, error_output)) return false;
	c.GetParam(it, "shift-time", shift_time);
	
	transmission.SetGearRatio(-1, ratio);
	transmission.SetShiftTime(shift_time);

	return true;
}

static bool LoadFuelTank(
	const CONFIG & c,
	CARFUELTANK & fuel_tank,
	std::ostream & error_output)
{
	btScalar capacity;
	btScalar volume;
	btScalar fuel_density;
	std::vector<btScalar> pos(3);

	CONFIG::const_iterator it;
	if (!c.GetSection("fuel-tank", it, error_output)) return false;
	if (!c.GetParam(it, "capacity", capacity, error_output)) return false;
	if (!c.GetParam(it, "volume", volume, error_output)) return false;
	if (!c.GetParam(it, "fuel-density", fuel_density, error_output)) return false;
	if (!c.GetParam(it, "position", pos, error_output)) return false;

	COORDINATESYSTEMS::ConvertV2toV1(pos[0], pos[1], pos[2]);
	btVector3 position(pos[0], pos[1], pos[2]);

	fuel_tank.SetCapacity(capacity);
	fuel_tank.SetVolume(volume);
	fuel_tank.SetDensity(fuel_density);
	fuel_tank.SetPosition(position);

	return true;
}

static bool LoadBrake(
	const CONFIG & c,
	const CONFIG::const_iterator & iwheel,
	CARBRAKE & brake,
	std::ostream & error_output)
{
	std::string brakename;
	if (!c.GetParam(iwheel, "brake", brakename, error_output)) return false;
	
	float friction, max_pressure, area, bias, radius, handbrake(0);
	CONFIG::const_iterator it;
	if (!c.GetSection(brakename, it, error_output)) return false;
	if (!c.GetParam(it, "friction", friction, error_output)) return false;
	if (!c.GetParam(it, "area", area, error_output)) return false;
	if (!c.GetParam(it, "radius", radius, error_output)) return false;
	if (!c.GetParam(it, "bias", bias, error_output)) return false;
	if (!c.GetParam(it, "max-pressure", max_pressure, error_output)) return false;
	c.GetParam(it, "handbrake", handbrake);

	brake.SetFriction(friction);
	brake.SetArea(area);
	brake.SetRadius(radius);
	brake.SetBias(bias);
	brake.SetMaxPressure(max_pressure*bias);
	brake.SetHandbrake(handbrake);

	return true;
}

static bool LoadWheel(
	const CONFIG & c,
	const CONFIG::const_iterator & iwheel,
	const CARTIRE & tire,
	CARWHEEL & wheel,
	std::ostream & error_output)
{
	btScalar mass, inertia;
	if (!c.GetParam(iwheel, "mass", mass) && !c.GetParam(iwheel, "inertia", inertia))
	{
		btScalar tire_radius = tire.GetRadius();
		btScalar tire_width = tire.GetSidewallWidth();
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
	}
	wheel.SetMass(mass);
	wheel.SetInertia(inertia);

	return true;
}

static bool LoadAeroDevices(
	const CONFIG & c,
	btAlignedObjectArray<CARAERO> & aerodynamics,
	std::ostream & error_output)
{
	CONFIG::const_iterator is;
	if (!c.GetSection("wing", is, error_output)) return true;
	
	int i = 0;
	aerodynamics.resize(is->second.size());
	for (CONFIG::SECTION::const_iterator iw = is->second.begin(); iw != is->second.end(); ++iw, ++i)
	{
		std::vector<btScalar> pos(3);
		btScalar drag_area, drag_coeff;
		btScalar lift_area(0), lift_coeff(0), lift_eff(0);
		
		CONFIG::const_iterator it;
		if (!c.GetSection(iw->second, it, error_output)) return false;
		if (!c.GetParam(it, "frontal-area", drag_area, error_output)) return false;
		if (!c.GetParam(it, "drag-coefficient", drag_coeff, error_output)) return false;
		if (!c.GetParam(it, "position", pos, error_output)) return false;
		c.GetParam(it, "surface-area", lift_area);
		c.GetParam(it, "lift-coefficient", lift_coeff);
		c.GetParam(it, "efficiency", lift_eff);
		
		COORDINATESYSTEMS::ConvertV2toV1(pos[0],pos[1],pos[2]);
		btVector3 position(pos[0], pos[1], pos[2]);
		
		aerodynamics[i].Set(position, drag_area, drag_coeff, lift_area, lift_coeff, lift_eff);
	}

	return true;
}

static bool LoadDifferential(
	const CONFIG & c,
	const CONFIG::const_iterator & it,
	CARDIFFERENTIAL & diff,
	std::ostream & error_output)
{
	btScalar final_drive(1), anti_slip(0), anti_slip_torque(0), anti_slip_torque_deceleration_factor(0);
	
	if (!c.GetParam(it, "final-drive", final_drive, error_output)) return false;
	if (!c.GetParam(it, "anti-slip", anti_slip, error_output)) return false;
	c.GetParam(it, "anti-slip-torque", anti_slip_torque);
	c.GetParam(it, "anti-slip-torque-deceleration-factor", anti_slip_torque_deceleration_factor);

	diff.SetFinalDrive(final_drive);
	diff.SetAntiSlip(anti_slip, anti_slip_torque, anti_slip_torque_deceleration_factor);

	return true;
}

static bool LoadMassParticles(
	const CONFIG & c,
	btAlignedObjectArray<std::pair<btScalar, btVector3> > & mass_particles,
	std::ostream & error_output)
{
	int num = 0;
	while(true)
	{
		btScalar mass;
		std::vector<btScalar> pos(3);
		
		std::stringstream str;
		str.width(2);
		str.fill('0');
		str << num;
		std::string name = "particle-"+str.str();
		
		CONFIG::const_iterator it;
		if (!c.GetSection(name, it)) break;
		if (!c.GetParam(it, "position", pos, error_output)) return false;
		if (!c.GetParam(it, "mass", mass)) return false;
		
		COORDINATESYSTEMS::ConvertV2toV1(pos[0], pos[1], pos[2]);
		btVector3 position(pos[0], pos[1], pos[2]);
		
		mass_particles.push_back(std::pair<btScalar, btVector3>(mass, position));
		
		num++;
	}

	return true;
}

// create collision shape from bounding box
static btMultiSphereShape * CreateCollisionShape(const btVector3 & center, const btVector3 & size)
{
	// use btMultiSphereShape(4 spheres) to approximate bounding box
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

bool CARDYNAMICS::Load(
	const CONFIG & cfg,
	const btVector3 & size,
	const btVector3 & center,
	const btVector3 & position,
	const btQuaternion & rotation,
	COLLISION_WORLD & world,
	std::ostream & error_output)
{
	if (!LoadAeroDevices(cfg, aerodynamics, error_output)) return false;
	if (!LoadClutch(cfg, clutch, error_output)) return false;
	if (!LoadTransmission(cfg, transmission, error_output)) return false;
	if (!LoadFuelTank(cfg, fuel_tank, error_output)) return false;
	
	CARENGINEINFO engine_info;
	if (!engine_info.Load(cfg, error_output)) return false;
	engine.Init(engine_info);
	AddMassParticle(engine.GetMass(), engine.GetPosition());
	
	CONFIG::const_iterator is;
	if (!cfg.GetSection("wheel", is, error_output)) return false;
	
	assert(is->second.size() == WHEEL_POSITION_SIZE); // temporary restriction
	tire.resize(is->second.size());
	brake.resize(is->second.size());
	wheel.resize(is->second.size());
	suspension.resize(is->second.size());
	
	int i = 0;
	for (CONFIG::SECTION::const_iterator iw = is->second.begin(); iw != is->second.end(); ++iw, ++i)
	{
		std::string section;
		CONFIG::const_iterator iwheel;
		if (!cfg.GetSection(iw->second, iwheel, error_output)) return false;
		
		if (!cfg.GetParam(iwheel, "tire", section, error_output)) return false;
		
		if (!tire[i].Load(cfg, section, error_output)) return false;
		
		if (!LoadBrake(cfg, iwheel, brake[i], error_output)) return false;
		
		if (!LoadWheel(cfg, iwheel, tire[i], wheel[i], error_output)) return false;
		
		CARSUSPENSION * sptr(0);
		if (!CARSUSPENSION::LoadSuspension(cfg, iw->second, sptr, error_output)) return false;
		suspension[i].reset(sptr);
		
		if (suspension[i]->GetMaxSteeringAngle() > maxangle) maxangle = suspension[i]->GetMaxSteeringAngle();
		
		AddMassParticle(wheel[i].GetMass(), suspension[i]->GetWheelPosition());
	}

	drive = NONE;
	CONFIG::const_iterator it;
	if (cfg.GetSection("differential-front", it))
	{
		if (!LoadDifferential(cfg, it, differential_front, error_output)) return false;
		drive = FWD;
	}
	if (cfg.GetSection("differential-rear", it))
	{
		if (!LoadDifferential(cfg, it, differential_rear, error_output)) return false;
		drive = (drive == FWD) ? AWD : RWD;
	}
	if (cfg.GetSection("differential-center", it) && drive == AWD)
	{
		if (!LoadDifferential(cfg, it, differential_center, error_output)) return false;
	}
	if (drive == NONE)
	{
		error_output << "No differential declared" << std::endl;
		return false;
	}

	// load driver mass, todo unify for all car components
	if (cfg.GetSection("driver", it))
	{
		btScalar mass;
		std::vector<btScalar> pos(3);
		if (!cfg.GetParam(it, "position", pos, error_output)) return false;
		if (!cfg.GetParam(it, "mass", mass, error_output)) return false;
		COORDINATESYSTEMS::ConvertV2toV1(pos[0], pos[1], pos[2]);
		btVector3 position(pos[0], pos[1], pos[2]);
		AddMassParticle(mass, position);
	}

	if (!LoadMassParticles(cfg, mass_particles, error_output)) return false;

	Init(world, size, center, position, rotation);

	// initialize telemetry
	telemetry.clear();
	//telemetry.push_back(CARTELEMETRY("brakes"));
	//telemetry.push_back(CARTELEMETRY("suspension"));
	//etc

	return true;
}

// calculate bounding box from body, wheels
void CARDYNAMICS::GetCollisionBox(
	const btVector3 & bodyMin,
	const btVector3 & bodyMax,
	btVector3 & center,
	btVector3 & size)
{
	btVector3 min = bodyMin - center_of_mass;
	btVector3 max = bodyMax - center_of_mass;
	btScalar minHeight = min.z() + 0.05; // add collision shape bottom margin
	for (int i = 0; i < 4; i++)
	{
		btVector3 wheelHSize(tire[i].GetRadius(), tire[i].GetSidewallWidth()*0.5, tire[i].GetRadius());
		btVector3 wheelPos = suspension[i]->GetWheelPosition(0.0);
		btVector3 wheelMin = wheelPos - wheelHSize;
		btVector3 wheelMax = wheelPos + wheelHSize;
		min.setMin(wheelMin);
		max.setMax(wheelMax);
	}
	min.setZ(minHeight);

	center = (max + min) * 0.5;
	size = max - min;
}

void CARDYNAMICS::Init(
	COLLISION_WORLD & world,
	const btVector3 & bodySize,
	const btVector3 & bodyCenter,
	const btVector3 & position,
	const btQuaternion & rotation)
{
	btScalar mass;
	btVector3 inertia;
	CalculateMass(center_of_mass, inertia, mass);

	transform.setOrigin(position - center_of_mass);
	transform.setRotation(rotation);
	motionState.setWorldTransform(transform);
	motionState.m_centerOfMassOffset.setOrigin(-center_of_mass);

	btVector3 bodyMin = bodyCenter - bodySize * 0.5;
	btVector3 bodyMax = bodyCenter + bodySize * 0.5;
	btVector3 origin, size;
	GetCollisionBox(bodyMin, bodyMax, origin, size);
	shape = CreateCollisionShape(origin, size);

	// create rigid body
	btRigidBody::btRigidBodyConstructionInfo info(mass, &motionState, shape, inertia);
	body = new btRigidBody(info);
	body->setActivationState(DISABLE_DEACTIVATION);
	body->setContactProcessingThreshold(0.0); // internal edge workaround(swept sphere shape required)

	// init wheels
	for (int i = 0; i < WHEEL_POSITION_SIZE; i++)
	{
		wheel_velocity[i].setValue(0, 0, 0);
		wheel_position[i] = LocalToWorld(suspension[i]->GetWheelPosition(0.0));
		wheel_orientation[i] = LocalToWorld(suspension[i]->GetWheelOrientation());
	}

	// add car to world
	this->world = &world;
	world.AddRigidBody(body);
	world.AddAction(this);
	AlignWithGround();
}

// executed as last function(after integration) in bullet singlestepsimulation
void CARDYNAMICS::updateAction(btCollisionWorld * collisionWorld, btScalar dt)
{
	// reset transform, before processing tire/suspension constraints
	// will break bullets collision clamping, tunneling prevention
	body->setCenterOfMassTransform(transform);
	
	UpdateWheelContacts();
	
	Tick(dt);
}

void CARDYNAMICS::debugDraw(btIDebugDraw* debugDrawer)
{

}

void CARDYNAMICS::Update()
{
	bodyRotation = motionState.m_graphicsWorldTrans.getRotation();
	bodyPosition = motionState.m_graphicsWorldTrans.getOrigin();
}

const btVector3 & CARDYNAMICS::GetPosition() const
{
	return bodyPosition;
}

const btQuaternion & CARDYNAMICS::GetOrientation() const
{
	return bodyRotation;
}

btVector3 CARDYNAMICS::GetEnginePosition() const
{
	return bodyPosition + quatRotate(bodyRotation, engine.GetPosition());
}

btVector3 CARDYNAMICS::GetWheelPosition(WHEEL_POSITION wp) const
{
	return bodyPosition + quatRotate(bodyRotation, suspension[wp]->GetWheelPosition());
}

btVector3 CARDYNAMICS::GetWheelPosition(WHEEL_POSITION wp, btScalar displacement_fraction) const
{
	return bodyPosition + quatRotate(bodyRotation, suspension[wp]->GetWheelPosition(displacement_fraction));
}

btQuaternion CARDYNAMICS::GetWheelOrientation(WHEEL_POSITION wp) const
{
	return bodyRotation * suspension[wp]->GetWheelOrientation() * wheel[wp].GetRotation();
}

btQuaternion CARDYNAMICS::GetUprightOrientation(WHEEL_POSITION wp) const
{
	return bodyRotation * suspension[wp]->GetWheelOrientation();
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

const btVector3 & CARDYNAMICS::GetCenterOfMassPosition() const
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

void CARDYNAMICS::SetClutch(btScalar value)
{
	clutch.SetClutch(value);
}

void CARDYNAMICS::SetBrake(btScalar value)
{
	for(int i = 0; i < brake.size(); i++)
	{
		brake[i].SetBrakeFactor(value);
	}
}

void CARDYNAMICS::SetHandBrake(btScalar value)
{
	for(int i = 0; i < brake.size(); i++)
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
	transform.setOrigin(position);
	body->translate(position - body->getCenterOfMassPosition());
}

void CARDYNAMICS::AlignWithGround()
{
	UpdateWheelTransform();
	UpdateWheelContacts();

	btScalar min_height = 0;
	bool no_min_height = true;
	for (int i = 0; i < WHEEL_POSITION_SIZE; i++)
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
	body->setAngularVelocity(btVector3(0, 0, 0));
	body->setLinearVelocity(btVector3(0, 0, 0));
	
	UpdateWheelVelocity();
	UpdateWheelTransform();
	UpdateWheelContacts();
}

// ugh, ugly code
void CARDYNAMICS::RolloverRecover()
{
	btQuaternion rot(0, 0, 0, 1);
	//btTransform transform = body->getCenterOfMassTransform();

	btVector3 z(0, 0, 1);
	btVector3 y_car = transform.getBasis().getColumn(0);
	y_car = y_car - z * z.dot(y_car);
	y_car.normalize();

	btVector3 z_car = transform.getBasis().getColumn(2);
	z_car = z_car - y_car * y_car.dot(z_car);
	z_car.normalize();

	btScalar angle = z_car.angle(z);
	if (fabs(angle) < M_PI / 4.0) return;

	rot.setRotation(y_car, angle);
	rot = rot * transform.getRotation();

	transform.setRotation(rot);
	body->setCenterOfMassTransform(transform);

	AlignWithGround();
}

void CARDYNAMICS::SetSteering(const btScalar value)
{
	for(int i = 0; i < WHEEL_POSITION_SIZE; i++)
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
	for (int i = 0; i != aerodynamics.size(); ++i)
	{
		downforce = downforce + aerodynamics[i].GetLiftVector() +  aerodynamics[i].GetDragVector();
	}
	return downforce;
}

btScalar CARDYNAMICS::GetAerodynamicDownforceCoefficient() const
{
	btScalar coeff = 0.0;
	for (int i = 0; i != aerodynamics.size(); ++i)
	{
		coeff += aerodynamics[i].GetAerodynamicDownforceCoefficient();
	}
	return coeff;
}

btScalar CARDYNAMICS::GetAeordynamicDragCoefficient() const
{
	btScalar coeff = 0.0;
	for (int i = 0; i != aerodynamics.size(); ++i)
	{
		coeff += aerodynamics[i].GetAeordynamicDragCoefficient();
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
		out << "Velocity: " << body->getLinearVelocity() << "\n";
		out << "Position: " << bodyPosition << "\n";
		out << "Center of mass: " << center_of_mass << "\n";
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
		for (int i = 0; i != aerodynamics.size(); ++i)
		{
			aerodynamics[i].DebugPrint(out);
			out <<  "\n";
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
	_SERIALIZE_(s,engine);
	_SERIALIZE_(s,clutch);
	_SERIALIZE_(s,transmission);
	_SERIALIZE_(s,differential_front);
	_SERIALIZE_(s,differential_rear);
	_SERIALIZE_(s,differential_center);
	_SERIALIZE_(s,fuel_tank);
	_SERIALIZE_(s,abs);
	_SERIALIZE_(s,abs_active);
	_SERIALIZE_(s,tcs);
	_SERIALIZE_(s,tcs_active);
	_SERIALIZE_(s,last_auto_clutch);
	_SERIALIZE_(s,remaining_shift_time);
	_SERIALIZE_(s,shift_gear);
	_SERIALIZE_(s,shifted);
	_SERIALIZE_(s,autoshift);
	
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
	
	if (!serialize(s, *body)) return false;
	if (!serialize(s, transform)) return false;
	if (!serialize(s, bodyPosition)) return false;
	if (!serialize(s, bodyRotation)) return false;

	return true;
}

btVector3 CARDYNAMICS::GetDownVector() const
{
	return -body->getCenterOfMassTransform().getBasis().getColumn(2);
}

btVector3 CARDYNAMICS::LocalToWorld(const btVector3 & local) const
{
	return body->getCenterOfMassTransform() * (local - center_of_mass);
}

btQuaternion CARDYNAMICS::LocalToWorld(const btQuaternion & local) const
{
	return body->getCenterOfMassTransform() * local;
}

void CARDYNAMICS::UpdateWheelVelocity()
{
	for(int i = 0; i < WHEEL_POSITION_SIZE; i++)
	{
		btVector3 offset = wheel_position[i] - body->getCenterOfMassPosition();
		wheel_velocity[i] = body->getVelocityInLocalPoint(offset);
	}
}

void CARDYNAMICS::UpdateWheelTransform()
{
	for(int i = 0; i < WHEEL_POSITION_SIZE; i++)
	{
		wheel_position[i] = LocalToWorld(suspension[i]->GetWheelPosition());
		wheel_orientation[i] = LocalToWorld(suspension[i]->GetWheelOrientation());
	}
}

void CARDYNAMICS::ApplyEngineTorqueToBody()
{
	btVector3 torque(-engine.GetTorque(), 0, 0);
	body->getCenterOfMassTransform().getBasis() * torque;
	body->applyTorque(torque);
}

void CARDYNAMICS::AddAerodynamics(btVector3 & force, btVector3 & torque)
{
	btMatrix3x3 inv = body->getCenterOfMassTransform().getBasis().inverse();
	btVector3 wind_force(0, 0, 0);
	btVector3 wind_torque(0, 0, 0);
	btVector3 air_velocity = inv * -GetVelocity();
	for(int i = 0; i != aerodynamics.size(); ++i)
	{
		btVector3 force = aerodynamics[i].GetForce(air_velocity);
		wind_force = wind_force + force;
		wind_torque = wind_torque + (aerodynamics[i].GetPosition() - center_of_mass).cross(force);
	}
	wind_force = body->getCenterOfMassTransform().getBasis() * wind_force;
	wind_torque = body->getCenterOfMassTransform().getBasis() * wind_torque;
	force = force + wind_force;
	torque = torque + wind_torque;
}

void CARDYNAMICS::UpdateSuspension(btScalar normal_force[], btScalar dt)
{
	// suspension
	btVector3 upright = -GetDownVector();
	btScalar normal_force_limit[WHEEL_POSITION_SIZE], cosn[WHEEL_POSITION_SIZE];
	for (int i = 0; i < WHEEL_POSITION_SIZE; ++i)
	{
		btVector3 position = wheel_contact[i].GetPosition();
		btVector3 normal = wheel_contact[i].GetNormal();
		
		btScalar normal_mass = 1 / body->computeImpulseDenominator(position, normal);
		btScalar normal_velocity = wheel_velocity[i].dot(normal);
		normal_force_limit[i] = -normal_velocity * normal_mass / dt;
		cosn[i] = upright.dot(normal);
		if (cosn[i] > 1) cosn[i] = 1; 				// make sure cosn <= 1
		else if (cosn[i] < 1E-3) cosn[i] = 1E-3;	// avoid division by zero
		
		// adjust displacement due to surface bumpiness
		btScalar displacement = 2.0 * tire[i].GetRadius() - wheel_contact[i].GetDepth();
		const TRACKSURFACE & surface = wheel_contact[i].GetSurface();
		if (surface.bumpWaveLength > 0.0001)
		{
			btScalar posx = position[0];
			btScalar posz = position[2];
			btScalar phase = 2.0 * 3.141593 * (posx + posz) / surface.bumpWaveLength;
			btScalar shift = 2.0 * sin(phase * 1.414214);
			btScalar amplitude = 0.25 * surface.bumpAmplitude;
			btScalar bumpoffset = amplitude * (sin(phase + shift) + sin(1.414214 * phase) - 2.0);
			displacement += bumpoffset;
		}
		
		suspension[i]->Update(normal_force_limit[i] * cosn[i], normal_velocity *  cosn[i], displacement);
	}

	// antiroll + hinge
	for (int i = 0; i < WHEEL_POSITION_SIZE; ++i)
	{
		int otheri = i;
		if ( i == 0 || i == 2 ) otheri++;
		else otheri--;
		
		btScalar antirollforce = suspension[i]->GetAntiRoll() * 
			(suspension[i]->GetDisplacement() - suspension[otheri]->GetDisplacement());
		
		btScalar suspension_force = suspension[i]->GetForce() + antirollforce;
		if (suspension_force < 0)
		{
			suspension_force = 0;
		}
		
		// combine lateral(suspenion geometry) and suspension force, a bit hacky
		btScalar force = suspension_force * cosn[i];
		if (suspension[i]->GetDisplacement() <= 0 || normal_force_limit[i] < 0)
		{
			normal_force_limit[i] = 0;
		}
		if (force < normal_force_limit[i])
		{
			force = suspension_force / cosn[i];
			if (force > normal_force_limit[i])
			{
				force = normal_force_limit[i];
			}
		}
		assert(force == force);
		normal_force[i] = force;
		
		btVector3 offset = wheel_contact[i].GetPosition() - body->getCenterOfMassPosition();
		body->applyForce(force * wheel_contact[i].GetNormal(), offset);
	}
}

void CARDYNAMICS::UpdateWheel(
	const int i,
	const btScalar dt,
	const btScalar normal_force,
	const btScalar drive_torque,
	const btQuaternion & wheel_space)
{
	CARWHEEL & wheel = this->wheel[i];
	CARTIRE & tire = this->tire[i];
	CARBRAKE & brake = this->brake[i];
	const COLLISION_CONTACT & contact = this->wheel_contact[i];
	const TRACKSURFACE & surface = contact.GetSurface();
	const btVector3 & position = contact.GetPosition();
	const btVector3 & normal = contact.GetNormal();

	// inclination positive when tire top tilts to right viewed from rear
	btVector3 wheel_axis = quatRotate(wheel_space, btVector3(0, 1, 0));
	btScalar axis_proj = wheel_axis.dot(normal);
	btScalar inclination = 90 - acos(axis_proj) * 180.0 / M_PI;
	if (!(i&1)) inclination = -inclination;
	
	// tire space(SAE Tire Coordinate System)
	// surface normal is negative z-axis
	// negative spin axis projected onto surface plane is y-axis
	btVector3 y = -(wheel_axis - normal * axis_proj).normalized();
	btVector3 x = normal.cross(y);

	// wheel velocity in tire space
	btVector3 velocity(0, 0, 0);
	wheel_velocity[i] = body->getVelocityInLocalPoint(wheel_position[i] - body->getCenterOfMassPosition());
	velocity[0] = x.dot(wheel_velocity[i]);
	velocity[1] = y.dot(wheel_velocity[i]);

	// wheel angular velocity
	btScalar ang_velocity = wheel.GetAngularVelocity();

	// friction force in tire space
	btScalar friction_coeff = tire.GetTread() * surface.frictionTread + (1.0 - tire.GetTread()) * surface.frictionNonTread;
	btVector3 friction = tire.GetForce(normal_force, friction_coeff, inclination, ang_velocity, velocity);
	
	// rolling resistance
	btScalar roll_friction_coeff = surface.rollResistanceCoefficient;
	friction[0] += tire.GetRollingResistance(velocity[0], normal_force, roll_friction_coeff);
	
	// friction impulse
	friction[0] = friction[0] * dt;
	friction[1] = friction[1] * dt;

	// limit lateral friction
	btScalar lat_mass = 1 / body->computeImpulseDenominator(position, y);
	btScalar lat_limit = -velocity[1] * lat_mass;
	if (friction[1] * lat_limit < 0)
	{
		// shouldn't happen, friction is dissipative
		//std::cerr << "Lateral friction impulse: " << friction[1] << ", limit: " << lat_limit << std::endl;
		friction[1] = 0;
	}
	if (friction[1] > 0 && friction[1] > lat_limit) friction[1] = lat_limit;
	else if (friction[1] < 0 && friction[1] < lat_limit) friction[1] = lat_limit;
	
	// limit longitudinal friction
	btScalar lon_mass = 1 / body->computeImpulseDenominator(position, x);
	btScalar patch_velocity = velocity[0] - ang_velocity * tire.GetRadius();
	btScalar lon_limit = -patch_velocity * lon_mass;
	if (friction[0] * lon_limit < 0)
	{
		// shouldn't happen, friction is dissipative
		//std::cerr << "Longitudinal friction impulse: " << friction[1] << ", limit: " << lon_limit << std::endl;
		friction[0] = 0;
	}
	if (friction[0] > 0 && friction[0] > lon_limit) friction[0] = lon_limit;
	else if (friction[0] < 0 && friction[0] < lon_limit) friction[0] = lon_limit;

	// wheel torque
	btScalar brake_torque = brake.GetTorque();
	btScalar brake_limit = wheel.GetTorque(0, dt);
	btScalar friction_torque = tire.GetRadius() * friction[0] / dt;
	btScalar wheel_torque = drive_torque - friction_torque;
	btScalar max_torque = brake_limit - wheel_torque;
	if(max_torque >= 0 && max_torque > brake_torque)
	{
		wheel_torque += brake_torque;
	}
	else if(max_torque < 0 && max_torque < -brake_torque)
	{
		wheel_torque -= brake_torque;
	}
	else
	{
		wheel_torque = brake_limit;
	}
	wheel.SetTorque(wheel_torque, dt);
	wheel.Integrate(dt);

	// apply wheel torque to body
	btVector3 world_wheel_torque = quatRotate(wheel_space, btVector3(0, -wheel_torque * dt, 0));
	body->applyTorqueImpulse(world_wheel_torque);
	
	// add viscous surface drag
	friction = friction - velocity * surface.rollingDrag * dt * 0.25;

	// apply friction force in world space
	btVector3 tire_friction = x * friction[0] + y * friction[1];
	btVector3 offset = position - body->getCenterOfMassPosition();
	body->applyImpulse(tire_friction, offset);
}

void CARDYNAMICS::UpdateBody(
	const btVector3 & ext_force,
	const btVector3 & ext_torque,
	const btScalar drive_torque[],
	const btScalar dt)
{
	body->clearForces();
	body->applyCentralForce(ext_force);
	body->applyTorque(ext_torque);
	
	ApplyEngineTorqueToBody();
	
	btScalar normal_force[WHEEL_POSITION_SIZE];
	UpdateSuspension(normal_force, dt);
	body->integrateVelocities(dt);
	body->clearForces();
	
	for(int i = 0; i < WHEEL_POSITION_SIZE; ++i)
	{
		UpdateWheel(i, dt, normal_force[i], drive_torque[i], wheel_orientation[i]);
	}
	
	body->predictIntegratedTransform(dt, transform);
	body->proceedToTransform(transform);
	
	//UpdateWheelVelocity();
	UpdateWheelTransform();
	InterpolateWheelContacts();
}

void CARDYNAMICS::Tick(const btScalar dt)
{
	// call before UpdateDriveline, overrides clutch, throttle
	UpdateTransmission(dt);

	// overrides throttle/brakes
	for(int i = 0; i < WHEEL_POSITION_SIZE; ++i)
	{
		if (abs) DoABS(i);
		if (tcs) DoTCS(i);
	}
	
	btVector3 ext_force(0, 0, 0), ext_torque(0, 0, 0); 
	AddAerodynamics(ext_force, ext_torque);

	const int num_repeats = 8;
	const btScalar internal_dt = dt / num_repeats;
	for(int i = 0; i < num_repeats; ++i)
	{
		btScalar drive_torque[WHEEL_POSITION_SIZE];

		UpdateDriveline(drive_torque, internal_dt);

		UpdateBody(ext_force, ext_torque, drive_torque, internal_dt);

		feedback += 0.5 * (tire[FRONT_LEFT].GetFeedback() + tire[FRONT_RIGHT].GetFeedback());
	}

	feedback /= (num_repeats + 1);

	fuel_tank.Consume(engine.FuelRate() * dt);
	engine.SetOutOfGas(fuel_tank.Empty());

	const btScalar tacho_factor = 0.1;
	tacho_rpm = engine.GetRPM() * tacho_factor + tacho_rpm * (1.0 - tacho_factor);

	UpdateTelemetry(dt);
}

void CARDYNAMICS::UpdateWheelContacts()
{
	btVector3 raydir = GetDownVector();
	btScalar raylen = 4;
	for (int i = 0; i < WHEEL_POSITION_SIZE; ++i)
	{
		COLLISION_CONTACT & contact = wheel_contact[WHEEL_POSITION(i)];
		btVector3 raystart = wheel_position[i] - raydir * tire[i].GetRadius();
		world->CastRay(raystart, raydir, raylen, body, contact);
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

// todo: Calculate principle axes calculation
void CARDYNAMICS::CalculateMass(
	btVector3 & center,
	btVector3 & inertia,
	btScalar & mass)
{
	center.setValue(0, 0, 0);
	mass = 0;
	
	// calculate the total mass, and center of mass
	for (int i = 0; i != mass_particles.size(); ++i)
	{
		center = center +  mass_particles[i].second *  mass_particles[i].first;
		mass +=  mass_particles[i].first;
	}
	
	// account for fuel
	mass += fuel_tank.GetMass();
	center = center + fuel_tank.GetPosition() * fuel_tank.GetMass();
	center = center * (1.0 / mass);
	
	// calculate the inertia tensor
	btScalar xx(0), yy(0), zz(0), xy(0), xz(0), yz(0);
	for (int i = 0; i != mass_particles.size(); ++i)
	{
		btVector3 p = mass_particles[i].second - center;
		btScalar m = mass_particles[i].first;
		
		// add the current mass to the inertia tensor
		xx += m * (p.y() * p.y() + p.z() * p.z()); //+mi*(yi^2+zi^2)
		yy += m * (p.x() * p.x() + p.z() * p.z()); //+mi*(xi^2+zi^2)
		zz += m * (p.x() * p.x() + p.y() * p.y()); //+mi*(xi^2+yi^2)
		
		xy -= m * (p.x() * p.y()); //-mi*xi*yi
		xz -= m * (p.x() * p.z()); //-mi*xi*zi
		yz -= m * (p.y() * p.z()); //-mi*yi*zi
	}
	inertia.setValue(xx, yy, zz);
}

void CARDYNAMICS::UpdateDriveline(btScalar drive_torque[], btScalar dt)
{
	btScalar driveshaft_speed = CalculateDriveshaftSpeed();
	btScalar clutch_speed = transmission.CalculateClutchSpeed(driveshaft_speed);
	btScalar crankshaft_speed = engine.GetAngularVelocity();
	btScalar clutch_drag = clutch.GetTorqueMax(crankshaft_speed, clutch_speed);
	if(transmission.GetGear() == 0) clutch_drag = 0;

	clutch_drag = engine.Update(clutch_drag, clutch_speed, dt);

	CalculateDriveTorque(drive_torque, -clutch_drag);
}

///calculate the drive torque that the engine applies to each wheel, and put the output into the supplied 4-element array
void CARDYNAMICS::CalculateDriveTorque(btScalar wheel_drive_torque[], btScalar clutch_torque)
{
	btScalar driveshaft_torque = transmission.GetTorque(clutch_torque);
	assert(!isnan(driveshaft_torque));

	for ( int i = 0; i < WHEEL_POSITION_SIZE; i++ )
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

	for (int i = 0; i < WHEEL_POSITION_SIZE; i++)
		assert(!isnan(wheel_drive_torque[WHEEL_POSITION(i)]));
}

btScalar CARDYNAMICS::CalculateDriveshaftSpeed()
{
	btScalar driveshaft_speed = 0.0;
	btScalar left_front_wheel_speed = wheel[FRONT_LEFT].GetAngularVelocity();
	btScalar right_front_wheel_speed = wheel[FRONT_RIGHT].GetAngularVelocity();
	btScalar left_rear_wheel_speed = wheel[REAR_LEFT].GetAngularVelocity();
	btScalar right_rear_wheel_speed = wheel[REAR_RIGHT].GetAngularVelocity();
	
	for ( int i = 0; i < 4; i++ )
		assert ( !isnan ( wheel[WHEEL_POSITION ( i ) ].GetAngularVelocity() ) );
	
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

	// brakes fully engaged (declutch)
	if (brake[0].GetBrakeFactor() == 1.0) clutch = 0.0;

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
	if(remaining_shift_time > 0.0)
	{
		if(engine.GetRPM() < driveshaft_rpm && engine.GetRPM() < engine.GetRedline())
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
		if(driveshaft_rpm < DownshiftRPM(gear) && gear > 1)
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

///do traction control system (wheelspin prevention) calculations and modify the throttle position if necessary
void CARDYNAMICS::DoTCS(int i)
{
	//if (!WheelDriven(i)) return;

	btScalar gasthresh = 0.1;
	btScalar gas = engine.GetThrottle();

	//only active if throttle commanded past threshold
	if (gas > gasthresh)
	{
		//see if we're spinning faster than the rest of the wheels
		btScalar maxspindiff = 0;
		btScalar myrotationalspeed = wheel[i].GetAngularVelocity();
		for (int i2 = 0; i2 < WHEEL_POSITION_SIZE; i2++)
		{
			btScalar spindiff = myrotationalspeed - wheel[i2].GetAngularVelocity();
			if (spindiff < 0) spindiff = -spindiff;
			if (spindiff > maxspindiff) maxspindiff = spindiff;
		}

		//don't engage if all wheels are moving at the same rate
		if (maxspindiff > 1.0)
		{
			btScalar slide = tire[i].GetSlide() / tire[i].GetIdealSlide();
			if (transmission.GetGear() < 0.0) slide *= -1.0;

			if (slide > 1.0) tcs_active[i] = true;
			else if (slide < 0.5) tcs_active[i] = false;

			if (tcs_active[i])
			{
				btScalar curclutch = clutch.GetClutch();
				assert(curclutch <= 1 && curclutch >= 0);

				gas -= curclutch * (slide - 0.5);
				if (gas < 0) gas = 0;
				assert(gas <= 1);
				engine.SetThrottle(gas);
			}
		}
		else
		{
			tcs_active[i] = false;
		}
	}
	else
	{
		tcs_active[i] = false;
	}
}

///do anti-lock brake system calculations and modify the brake force if necessary
void CARDYNAMICS::DoABS(int i)
{
	btScalar braketresh = 0.1;
	btScalar brakesetting = brake[i].GetBrakeFactor();

	//only active if brakes commanded past threshold
	if (brakesetting > braketresh)
	{
		btScalar maxspeed = 0;
		for (int i2 = 0; i2 < WHEEL_POSITION_SIZE; i2++)
		{
			if (wheel[i2].GetAngularVelocity() > maxspeed)
				maxspeed = wheel[i2].GetAngularVelocity();
		}

		//don't engage ABS if all wheels are moving slowly
		if (maxspeed > 6.0)
		{
			btScalar sp = tire[i].GetIdealSlide();
			//btScalar ah = tire[i].GetIdealSlip();

			btScalar error = - tire[i].GetSlide() - sp;
			btScalar thresholdeng = 0.0;
			btScalar thresholddis = -sp/2.0;

			if (error > thresholdeng && !abs_active[i])
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

void CARDYNAMICS::AddMassParticle(const btScalar mass, const btVector3 & pos)
{
	mass_particles.push_back(std::pair<btScalar, btVector3>(mass, pos));
}
