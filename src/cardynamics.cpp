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
	int version(1);
	
	if (!c.GetParam("engine.peak-engine-rpm", info.redline, error_output)) return false; //used only for the redline graphics
	if (!c.GetParam("engine.rpm-limit", info.rpm_limit, error_output)) return false;
	if (!c.GetParam("engine.inertia", info.inertia, error_output)) return false;
	if (!c.GetParam("engine.start-rpm", info.start_rpm, error_output)) return false;
	if (!c.GetParam("engine.stall-rpm", info.stall_rpm, error_output)) return false;
	if (!c.GetParam("engine.fuel-consumption", info.fuel_consumption, error_output)) return false;
	if (!c.GetParam("engine.mass", info.mass, error_output)) return false;
	if (!c.GetParam("engine.position", temp_vec3, error_output)) return false;
	c.GetParam("version", version);
	if (version == 2)
	{
		COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(temp_vec3[0],temp_vec3[1],temp_vec3[2]);
	}
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
	int version(1);
	
	c.GetParam("version", version);
	if (!c.GetParam("fuel-tank.capacity", capacity, error_output)) return false;
	if (!c.GetParam("fuel-tank.volume", volume, error_output)) return false;
	if (!c.GetParam("fuel-tank.fuel-density", fuel_density, error_output)) return false;
	if (!c.GetParam("fuel-tank.position", pos, error_output)) return false;
	if (version == 2)
	{
		COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(pos[0],pos[1],pos[2]);
	}
	position.Set(pos[0],pos[1],pos[2]);
	
	fuel_tank.SetCapacity(capacity);
	fuel_tank.SetVolume(volume);
	fuel_tank.SetDensity(fuel_density);
	fuel_tank.SetPosition(position);
	
	return true;
}

bool LoadCoilover(
	const CONFIGFILE & c,
	const std::string & coilovername,
	CARSUSPENSIONINFO <T> & info,
	std::ostream & error_output)
{
	std::vector <std::pair <double, double> > damper_factor_points;
	std::vector <std::pair <double, double> > spring_factor_points;

	if (!c.GetParam(coilovername+".spring-constant", info.spring_constant, error_output)) return false;
	if (!c.GetParam(coilovername+".bounce", info.bounce, error_output)) return false;
	if (!c.GetParam(coilovername+".rebound", info.rebound, error_output)) return false;
	if (!c.GetParam(coilovername+".travel", info.travel, error_output)) return false;
	if (!c.GetParam(coilovername+".anti-roll", info.anti_roll, error_output)) return false;
	c.GetPoints(coilovername, "damper-factor", damper_factor_points);
	c.GetPoints(coilovername, "spring-factor", spring_factor_points);
	info.SetDamperFactorPoints(damper_factor_points);
	info.SetSpringFactorPoints(spring_factor_points);

	return true;
}

bool LoadSuspension(
	const CONFIGFILE & c,
	const std::string & suspensionname,
	CARSUSPENSION<T> & suspension,
	std::ostream & error_output)
{
	CARSUSPENSIONINFO <T> info;
	float h[3], p[3];
	std::string coilovername;
	
	c.GetParam(suspensionname+".steering", info.max_steering_angle);
	c.GetParam(suspensionname+".ackermann", info.ackermann);
	if (!c.GetParam(suspensionname+".toe", info.toe, error_output)) return false;
	if (!c.GetParam(suspensionname+".caster", info.caster, error_output)) return false;
	if (!c.GetParam(suspensionname+".camber", info.camber, error_output)) return false;
	if (!c.GetParam(suspensionname+".hinge", h, error_output)) return false;
	if (!c.GetParam(suspensionname+".wheel-hub", p, error_output)) return false;
	if (!c.GetParam(suspensionname+".coilover", coilovername, error_output)) return false;
	if (!LoadCoilover(c, coilovername, info, error_output)) return false;
	
	int version(1);
	c.GetParam("version", version);
	if (version == 2)
	{
		COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(h[0], h[1], h[2]);
		COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(p[0], p[1], p[2]);
	}
	info.hinge.Set(h[0], h[1], h[2]);
	info.extended_position.Set(p[0], p[1], p[2]);
	suspension.Init(info);

	return true;
}

bool LoadBrake(
	const CONFIGFILE & c,
	const std::string & brakename,
	CARBRAKE<T> & brake,
	std::ostream & error_output)
{
	float friction, max_pressure, area, bias, radius, handbrake(0);

	if (!c.GetParam(brakename+".friction", friction, error_output)) return false;
	if (!c.GetParam(brakename+".area", area, error_output)) return false;
	if (!c.GetParam(brakename+".radius", radius, error_output)) return false;
	if (!c.GetParam(brakename+".bias", bias, error_output)) return false;
	if (!c.GetParam(brakename+".max-pressure", max_pressure, error_output)) return false;
	c.GetParam(brakename+".handbrake", handbrake);
	
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
		str << "a" << numinfile;
		float value;
		if (!c.GetParam(str.str(), value, error_output)) return false;
		info.lateral[i] = value;
	}

	//read longitudinal, error_output)) return false;
	for (int i = 0; i < 11; i++)
	{
		std::stringstream str;
		str << "b" << i;
		float value;
		if (!c.GetParam(str.str(), value, error_output)) return false;
		info.longitudinal[i] = value;
	}

	//read aligning, error_output)) return false;
	for (int i = 0; i < 18; i++)
	{
		std::stringstream str;
		str << "c" << i;
		float value;
		if (!c.GetParam(str.str(), value, error_output)) return false;
		info.aligning[i] = value;
	}

	float rolling_resistance[3];
	if (!c.GetParam("rolling-resistance", rolling_resistance, error_output)) return false;
	info.rolling_resistance_linear = rolling_resistance[0];
	info.rolling_resistance_quadratic = rolling_resistance[1];

	if (!c.GetParam("tread", info.tread, error_output)) return false;

	return true;
}

bool LoadTire(
	const CONFIGFILE & c,
	const std::string & tirename,
	const std::string & sharedpartspath,
	CARTIRE<T> & tire,
	std::ostream & error_output)
{
	std::string tiresize;
	std::string tiretype;
	CONFIGFILE tc;
	CARTIREINFO <T> info;
	float section_width(0);
	float aspect_ratio(0);
	float rim_diameter(0);
	
	if (!c.GetParam(tirename+".size", tiresize, error_output)) return false;
	if (!c.GetParam(tirename+".type", tiretype, error_output)) return false;
	if (!tc.Load(sharedpartspath+"/tire/"+tiretype)) return false;
	if (!LoadTireParameters(tc, info, error_output)) return false;
	
	// tire dimensions
	std::string modsize = tiresize;
	for (unsigned int i = 0; i < modsize.length(); i++)
	{
		if (modsize[i] < '0' || modsize[i] > '9')
			modsize[i] = ' ';
	}
	std::stringstream parser(modsize);
	parser >> section_width >> aspect_ratio >> rim_diameter;
	if (section_width <= 0 || aspect_ratio <= 0 || rim_diameter <= 0)
	{
		error_output << "Error parsing " << tirename
					<< ".size, expected something like 225/50r16 but got: "
					<< tiresize << std::endl;
		return false;
	}
	
	info.radius = section_width * 0.001 * aspect_ratio * 0.01 + rim_diameter * 0.0254 * 0.5;
	info.sidewall_width = section_width * 0.001;
	info.aspect_ratio = aspect_ratio * 0.01;
	tire.Init(info);
	
	return true;
}

bool LoadWheel(
	const CONFIGFILE & c,
	const std::string & wheelname,
	const std::string & partspath,
	CARWHEEL<T> & wheel,
	CARTIRE<T> & tire,
	CARBRAKE<T> & brake,
	std::ostream & error_output)
{
	std::string brakename;
	std::string tirename;
	
	if (!c.GetParam(wheelname+".brake", brakename, error_output)) return false;
	if (!LoadBrake(c, brakename, brake, error_output)) return false;
	if (!c.GetParam(wheelname+".tire", tirename, error_output)) return false;
	if (!LoadTire(c, tirename, partspath, tire, error_output)) return false;
	
	// calculate wheel inertia/mass
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
	
	wheel.SetMass(tire_mass + rim_mass);
	wheel.SetInertia((tire_inertia + rim_inertia)*4); // scale inertia fixme

	return true;
}

// max number of aero devices is 8
bool LoadAeroDevices(
	CONFIGFILE & c,
	std::vector< CARAERO <T> > & aerodynamics,
	std::ostream & error_output)
{
	int version(1);
	c.GetParam("version", version);

	for(int i = 0; i < 8; i++)
	{
		float drag_area, drag_coeff;
		float lift_area = 0, lift_coeff = 0, lift_eff = 0;
		float pos[3];
		MATHVECTOR <double, 3> position;
		
		std::stringstream num;
		num << i;
		
		const std::string wingname("aerodevice-"+num.str());
		if (!c.GetParam(wingname+".frontal-area", drag_area)) break;
		if (!c.GetParam(wingname+".drag-coefficient", drag_coeff, error_output)) return false;
		if (!c.GetParam(wingname+".position", pos, error_output)) return false;
		c.GetParam(wingname+".surface-area", lift_area);
		c.GetParam(wingname+".lift-coefficient", lift_coeff);
		c.GetParam(wingname+".efficiency", lift_eff);
		
		if (version == 2)
		{
			COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(pos[0],pos[1],pos[2]);
		}
		position.Set(pos[0], pos[1], pos[2]);
		
		aerodynamics.push_back(CARAERO<T>());
		aerodynamics.back().Set(position, drag_area, drag_coeff, lift_area, lift_coeff, lift_eff);
	}

	return true;
}

bool LoadMassParticles(
	CONFIGFILE & c,
	std::list <std::pair <T, MATHVECTOR <T, 3> > > & mass_particles,
	std::ostream & error_output)
{
	int version(1);
	c.GetParam("version", version);

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
		
		if (version == 2)
		{
			COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(pos[0], pos[1], pos[2]);
		}
		position.Set(pos[0], pos[1], pos[2]);
		
		mass_particles.push_back(std::pair <T, MATHVECTOR <T, 3> > (mass, position));
		
		num++;
	}
	
	return true;
}

bool CARDYNAMICS::Load(
	CONFIGFILE & c,
	const std::string & sharedpartspath,
	std::ostream & error_output)
{
	std::string drive = "RWD";

	if (!LoadAeroDevices(c, aerodynamics, error_output)) return false;
	if (!LoadClutch(c, clutch, error_output)) return false;
	if (!LoadTransmission(c, transmission, error_output)) return false;
	if (!LoadEngine(c, engine, error_output)) return false;
	if (!LoadFuelTank(c, fuel_tank, error_output)) return false;

	AddMassParticle(engine.GetMass(), engine.GetPosition());
	//AddMassParticle(fuel_tank.GetMass(), fuel_tank.GetPosition()); is added in UpdateMass()

	//load wheels (four wheels hardcoded)
	maxangle = 0.0;
	for (int i = 0; i < WHEEL_POSITION_SIZE; i++)
	{
		std::stringstream num;
		num << i;
		const std::string suspensionname("suspension-"+num.str());
		const std::string wheelname("wheel-"+num.str());
		
		if (!LoadSuspension(c, suspensionname, suspension[i], error_output)) return false;
		if (!LoadWheel(c, wheelname, sharedpartspath, wheel[i], tire[i], brake[i], error_output)) return false;
		
		if (suspension[i].GetMaxSteeringAngle() > maxangle) maxangle = suspension[i].GetMaxSteeringAngle();

		AddMassParticle(wheel[i].GetMass(), suspension[i].GetWheelPosition());
	}

	//load the differential(s)
	{
		float final_drive, anti_slip, anti_slip_torque(0), anti_slip_torque_deceleration_factor(0);

		if (!c.GetParam("differential.final-drive", final_drive, error_output)) return false;
		if (!c.GetParam("differential.anti-slip", anti_slip, error_output)) return false;
		c.GetParam("differential.anti-slip-torque", anti_slip_torque);
		c.GetParam("differential.anti-slip-torque-deceleration-factor", anti_slip_torque_deceleration_factor);

		std::string drivetype;
		if (!c.GetParam("drive", drivetype, error_output)) return false;
		SetDrive(drivetype);

		if (drivetype == "RWD")
		{
			rear_differential.SetFinalDrive(final_drive);
			rear_differential.SetAntiSlip(anti_slip, anti_slip_torque, anti_slip_torque_deceleration_factor);
		}
		else if (drivetype == "FWD")
		{
			front_differential.SetFinalDrive(final_drive);
			front_differential.SetAntiSlip(anti_slip, anti_slip_torque, anti_slip_torque_deceleration_factor);
		}
		else if (drivetype == "AWD")
		{
			rear_differential.SetFinalDrive(1.0);
			rear_differential.SetAntiSlip(anti_slip, anti_slip_torque, anti_slip_torque_deceleration_factor);
			front_differential.SetFinalDrive(1.0);
			front_differential.SetAntiSlip(anti_slip, anti_slip_torque, anti_slip_torque_deceleration_factor);
			center_differential.SetFinalDrive(final_drive);
			center_differential.SetAntiSlip(anti_slip, anti_slip_torque, anti_slip_torque_deceleration_factor);
		}
		else
		{
			error_output << "Unknown drive type: " << drive << std::endl;
			return false;
		}
	}

	if(!LoadMassParticles(c, mass_particles, error_output)) return false;

	UpdateMass();

	// init body, powertrain, wheels for performance testing
	body.SetInitialForce(MATHVECTOR <T, 3> (0));
	body.SetInitialTorque(MATHVECTOR <T, 3> (0));
	engine.SetInitialConditions();
	for (int i = 0; i < WHEEL_POSITION_SIZE; i++) wheel[i].SetInitialConditions();
	
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
	const MATHVECTOR <T, 3> chassisSize,
	const MATHVECTOR <T, 3> chassisCenter,
	const MATHVECTOR <T, 3> & position,
	const QUATERNION <T> & orientation)
{
	this->world = &world;

	MATHVECTOR <T, 3> zero(0, 0, 0);
	body.SetPosition(position - center_of_mass);
	body.SetOrientation(orientation);
	body.SetInitialForce(zero);
	body.SetInitialTorque(zero);

	// init engine
	engine.SetInitialConditions();

	// init chassis
	T chassisMass = body.GetMass();
	MATRIX3 <T> inertia = body.GetInertia();
	//MATRIX3 <T> inertia_basis;
	//MATHVECTOR <T, 3> inertia_vector;
	//MATRIX3<T>::Diagonalize(inertia, inertia_basis, inertia_vector);
	btVector3 chassisInertia(inertia[0], inertia[4], inertia[8]);

	btTransform transform;
	transform.setOrigin(ToBulletVector(position-center_of_mass));
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
		wheel[i].SetInitialConditions();
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
	
	MATHVECTOR <T, 3> trimmed_position = Position() + GetDownVector() * min_height;
	SetPosition(trimmed_position);
	UpdateWheelTransform();
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
	if (fabs(angle) < M_PI_4) return;
	
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
		out.precision ( 2 );
		out << "---Body---" << std::endl;
		out << "Position: " << chassisPosition << std::endl;
		out << "Center of mass: " << center_of_mass << std::endl;
		out.precision ( 6 );
		out << "Total mass: " << body.GetMass() << std::endl;
		out << std::endl;
		engine.DebugPrint ( out );
		out << std::endl;
		fuel_tank.DebugPrint ( out );
		out << std::endl;
		clutch.DebugPrint ( out );
		out << std::endl;
		transmission.DebugPrint ( out );
		out << std::endl;
		if ( drive == RWD )
		{
			out << "(rear)" << std::endl;
			rear_differential.DebugPrint ( out );
		}
		else if ( drive == FWD )
		{
			out << "(front)" << std::endl;
			front_differential.DebugPrint ( out );
		}
		else if ( drive == AWD )
		{
			out << "(center)" << std::endl;
			center_differential.DebugPrint ( out );

			out << "(front)" << std::endl;
			front_differential.DebugPrint ( out );

			out << "(rear)" << std::endl;
			rear_differential.DebugPrint ( out );
		}
		out << std::endl;
	}

	if ( p2 )
	{
		out << "(front left)" << std::endl;
		suspension[FRONT_LEFT].DebugPrint ( out );
		out << std::endl;
		out << "(front right)" << std::endl;
		suspension[FRONT_RIGHT].DebugPrint ( out );
		out << std::endl;
		out << "(rear left)" << std::endl;
		suspension[REAR_LEFT].DebugPrint ( out );
		out << std::endl;
		out << "(rear right)" << std::endl;
		suspension[REAR_RIGHT].DebugPrint ( out );
		out << std::endl;

		out << "(front left)" << std::endl;
		brake[FRONT_LEFT].DebugPrint ( out );
		out << std::endl;
		out << "(front right)" << std::endl;
		brake[FRONT_RIGHT].DebugPrint ( out );
		out << std::endl;
		out << "(rear left)" << std::endl;
		brake[REAR_LEFT].DebugPrint ( out );
		out << std::endl;
		out << "(rear right)" << std::endl;
		brake[REAR_RIGHT].DebugPrint ( out );
	}

	if ( p3 )
	{
		out << std::endl;
		out << "(front left)" << std::endl;
		wheel[FRONT_LEFT].DebugPrint ( out );
		out << std::endl;
		out << "(front right)" << std::endl;
		wheel[FRONT_RIGHT].DebugPrint ( out );
		out << std::endl;
		out << "(rear left)" << std::endl;
		wheel[REAR_LEFT].DebugPrint ( out );
		out << std::endl;
		out << "(rear right)" << std::endl;
		wheel[REAR_RIGHT].DebugPrint ( out );

		out << std::endl;
		out << "(front left)" << std::endl;
		tire[FRONT_LEFT].DebugPrint ( out );
		out << std::endl;
		out << "(front right)" << std::endl;
		tire[FRONT_RIGHT].DebugPrint ( out );
		out << std::endl;
		out << "(rear left)" << std::endl;
		tire[REAR_LEFT].DebugPrint ( out );
		out << std::endl;
		out << "(rear right)" << std::endl;
		tire[REAR_RIGHT].DebugPrint ( out );
	}

	if ( p4 )
	{
		for ( std::vector <CARAERO<T> >::const_iterator i = aerodynamics.begin(); i != aerodynamics.end(); ++i )
		{
			i->DebugPrint ( out );
			out << std::endl;
		}
	}
}

bool CARDYNAMICS::Serialize ( joeserialize::Serializer & s )
{
	_SERIALIZE_(s,body);
	_SERIALIZE_(s,engine);
	_SERIALIZE_(s,clutch);
	_SERIALIZE_(s,transmission);
	_SERIALIZE_(s,front_differential);
	_SERIALIZE_(s,rear_differential);
	_SERIALIZE_(s,center_differential);
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
		rear_differential.ComputeWheelTorques(driveshaft_torque);
		wheel_drive_torque[REAR_LEFT] = rear_differential.GetSide1Torque();
		wheel_drive_torque[REAR_RIGHT] = rear_differential.GetSide2Torque();
	}
	else if ( drive == FWD )
	{
		front_differential.ComputeWheelTorques(driveshaft_torque);
		wheel_drive_torque[FRONT_LEFT] = front_differential.GetSide1Torque();
		wheel_drive_torque[FRONT_RIGHT] = front_differential.GetSide2Torque();
	}
	else if ( drive == AWD )
	{
		center_differential.ComputeWheelTorques(driveshaft_torque);
		front_differential.ComputeWheelTorques(center_differential.GetSide1Torque());
		rear_differential.ComputeWheelTorques(center_differential.GetSide2Torque());
		wheel_drive_torque[FRONT_LEFT] = front_differential.GetSide1Torque();
		wheel_drive_torque[FRONT_RIGHT] = front_differential.GetSide2Torque();
		wheel_drive_torque[REAR_LEFT] = rear_differential.GetSide1Torque();
		wheel_drive_torque[REAR_RIGHT] = rear_differential.GetSide2Torque();
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
		driveshaft_speed = rear_differential.CalculateDriveshaftSpeed ( left_rear_wheel_speed, right_rear_wheel_speed );
	}
	else if ( drive == FWD )
	{
		driveshaft_speed = front_differential.CalculateDriveshaftSpeed ( left_front_wheel_speed, right_front_wheel_speed );
	}
	else if ( drive == AWD )
	{
		driveshaft_speed = center_differential.CalculateDriveshaftSpeed (
		                       front_differential.CalculateDriveshaftSpeed ( left_front_wheel_speed, right_front_wheel_speed ),
		                       rear_differential.CalculateDriveshaftSpeed ( left_rear_wheel_speed, right_rear_wheel_speed ) );
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
		driveshaft_speed = rear_differential.GetDriveshaftSpeed ( left_rear_wheel_speed, right_rear_wheel_speed );
	}
	else if ( drive == FWD )
	{
		driveshaft_speed = front_differential.GetDriveshaftSpeed ( left_front_wheel_speed, right_front_wheel_speed );
	}
	else if ( drive == AWD )
	{
		T front_speed = front_differential.GetDriveshaftSpeed ( left_front_wheel_speed, right_front_wheel_speed );
		T rear_speed = rear_differential.GetDriveshaftSpeed ( left_rear_wheel_speed, right_rear_wheel_speed );
		driveshaft_speed = center_differential.GetDriveshaftSpeed ( front_speed, rear_speed );
	}

	return transmission.GetClutchSpeed ( driveshaft_speed ) * 30.0 / 3.141593;
}

bool CARDYNAMICS::WheelDriven(int i) const
{
	return (1 << i) & drive;
}

T CARDYNAMICS::AutoClutch(T last_clutch, T dt) const
{
	const T threshold = 1000.0;
	const T margin = 100.0;
	const T geareffect = 1.0; //zero to 1, defines special consideration of first/reverse gear

	//take into account locked brakes
	bool willlock(true);
	for (int i = 0; i < WHEEL_POSITION_SIZE; i++)
	{
		if (WheelDriven(WHEEL_POSITION(i)))
		{
            willlock = willlock && brake[i].WillLock();
		}
	}
	if (willlock) return 0;

	const T rpm = engine.GetRPM();
	const T maxrpm = engine.GetRPMLimit();
	const T stallrpm = engine.GetStallRPM() + margin * (maxrpm / 2000.0);
	const int gear = transmission.GetGear();

	T gearfactor = 1.0;
	if (gear <= 1)
		gearfactor = 2.0;
	T thresh = threshold * (maxrpm/7000.0) * ((1.0-geareffect)+gearfactor*geareffect) + stallrpm;
	if (clutch.IsLocked())
		thresh *= 0.5;
	T clutch = (rpm-stallrpm) / (thresh-stallrpm);

	//std::cout << rpm << ", " << stallrpm << ", " << threshold << ", " << clutch << std::endl;

	if (clutch < 0)
		clutch = 0;
	if (clutch > 1.0)
		clutch = 1.0;

	T newauto = clutch * ShiftAutoClutch();

	//rate limit the autoclutch
	const T min_engage_time = 0.05; //the fastest time in seconds for auto-clutch engagement
	const T engage_rate_limit = 1.0/min_engage_time;
	const T rate = (last_clutch - newauto)/dt; //engagement rate in clutch units per second
	if (rate > engage_rate_limit)
		newauto = last_clutch - engage_rate_limit*dt;

    return newauto;
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
		if ( maxspindiff > 1.0 )
		{
			T sp = tire[i].GetIdealSlide();
			//T ah = tire[i].GetIdealSlip();

			T sense = 1.0;
			if (transmission.GetGear() < 0)
				sense = -1.0;

			T error = tire[i].GetSlide() * sense - sp;
			T thresholdeng = 0.0;
			T thresholddis = -sp/2.0;

			if (error > thresholdeng && !tcs_active[i])
				tcs_active[i] = true;

			if (error < thresholddis && tcs_active[i])
				tcs_active[i] = false;

			if (tcs_active[i])
			{
				T curclutch = clutch.GetClutch();
				if (curclutch > 1) curclutch = 1;
				if (curclutch < 0) curclutch = 0;

				gas = gas - error * 10.0 * curclutch;
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

void CARDYNAMICS::SetDrive ( const std::string & newdrive )
{
	if ( newdrive == "RWD" )
		drive = RWD;
	else if ( newdrive == "FWD" )
		drive = FWD;
	else if ( newdrive == "AWD" )
		drive = AWD;
	else
		assert ( 0 ); //shouldn't ever happen unless there's an error in the code
}

void CARDYNAMICS::AddMassParticle(T mass, MATHVECTOR <T, 3> pos)
{
	mass_particles.push_back(std::pair <T, MATHVECTOR <T, 3> > (mass, pos));
}
