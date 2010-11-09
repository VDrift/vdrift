#include "cardynamics.h"

#include "configfile.h"
#include "tracksurface.h"
#include "coordinatesystems.h"
#include "collision_world.h"
#include "tobullet.h"
#include "model.h"

#if defined(_WIN32) || defined(__APPLE__)
bool isnan(float number);
bool isnan(double number);
#endif

//#define _BULLET_
//#include "suspensionconstraint.h"

typedef CARDYNAMICS::T T;

CARDYNAMICS::CARDYNAMICS()
{
	Init();
}

void CARDYNAMICS::Init()
{
	world = NULL;
	chassis = NULL;
	drive = RWD;
	tacho_rpm = 0;
	autoclutch = true;
	autoshift = false;
	shifted = true;
	shift_gear = 0;
	last_auto_clutch = 1.0;
	remaining_shift_time = 0.0;
	abs = false;
	tcs = false;
	maxangle = 45.0;

#ifdef _BULLET_
	new_suspension.resize(WHEEL_POSITION_SIZE);
#endif
	suspension.resize ( WHEEL_POSITION_SIZE );
	wheel.resize ( WHEEL_POSITION_SIZE );
	tire.resize ( WHEEL_POSITION_SIZE );
	wheel_velocity.resize (WHEEL_POSITION_SIZE);
	wheel_position.resize ( WHEEL_POSITION_SIZE );
	wheel_orientation.resize ( WHEEL_POSITION_SIZE );
	wheel_contact.resize ( WHEEL_POSITION_SIZE );
	brake.resize ( WHEEL_POSITION_SIZE );
	abs_active.resize ( WHEEL_POSITION_SIZE, false );
	tcs_active.resize ( WHEEL_POSITION_SIZE, false );
}

bool LoadEngine(
	const CONFIGFILE & c,
	CARENGINE<T> & engine,
	std::ostream & error_output)
{
	CARENGINEINFO<T> info;
	std::vector < std::pair <T, T> > torque;
	float temp_vec3[3];

	if (!c.GetParam("engine.peak-engine-rpm", info.redline, error_output)) return false; //used only for the redline graphics
	if (!c.GetParam("engine.rpm-limit", info.rpm_limit, error_output)) return false;
	if (!c.GetParam("engine.inertia", info.inertia, error_output)) return false;
	if (!c.GetParam("engine.start-rpm", info.start_rpm, error_output)) return false;
	if (!c.GetParam("engine.stall-rpm", info.stall_rpm, error_output)) return false;
	if (!c.GetParam("engine.fuel-consumption", info.fuel_consumption, error_output)) return false;
	if (!c.GetParam("engine.mass", info.mass, error_output)) return false;
	if (!c.GetParam("engine.position", temp_vec3, error_output)) return false;

	COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(temp_vec3[0],temp_vec3[1],temp_vec3[2]);
	info.position.Set(temp_vec3[0],temp_vec3[1],temp_vec3[2]);

	int curve_num = 0;
	float torque_point[3];
	std::string torque_str("engine.torque-curve-00");
	while (c.GetParam(torque_str, torque_point))
	{
		torque.push_back(std::pair <float, float> (torque_point[0], torque_point[1]));

		curve_num++;
		std::stringstream str;
		str << "engine.torque-curve-";
		str.width(2);
		str.fill('0');
		str << curve_num;
		torque_str = str.str();
	}
	if (torque.size() <= 1)
	{
		error_output << "You must define at least 2 torque curve points." << std::endl;
		return false;
	}
	info.SetTorqueCurve(info.redline, torque);

	engine.Init(info);

	return true;
}

bool LoadClutch(
	const CONFIGFILE & c,
	CARCLUTCH<T> & clutch,
	std::ostream & error_output)
{
	float sliding, radius, area, max_pressure;

	if (!c.GetParam("clutch.sliding", sliding, error_output)) return false;
	if (!c.GetParam("clutch.radius", radius, error_output)) return false;
	if (!c.GetParam("clutch.area", area, error_output)) return false;
	if (!c.GetParam("clutch.max-pressure", max_pressure, error_output)) return false;

	clutch.SetSlidingFriction(sliding);
	clutch.SetRadius(radius);
	clutch.SetArea(area);
	clutch.SetMaxPressure(max_pressure);

	return true;
}

bool LoadTransmission(
	const CONFIGFILE & c,
	CARTRANSMISSION<T> & transmission,
	std::ostream & error_output)
{
	float shift_time = 0;
	float ratio;
	int gears;

	c.GetParam("transmission.shift-time", shift_time);
	transmission.SetShiftTime(shift_time);

	if (!c.GetParam("transmission.gear-ratio-r", ratio, error_output)) return false;
	transmission.SetGearRatio(-1, ratio);

	if (!c.GetParam("transmission.gears", gears, error_output)) return false;
	for (int i = 0; i < gears; i++)
	{
		std::stringstream s;
		s << "transmission.gear-ratio-" << i+1;
		if (!c.GetParam(s.str(), ratio, error_output)) return false;
		transmission.SetGearRatio(i+1, ratio);
	}

	return true;
}

bool LoadFuelTank(
	const CONFIGFILE & c,
	CARFUELTANK<T> & fuel_tank,
	std::ostream & error_output)
{
	float pos[3];
	MATHVECTOR <double, 3> position;
	float capacity;
	float volume;
	float fuel_density;

	if (!c.GetParam("fuel-tank.capacity", capacity, error_output)) return false;
	if (!c.GetParam("fuel-tank.volume", volume, error_output)) return false;
	if (!c.GetParam("fuel-tank.fuel-density", fuel_density, error_output)) return false;
	if (!c.GetParam("fuel-tank.position", pos, error_output)) return false;

	COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(pos[0],pos[1],pos[2]);
	position.Set(pos[0],pos[1],pos[2]);

	fuel_tank.SetCapacity(capacity);
	fuel_tank.SetVolume(volume);
	fuel_tank.SetDensity(fuel_density);
	fuel_tank.SetPosition(position);

	return true;
}

bool LoadCoilover(
	const CONFIGFILE & c,
	const std::string & id,
	CARSUSPENSIONINFO <T> & info,
	std::ostream & error_output)
{
	const std::string name("coilover-"+id);
	std::vector <std::pair <double, double> > damper_factor_points;
	std::vector <std::pair <double, double> > spring_factor_points;

	if (!c.GetParam(name+".spring-constant", info.spring_constant, error_output)) return false;
	if (!c.GetParam(name+".bounce", info.bounce, error_output)) return false;
	if (!c.GetParam(name+".rebound", info.rebound, error_output)) return false;
	if (!c.GetParam(name+".travel", info.travel, error_output)) return false;
	if (!c.GetParam(name+".anti-roll", info.anti_roll, error_output)) return false;
	c.GetPoints(name, "damper-factor", damper_factor_points);
	c.GetPoints(name, "spring-factor", spring_factor_points);
	info.SetDamperFactorPoints(damper_factor_points);
	info.SetSpringFactorPoints(spring_factor_points);

	return true;
}

bool LoadBrake(
	const CONFIGFILE & c,
	const std::string & id,
	CARBRAKE<T> & brake,
	std::ostream & error_output)
{
	const std::string name("brake-"+id);
	float friction, max_pressure, area, bias, radius, handbrake(0);

	if (!c.GetParam(name+".friction", friction, error_output)) return false;
	if (!c.GetParam(name+".area", area, error_output)) return false;
	if (!c.GetParam(name+".radius", radius, error_output)) return false;
	if (!c.GetParam(name+".bias", bias, error_output)) return false;
	if (!c.GetParam(name+".max-pressure", max_pressure, error_output)) return false;
	c.GetParam(name+".handbrake", handbrake);

	brake.SetFriction(friction);
	brake.SetArea(area);
	brake.SetRadius(radius);
	brake.SetBias(bias);
	brake.SetMaxPressure(max_pressure*bias);
	brake.SetHandbrake(handbrake);

	return true;
}

bool LoadTireParameters(
	const CONFIGFILE & c,
	const std::string & tire,
	CARTIREINFO <T> & info,
	std::ostream & error_output)
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
		std::stringstream str;
		str << ".a" << numinfile;
		float value;
		if (!c.GetParam(tire + str.str(), value, error_output)) return false;
		info.lateral[i] = value;
	}

	//read longitudinal, error_output)) return false;
	for (int i = 0; i < 11; i++)
	{
		std::stringstream str;
		str << ".b" << i;
		float value;
		if (!c.GetParam(tire + str.str(), value, error_output)) return false;
		info.longitudinal[i] = value;
	}

	//read aligning, error_output)) return false;
	for (int i = 0; i < 18; i++)
	{
		std::stringstream str;
		str << ".c" << i;
		float value;
		if (!c.GetParam(tire + str.str(), value, error_output)) return false;
		info.aligning[i] = value;
	}

	float rolling_resistance[3];
	if (!c.GetParam(tire + ".rolling-resistance", rolling_resistance, error_output)) return false;
	info.rolling_resistance_linear = rolling_resistance[0];
	info.rolling_resistance_quadratic = rolling_resistance[1];

	if (!c.GetParam(tire + ".tread", info.tread, error_output)) return false;

	return true;
}

bool LoadTire(
	const CONFIGFILE & c,
	const std::string & id,
	CARTIRE<T> & tire,
	std::ostream & error_output)
{
	CARTIREINFO<T> info;
	std::string size, type;
	if (!c.GetParam("tire-"+id+".size",size, error_output)) return false;
	if (!c.GetParam("tire-"+id+".type", type, error_output)) return false;
	if (!LoadTireParameters(c, type, info, error_output)) return false;
	if (!info.Parse(size, error_output)) return false;

	tire.Init(info);

	return true;
}

bool LoadSuspension(
	const CONFIGFILE & c,
	const std::string & id,
	CARSUSPENSION<T> & suspension,
	std::ostream & error_output)
{
	float h[3], p[3];
	CARSUSPENSIONINFO<T> info;

	if (!LoadCoilover(c, id, info, error_output)) return false;
	if (!c.GetParam("wheel-"+id+".position", p, error_output)) return false;
	if (!c.GetParam("wheel-"+id+".hinge", h, error_output)) return false;
	if (!c.GetParam("wheel-"+id+".camber", info.camber, error_output)) return false;
	if (!c.GetParam("wheel-"+id+".caster", info.caster, error_output)) return false;
	if (!c.GetParam("wheel-"+id+".toe", info.toe, error_output)) return false;
	c.GetParam("wheel-"+id+".steering", info.max_steering_angle);
	c.GetParam("wheel-"+id+".ackermann", info.ackermann);

	COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(h[0], h[1], h[2]);
	COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(p[0], p[1], p[2]);
	info.hinge.Set(h[0], h[1], h[2]);
	info.extended_position.Set(p[0], p[1], p[2]);

	suspension.Init(info);

	return true;
}

bool LoadWheel(
	const CONFIGFILE & c,
	const std::string & id,
	const CARTIRE<T> & tire,
	CARWHEEL<T> & wheel,
	std::ostream & error_output)
{
	float mass, inertia;
	if (!c.GetParam("wheel-"+id+".mass", mass) && !c.GetParam("wheel-"+id+".inertia", inertia))
	{
		float tire_radius = tire.GetRadius();
		float tire_width = tire.GetSidewallWidth();
		float tire_thickness = 0.05;
		float tire_density = 8E3;

		float rim_radius = tire_radius - tire_width * tire.GetAspectRatio();
		float rim_width = tire_width;
		float rim_thickness = 0.01;
		float rim_density = 3E5;

		float tire_volume = tire_width * M_PI * tire_thickness * tire_thickness * (2 * tire_radius  - tire_thickness);
		float rim_volume = rim_width * M_PI * rim_thickness * rim_thickness * (2 * rim_radius - rim_thickness);
		float tire_mass = tire_density * tire_volume;
		float rim_mass = rim_density * rim_volume;
		float tire_inertia = tire_mass * tire_radius * tire_radius;
		float rim_inertia = rim_mass * rim_radius * rim_radius;

		mass = tire_mass + rim_mass;
		inertia = (tire_inertia + rim_inertia) * 4;	// scale inertia fixme
	}
	wheel.SetMass(mass);
	wheel.SetInertia(inertia);

	return true;
}

bool LoadAeroDevices(
	const CONFIGFILE & c,
	std::vector< CARAERO <T> > & aerodynamics,
	std::ostream & error_output)
{
	const std::string aero[] = {"wing-front", "wing-center", "wing-rear"};
	for(int i = 0; i < 3; ++i)
	{
		float pos[] = {0, 0, 0};
		float drag_area, drag_coeff;
		float lift_area = 0, lift_coeff = 0, lift_eff = 0;

		if (!c.GetParam(aero[i] + ".frontal-area", drag_area)) break;
		if (!c.GetParam(aero[i] + ".drag-coefficient", drag_coeff, error_output)) return false;
		if (!c.GetParam(aero[i] + ".position", pos, error_output)) return false;
		c.GetParam(aero[i] + ".surface-area", lift_area);
		c.GetParam(aero[i] + ".lift-coefficient", lift_coeff);
		c.GetParam(aero[i] + ".efficiency", lift_eff);

		COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(pos[0],pos[1],pos[2]);
		MATHVECTOR <double, 3> position(pos[0], pos[1], pos[2]);

		aerodynamics.push_back(CARAERO<T>());
		aerodynamics.back().Set(position, drag_area, drag_coeff, lift_area, lift_coeff, lift_eff);
	}

	return true;
}

//load the differential(s)
bool LoadDifferential(
	const CONFIGFILE & c,
	const std::string & name,
	CARDIFFERENTIAL<T> & diff,
	std::ostream & error_output)
{
	T final_drive(1), anti_slip(0), anti_slip_torque(0), anti_slip_torque_deceleration_factor(0);

	if (!c.GetParam(name+".final-drive", final_drive, error_output)) return false;
	if (!c.GetParam(name+".anti-slip", anti_slip, error_output)) return false;
	c.GetParam(name+".anti-slip-torque", anti_slip_torque);
	c.GetParam(name+".anti-slip-torque-deceleration-factor", anti_slip_torque_deceleration_factor);

	diff.SetFinalDrive(final_drive);
	diff.SetAntiSlip(anti_slip, anti_slip_torque, anti_slip_torque_deceleration_factor);

	return true;
}

bool LoadMassParticles(
	const CONFIGFILE & c,
	std::list <std::pair <T, MATHVECTOR <T, 3> > > & mass_particles,
	std::ostream & error_output)
{
	int num = 0;
	while(true)
	{
		float mass;
		float pos[3];
		MATHVECTOR <double, 3> position;
		std::stringstream str;
		str.width(2);
		str.fill('0');
		str << num;
		std::string paramstr = "particle-"+str.str();

		if (!c.GetParam(paramstr+".mass", mass)) break;
		if (!c.GetParam(paramstr+".position", pos, error_output)) return false;

		COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(pos[0], pos[1], pos[2]);
		position.Set(pos[0], pos[1], pos[2]);

		mass_particles.push_back(std::pair <T, MATHVECTOR <T, 3> > (mass, position));

		num++;
	}

	return true;
}

bool CARDYNAMICS::Load(const CONFIGFILE & c, std::ostream & error_output)
{
	if (!LoadAeroDevices(c, aerodynamics, error_output)) return false;
	if (!LoadClutch(c, clutch, error_output)) return false;
	if (!LoadTransmission(c, transmission, error_output)) return false;
	if (!LoadEngine(c, engine, error_output)) return false;
	if (!LoadFuelTank(c, fuel_tank, error_output)) return false;
	AddMassParticle(engine.GetMass(), engine.GetPosition());

	maxangle = 0.0;
	const std::string id[] = {"fl", "fr", "rl", "rr"};
	for (int i = 0; i < WHEEL_POSITION_SIZE; ++i)
	{
		if (!LoadTire(c, id[i], tire[i], error_output)) return false;
		if (!LoadBrake(c, id[i], brake[i], error_output)) return false;
		if (!LoadSuspension(c, id[i], suspension[i], error_output)) return false;
		if (!LoadWheel(c, id[i], tire[i], wheel[i], error_output)) return false;
		if (suspension[i].GetMaxSteeringAngle() > maxangle) maxangle = suspension[i].GetMaxSteeringAngle();
		AddMassParticle(wheel[i].GetMass(), suspension[i].GetWheelPosition());
	}

	T temp;
	drive = NONE;
	if (c.GetParam("differential-front.final-drive", temp))
	{
		if (!LoadDifferential(c, "differential-front", differential_front, error_output)) return false;
		drive = FWD;
	}
	if (c.GetParam("differential-rear.final-drive", temp))
	{
		if (!LoadDifferential(c, "differential-rear", differential_rear, error_output)) return false;
		drive = (drive == NONE) ? RWD : AWD;
	}
	if (c.GetParam("differential-center.final-drive", temp))
	{
		if (!LoadDifferential(c, "differential-center", differential_center, error_output)) return false;
	}
	if (drive == NONE)
	{
		error_output << "No differential declared" << std::endl;
		return false;
	}

	// load driver mass, todo unify for all car components
	float mass;
	if (c.GetParam("driver.mass", mass))
	{
		float pos[3];
		if (!c.GetParam("driver.position", pos, error_output)) return false;
		COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(pos[0], pos[1], pos[2]);
		MATHVECTOR<T, 3> position(pos[0], pos[1], pos[2]);
		AddMassParticle(mass, position);
	}

	if (!LoadMassParticles(c, mass_particles, error_output)) return false;

	UpdateMass();

	// initialize telemetry
	telemetry.clear();
	//telemetry.push_back(CARTELEMETRY("brakes"));
	//telemetry.push_back(CARTELEMETRY("suspension"));
	//etc

	return true;
}

// calculate bounding box from chassis, wheels
void CARDYNAMICS::GetCollisionBox(
	const btVector3 & chassisMin,
	const btVector3 & chassisMax,
	btVector3 & center,
	btVector3 & size)
{
	btVector3 min = chassisMin - ToBulletVector(center_of_mass);
	btVector3 max = chassisMax - ToBulletVector(center_of_mass);
	float minHeight = min.z() + 0.05; // add collision shape bottom margin
	for (int i = 0; i < 4; i++)
	{
		btVector3 wheelHSize(tire[i].GetRadius(), tire[i].GetSidewallWidth()*0.5, tire[i].GetRadius());
		btVector3 wheelPos = ToBulletVector(suspension[i].GetWheelPosition(0.0));
		btVector3 wheelMin = wheelPos - wheelHSize;
		btVector3 wheelMax = wheelPos + wheelHSize;
		min.setMin(wheelMin);
		max.setMax(wheelMax);
	}
	min.setZ(minHeight);

	center = (max + min) * 0.5;
	size = max - min;
}

// create collision shape from bounding box
btCollisionShape * CreateCollisionShape(const btVector3 & center, const btVector3 & size)
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
	btMultiSphereShape * shape = new btMultiSphereShape(positions, radii, numSpheres);
	return shape;
}

void CARDYNAMICS::Init(
	COLLISION_WORLD & world,
	MATHVECTOR <T, 3> chassisSize,
	MATHVECTOR <T, 3> chassisCenter,
	MATHVECTOR <T, 3> position,
	QUATERNION <T> orientation)
{
	this->world = &world;

	position = position - center_of_mass;
	orientation.Normalize();

	body.SetPosition(position);
	body.SetOrientation(orientation);

	// init chassis
	T chassisMass = body.GetMass();
	MATRIX3 <T> inertia = body.GetInertia();
	//MATRIX3 <T> inertia_basis;
	//MATHVECTOR <T, 3> inertia_vector;
	//MATRIX3<T>::Diagonalize(inertia, inertia_basis, inertia_vector);
	btVector3 chassisInertia(inertia[0], inertia[4], inertia[8]);

	btTransform transform;
	transform.setOrigin(ToBulletVector(position));
	transform.setRotation(ToBulletQuaternion(orientation));
	btDefaultMotionState * chassisState = new btDefaultMotionState();
	chassisState->setWorldTransform(transform);

	btVector3 chassisMin = ToBulletVector(chassisCenter - chassisSize * 0.5);
	btVector3 chassisMax = ToBulletVector(chassisCenter + chassisSize * 0.5);
	btVector3 origin, size;
	GetCollisionBox(chassisMin, chassisMax, origin, size);

	btCollisionShape * chassisShape = NULL;
	chassisShape = CreateCollisionShape(origin, size);

	// create rigid body
	btRigidBody::btRigidBodyConstructionInfo info(chassisMass, chassisState, chassisShape, chassisInertia);
	info.m_angularDamping = 0.5;
	info.m_friction = 0.5;
	chassis = world.AddRigidBody(info);
	chassis->setContactProcessingThreshold(0.0); // internal edge workaround(swept sphere shape required)
	world.AddAction(this);

	// init wheels
	for (int i = 0; i < WHEEL_POSITION_SIZE; i++)
	{
		wheel_velocity[i].Set(0.0);
		wheel_position[i] = LocalToWorld(suspension[i].GetWheelPosition(0.0));
		wheel_orientation[i] = Orientation() * suspension[i].GetWheelOrientation();
#ifdef _BULLET_
		new_suspension[i] = new SuspensionConstraint(world, *chassis);
		new_suspension[i]->setPosition(ToBulletVector(suspension[i].GetWheelPosition(0.0)));
		new_suspension[i]->setTravel(suspension[i].GetTravel());
		new_suspension[i]->setOffset(tire[i].GetRadius());
		new_suspension[i]->setStiffness(10000);//25000);
		new_suspension[i]->setDamping(5000);
		world.AddConstraint(new_suspension[i]);
#endif
	}

	AlignWithGround();
}

// executed as last function(after integration) in bullet singlestepsimulation
void CARDYNAMICS::updateAction(btCollisionWorld * collisionWorld, btScalar dt)
{
#ifndef _BULLET_
	// get external force torque
	MATHVECTOR<T, 3> v0 = body.GetVelocity();
	MATHVECTOR<T, 3> w0 = body.GetAngularVelocity();
	MATHVECTOR<T, 3> v1 = ToMathVector<T>(chassis->getLinearVelocity());
	MATHVECTOR<T, 3> w1 = ToMathVector<T>(chassis->getAngularVelocity());
	MATHVECTOR<T, 3> dv = v1 - v0;
	MATHVECTOR<T, 3> dw = w1 - w0;
	MATHVECTOR<T, 3> ext_force = dv * body.GetMass() / dt;
	MATHVECTOR<T, 3> ext_torque = body.GetWorldInertia().Multiply(w1 - w0) / dt;

	// wheel ray cast
	UpdateWheelContacts();

	// run internal simulation
	Tick(ext_force, ext_torque, dt);

	// synchronize bullet
	btVector3 bv = ToBulletVector(body.GetVelocity());
	btVector3 bw = ToBulletVector(body.GetAngularVelocity());
	btVector3 bp = ToBulletVector(body.GetPosition());
	btQuaternion bq = ToBulletQuaternion(body.GetOrientation());
	btTransform bt(bq, bp);
	chassis->setLinearVelocity(bv);
	chassis->setAngularVelocity(bw);
	chassis->setCenterOfMassTransform(bt);
#endif
}

void CARDYNAMICS::debugDraw(btIDebugDraw* debugDrawer)
{

}

void CARDYNAMICS::Update()
{
	btTransform chassisTrans;
	chassis->getMotionState()->getWorldTransform(chassisTrans);
	chassisRotation = ToMathQuaternion<T>(chassisTrans.getRotation());
	chassisCenterOfMass = ToMathVector<T>(chassisTrans.getOrigin());
	MATHVECTOR <T, 3> com = center_of_mass;
	chassisRotation.RotateVector(com);
	chassisPosition = chassisCenterOfMass - com;
}

const MATHVECTOR <T, 3> & CARDYNAMICS::GetCenterOfMassPosition() const
{
	return chassisCenterOfMass;
}

const MATHVECTOR <T, 3> & CARDYNAMICS::GetPosition() const
{
	return chassisPosition;
}

const QUATERNION <T> & CARDYNAMICS::GetOrientation() const
{
	return chassisRotation;
}

MATHVECTOR <T, 3> CARDYNAMICS::GetWheelPosition(WHEEL_POSITION wp) const
{
	MATHVECTOR <T, 3> pos = suspension[wp].GetWheelPosition();
	chassisRotation.RotateVector(pos);
	return pos + chassisPosition;
}

MATHVECTOR <T, 3> CARDYNAMICS::GetWheelPosition(WHEEL_POSITION wp, T displacement_fraction) const
{
	MATHVECTOR <T, 3> pos = suspension[wp].GetWheelPosition(displacement_fraction);
	chassisRotation.RotateVector(pos);
	return pos + chassisPosition;
}

QUATERNION <T> CARDYNAMICS::GetWheelOrientation(WHEEL_POSITION wp) const
{
	return chassisRotation * suspension[wp].GetWheelOrientation() * wheel[wp].GetRotation();
}

QUATERNION <T> CARDYNAMICS::GetUprightOrientation(WHEEL_POSITION wp) const
{
	return chassisRotation * suspension[wp].GetWheelOrientation();
}

/// worldspace wheel center position
MATHVECTOR <T, 3> CARDYNAMICS::GetWheelVelocity(WHEEL_POSITION wp) const
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

float CARDYNAMICS::GetMass() const
{
	return body.GetMass();
}

T CARDYNAMICS::GetSpeed() const
{
	return GetVelocity().Magnitude();
}

MATHVECTOR <T, 3> CARDYNAMICS::GetVelocity() const
{
#ifdef _BULLET_
	return ToMathVector<T>(chassis->getLinearVelocity());
#else
	return body.GetVelocity();
#endif
}

MATHVECTOR <T, 3> CARDYNAMICS::GetEnginePosition() const
{
	MATHVECTOR <T, 3> offset = engine.GetPosition();
	Orientation().RotateVector(offset);
	return offset + chassisPosition;
}

void CARDYNAMICS::StartEngine()
{
	engine.StartEngine();
}

void CARDYNAMICS::ShiftGear(int value)
{
	if (transmission.GetGear() != value && shifted)
	{
		if (value <= transmission.GetForwardGears() && value >= -transmission.GetReverseGears())
		{
			remaining_shift_time = transmission.GetShiftTime();
			shift_gear = value;
			shifted = false;
		}
	}
}

void CARDYNAMICS::SetThrottle(float value)
{
	engine.SetThrottle(value);
}

void CARDYNAMICS::SetClutch(float value)
{
	clutch.SetClutch(value);
}

void CARDYNAMICS::SetBrake(float value)
{
	for(unsigned int i = 0; i < brake.size(); i++)
	{
		brake[i].SetBrakeFactor(value);
	}
}

void CARDYNAMICS::SetHandBrake(float value)
{
	for(unsigned int i = 0; i < brake.size(); i++)
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

T CARDYNAMICS::GetSpeedMPS() const
{
	T left_front_wheel_speed = wheel[FRONT_LEFT].GetAngularVelocity();
	T right_front_wheel_speed = wheel[FRONT_RIGHT].GetAngularVelocity();
	T left_rear_wheel_speed = wheel[REAR_LEFT].GetAngularVelocity();
	T right_rear_wheel_speed = wheel[REAR_RIGHT].GetAngularVelocity();
	for ( int i = 0; i < 4; i++ ) assert ( !isnan ( wheel[WHEEL_POSITION ( i ) ].GetAngularVelocity() ) );
	if ( drive == RWD )
	{
		return ( left_rear_wheel_speed+right_rear_wheel_speed ) * 0.5 * tire[REAR_LEFT].GetRadius();
	}
	else if ( drive == FWD )
	{
		return ( left_front_wheel_speed+right_front_wheel_speed ) * 0.5 * tire[FRONT_LEFT].GetRadius();
	}
	else if ( drive == AWD )
	{
		return ( ( left_rear_wheel_speed+right_rear_wheel_speed ) * 0.5 * tire[REAR_LEFT].GetRadius() +
		         ( left_front_wheel_speed+right_front_wheel_speed ) * 0.5 * tire[FRONT_LEFT].GetRadius() ) *0.5;
	}

	assert ( 0 );
	return 0;
/*
	// use max wheel speed
	T speed = 0;
	for (int i = 0; i < WHEEL_POSITION_SIZE; ++i)
	{
		T wheel_speed = wheel[i].GetAngularVelocity() * tire[i].GetRadius();
		if(speed < wheel_speed) speed = wheel_speed;
	}
	return speed;
*/
}

T CARDYNAMICS::GetTachoRPM() const
{
	return tacho_rpm;
}

void CARDYNAMICS::SetABS ( const bool newabs )
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

void CARDYNAMICS::SetPosition(const MATHVECTOR<T, 3> & position)
{
	body.SetPosition(position);
	//chassis->translate(ToBulletVector(position) - chassis->getCenterOfMassPosition());
}

void CARDYNAMICS::AlignWithGround()
{
	UpdateWheelTransform();
	UpdateWheelContacts();

	T min_height = 0;
	bool no_min_height = true;
	for (int i = 0; i < WHEEL_POSITION_SIZE; i++)
	{
		T height = wheel_contact[i].GetDepth() - 2 * tire[i].GetRadius();
		if (height < min_height || no_min_height)
		{
			min_height = height;
			no_min_height = false;
		}
	}

	MATHVECTOR <T, 3> delta = GetDownVector() * min_height;
	MATHVECTOR <T, 3> trimmed_position = Position() + delta;
	SetPosition(trimmed_position);

	UpdateWheelTransform();
	UpdateWheelContacts();
}

// ugh, ugly code
void CARDYNAMICS::RolloverRecover()
{
	btQuaternion rot(0, 0, 0, 1);
	btTransform transform = chassis->getCenterOfMassTransform();

	btVector3 z(0, 0, 1);
	btVector3 y_car = transform.getBasis().getColumn(0);
	y_car = y_car - z * z.dot(y_car);
	y_car.normalize();

	btVector3 z_car = transform.getBasis().getColumn(2);
	z_car = z_car - y_car * y_car.dot(z_car);
	z_car.normalize();

	T angle = z_car.angle(z);
	if (fabs(angle) < M_PI / 4.0) return;

	rot.setRotation(y_car, angle);
	rot = rot * transform.getRotation();

	transform.setRotation(rot);
	chassis->setCenterOfMassTransform(transform);

	body.SetOrientation(ToMathQuaternion<T>(rot));

	AlignWithGround();
}

void CARDYNAMICS::SetSteering(const T value)
{
	for(int i = 0; i < WHEEL_POSITION_SIZE; i++)
	{
		suspension[i].SetSteering(value);
	}
}

T CARDYNAMICS::GetMaxSteeringAngle() const
{
	return maxangle;
}

MATHVECTOR <T, 3> CARDYNAMICS::GetTotalAero() const
{
	MATHVECTOR <T, 3> downforce = 0;
	for ( std::vector <CARAERO<T> >::const_iterator i = aerodynamics.begin(); i != aerodynamics.end(); ++i )
	{
		downforce = downforce + i->GetLiftVector() +  i->GetDragVector();
	}
	return downforce;
}

T CARDYNAMICS::GetAerodynamicDownforceCoefficient() const
{
	T coeff = 0.0;
	for ( std::vector <CARAERO <T> >::const_iterator i = aerodynamics.begin(); i != aerodynamics.end(); ++i )
		coeff += i->GetAerodynamicDownforceCoefficient();
	return coeff;
}

T CARDYNAMICS::GetAeordynamicDragCoefficient() const
{
	T coeff = 0.0;
	for ( std::vector <CARAERO <T> >::const_iterator i = aerodynamics.begin(); i != aerodynamics.end(); ++i )
		coeff += i->GetAeordynamicDragCoefficient();
	return coeff;
}

MATHVECTOR< T, 3 > CARDYNAMICS::GetLastBodyForce() const
{
	return lastbodyforce;
}

T CARDYNAMICS::GetFeedback() const
{
	return feedback;
}

void CARDYNAMICS::UpdateTelemetry ( float dt )
{
	for (std::list <CARTELEMETRY>::iterator i = telemetry.begin(); i != telemetry.end(); i++)
		i->Update ( dt );
}

/// print debug info to the given ostream.  set p1, p2, etc if debug info part 1, and/or part 2, etc is desired
void CARDYNAMICS::DebugPrint ( std::ostream & out, bool p1, bool p2, bool p3, bool p4 ) const
{
	if ( p1 )
	{
		out << std::fixed << std::setprecision(3);
		out << "---Body---\n";
		out << "Position: " << chassisPosition << "\n";
		out << "Center of mass: " << center_of_mass << "\n";
		out << "Total mass: " << body.GetMass() << "\n";
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
		suspension[FRONT_LEFT].DebugPrint ( out );
		out << "\n";
		out << "(front right)" << "\n";
		suspension[FRONT_RIGHT].DebugPrint ( out );
		out << "\n";
		out << "(rear left)" << "\n";
		suspension[REAR_LEFT].DebugPrint ( out );
		out << "\n";
		out << "(rear right)" << "\n";
		suspension[REAR_RIGHT].DebugPrint ( out );
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
		for ( std::vector <CARAERO<T> >::const_iterator i = aerodynamics.begin(); i != aerodynamics.end(); ++i )
		{
			i->DebugPrint ( out );
			out <<  "\n";
		}
	}
}

bool CARDYNAMICS::Serialize ( joeserialize::Serializer & s )
{
	_SERIALIZE_(s,body);
	_SERIALIZE_(s,engine);
	_SERIALIZE_(s,clutch);
	_SERIALIZE_(s,transmission);
	_SERIALIZE_(s,differential_front);
	_SERIALIZE_(s,differential_rear);
	_SERIALIZE_(s,differential_center);
	_SERIALIZE_(s,fuel_tank);
	_SERIALIZE_(s,suspension);
	_SERIALIZE_(s,wheel);
	_SERIALIZE_(s,brake);
	_SERIALIZE_(s,tire);
	_SERIALIZE_(s,aerodynamics);
	_SERIALIZE_(s,wheel_velocity);
	_SERIALIZE_(s,abs);
	_SERIALIZE_(s,abs_active);
	_SERIALIZE_(s,tcs);
	_SERIALIZE_(s,tcs_active);
	_SERIALIZE_(s,last_auto_clutch);
	_SERIALIZE_(s,remaining_shift_time);
	_SERIALIZE_(s,shift_gear);
	_SERIALIZE_(s,shifted);
	_SERIALIZE_(s,autoshift);
	return true;
}

MATHVECTOR <T, 3> CARDYNAMICS::GetDownVector() const
{
	MATHVECTOR <T, 3> v(0, 0, -1);
	Orientation().RotateVector(v);
	return v;
}

QUATERNION <T> CARDYNAMICS::Orientation() const
{
#ifdef _BULLET_
	return ToMathQuaternion<T>(chassis->getOrientation());
#else
	return body.GetOrientation();
#endif
}

MATHVECTOR <T, 3> CARDYNAMICS::Position() const
{
#ifdef _BULLET_
	return ToMathVector<T>(chassis->getCenterOfMassPosition());
#else
	return body.GetPosition();
#endif
}

MATHVECTOR <T, 3> CARDYNAMICS::LocalToWorld(const MATHVECTOR <T, 3> & local) const
{
#ifdef _BULLET_
	btVector3 position = chassis->getCenterOfMassTransform().getBasis() * ToBulletVector(local - center_of_mass);
	position = position + chassis->getCenterOfMassTransform().getOrigin();
	return ToMathVector <T> (position);
#else
	MATHVECTOR <T,3> position = local - center_of_mass;
	body.GetOrientation().RotateVector(position);
	return position + body.GetPosition();
#endif
}

void CARDYNAMICS::ApplyForce(const MATHVECTOR <T, 3> & force)
{
#ifdef _BULLET_
	chassis->applyCentralForce(ToBulletVector(force));
#else
	body.ApplyForce(force);
#endif
}

void CARDYNAMICS::ApplyForce(const MATHVECTOR <T, 3> & force, const MATHVECTOR <T, 3> & offset)
{
#ifdef _BULLET_
	chassis->applyForce(ToBulletVector(force), ToBulletVector(offset));
#else
	body.ApplyForce(force, offset);
#endif
}

void CARDYNAMICS::ApplyTorque(const MATHVECTOR <T, 3> & torque)
{
#ifdef _BULLET_
	if(torque.MagnitudeSquared() > 1E-6)
		chassis->applyTorque(ToBulletVector(torque));
#else
	body.ApplyTorque(torque);
#endif
}

void CARDYNAMICS::UpdateWheelVelocity()
{
	for(int i = 0; i < WHEEL_POSITION_SIZE; i++)
	{
#ifdef _BULLET_
		btVector3 offset = ToBulletVector(wheel_position[i]) - chassis->getCenterOfMassPosition();
		wheel_velocity[i] = ToMathVector<T>(chassis->getVelocityInLocalPoint(offset));
#else
		wheel_velocity[i] = body.GetVelocity(wheel_position[i] - body.GetPosition());
#endif
	}
}

void CARDYNAMICS::UpdateWheelTransform()
{
	for(int i = 0; i < WHEEL_POSITION_SIZE; i++)
	{
		wheel_position[i] = LocalToWorld(suspension[i].GetWheelPosition());
		wheel_orientation[i] = Orientation() * suspension[i].GetWheelOrientation();
	}
}

void CARDYNAMICS::ApplyEngineTorqueToBody()
{
	MATHVECTOR <T, 3> engine_torque(-engine.GetTorque(), 0, 0);
	Orientation().RotateVector(engine_torque);
	ApplyTorque(engine_torque);
}

void CARDYNAMICS::AddAerodynamics(MATHVECTOR<T, 3> & force, MATHVECTOR<T, 3> & torque)
{
	MATHVECTOR <T, 3> wind_force(0);
	MATHVECTOR <T, 3> wind_torque(0);
	MATHVECTOR <T, 3> air_velocity = -GetVelocity();
	(-Orientation()).RotateVector(air_velocity);
	for(std::vector <CARAERO <T> >::iterator i = aerodynamics.begin(); i != aerodynamics.end(); ++i)
	{
		MATHVECTOR <T, 3> force = i->GetForce(air_velocity);
		wind_force = wind_force + force;
		wind_torque = wind_torque + (i->GetPosition() - center_of_mass).cross(force);
	}
	Orientation().RotateVector(wind_force);
	Orientation().RotateVector(wind_torque);
	force = force + wind_force;
	torque = torque + wind_torque;
}

// returns normal force(wheel force)
T CARDYNAMICS::UpdateSuspension(int i, T dt)
{
	// velocity, displacment along wheel ray
	T velocity = 0;//-GetDownVector().dot(wheel_velocity[i]);
	T displacement = 2.0 * tire[i].GetRadius() - wheel_contact[i].GetDepth();

	// adjust displacement due to surface bumpiness
	const TRACKSURFACE & surface = wheel_contact[i].GetSurface();
	if (surface.bumpWaveLength > 0.0001)
	{
		T posx = wheel_contact[i].GetPosition()[0];
		T posz = wheel_contact[i].GetPosition()[2];
		T phase = 2.0 * 3.141593 * (posx + posz) / surface.bumpWaveLength;
		T shift = 2.0 * sin(phase * 1.414214);
		T amplitude = 0.25 * surface.bumpAmplitude;
		T bumpoffset = amplitude * (sin(phase + shift) + sin(1.414214 * phase) - 2.0);
		displacement += bumpoffset;
	}

	suspension[i].Update(displacement, velocity, dt);

	int otheri = i;
	if ( i == 0 || i == 2 ) otheri++;
	else otheri--;
	T antirollforce = suspension[i].GetAntiRoll() * (suspension[i].GetDisplacement() - suspension[otheri].GetDisplacement());

	T suspension_force = suspension[i].GetForce() + antirollforce;
	if (suspension_force < 0.0) suspension_force = 0.0;
	assert(suspension_force == suspension_force);

	// overtravel constraint (calculate impulse to reduce relative velocity to zero)
	if (suspension[i].GetOvertravel() > 0.0)
	{
		MATHVECTOR <T, 3> normal = wheel_contact[i].GetNormal();
		MATHVECTOR <T, 3> offset = wheel_contact[i].GetPosition() - body.GetPosition();

		T velocity_error = wheel_velocity[i].dot(normal);
		if (velocity_error < 0.0)
		{
			T mass = 1.0 / body.GetInvEffectiveMass(normal, offset);
			T impulse = -velocity_error * mass;
			T constraint_force = impulse / dt;
			if (constraint_force > suspension_force) suspension_force = constraint_force;
		}
	}

	return suspension_force;
}

T CARDYNAMICS::ApplyTireForce(int i, const T normal_force, const QUATERNION <T> & wheel_space)
{
	CARWHEEL<T> & wheel = this->wheel[i];
	CARTIRE<T> & tire = this->tire[i];
	const COLLISION_CONTACT & wheel_contact = this->wheel_contact[i];
	const TRACKSURFACE & surface = wheel_contact.GetSurface();
	const MATHVECTOR <T, 3> surface_normal = wheel_contact.GetNormal();

	// spin axis is the wheel plane normal
	// positive inclination is in clockwise direction
	MATHVECTOR <T, 3> spin_axis(0, 1, 0);
	wheel_space.RotateVector(spin_axis);
	T axis_proj = spin_axis.dot(surface_normal);
	T inclination = 90 - acos(axis_proj)  * 180.0 / M_PI;
	if (!(i&1)) inclination = -inclination; // ????

	// tire space(SAE Tire Coordinate System)
	// surface normal is negative z-axis
	// negative spin axis projected onto surface plane is y-axis
	MATHVECTOR <T, 3> y = -(spin_axis - surface_normal * axis_proj).Normalize();
	MATHVECTOR <T, 3> x = -y.cross(surface_normal);

	// wheel velocity in tire space
	MATHVECTOR <T, 3> velocity;
	velocity[0] = x.dot(wheel_velocity[i]);
	velocity[1] = y.dot(wheel_velocity[i]);

	// wheel angular velocity
	T ang_velocity = wheel.GetAngularVelocity();

	// friction force in tire space
	T friction_coeff = tire.GetTread() * surface.frictionTread + (1.0 - tire.GetTread()) * surface.frictionNonTread;
	MATHVECTOR <T, 3> friction_force(0);
	if(friction_coeff > 0)
	{
		friction_force = tire.GetForce(normal_force, friction_coeff, velocity, ang_velocity, inclination);
	}

	// rolling resistance due to tire/surface deformation proportional to normal force
	T roll_friction_coeff = surface.rollResistanceCoefficient;
	MATHVECTOR <T, 3> roll_resistance = x * tire.GetRollingResistance(velocity[0], normal_force, roll_friction_coeff);
	MATHVECTOR <T, 3> rel_wheel_pos = wheel_position[i] - Position();
	ApplyForce(roll_resistance, rel_wheel_pos);

	// friction force in world space
	MATHVECTOR <T, 3> tire_friction = x * friction_force[0] + y * friction_force[1];

	// fake viscous friction (sand, gravel, mud) proportional to wheel center velocity
	MATHVECTOR <T, 3> wheel_drag = -(x * velocity[0] + y * velocity[1]) * surface.rollingDrag * 0.25; // scale wheel drag by 1/4

	// tire friction + tire normal force
	MATHVECTOR <T, 3> rel_contact_pos = wheel_contact.GetPosition() - Position();
	ApplyForce(surface_normal * normal_force + tire_friction + wheel_drag, rel_contact_pos);

	return friction_force[0];
}

T CARDYNAMICS::CalculateWheelTorque(
	int i,
	const T tire_friction,
	T drive_torque,
	T dt)
{
	CARWHEEL<T> & wheel = this->wheel[i];
	CARTIRE<T> & tire = this->tire[i];
	CARBRAKE<T> & brake = this->brake[i];

	wheel.Integrate1(dt);

	// torques acting on wheel
	T friction_torque = tire_friction * tire.GetRadius();
	T wheel_torque = drive_torque - friction_torque;
	T lock_up_torque = wheel.GetLockUpTorque(dt) - wheel_torque;	// torque needed to lock the wheel
	T brake_torque = brake.GetTorque();

	// brake and rolling resistance torque should never exceed lock up torque
	if(lock_up_torque >= 0 && lock_up_torque > brake_torque)
	{
		brake.WillLock(false);
		wheel_torque += brake_torque;   // brake torque has same direction as lock up torque
	}
	else if(lock_up_torque < 0 && lock_up_torque < -brake_torque)
	{
		brake.WillLock(false);
		wheel_torque -= brake_torque;
	}
	else
	{
		brake.WillLock(true);
		wheel_torque = wheel.GetLockUpTorque(dt);
	}

	wheel.SetTorque(wheel_torque);
	wheel.Integrate2(dt);

	return wheel_torque;
}

void CARDYNAMICS::UpdateBody(
	const MATHVECTOR <T, 3> & ext_force,
	const MATHVECTOR <T, 3> & ext_torque,
	T drive_torque[],
	T dt)
{
	body.Integrate1(dt);

	ApplyEngineTorqueToBody();

	// apply external force/torque
	body.ApplyForce(ext_force);
	body.ApplyTorque(ext_torque);

	// update suspension/wheels
	//static int count = 0;
	T normal_force[WHEEL_POSITION_SIZE];
	for(int i = 0; i < WHEEL_POSITION_SIZE; i++)
	{
		normal_force[i] = UpdateSuspension(i, dt);
		T tire_friction = ApplyTireForce(i, normal_force[i], wheel_orientation[i]);
		T wheel_torque = CalculateWheelTorque(i, tire_friction, drive_torque[i], dt);

		// apply wheel torque to body
		MATHVECTOR <T, 3> world_wheel_torque(0, -wheel_torque, 0);
		wheel_orientation[i].RotateVector(world_wheel_torque);
		ApplyTorque(world_wheel_torque);

		//std::cerr << std::setw(4) << count << "; " << std::setw(7) << tire_friction[0] << " ";
	}
	//std::cerr << "\n";
	//count++;

	body.Integrate2(dt);

	UpdateWheelVelocity();
	UpdateWheelTransform();
	InterpolateWheelContacts();

	for(int i = 0; i < WHEEL_POSITION_SIZE; i++)
	{
		if (abs) DoABS(i, normal_force[i]);
		if (tcs) DoTCS(i, normal_force[i]);
	}
}

void CARDYNAMICS::Tick(MATHVECTOR<T, 3> ext_force, MATHVECTOR<T, 3> ext_torque, T dt)
{
	// has to happen before UpdateDriveline, overrides clutch, throttle
	UpdateTransmission(dt);

	AddAerodynamics(ext_force, ext_torque);

	const int num_repeats = 10;
	const float internal_dt = dt / num_repeats;
	for(int i = 0; i < num_repeats; ++i)
	{
		T drive_torque[WHEEL_POSITION_SIZE];

		UpdateDriveline(drive_torque, internal_dt);

		UpdateBody(ext_force, ext_torque, drive_torque, internal_dt);

		feedback += 0.5 * (tire[FRONT_LEFT].GetFeedback() + tire[FRONT_RIGHT].GetFeedback());
	}

	feedback /= (num_repeats + 1);

	fuel_tank.Consume(engine.FuelRate() * dt);
	engine.SetOutOfGas(fuel_tank.Empty());

	const float tacho_factor = 0.1;
	tacho_rpm = engine.GetRPM() * tacho_factor + tacho_rpm * (1.0 - tacho_factor);

	UpdateTelemetry(dt);
}

void CARDYNAMICS::UpdateWheelContacts()
{
	MATHVECTOR <float, 3> raydir = GetDownVector();
	for (int i = 0; i < WHEEL_POSITION_SIZE; i++)
	{
		COLLISION_CONTACT & wheelContact = wheel_contact[WHEEL_POSITION(i)];
		MATHVECTOR <float, 3> raystart = wheel_position[i];
		raystart = raystart - raydir * tire[i].GetRadius();
		float raylen = 4;
		world->CastRay(raystart, raydir, raylen, chassis, wheelContact);
	}
}

void CARDYNAMICS::InterpolateWheelContacts()
{
	MATHVECTOR <float, 3> raydir = GetDownVector();
	for (int i = 0; i < WHEEL_POSITION_SIZE; i++)
	{
		MATHVECTOR <float, 3> raystart = wheel_position[i];
		raystart = raystart - raydir * tire[i].GetRadius();
		float raylen = 4;
		GetWheelContact(WHEEL_POSITION(i)).CastRay(raystart, raydir, raylen);
	}
}

///calculate the center of mass, calculate the total mass of the body, calculate the inertia tensor
/// then store this information in the rigid body
void CARDYNAMICS::UpdateMass()
{
	typedef std::pair <T, MATHVECTOR <T, 3> > MASS_PAIR;

	T total_mass(0);
	center_of_mass.Set(0, 0, 0);

	//calculate the total mass, and center of mass
	for (std::list <MASS_PAIR>::iterator i = mass_particles.begin(); i != mass_particles.end(); ++i )
	{
		//add the current mass to the total mass
		total_mass += i->first;

		//incorporate the current mass into the center of mass
		center_of_mass = center_of_mass + i->second * i->first;
	}

	//account for fuel
	total_mass += fuel_tank.GetMass();
	center_of_mass = center_of_mass + fuel_tank.GetPosition() * fuel_tank.GetMass();

	body.SetMass(total_mass);

	center_of_mass = center_of_mass * (1.0 / total_mass);

	//calculate the inertia tensor (is symmetric)
	MATRIX3 <T> inertia;
	for (int i = 0; i < 9; ++i) inertia[i] = 0;
	for (std::list <MASS_PAIR>::iterator i = mass_particles.begin(); i != mass_particles.end(); ++i)
	{
		//transform into the rigid body coordinates
		MATHVECTOR <T, 3> position = i->second - center_of_mass;
		T mass = i->first;

		//add the current mass to the inertia tensor
		inertia[0] += mass * ( position[1] * position[1] + position[2] * position[2] ); //mi*(yi^2+zi^2)
		inertia[1] -= mass * ( position[0] * position[1] ); //-mi*xi*yi
		inertia[2] -= mass * ( position[0] * position[2] ); //-mi*xi*zi
		inertia[3] = inertia[1];
		inertia[4] += mass * ( position[2] * position[2] + position[0] * position[0] ); //mi*(xi^2+zi^2)
		inertia[5] -= mass * ( position[1] * position[2] ); //-mi*yi*zi
		inertia[6] = inertia[2];
		inertia[7] = inertia[5];
		inertia[8] += mass * ( position[0] * position[0] + position[1] * position[1] ); //mi*(xi^2+yi^2)
	}
	//inertia.DebugPrint(std::cout);
	body.SetInertia(inertia);
}

void CARDYNAMICS::UpdateDriveline(T drive_torque[], T dt)
{
	engine.Integrate1(dt);

	T driveshaft_speed = CalculateDriveshaftSpeed();
	T clutch_speed = transmission.CalculateClutchSpeed(driveshaft_speed);
	T crankshaft_speed = engine.GetAngularVelocity();
	T clutch_drag = clutch.GetTorqueMax(crankshaft_speed, clutch_speed);

	if(transmission.GetGear() == 0) clutch_drag = 0;

	clutch_drag = engine.ComputeForces(clutch_drag, clutch_speed, dt);

	CalculateDriveTorque(drive_torque, -clutch_drag);

	engine.Integrate2(dt);
}

///calculate the drive torque that the engine applies to each wheel, and put the output into the supplied 4-element array
void CARDYNAMICS::CalculateDriveTorque(T * wheel_drive_torque, T clutch_torque)
{
	T driveshaft_torque = transmission.GetTorque(clutch_torque);
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

	for (int i = 0; i < WHEEL_POSITION_SIZE; i++) assert(!isnan(wheel_drive_torque[WHEEL_POSITION(i)]));
}

T CARDYNAMICS::CalculateDriveshaftSpeed()
{
	T driveshaft_speed = 0.0;
	T left_front_wheel_speed = wheel[FRONT_LEFT].GetAngularVelocity();
	T right_front_wheel_speed = wheel[FRONT_RIGHT].GetAngularVelocity();
	T left_rear_wheel_speed = wheel[REAR_LEFT].GetAngularVelocity();
	T right_rear_wheel_speed = wheel[REAR_RIGHT].GetAngularVelocity();
	for ( int i = 0; i < 4; i++ ) assert ( !isnan ( wheel[WHEEL_POSITION ( i ) ].GetAngularVelocity() ) );
	if ( drive == RWD )
	{
		driveshaft_speed = differential_rear.CalculateDriveshaftSpeed ( left_rear_wheel_speed, right_rear_wheel_speed );
	}
	else if ( drive == FWD )
	{
		driveshaft_speed = differential_front.CalculateDriveshaftSpeed ( left_front_wheel_speed, right_front_wheel_speed );
	}
	else if ( drive == AWD )
	{
		driveshaft_speed = differential_center.CalculateDriveshaftSpeed (
		                       differential_front.CalculateDriveshaftSpeed ( left_front_wheel_speed, right_front_wheel_speed ),
		                       differential_rear.CalculateDriveshaftSpeed ( left_rear_wheel_speed, right_rear_wheel_speed ) );
	}

	return driveshaft_speed;
}

void CARDYNAMICS::UpdateTransmission(T dt)
{
	driveshaft_rpm = CalculateDriveshaftRPM();

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
		    std::cout << "start engine" << std::endl;
		}

		T throttle = engine.GetThrottle();
		throttle = ShiftAutoClutchThrottle(throttle, dt);
		engine.SetThrottle(throttle);

		T new_clutch = AutoClutch(last_auto_clutch, dt);
		clutch.SetClutch(new_clutch);
		last_auto_clutch = new_clutch;
	}
}

T CARDYNAMICS::CalculateDriveshaftRPM() const
{
	T driveshaft_speed = 0.0;
	T left_front_wheel_speed = wheel[FRONT_LEFT].GetAngularVelocity();
	T right_front_wheel_speed = wheel[FRONT_RIGHT].GetAngularVelocity();
	T left_rear_wheel_speed = wheel[REAR_LEFT].GetAngularVelocity();
	T right_rear_wheel_speed = wheel[REAR_RIGHT].GetAngularVelocity();
	for ( int i = 0; i < 4; i++ ) assert ( !isnan ( wheel[WHEEL_POSITION ( i ) ].GetAngularVelocity() ) );
	if ( drive == RWD )
	{
		driveshaft_speed = differential_rear.GetDriveshaftSpeed ( left_rear_wheel_speed, right_rear_wheel_speed );
	}
	else if ( drive == FWD )
	{
		driveshaft_speed = differential_front.GetDriveshaftSpeed ( left_front_wheel_speed, right_front_wheel_speed );
	}
	else if ( drive == AWD )
	{
		T front_speed = differential_front.GetDriveshaftSpeed ( left_front_wheel_speed, right_front_wheel_speed );
		T rear_speed = differential_rear.GetDriveshaftSpeed ( left_rear_wheel_speed, right_rear_wheel_speed );
		driveshaft_speed = differential_center.GetDriveshaftSpeed ( front_speed, rear_speed );
	}

	return transmission.GetClutchSpeed ( driveshaft_speed ) * 30.0 / 3.141593;
}

bool CARDYNAMICS::WheelDriven(int i) const
{
	return (1 << i) & drive;
}

T CARDYNAMICS::AutoClutch(T last_clutch, T dt) const
{
	T rpm = engine.GetRPM();
	T stallrpm = engine.GetStallRPM();
	T clutchrpm = driveshaft_rpm; //clutch rpm on driveshaft/transmission side

	// clutch slip
	T clutch = (5.0 * rpm + clutchrpm) / (9.0 * stallrpm) - 1.5;
	if (clutch < 0.0) clutch = 0.0;
	else if (clutch > 1.0) clutch = 1.0;

	// shift time
	clutch *= ShiftAutoClutch();

	// brakes fully engaged (declutch)
	if (brake[0].GetBrakeFactor() == 1.0) clutch = 0.0;

	// rate limit the autoclutch
	T min_engage_time = 0.05;
	T engage_limit = dt / min_engage_time;
	if (last_clutch - clutch > engage_limit)
	{
		clutch = last_clutch - engage_limit;
	}

	return clutch;
}

T CARDYNAMICS::ShiftAutoClutch() const
{
	const T shift_time = transmission.GetShiftTime();
	T shift_clutch = 1.0;
	if (remaining_shift_time > shift_time * 0.5)
	    shift_clutch = 0.0;
	else if (remaining_shift_time > 0.0)
	    shift_clutch = 1.0 - remaining_shift_time / (shift_time * 0.5);
	return shift_clutch;
}

T CARDYNAMICS::ShiftAutoClutchThrottle(T throttle, T dt)
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
	if (shifted)
	{
        if (clutch.GetClutch() == 1.0)
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
    }
	return gear;
}

T CARDYNAMICS::DownshiftRPM(int gear) const
{
	T shift_down_point = 0.0;
	if (gear > 1)
	{
        T current_gear_ratio = transmission.GetGearRatio(gear);
        T lower_gear_ratio = transmission.GetGearRatio(gear - 1);
		T peak_engine_speed = engine.GetRedline();
		shift_down_point = 0.7 * peak_engine_speed / lower_gear_ratio * current_gear_ratio;
	}
	return shift_down_point;
}

///do traction control system (wheelspin prevention) calculations and modify the throttle position if necessary
void CARDYNAMICS::DoTCS(int i, T suspension_force)
{
	//if (!WheelDriven(i)) return;

	T gasthresh = 0.1;
	T gas = engine.GetThrottle();

	//only active if throttle commanded past threshold
	if (gas > gasthresh)
	{
		//see if we're spinning faster than the rest of the wheels
		T maxspindiff = 0;
		T myrotationalspeed = wheel[i].GetAngularVelocity();
		for (int i2 = 0; i2 < WHEEL_POSITION_SIZE; i2++)
		{
			T spindiff = myrotationalspeed - wheel[i2].GetAngularVelocity();
			if (spindiff < 0)
				spindiff = -spindiff;
			if (spindiff > maxspindiff)
				maxspindiff = spindiff;
		}

		//don't engage if all wheels are moving at the same rate
		if (maxspindiff > 1.0)
		{
			T slide = tire[i].GetSlide() / tire[i].GetIdealSlide();
			if (transmission.GetGear() < 0.0) slide *= -1.0;

			if (slide > 1.0) tcs_active[i] = true;
			else if (slide < 0.5) tcs_active[i] = false;

			if (tcs_active[i])
			{
				T curclutch = clutch.GetClutch();
				assert(curclutch <= 1.0 && curclutch >= 0.0);

				gas -= curclutch * (slide - 0.5);
				if (gas < 0) gas = 0;
				if (gas > 1) gas = 1;
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
void CARDYNAMICS::DoABS(int i, T suspension_force)
{
	T braketresh = 0.1;
	T brakesetting = brake[i].GetBrakeFactor();

	//only active if brakes commanded past threshold
	if (brakesetting > braketresh)
	{
		T maxspeed = 0;
		for (int i2 = 0; i2 < WHEEL_POSITION_SIZE; i2++)
		{
			if (wheel[i2].GetAngularVelocity() > maxspeed)
				maxspeed = wheel[i2].GetAngularVelocity();
		}

		//don't engage ABS if all wheels are moving slowly
		if (maxspeed > 6.0)
		{
			T sp = tire[i].GetIdealSlide();
			//T ah = tire[i].GetIdealSlip();

			T error = - tire[i].GetSlide() - sp;
			T thresholdeng = 0.0;
			T thresholddis = -sp/2.0;

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

void CARDYNAMICS::AddMassParticle(T mass, MATHVECTOR <T, 3> pos)
{
	mass_particles.push_back(std::pair <T, MATHVECTOR <T, 3> > (mass, pos));
}
