#include "cardynamics.h"

#include "configfile.h"
#include "tracksurface.h"
#include "coordinatesystems.h"
#include "collision_world.h"
#include "tobullet.h"
#include "model.h"

typedef CARDYNAMICS::T T;

CARDYNAMICS::CARDYNAMICS()
: world(NULL),
  chassis(NULL),
  drive(RWD),
  tacho_rpm(0),
  autoclutch(true),
  autoshift(false),
  shifted(true),
  shift_gear(0),
  last_auto_clutch(1.0),
  remaining_shift_time(0.0),
  shift_time(0.2),
  abs(false),
  tcs(false),
  maxangle(45.0),
  telemetry("telemetry")
{
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
	//wheelBody.resize(WHEEL_POSITION_SIZE);
}

bool CARDYNAMICS::Load(CONFIGFILE & c, std::ostream & error_output)
{
	std::string drive = "RWD";
	int version(1);
	c.GetParam("version", version);
	if (version > 2)
	{
		error_output << "Unsupported car version: " << version << std::endl;
		return false;
	}
	float temp_vec3[3];

	//load the engine
	{
		float engine_mass, engine_redline, engine_rpm_limit, engine_inertia,
			engine_start_rpm, engine_stall_rpm, engine_fuel_consumption;
		MATHVECTOR <double, 3> engine_position;

		if (!c.GetParam("engine.peak-engine-rpm", engine_redline, error_output)) return false; //used only for the redline graphics
		engine.SetRedline(engine_redline);

		if (!c.GetParam("engine.rpm-limit", engine_rpm_limit, error_output)) return false;
		engine.SetRPMLimit(engine_rpm_limit);

		if (!c.GetParam("engine.inertia", engine_inertia, error_output)) return false;
		engine.SetInertia(engine_inertia);

		if (!c.GetParam("engine.start-rpm", engine_start_rpm, error_output)) return false;
		engine.SetStartRPM(engine_start_rpm);

		if (!c.GetParam("engine.stall-rpm", engine_stall_rpm, error_output)) return false;
		engine.SetStallRPM(engine_stall_rpm);

		if (!c.GetParam("engine.fuel-consumption", engine_fuel_consumption, error_output)) return false;
		engine.SetFuelConsumption(engine_fuel_consumption);

		if (!c.GetParam("engine.mass", engine_mass, error_output)) return false;
		if (!c.GetParam("engine.position", temp_vec3, error_output)) return false;
		if (version == 2)
		{
			COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(temp_vec3[0],temp_vec3[1],temp_vec3[2]);
		}
		engine_position.Set(temp_vec3[0],temp_vec3[1],temp_vec3[2]);
		engine.SetMass(engine_mass);
		engine.SetPosition(engine_position);
		AddMassParticle(engine_mass, engine_position);

		float torque_point[3];
		std::string torque_str("engine.torque-curve-00");
		std::vector <std::pair <double, double> > torques;
		int curve_num = 0;
		while (c.GetParam(torque_str, torque_point))
		{
			torques.push_back(std::pair <float, float> (torque_point[0], torque_point[1]));

			curve_num++;
			std::stringstream str;
			str << "engine.torque-curve-";
			str.width(2);
			str.fill('0');
			str << curve_num;
			torque_str = str.str();
		}
		if (torques.size() <= 1)
		{
			error_output << "You must define at least 2 torque curve points." << std::endl;
			return false;
		}
		engine.SetTorqueCurve(engine_redline, torques);
	}

	//load the clutch
	{
		float sliding, radius, area, max_pressure;

		if (!c.GetParam("clutch.sliding", sliding, error_output)) return false;
		clutch.SetSlidingFriction(sliding);

		if (!c.GetParam("clutch.radius", radius, error_output)) return false;
		clutch.SetRadius(radius);

		if (!c.GetParam("clutch.area", area, error_output)) return false;
		clutch.SetArea(area);

		if (!c.GetParam("clutch.max-pressure", max_pressure, error_output)) return false;
		clutch.SetMaxPressure(max_pressure);
	}

	//load the transmission
	{
		float time = 0;
		float ratio;
		int gears;

		c.GetParam("transmission.shift-time", time);
		shift_time = time;

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

	//load the brake
	{
		for (int i = 0; i < 2; i++)
		{
			std::string pos = "front";
			WHEEL_POSITION left = FRONT_LEFT;
			WHEEL_POSITION right = FRONT_RIGHT;
			if (i == 1)
			{
				left = REAR_LEFT;
				right = REAR_RIGHT;
				pos = "rear";
			}

			float friction, max_pressure, area, bias, radius, handbrake(0);

			if (!c.GetParam("brakes-"+pos+".friction", friction, error_output)) return false;
			brake[left].SetFriction(friction);
			brake[right].SetFriction(friction);

			if (!c.GetParam("brakes-"+pos+".area", area, error_output)) return false;
			brake[left].SetArea(area);
			brake[right].SetArea(area);

			if (!c.GetParam("brakes-"+pos+".radius", radius, error_output)) return false;
			brake[left].SetRadius(radius);
			brake[right].SetRadius(radius);

			c.GetParam("brakes-"+pos+".handbrake", handbrake);
			brake[left].SetHandbrake(handbrake);
			brake[right].SetHandbrake(handbrake);

			if (!c.GetParam("brakes-"+pos+".bias", bias, error_output)) return false;
			brake[left].SetBias(bias);
			brake[right].SetBias(bias);

			if (!c.GetParam("brakes-"+pos+".max-pressure", max_pressure, error_output)) return false;
			brake[left].SetMaxPressure(max_pressure*bias);
			brake[right].SetMaxPressure(max_pressure*bias);
		}
	}

	//load the fuel tank
	{
		float pos[3];
		MATHVECTOR <double, 3> position;
		float capacity;
		float volume;
		float fuel_density;

		if (!c.GetParam("fuel-tank.capacity", capacity, error_output)) return false;
		fuel_tank.SetCapacity(capacity);

		if (!c.GetParam("fuel-tank.volume", volume, error_output)) return false;
		fuel_tank.SetVolume(volume);

		if (!c.GetParam("fuel-tank.fuel-density", fuel_density, error_output)) return false;
		fuel_tank.SetDensity(fuel_density);

		if (!c.GetParam("fuel-tank.position", pos, error_output)) return false;
		if (version == 2)
			COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(pos[0],pos[1],pos[2]);
		position.Set(pos[0],pos[1],pos[2]);
		fuel_tank.SetPosition(position);
		//AddMassParticle(fuel_density*volume, position);
	}

	//load the suspension
	{
		for (int i = 0; i < 2; i++)
		{
			std::string posstr = "front";
			std::string posshortstr = "F";
			WHEEL_POSITION posl = FRONT_LEFT;
			WHEEL_POSITION posr = FRONT_RIGHT;
			if (i == 1)
			{
				posstr = "rear";
				posshortstr = "R";
				posl = REAR_LEFT;
				posr = REAR_RIGHT;
			}

			float spring_constant, bounce, rebound, travel, camber, caster, toe, anti_roll, maxcompvel;
			float hinge[3];
			MATHVECTOR <double, 3> tempvec;

			if (!c.GetParam("suspension-"+posstr+".spring-constant", spring_constant, error_output)) return false;
			suspension[posl].SetSpringConstant(spring_constant);
			suspension[posr].SetSpringConstant(spring_constant);

			if (!c.GetParam("suspension-"+posstr+".bounce", bounce, error_output)) return false;
			suspension[posl].SetBounce(bounce);
			suspension[posr].SetBounce(bounce);

			if (!c.GetParam("suspension-"+posstr+".rebound", rebound, error_output)) return false;
			suspension[posl].SetRebound(rebound);
			suspension[posr].SetRebound(rebound);

			std::vector <std::pair <double, double> > damper_factor_points;
			c.GetPoints("suspension-"+posstr, "damper-factor", damper_factor_points);
			suspension[posl].SetDamperFactorPoints(damper_factor_points);
			suspension[posr].SetDamperFactorPoints(damper_factor_points);

			std::vector <std::pair <double, double> > spring_factor_points;
			c.GetPoints("suspension-"+posstr, "spring-factor", spring_factor_points);
			suspension[posl].SetSpringFactorPoints(spring_factor_points);
			suspension[posr].SetSpringFactorPoints(spring_factor_points);

			// deprecated
			//if (!c.GetParam("suspension-"+posstr+".max-compression-velocity", maxcompvel, error_output)) return false;
			//suspension[posl].SetMaxCompressionVelocity(maxcompvel);
			//suspension[posr].SetMaxCompressionVelocity(maxcompvel);

			if (!c.GetParam("suspension-"+posstr+".travel", travel, error_output)) return false;
			suspension[posl].SetTravel(travel);
			suspension[posr].SetTravel(travel);

			if (!c.GetParam("suspension-"+posstr+".camber", camber, error_output)) return false;
			suspension[posl].SetCamber(camber);
			suspension[posr].SetCamber(camber);

			if (!c.GetParam("suspension-"+posstr+".caster", caster, error_output)) return false;
			suspension[posl].SetCaster(caster);
			suspension[posr].SetCaster(caster);

			if (!c.GetParam("suspension-"+posstr+".toe", toe, error_output)) return false;
			suspension[posl].SetToe(toe);
			suspension[posr].SetToe(toe);

			if (!c.GetParam("suspension-"+posstr+".anti-roll", anti_roll, error_output)) return false;
			suspension[posl].SetAntiRollK(anti_roll);
			suspension[posr].SetAntiRollK(anti_roll);

			if (!c.GetParam("suspension-"+posshortstr+"L.hinge", hinge, error_output)) return false;
			//cap hinge to reasonable values
			for (int i = 0; i < 3; i++)
			{
				if (hinge[i] < -100)
					hinge[i] = -100;
				if (hinge[i] > 100)
					hinge[i] = 100;
			}
			if (version == 2)
				COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(hinge[0],hinge[1],hinge[2]);
			tempvec.Set(hinge[0],hinge[1], hinge[2]);
			suspension[posl].SetHinge(tempvec);

			if (!c.GetParam("suspension-"+posshortstr+"R.hinge", hinge, error_output)) return false;
			for (int i = 0; i < 3; i++)
			{
				if (hinge[i] < -100)
					hinge[i] = -100;
				if (hinge[i] > 100)
					hinge[i] = 100;
			}
			if (version == 2)
				COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(hinge[0],hinge[1],hinge[2]);
			tempvec.Set(hinge[0],hinge[1], hinge[2]);
			suspension[posr].SetHinge(tempvec);
		}
	}

	//load the wheels
	{
		for (int i = 0; i < 4; i++)
		{
			std::string posstr;
			WHEEL_POSITION pos;
			if (i == 0)
			{
				posstr = "FL";
				pos = FRONT_LEFT;
			}
			else if (i == 1)
			{
				posstr = "FR";
				pos = FRONT_RIGHT;
			}
			else if (i == 2)
			{
				posstr = "RL";
				pos = REAR_LEFT;
			}
			else
			{
				posstr = "RR";
				pos = REAR_RIGHT;
			}

			float roll_height, mass;
			float position[3];
			MATHVECTOR <double, 3> tempvec;

			if (!c.GetParam("wheel-"+posstr+".mass", mass, error_output)) return false;
			wheel[pos].SetMass(mass);

			if (!c.GetParam("wheel-"+posstr+".roll-height", roll_height, error_output)) return false;
			wheel[pos].SetRollHeight(roll_height);

			if (!c.GetParam("wheel-"+posstr+".position", position, error_output)) return false;
			if (version == 2)
				COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(position[0],position[1],position[2]);
			tempvec.Set(position[0],position[1], position[2]);
			wheel[pos].SetExtendedPosition(tempvec);

			AddMassParticle(mass, tempvec);
		}

		//load the rotational inertia parameter from the tire section
		float front_inertia;
		float rear_inertia;
		if (!c.GetParam("tire-front.rotational-inertia", front_inertia, error_output)) return false;
		wheel[FRONT_LEFT].SetInertia(front_inertia);
		wheel[FRONT_RIGHT].SetInertia(front_inertia);

		if (!c.GetParam("tire-rear.rotational-inertia", rear_inertia, error_output)) return false;
		wheel[REAR_LEFT].SetInertia(rear_inertia);
		wheel[REAR_RIGHT].SetInertia(rear_inertia);
	}

	//load the tire parameters
	{
		WHEEL_POSITION leftside = FRONT_LEFT;
		WHEEL_POSITION rightside = FRONT_RIGHT;
		std::string posstr = "front";

		for (int p = 0; p < 2; p++)
		{
			if (p == 1)
			{
				leftside = REAR_LEFT;
				rightside = REAR_RIGHT;
				posstr = "rear";
			}

			std::vector <double> longitudinal;
			std::vector <double> lateral;
			std::vector <double> aligning;
			longitudinal.resize(11);
			lateral.resize(15);
			aligning.resize(18);

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
				str << "tire-"+posstr+".a" << numinfile;
				float value;
				if (!c.GetParam(str.str(), value, error_output)) return false;
				lateral[i] = value;
			}

			//read longitudinal, error_output)) return false;
			for (int i = 0; i < 11; i++)
			{
				std::stringstream str;
				str << "tire-"+posstr+".b" << i;
				float value;
				if (!c.GetParam(str.str(), value, error_output)) return false;
				longitudinal[i] = value;
			}

			//read aligning, error_output)) return false;
			for (int i = 0; i < 18; i++)
			{
				std::stringstream str;
				str << "tire-"+posstr+".c" << i;
				float value;
				if (!c.GetParam(str.str(), value, error_output)) return false;
				aligning[i] = value;
			}

			tire[leftside].SetPacejkaParameters(longitudinal, lateral, aligning);
			tire[rightside].SetPacejkaParameters(longitudinal, lateral, aligning);

			float rolling_resistance[3];
			if (!c.GetParam("tire-"+posstr+".rolling-resistance", rolling_resistance, error_output)) return false;
			tire[leftside].SetRollingResistance(rolling_resistance[0], rolling_resistance[1]);
			tire[rightside].SetRollingResistance(rolling_resistance[0], rolling_resistance[1]);

			float tread;
			float radius;
			if (!c.GetParam("tire-"+posstr+".radius", radius, error_output)) return false;
			tire[leftside].SetRadius(radius);
			tire[rightside].SetRadius(radius);
			if (!c.GetParam("tire-"+posstr+".tread", tread, error_output)) return false;
			tire[leftside].SetTread(tread);
			tire[rightside].SetTread(tread);
		}

		for (int i = 0; i < 4; i++)
		{
			tire[WHEEL_POSITION(i)].CalculateSigmaHatAlphaHat();
		}
	}

	//load the mass-only particles
	{
		MATHVECTOR <double, 3> position;
		float pos[3];
		float mass;

		if (c.GetParam("contact-points.mass", mass))
		{
			int paramnum(0);
			std::string paramname("contact-points.position-00");
			std::stringstream output_supression;
			while (c.GetParam(paramname, pos))
			{
				if (version == 2)
					COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(pos[0],pos[1],pos[2]);
				position.Set(pos[0],pos[1],pos[2]);
				AddMassParticle(mass, position);
				paramnum++;
				std::stringstream str;
				str << "contact-points.position-";
				str.width(2);
				str.fill('0');
				str << paramnum;
				paramname = str.str();
			}
		}

		std::string paramname = "particle-00";
		int paramnum = 0;
		while (c.GetParam(paramname+".mass", mass))
		{
			if (!c.GetParam(paramname+".position", pos, error_output)) return false;
			if (version == 2)
				COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(pos[0],pos[1],pos[2]);
			position.Set(pos[0],pos[1],pos[2]);
			AddMassParticle(mass, position);
			paramnum++;
			std::stringstream str;
			str << "particle-";
			str.width(2);
			str.fill('0');
			str << paramnum;
			paramname = str.str();
		}
	}

	//load the max steering angle
	{
		float maxangle = 45.0;
		if (!c.GetParam("steering.max-angle", maxangle, error_output)) return false;
		SetMaxSteeringAngle ( maxangle );
	}

	//load the driver
	{
		float mass;
		float pos[3];
		MATHVECTOR <double, 3> position;

		if (!c.GetParam("driver.mass", mass, error_output)) return false;
		if (!c.GetParam("driver.position", pos, error_output)) return false;
		if (version == 2)
		{
			COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(pos[0],pos[1],pos[2]);
		}
		position.Set(pos[0], pos[1], pos[2]);
		AddMassParticle(mass, position);
	}

	//load the aerodynamics
	{
		float drag_area, drag_c, lift_area, lift_c, lift_eff;
		float pos[3];
		MATHVECTOR <double, 3> position;

		if (!c.GetParam("drag.frontal-area", drag_area, error_output)) return false;
		if (!c.GetParam("drag.drag-coefficient", drag_c, error_output)) return false;
		if (!c.GetParam("drag.position", pos, error_output)) return false;
		if (version == 2)
			COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(pos[0],pos[1],pos[2]);
		position.Set(pos[0], pos[1], pos[2]);
		AddAerodynamicDevice(position, drag_area, drag_c, 0,0,0);

		for (int i = 0; i < 2; i++)
		{
			std::string wingpos = "front";
			if (i == 1)
				wingpos = "rear";
			if (!c.GetParam("wing-"+wingpos+".frontal-area", drag_area, error_output)) return false;
			if (!c.GetParam("wing-"+wingpos+".drag-coefficient", drag_c, error_output)) return false;
			if (!c.GetParam("wing-"+wingpos+".surface-area", lift_area, error_output)) return false;
			if (!c.GetParam("wing-"+wingpos+".lift-coefficient", lift_c, error_output)) return false;
			if (!c.GetParam("wing-"+wingpos+".efficiency", lift_eff, error_output)) return false;
			if (!c.GetParam("wing-"+wingpos+".position", pos, error_output)) return false;
			if (version == 2)
				COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(pos[0],pos[1],pos[2]);
			position.Set(pos[0], pos[1], pos[2]);
			AddAerodynamicDevice(position, drag_area, drag_c, lift_area, lift_c, lift_eff);
		}
	}

	UpdateMass();

	return true;
}

void CARDYNAMICS::Init(
	COLLISION_WORLD & world,
	const MODEL & chassisModel,
	const MODEL & wheelModelFront,
	const MODEL & wheelModelRear,
	const MATHVECTOR <T, 3> & position,
	const QUATERNION <T> & orientation)
{
	this->world = &world;

	MATHVECTOR <T, 3> zero(0, 0, 0);
	body.SetPosition(position);
	body.SetOrientation(orientation);
	body.SetInitialForce(zero);
	body.SetInitialTorque(zero);

	// init engine
	engine.SetInitialConditions();

	// init chassis
	btTransform localtransform;
	localtransform.setIdentity();
/*
	// convex hull collision shape (one order of magnitude framerate drop)
	localtransform.setOrigin(ToBulletVector(-center_of_mass));
	const float * vertices = NULL;
	int vertices_size = 0;
	chassisModel.GetVertexArray().GetVertices(vertices, vertices_size);
	btConvexHullShape * hull = new btConvexHullShape(vertices, vertices_size / 3, 3 * sizeof(float));
*/
	AABB <float> box = chassisModel.GetAABB();
	for (int i = 0; i < 4; i++)
	{
		MATHVECTOR <float, 3> wheelpos = GetLocalWheelPosition(WHEEL_POSITION(i), 0);

		const MODEL * wheelmodel = &wheelModelFront;
		if (i > 1) wheelmodel = &wheelModelRear;

		AABB <float> wheelaabb;
		float sidefactor = 1.0;
		if (i == 1 || i == 3) sidefactor = -1.0;

		wheelaabb.SetFromCorners(
			wheelpos - wheelmodel->GetAABB().GetSize() * 0.5 * sidefactor,
			wheelpos + wheelmodel->GetAABB().GetSize() * 0.5 * sidefactor);
		box.CombineWith(wheelaabb);
	}

	const MATHVECTOR <T, 3> verticalMargin(0, 0, 0.3);
	btVector3 origin = ToBulletVector(box.GetCenter() + verticalMargin - center_of_mass);
	btVector3 size = ToBulletVector(box.GetSize() - verticalMargin);

	localtransform.setOrigin(origin);
	btBoxShape * hull = new btBoxShape(size * 0.5);

	btCompoundShape * chassisShape = new btCompoundShape(false);
	chassisShape->addChildShape(localtransform, hull);

	T chassisMass = body.GetMass();
	MATRIX3 <T> inertia = body.GetInertia();
	btVector3 chassisInertia(inertia[0], inertia[4], inertia[8]);

	btTransform transform;
	transform.setOrigin(ToBulletVector(position));
	transform.setRotation(ToBulletQuaternion(orientation));
	btDefaultMotionState * chassisState = new btDefaultMotionState();
	chassisState->setWorldTransform(transform);

	btRigidBody::btRigidBodyConstructionInfo info(chassisMass, chassisState, chassisShape, chassisInertia);
	info.m_angularDamping = 0.5;
	info.m_friction = 0.75;
	chassis = world.AddRigidBody(info);
	world.AddAction(this);

	// init wheels, suspension
	for (int i = 0; i < WHEEL_POSITION_SIZE; i++)
	{
		wheel[WHEEL_POSITION(i)].SetInitialConditions();
		wheel_velocity[i].Set(0.0);
		wheel_position[i] = GetWheelPositionAtDisplacement(WHEEL_POSITION(i), 0);
		wheel_orientation[i] = orientation * GetWheelSteeringAndSuspensionOrientation(WHEEL_POSITION(i));
	}
/*
	for(unsigned int i = 0; i < WHEEL_POSITION_SIZE; i++)
	{
		// wheel: got mass, (y-axis), radius, width is missing
		btScalar wheelMass = wheel[i].GetMass();
		btScalar wheelRadius = tire[i].GetRadius();
		btScalar wheelWidth = 0.3;
		btScalar inertiaY = wheel[i].GetInertia(); // cylinder: 1/2*M*R^2
		btScalar inertiaXZ = 1/12.0 * wheelMass * wheelWidth * wheelWidth + 0.5 * inertiaY; // 1/12*M*h^2+1/4*M*R^2

		btVector3 wheelInertia(inertiaXZ, inertiaY, inertiaXZ);
		btVector3 wheelHalfExtents(wheelRadius, 0.5*wheelWidth, wheelRadius);
		btVector3 wheelPosition = ToBulletVector(LocalToWorld(wheel[i].GetExtendedPosition()));
		btQuaternion wheelRotation = ToBulletQuaternion(orientation);

		btCylinderShape * wheelShape = new btCylinderShape(wheelHalfExtents);
		btTransform wheelTransform;
		wheelTransform.setOrigin(wheelPosition);
		wheelTransform.setRotation(wheelRotation);
		btDefaultMotionState * wheelState = new btDefaultMotionState();
		wheelState->setWorldTransform(wheelTransform);

		btRigidBody::btRigidBodyConstructionInfo wheelInfo(wheelMass, wheelState, wheelShape, wheelInertia);
		wheelInfo.m_friction = 0;
		wheelBody[i] = world.AddRigidBody(wheelInfo);

		// suspension(hinge)
		btGeneric6DofSpringConstraint * joint = NULL;
		bool useLinearReferenceFrameA = true;
		bool disableCollisionsBetweenLinked = true;

		btTransform chassisFrame;
		btTransform wheelFrame;
		chassisFrame.setIdentity();
		chassisFrame.setOrigin(ToBulletVector(suspension[i].GetHinge() - center_of_mass));
		wheelFrame.setIdentity();
		wheelFrame.setOrigin(ToBulletVector(suspension[i].GetHinge() - wheel[i].GetExtendedPosition()));

		joint = new btGeneric6DofSpringConstraint(*chassis, *wheelBody[i], chassisFrame, wheelFrame, useLinearReferenceFrameA);
		joint->setAngularLowerLimit(btVector3(0, 0, 0));
		joint->setAngularUpperLimit(btVector3(0, 0, 0));
		joint->setLinearLowerLimit(btVector3(0, 0, 0));
		joint->setLinearUpperLimit(btVector3(0, 0, 0));
		if (i%2) joint->setAngularUpperLimit(btVector3(0.15, 0, 0));
		else joint->setAngularLowerLimit(btVector3(-0.15, 0, 0));
		joint->enableSpring(3, true);
		joint->setStiffness(3, suspension[i].GetSpringConstant()*2);
		joint->setDamping(3, suspension[i].GetBounce()*2);
		joint->setEquilibriumPoint();

		world.AddConstraint(joint, disableCollisionsBetweenLinked);
	}
*/
	AlignWithGround();
}

// executed as last function(after integration) in bullet singlestepsimulation
void CARDYNAMICS::updateAction(btCollisionWorld * collisionWorld, btScalar dt)
{
	// get velocity, position orientation after dt
	SynchronizeBody();

	// update wheel contacts given new velocity, position
	UpdateWheelContacts();

	// run internal simulation
	Tick(dt);

	// update velocity
	SynchronizeChassis();
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
	MATHVECTOR <T, 3> pos = GetLocalWheelPosition(wp, suspension[wp].GetDisplacementPercent());
	chassisRotation.RotateVector(pos);
	return pos + chassisPosition;
	
	//btTransform wheelTrans;
	//wheelBody[WHEEL_POSITION(wp)]->getMotionState()->getWorldTransform(wheelTrans);
	//return ToMathVector<T>(wheelTrans.getOrigin());
}

MATHVECTOR <T, 3> CARDYNAMICS::GetWheelPosition(WHEEL_POSITION wp, T displacement_percent) const
{
	MATHVECTOR <T, 3> pos = GetLocalWheelPosition(wp, displacement_percent);
	chassisRotation.RotateVector(pos);
	return pos + chassisPosition;
}

QUATERNION <T> CARDYNAMICS::GetWheelOrientation(WHEEL_POSITION wp) const
{
	QUATERNION <T> siderot;
	if(wp == FRONT_RIGHT || wp == REAR_RIGHT)
	{
		siderot.Rotate(3.141593, 0, 0, 1);
	}
	return chassisRotation * GetWheelSteeringAndSuspensionOrientation(wp) * wheel[wp].GetOrientation() * siderot;

	//btTransform wheelTrans;
	//wheelBody[WHEEL_POSITION(wp)]->getMotionState()->getWorldTransform(wheelTrans);
	//return ToMathQuaternion<T>(wheelTrans.getRotation()) * siderot;
}

QUATERNION <T> CARDYNAMICS::GetUprightOrientation(WHEEL_POSITION wp) const
{
	return chassisRotation * GetWheelSteeringAndSuspensionOrientation(wp);

	//btTransform wheelTrans;
	//wheelBody[WHEEL_POSITION(wp)]->getMotionState()->getWorldTransform(wheelTrans);
	//return ToMathQuaternion<T>(wheelTrans.getRotation());
}

/// worldspace wheel center position
MATHVECTOR <T, 3> CARDYNAMICS::GetWheelVelocity ( WHEEL_POSITION wp ) const
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
	return body.GetVelocity().Magnitude();
	//return chassis->getLinearVelocity().length();
}

MATHVECTOR <T, 3> CARDYNAMICS::GetVelocity() const
{
	return body.GetVelocity();
	//return ToMathVector<T>(chassis->getLinearVelocity());
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
			remaining_shift_time = shift_time;
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
	chassis->translate(ToBulletVector(position) - chassis->getCenterOfMassPosition());
}

//find the precise starting position for the car (trim out the extra space)
void CARDYNAMICS::AlignWithGround()
{
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
	
	MATHVECTOR <T, 3> trimmed_position = Position();
	trimmed_position[2] -= min_height;
	SetPosition(trimmed_position);
}

// proof of concept: doesn't respect car current orientation
void CARDYNAMICS::RolloverRecover()
{
	
	btTransform transform = chassis->getCenterOfMassTransform();
	btQuaternion rot(0, 0, 0, 1);
	
	transform.setRotation(rot);
	chassis->setCenterOfMassTransform(transform);
	
	QUATERNION <T> orient = ToMathQuaternion<T>(transform.getRotation());
	body.SetOrientation(orient);
	
	AlignWithGround();
}

//TODO: adjustable ackermann-like parameters
///set the steering angle to "value", where 1.0 is maximum right lock and -1.0 is maximum left lock.
void CARDYNAMICS::SetSteering ( const T value )
{
	T steerangle = value * maxangle; //steering angle in degrees

	//ackermann stuff
	T alpha = std::abs ( steerangle * 3.141593/180.0 ); //outside wheel steering angle in radians
	T B = wheel[FRONT_LEFT].GetExtendedPosition() [1] - wheel[FRONT_RIGHT].GetExtendedPosition() [1]; //distance between front wheels
	T L = wheel[FRONT_LEFT].GetExtendedPosition() [0] - wheel[REAR_LEFT].GetExtendedPosition() [0]; //distance between front and rear wheels
	T beta = atan2 ( 1.0, ( ( 1.0/ ( tan ( alpha ) ) )-B/L ) ); //inside wheel steering angle in radians

	T left_wheel_angle = 0;
	T right_wheel_angle = 0;

	if ( value >= 0 )
	{
		left_wheel_angle = alpha;
		right_wheel_angle = beta;
	}
	else
	{
		right_wheel_angle = -alpha;
		left_wheel_angle = -beta;
	}

	left_wheel_angle *= 180.0/3.141593;
	right_wheel_angle *= 180.0/3.141593;

	wheel[FRONT_LEFT].SetSteerAngle ( left_wheel_angle );
	wheel[FRONT_RIGHT].SetSteerAngle ( right_wheel_angle );
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
	telemetry.Update ( dt );
}

/// print debug info to the given ostream.  set p1, p2, etc if debug info part 1, and/or part 2, etc is desired
void CARDYNAMICS::DebugPrint ( std::ostream & out, bool p1, bool p2, bool p3, bool p4 )
{
	if ( p1 )
	{
		out.precision ( 2 );
		out << "---Body---" << std::endl;
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
		for ( std::vector <CARAERO<T> >::iterator i = aerodynamics.begin(); i != aerodynamics.end(); ++i )
		{
			i->DebugPrint ( out );
			out << std::endl;
		}
	}
}

bool CARDYNAMICS::Serialize ( joeserialize::Serializer & s )
{
	_SERIALIZE_ ( s,body );
	_SERIALIZE_ ( s,engine );
	_SERIALIZE_ ( s,clutch );
	_SERIALIZE_ ( s,transmission );
	_SERIALIZE_ ( s,front_differential );
	_SERIALIZE_ ( s,rear_differential );
	_SERIALIZE_ ( s,center_differential );
	_SERIALIZE_ ( s,fuel_tank );
	_SERIALIZE_ ( s,suspension );
	_SERIALIZE_ ( s,wheel );
	_SERIALIZE_ ( s,brake );
	_SERIALIZE_ ( s,tire );
	_SERIALIZE_ ( s,aerodynamics );
	_SERIALIZE_ ( s,wheel_velocity );
	_SERIALIZE_ ( s,abs );
	_SERIALIZE_ ( s,abs_active );
	_SERIALIZE_ ( s,tcs );
	_SERIALIZE_ ( s,tcs_active );
	_SERIALIZE_ (s,last_auto_clutch);
	_SERIALIZE_ (s,remaining_shift_time);
	_SERIALIZE_ (s,shift_gear);
	_SERIALIZE_ (s,shifted);
	_SERIALIZE_ (s,autoshift);
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
	return body.GetOrientation();
	//return ToMathQuaternion<T>(chassis->getOrientation());
}

MATHVECTOR <T, 3> CARDYNAMICS::Position() const
{
	return body.GetPosition();
	//return ToMathVector<T>(chassis->getCenterOfMassPosition());
}

MATHVECTOR <T, 3> CARDYNAMICS::LocalToWorld(const MATHVECTOR <T, 3> & local) const
{
	MATHVECTOR <T,3> position = local - center_of_mass;
	body.GetOrientation().RotateVector(position);
	return position + body.GetPosition();
	
	//btVector3 position = chassis->getCenterOfMassTransform().getBasis() * ToBulletVector(local - center_of_mass);
	//position = position + chassis->getCenterOfMassTransform().getOrigin();
	//return ToMathVector <T> (position);
}

//simple hinge (arc) suspension displacement
MATHVECTOR <T, 3> CARDYNAMICS::GetLocalWheelPosition(WHEEL_POSITION wp, T displacement_percent) const
{
	//const
	const MATHVECTOR <T, 3> & wheelext = wheel[wp].GetExtendedPosition();
	const MATHVECTOR <T, 3> & hinge = suspension[wp].GetHinge();
	MATHVECTOR <T, 3> relwheelext = wheelext - hinge;
	MATHVECTOR <T, 3> up (0, 0, 1);
	MATHVECTOR <T, 3> rotaxis = up.cross ( relwheelext.Normalize() );
	T hingeradius = relwheelext.Magnitude();
	T travel = suspension[wp].GetTravel();
	//const

	T displacement = displacement_percent * travel;
	T displacementradians = displacement / hingeradius;
	QUATERNION <T> hingerotate;
	hingerotate.Rotate ( -displacementradians, rotaxis[0], rotaxis[1], rotaxis[2] );
	MATHVECTOR <T, 3> localwheelpos = relwheelext;
	hingerotate.RotateVector ( localwheelpos );
	return localwheelpos + hinge;
}

///returns the orientation of the wheel due only to steering and suspension
QUATERNION <T> CARDYNAMICS::GetWheelSteeringAndSuspensionOrientation ( WHEEL_POSITION wp ) const
{
	QUATERNION <T> steer;
	steer.Rotate ( -wheel[wp].GetSteerAngle() * 3.141593/180.0, 0, 0, 1 );

	QUATERNION <T> camber;
	T camber_rotation = -suspension[wp].GetCamber() * 3.141593/180.0;
	if ( wp == 1 || wp == 3 )
		camber_rotation = -camber_rotation;
	camber.Rotate ( camber_rotation, 1, 0, 0 );

	QUATERNION <T> toe;
	T toe_rotation = suspension[wp].GetToe() * 3.141593/180.0;
	if ( wp == 0 || wp == 2 )
		toe_rotation = -toe_rotation;
	toe.Rotate ( toe_rotation, 0, 0, 1 );

	return camber * toe * steer;
}

/// worldspace position of the center of the wheel when the suspension is compressed by the displacement_percent where 1.0 is fully compressed
MATHVECTOR <T, 3> CARDYNAMICS::GetWheelPositionAtDisplacement(WHEEL_POSITION wp, T displacement_percent) const
{
	return LocalToWorld(GetLocalWheelPosition(wp, displacement_percent));
}

void CARDYNAMICS::ApplyForce(const MATHVECTOR <T, 3> & force)
{
	body.ApplyForce(force);
	//chassis->applyCentralForce(ToBulletVector(force));
}

void CARDYNAMICS::ApplyForce(const MATHVECTOR <T, 3> & force, const MATHVECTOR <T, 3> & offset)
{
	body.ApplyForce(force, offset);
	//chassis->applyForce(ToBulletVector(force), ToBulletVector(offset));
}

void CARDYNAMICS::ApplyTorque(const MATHVECTOR <T, 3> & torque)
{
	body.ApplyTorque(torque);
	//if(torque.MagnitudeSquared() > 1E-6)
	//	chassis->applyTorque(ToBulletVector(torque));
}

void CARDYNAMICS::UpdateWheelVelocity()
{
	for(int i = 0; i < WHEEL_POSITION_SIZE; i++)
	{
		wheel_velocity[i] = body.GetVelocity(wheel_position[i] - body.GetPosition());
		//btVector3 offset = ToBulletVector(wheel_position[i]) - chassis->getCenterOfMassPosition();
		//wheel_velocity[i] = ToMathVector<T>(chassis->getVelocityInLocalPoint(offset));
	}
}

void CARDYNAMICS::ApplyEngineTorqueToBody()
{
	MATHVECTOR <T, 3> engine_torque(-engine.GetTorque(), 0, 0);
	Orientation().RotateVector(engine_torque);
	ApplyTorque(engine_torque);
}

void CARDYNAMICS::ApplyAerodynamicsToBody(T dt)
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
	ApplyForce(wind_force);
	ApplyTorque(wind_torque);

	//apply rotational damping
	//MATHVECTOR <T, 3> rotational_aero_drag = - ToMathVector<T>(chassis->getAngularVelocity()) * 1000.0f;
	//ApplyTorque(rotational_aero_drag);
}

MATHVECTOR <T, 3> CARDYNAMICS::UpdateSuspension ( int i , T dt )
{
	// displacement
	const T posx = wheel_contact[i].GetPosition() [0];
	const T posz = wheel_contact[i].GetPosition() [2];
	const TRACKSURFACE & surface = wheel_contact[i].GetSurface();
	T phase = 0;
	if (surface.bumpWaveLength > 0.0001)
		phase = 2 * 3.141593 * ( posx+posz ) / surface.bumpWaveLength;
	T shift = 2.0 * sin ( phase*1.414214 );
	T amplitude = 0.25 * surface.bumpAmplitude;
	T bumpoffset = amplitude * ( sin ( phase + shift ) + sin ( 1.414214*phase ) - 2.0 );
	T displacement = suspension[i].GetDisplacement() + 2 * tire[i].GetRadius() + bumpoffset - wheel_contact[i].GetDepth();

	// compute suspension force
	T springdampforce = suspension[WHEEL_POSITION ( i ) ].Update ( dt , displacement );

	//do anti-roll
	int otheri = i;
	if ( i == 0 || i == 2 )
		otheri++;
	else
		otheri--;
	T antirollforce = suspension[WHEEL_POSITION ( i ) ].GetAntiRollK() *
	                  ( suspension[WHEEL_POSITION ( i ) ].GetDisplacement()-
	                    suspension[WHEEL_POSITION ( otheri ) ].GetDisplacement() );
	//suspension[WHEEL_POSITION(i)].SetAntiRollInfo(antirollforce);
	assert ( !isnan ( antirollforce ) );

	//find the vector direction to apply the suspension force
#ifdef SUSPENSION_FORCE_DIRECTION
	const MATHVECTOR <T, 3> & wheelext = wheel[i].GetExtendedPosition();
	const MATHVECTOR <T, 3> & hinge = suspension[i].GetHinge();
	MATHVECTOR <T, 3> relwheelext = wheelext - hinge;
	MATHVECTOR <T, 3> up ( 0,0,1 );
	MATHVECTOR <T, 3> rotaxis = up.cross ( relwheelext.Normalize() );
	MATHVECTOR <T, 3> forcedirection = relwheelext.Normalize().cross ( rotaxis );
	//std::cout << i << ". " << forcedirection << std::endl;
	MATHVECTOR <T, 3> suspension_force = forcedirection * ( antirollforce+springdampforce );
#else
	MATHVECTOR <T, 3> suspension_force(0, 0, antirollforce + springdampforce);
#endif
	Orientation().RotateVector(suspension_force);
	return suspension_force;
}

// aplies tire friction  to car, returns friction in world space
MATHVECTOR <T, 3> CARDYNAMICS::ApplyTireForce(int i, const T normal_force, const QUATERNION <T> & wheel_space)
{
	CARWHEEL<T> & wheel = this->wheel[WHEEL_POSITION(i)];
	CARTIRE<T> & tire = this->tire[WHEEL_POSITION(i)];
	const COLLISION_CONTACT & wheel_contact = this->wheel_contact[WHEEL_POSITION(i)];
	const TRACKSURFACE & surface = wheel_contact.GetSurface();
	const MATHVECTOR <T, 3> surface_normal = wheel_contact.GetNormal();

	// camber relative to surface(clockwise in wheel heading direction)
	MATHVECTOR <T, 3> wheel_axis(0, 1, 0);
	wheel_space.RotateVector(wheel_axis); // wheel axis in world space (wheel plane normal)
	T camber_sin = wheel_axis.dot(surface_normal);
	T camber_rad = asin(camber_sin);
	wheel.SetCamberDeg(camber_rad * 180.0/3.141593);

	// tire space(SAE Tire Coordinate System)
	// surface normal is z-axis
	// wheel axis projected on surface plane is y-axis
	MATHVECTOR <T, 3> y_axis = wheel_axis - surface_normal * camber_sin;
	MATHVECTOR <T, 3> x_axis = y_axis.cross(surface_normal);

	// wheel center velocity in tire space
	MATHVECTOR <T, 3> hub_velocity;
	hub_velocity[0] = x_axis.dot(wheel_velocity[WHEEL_POSITION(i)]);
	hub_velocity[1] = y_axis.dot(wheel_velocity[WHEEL_POSITION(i)]);
	hub_velocity[2] = 0; // unused

	// rearward speed of the contact patch
	T patch_speed = wheel.GetAngularVelocity() * tire.GetRadius();

	// friction force in tire space
	T friction_coeff = tire.GetTread() * surface.frictionTread + (1.0 - tire.GetTread()) * surface.frictionNonTread;
	T roll_friction_coeff = surface.rollResistanceCoefficient;
	MATHVECTOR <T, 3> friction_force(0);
	if(friction_coeff > 0)
		friction_force = tire.GetForce(normal_force, friction_coeff, roll_friction_coeff, hub_velocity, patch_speed, camber_rad);

	// set force feedback (aligning torque in tire space)
	tire.SetFeedback(friction_force[2]);

	// friction force in world space
	MATHVECTOR <T, 3> world_friction_force = x_axis * friction_force[0] + y_axis * friction_force[1];

	// fake viscous friction (sand, gravel, mud)
	MATHVECTOR <T, 3> wheel_drag = - (x_axis * hub_velocity[0] + y_axis * hub_velocity[1]) * surface.rollingDrag;

	// apply forces to body
	MATHVECTOR <T, 3> wheel_normal(0, 0, 1);
	wheel_space.RotateVector(wheel_normal);
	MATHVECTOR <T, 3> contactpos = wheel_position[WHEEL_POSITION(i)] - wheel_normal * tire.GetRadius();
	ApplyForce(world_friction_force + surface_normal * normal_force + wheel_drag, contactpos - Position());

	return world_friction_force;
}

void CARDYNAMICS::ApplyWheelTorque(T dt, T drive_torque, int i, MATHVECTOR <T, 3> tire_friction, const QUATERNION <T> & wheel_space)
{
	CARWHEEL<T> & wheel = this->wheel[WHEEL_POSITION(i)];
	CARTIRE<T> & tire = this->tire[WHEEL_POSITION(i)];
	CARBRAKE<T> & brake = this->brake[WHEEL_POSITION(i)];

	// tire force / torque
	wheel.Integrate1(dt);

	(-wheel_space).RotateVector(tire_friction);

	// torques acting on wheel
	T friction_torque = tire_friction[0] * tire.GetRadius();
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

	// apply torque to body
	MATHVECTOR <T, 3> world_wheel_torque(0, -wheel_torque, 0);
	wheel_space.RotateVector(world_wheel_torque);
	ApplyTorque(world_wheel_torque);
}

void CARDYNAMICS::UpdateBody(T dt, T drive_torque[])
{
	body.Integrate1(dt);
	//chassis->clearForces();

	UpdateWheelVelocity();

	ApplyEngineTorqueToBody();

	ApplyAerodynamicsToBody(dt);

	T normal_force[WHEEL_POSITION_SIZE];
	for(int i = 0; i < WHEEL_POSITION_SIZE; i++)
	{
		MATHVECTOR <T, 3> suspension_force = UpdateSuspension(i, dt);
		normal_force[i] = suspension_force.dot(wheel_contact[i].GetNormal());
		if (normal_force[i] < 0) normal_force[i] = 0;

		MATHVECTOR <T, 3> tire_friction = ApplyTireForce(i, normal_force[i], wheel_orientation[i]);
		ApplyWheelTorque(dt, drive_torque[i], i, tire_friction, wheel_orientation[i]);
	}

	body.Integrate2(dt);
	//chassis->integrateVelocities(dt);

	// update wheel state
	for(int i = 0; i < WHEEL_POSITION_SIZE; i++)
	{
		wheel_position[i] = GetWheelPositionAtDisplacement(WHEEL_POSITION(i), suspension[i].GetDisplacementPercent());
		wheel_orientation[i] = Orientation() * GetWheelSteeringAndSuspensionOrientation(WHEEL_POSITION(i));
	}
	InterpolateWheelContacts(dt);

	for(int i = 0; i < WHEEL_POSITION_SIZE; i++)
	{
		if (abs) DoABS(i, normal_force[i]);
		if (tcs) DoTCS(i, normal_force[i]);
	}
}

void CARDYNAMICS::Tick ( T dt )
{
	// has to happen before UpdateDriveline, overrides clutch, throttle
	UpdateTransmission(dt);

	const int num_repeats = 10;
	const float internal_dt = dt / num_repeats;
	for(int i = 0; i < num_repeats; ++i)
	{
		T drive_torque[WHEEL_POSITION_SIZE];

		UpdateDriveline(internal_dt, drive_torque);

		UpdateBody(internal_dt, drive_torque);

		feedback += 0.5 * (tire[FRONT_LEFT].GetFeedback() + tire[FRONT_RIGHT].GetFeedback());
	}

	feedback /= (num_repeats + 1);

	fuel_tank.Consume ( engine.FuelRate() * dt );
	engine.SetOutOfGas ( fuel_tank.Empty() );

	const float tacho_factor = 0.1;
	tacho_rpm = engine.GetRPM() * tacho_factor + tacho_rpm * (1.0 - tacho_factor);

	UpdateTelemetry(dt);
}

void CARDYNAMICS::SynchronizeBody()
{
	MATHVECTOR<T, 3> v = ToMathVector<T>(chassis->getLinearVelocity());
	MATHVECTOR<T, 3> w = ToMathVector<T>(chassis->getAngularVelocity());
	MATHVECTOR<T, 3> p = ToMathVector<T>(chassis->getCenterOfMassPosition());
	QUATERNION<T> q = ToMathQuaternion<T>(chassis->getOrientation());
	body.SetPosition(p);
	body.SetOrientation(q);
	body.SetVelocity(v);
	body.SetAngularVelocity(w);
}

void CARDYNAMICS::SynchronizeChassis()
{
	chassis->setLinearVelocity(ToBulletVector(body.GetVelocity()));
	chassis->setAngularVelocity(ToBulletVector(body.GetAngularVelocity()));
}

void CARDYNAMICS::UpdateWheelContacts()
{
	MATHVECTOR <float, 3> raydir = GetDownVector();
	for (int i = 0; i < WHEEL_POSITION_SIZE; i++)
	{
		COLLISION_CONTACT & wheelContact = wheel_contact[WHEEL_POSITION(i)];
		MATHVECTOR <float, 3> raystart = wheel_position[i];
		raystart = raystart - raydir * tire[i].GetRadius();
		float raylen = 5;
		world->CastRay(raystart, raydir, raylen, wheelContact);
	}
}

void CARDYNAMICS::InterpolateWheelContacts(T dt)
{
	MATHVECTOR <float, 3> raydir = GetDownVector();
	for (int i = 0; i < WHEEL_POSITION_SIZE; i++)
	{
		MATHVECTOR <float, 3> raystart = wheel_position[i];
		raystart = raystart - raydir * tire[i].GetRadius();
		float raylen = 5;
		GetWheelContact(WHEEL_POSITION(i)).CastRay(raystart, raydir, raylen);
	}
}

///calculate the center of mass, calculate the total mass of the body, calculate the inertia tensor
/// then store this information in the rigid body
void CARDYNAMICS::UpdateMass()
{
	typedef std::pair <T, MATHVECTOR <T, 3> > MASS_PAIR;

	T total_mass ( 0 );

	center_of_mass.Set ( 0,0,0 );

	//calculate the total mass, and center of mass
	for ( std::list <MASS_PAIR>::iterator i = mass_only_particles.begin(); i != mass_only_particles.end(); ++i )
	{
		//add the current mass to the total mass
		total_mass += i->first;

		//incorporate the current mass into the center of mass
		center_of_mass = center_of_mass + i->second * i->first;
	}

	//account for fuel
	total_mass += fuel_tank.GetMass();
	center_of_mass =  center_of_mass + fuel_tank.GetPosition() * fuel_tank.GetMass();

	body.SetMass ( total_mass );

	center_of_mass = center_of_mass * ( 1.0 / total_mass );

	//calculate the inertia tensor (is symmetric)
	MATRIX3 <T> inertia;
	for ( int i = 0; i < 9; i++ ) inertia[i] = 0;
	for ( std::list <MASS_PAIR>::iterator i = mass_only_particles.begin(); i != mass_only_particles.end(); ++i )
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
	body.SetInertia ( inertia );
}

void CARDYNAMICS::UpdateDriveline(T dt, T drive_torque[])
{
	engine.Integrate1(dt);

	T driveshaft_speed = CalculateDriveshaftSpeed();
	T clutch_speed = transmission.CalculateClutchSpeed(driveshaft_speed);
	T crankshaft_speed = engine.GetAngularVelocity();
	T engine_drag = clutch.GetTorque(crankshaft_speed, clutch_speed);

	engine.ComputeForces();

	ApplyClutchTorque(engine_drag, clutch_speed);

	engine.ApplyForces();

	CalculateDriveTorque(drive_torque, engine_drag);

	engine.Integrate2(dt);
}

///apply forces on the engine due to drag from the clutch
void CARDYNAMICS::ApplyClutchTorque ( T engine_drag, T clutch_speed )
{
	if ( transmission.GetGear() == 0 )
	{
		engine.SetClutchTorque ( 0.0 );
	}
	else
	{
		engine.SetClutchTorque ( engine_drag );
	}
}

///calculate the drive torque that the engine applies to each wheel, and put the output into the supplied 4-element array
void CARDYNAMICS::CalculateDriveTorque ( T * wheel_drive_torque, T clutch_torque )
{
	T driveshaft_torque = transmission.GetTorque ( clutch_torque );
	assert ( !isnan ( driveshaft_torque ) );

	for ( int i = 0; i < WHEEL_POSITION_SIZE; i++ )
		wheel_drive_torque[i] = 0;

	if ( drive == RWD )
	{
		rear_differential.ComputeWheelTorques ( driveshaft_torque );
		wheel_drive_torque[REAR_LEFT] = rear_differential.GetSide1Torque();
		wheel_drive_torque[REAR_RIGHT] = rear_differential.GetSide2Torque();
	}
	else if ( drive == FWD )
	{
		front_differential.ComputeWheelTorques ( driveshaft_torque );
		wheel_drive_torque[FRONT_LEFT] = front_differential.GetSide1Torque();
		wheel_drive_torque[FRONT_RIGHT] = front_differential.GetSide2Torque();
	}
	else if ( drive == AWD )
	{
		center_differential.ComputeWheelTorques ( driveshaft_torque );
		front_differential.ComputeWheelTorques ( center_differential.GetSide1Torque() );
		rear_differential.ComputeWheelTorques ( center_differential.GetSide2Torque() );
		wheel_drive_torque[FRONT_LEFT] = front_differential.GetSide1Torque();
		wheel_drive_torque[FRONT_RIGHT] = front_differential.GetSide2Torque();
		wheel_drive_torque[REAR_LEFT] = rear_differential.GetSide1Torque();
		wheel_drive_torque[REAR_RIGHT] = rear_differential.GetSide2Torque();
	}

	for ( int i = 0; i < WHEEL_POSITION_SIZE; i++ ) assert ( !isnan ( wheel_drive_torque[WHEEL_POSITION ( i ) ] ) );
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

	if (remaining_shift_time <= shift_time * 0.5 && !shifted)
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
void CARDYNAMICS::DoTCS ( int i, T suspension_force )
{
	T gasthresh = 0.1;
	T gas = engine.GetThrottle();

	//only active if throttle commanded past threshold
	if ( gas > gasthresh )
	{
		//see if we're spinning faster than the rest of the wheels
		T maxspindiff = 0;
		T myrotationalspeed = wheel[WHEEL_POSITION ( i ) ].GetAngularVelocity();
		for ( int i2 = 0; i2 < WHEEL_POSITION_SIZE; i2++ )
		{
			T spindiff = myrotationalspeed - wheel[WHEEL_POSITION ( i2 ) ].GetAngularVelocity();
			if ( spindiff < 0 )
				spindiff = -spindiff;
			if ( spindiff > maxspindiff )
				maxspindiff = spindiff;
		}

		//don't engage if all wheels are moving at the same rate
		if ( maxspindiff > 1.0 )
		{
			//sp is the ideal slip ratio given tire loading
			T sp ( 0 ), ah ( 0 );
			tire[WHEEL_POSITION ( i ) ].LookupSigmaHatAlphaHat ( suspension_force, sp, ah );

			T sense = 1.0;
			if ( transmission.GetGear() < 0 )
				sense = -1.0;

			T error = tire[WHEEL_POSITION ( i ) ].GetSlide() * sense - sp;
			T thresholdeng = 0.0;
			T thresholddis = -sp/2.0;

			if ( error > thresholdeng && ! tcs_active[i] )
				tcs_active[i] = true;

			if ( error < thresholddis && tcs_active[i] )
				tcs_active[i] = false;

			if ( tcs_active[i] )
			{
				T curclutch = clutch.GetClutch();
				if ( curclutch > 1 ) curclutch = 1;
				if ( curclutch < 0 ) curclutch = 0;

				gas = gas - error * 10.0 * curclutch;
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
void CARDYNAMICS::DoABS ( int i, T suspension_force )
{
	T braketresh = 0.1;
	T brakesetting = brake[WHEEL_POSITION ( i ) ].GetBrakeFactor();

	//only active if brakes commanded past threshold
	if ( brakesetting > braketresh )
	{
		T maxspeed = 0;
		for ( int i2 = 0; i2 < WHEEL_POSITION_SIZE; i2++ )
		{
			if ( wheel[WHEEL_POSITION ( i2 ) ].GetAngularVelocity() > maxspeed )
				maxspeed = wheel[WHEEL_POSITION ( i2 ) ].GetAngularVelocity();
		}

		//don't engage ABS if all wheels are moving slowly
		if ( maxspeed > 6.0 )
		{
			//sp is the ideal slip ratio given tire loading
			T sp ( 0 ), ah ( 0 );
			tire[WHEEL_POSITION ( i ) ].LookupSigmaHatAlphaHat ( suspension_force, sp, ah );

			T error = - tire[WHEEL_POSITION ( i ) ].GetSlide() - sp;
			T thresholdeng = 0.0;
			T thresholddis = -sp/2.0;

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
		brake[WHEEL_POSITION ( i ) ].SetBrakeFactor ( 0.0 );
}

///Set the maximum steering angle in degrees
void CARDYNAMICS::SetMaxSteeringAngle ( T newangle )
{
	maxangle = newangle;
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

void CARDYNAMICS::AddMassParticle ( T newmass, MATHVECTOR <T, 3> newpos )
{
	mass_only_particles.push_back ( std::pair <T, MATHVECTOR <T, 3> > ( newmass, newpos ) );
	//std::cout << "adding mass particle " << newmass << " at " << newpos << std::endl;
}

void CARDYNAMICS::AddAerodynamicDevice (
	const MATHVECTOR <T, 3> & newpos,
	T drag_frontal_area,
	T drag_coefficient,
	T lift_surface_area,
	T lift_coefficient,
	T lift_efficiency )
{
	aerodynamics.push_back ( CARAERO<T>() );
	aerodynamics.back().Set ( newpos, drag_frontal_area, drag_coefficient, lift_surface_area,
	                          lift_coefficient, lift_efficiency );
}
