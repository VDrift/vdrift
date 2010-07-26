#include "cardynamics.h"

#include "configfile.h"
#include "tracksurface.h"
#include "coordinatesystems.h"
#include "collision_world.h"
#include "tobullet.h"

#if defined(_WIN32) || defined(__APPLE__)
template <typename T> bool isnan(T number) {return (number != number);}
#endif

typedef CARDYNAMICS::T T;

CARDYNAMICS::CARDYNAMICS() :
	world(NULL),
	chassis(NULL),
	maxangle(45.0),
	telemetry("telemetry")
{
	Init();
}

void CARDYNAMICS::Init()
{
	suspension.resize ( WHEEL_POSITION_SIZE );
	wheel_velocity.resize (WHEEL_POSITION_SIZE);
	wheel_position.resize ( WHEEL_POSITION_SIZE );
	wheel_orientation.resize ( WHEEL_POSITION_SIZE );
	wheel_contact.resize ( WHEEL_POSITION_SIZE );
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
		WHEEL_POSITION pos = WHEEL_POSITION(i);
		btVector3 wheelHSize( GetTire(pos).GetRadius(), GetTire(pos).GetSidewallWidth() * 0.5, GetTire(pos).GetRadius()); 
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
		wheel_velocity[i].Set(0.0);
		wheel_position[i] = LocalToWorld(suspension[i].GetWheelPosition(0.0));
		wheel_orientation[i] = body.GetOrientation() * suspension[i].GetWheelOrientation();
	}
	
	AlignWithGround();

	StartEngine();
}

// executed as last function(after integration) in bullet singlestepsimulation
void CARDYNAMICS::updateAction(btCollisionWorld * collisionWorld, btScalar dt)
{
	// get external force, torque
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
	return chassisRotation * suspension[wp].GetWheelOrientation() * powertrain.GetWheel(wp).GetRotation();
}

QUATERNION <T> CARDYNAMICS::GetUprightOrientation(WHEEL_POSITION wp) const
{
	return chassisRotation * suspension[wp].GetWheelOrientation();
}

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
	return body.GetVelocity();
}

MATHVECTOR <T, 3> CARDYNAMICS::GetEnginePosition() const
{
	btTransform trans;
	chassis->getMotionState()->getWorldTransform(trans);
	btVector3 pos = trans * ToBulletVector(GetEngine().GetPosition() - center_of_mass);
	return ToMathVector<T>(pos);
}

void CARDYNAMICS::SetPosition(const MATHVECTOR<T, 3> & position)
{
	body.SetPosition(position);
}

void CARDYNAMICS::AlignWithGround()
{
	UpdateWheelTransform();
	UpdateWheelContacts();
	
	T min_height = 0;
	bool no_min_height = true;
	for (int i = 0; i < WHEEL_POSITION_SIZE; i++)
	{
		T height = wheel_contact[i].GetDepth() - 2 * GetTire(WHEEL_POSITION(i)).GetRadius();
		if (height < min_height || no_min_height)
		{
			min_height = height;
			no_min_height = false;
		}
	}
	
	MATHVECTOR <T, 3> trimmed_position = body.GetPosition() + GetDownVector() * min_height;
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
	telemetry.Update ( dt );
}

/// print debug info to the given ostream.  set p1, p2, etc if debug info part 1, and/or part 2, etc is desired
void CARDYNAMICS::DebugPrint ( std::ostream & out, bool p1, bool p2, bool p3, bool p4 ) const
{
	if (p1)
	{
		out.precision(2);
		out << "---Body---" << std::endl;
		out << "Position: " << chassisPosition << std::endl;
		out << "Center of mass: " << center_of_mass << std::endl;
		out.precision(6);
		out << "Total mass: " << body.GetMass() << std::endl;
		out << std::endl;
	}

	powertrain.DebugPrint(out, p1, p2);

	if (p3)
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
	}

	if (p4)
	{
		for (std::vector <CARAERO <T> >::const_iterator i = aerodynamics.begin(); i != aerodynamics.end(); ++i)
		{
			i->DebugPrint(out);
			out << std::endl;
		}
	}
}

bool CARDYNAMICS::Serialize(joeserialize::Serializer & s)
{
	_SERIALIZE_(s, body);
	_SERIALIZE_(s, powertrain);
	_SERIALIZE_(s, suspension);
	_SERIALIZE_(s, aerodynamics);
	_SERIALIZE_(s, wheel_velocity);
	return true;
}

MATHVECTOR <T, 3> CARDYNAMICS::GetDownVector() const
{
	MATHVECTOR <T, 3> v(0, 0, -1);
	body.GetOrientation().RotateVector(v);
	return v;
}

QUATERNION <T> CARDYNAMICS::LocalToWorld(const QUATERNION <T> & local) const
{
	return body.GetOrientation() * local;
}

MATHVECTOR <T, 3> CARDYNAMICS::LocalToWorld(const MATHVECTOR <T, 3> & local) const
{
	MATHVECTOR <T,3> position = local - center_of_mass;
	body.GetOrientation().RotateVector(position);
	return position + body.GetPosition();
}

void CARDYNAMICS::ApplyForce(const MATHVECTOR <T, 3> & force)
{
	body.ApplyForce(force);
}

void CARDYNAMICS::ApplyForce(const MATHVECTOR <T, 3> & force, const MATHVECTOR <T, 3> & offset)
{
	body.ApplyForce(force, offset);
}

void CARDYNAMICS::ApplyTorque(const MATHVECTOR <T, 3> & torque)
{
	body.ApplyTorque(torque);
}

void CARDYNAMICS::UpdateWheelVelocity()
{
	for(int i = 0; i < WHEEL_POSITION_SIZE; i++)
	{
		wheel_velocity[i] = body.GetVelocity(wheel_position[i] - body.GetPosition());
	}
}

void CARDYNAMICS::UpdateWheelTransform()
{
	for(int i = 0; i < WHEEL_POSITION_SIZE; i++)
	{
		wheel_position[i] = LocalToWorld(suspension[i].GetWheelPosition());
		wheel_orientation[i] = LocalToWorld(suspension[i].GetWheelOrientation());
	}
}

void CARDYNAMICS::ApplyEngineTorqueToBody()
{
	MATHVECTOR <T, 3> engine_torque(-GetEngine().GetTorque(), 0, 0);
	body.GetOrientation().RotateVector(engine_torque);
	ApplyTorque(engine_torque);
}

void CARDYNAMICS::AddAerodynamics(MATHVECTOR<T, 3> & force, MATHVECTOR<T, 3> & torque)
{
	MATHVECTOR <T, 3> wind_force(0);
	MATHVECTOR <T, 3> wind_torque(0);
	MATHVECTOR <T, 3> air_velocity = -GetVelocity();
	(-body.GetOrientation()).RotateVector(air_velocity);
	for(std::vector <CARAERO <T> >::iterator i = aerodynamics.begin(); i != aerodynamics.end(); ++i)
	{
		MATHVECTOR <T, 3> force = i->GetForce(air_velocity);
		wind_force = wind_force + force;
		wind_torque = wind_torque + (i->GetPosition() - center_of_mass).cross(force);
	}
	body.GetOrientation().RotateVector(wind_force);
	body.GetOrientation().RotateVector(wind_torque);
	force = force + wind_force;
	torque = torque + wind_torque;
}

// returns normal force(wheel force)
T CARDYNAMICS::UpdateSuspension(int i, T dt)
{
	// velocity, displacment along wheel ray
	T velocity = 0;//-GetDownVector().dot(wheel_velocity[i]);
	T displacement = 2.0 * GetTire(WHEEL_POSITION(i)).GetRadius() - wheel_contact[i].GetDepth();
	
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

T CARDYNAMICS::ApplyTireForce(WHEEL_POSITION i, const T normal_force, const QUATERNION <T> & wheel_space)
{
	const CARWHEEL <T> & wheel = GetWheel(i);
	const CARTIRE <T> & tire = GetTire(i);
	const COLLISION_CONTACT & wheel_contact = GetWheelContact(i);
	const TRACKSURFACE & surface = wheel_contact.GetSurface();
	const MATHVECTOR <T, 3> surface_normal = wheel_contact.GetNormal();
	
	// spin axis is the wheel plane normal
	// positive inclination is in clockwise direction
	MATHVECTOR <T, 3> spin_axis(0, 1, 0);	
	wheel_space.RotateVector(spin_axis);
	T axis_proj = spin_axis.dot(surface_normal);
	T inclination = 90 - acos(axis_proj)  * 180.0 / M_PI;
	if (!(i & 1)) inclination = -inclination;
	
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
		friction_force = powertrain.GetTireForce(i, normal_force, friction_coeff, velocity, ang_velocity, inclination);
	}
	
	// rolling resistance due to tire/surface deformation proportional to normal force
	T roll_resistance = 0;//tire.GetRollingResistance(ang_velocity, normal_force, surface.rollResistanceCoefficient);
	
	// friction force in world space
	MATHVECTOR <T, 3> tire_friction = x * (friction_force[0] + roll_resistance) + y * friction_force[1];
	
	// fake viscous friction (sand, gravel, mud) proportional to wheel center velocity
	MATHVECTOR <T, 3> wheel_drag = -(x * velocity[0] + y * velocity[1]) * surface.rollingDrag * 0.25; // scale wheel drag by 1/4
	
	// tire friction + tire normal force
	MATHVECTOR <T, 3> rel_contact_pos = wheel_contact.GetPosition() - body.GetPosition();
	ApplyForce(surface_normal * normal_force + tire_friction + wheel_drag, rel_contact_pos);
	
	return friction_force[0] * tire.GetRadius();
}

void CARDYNAMICS::UpdateBody(const MATHVECTOR <T, 3> & ext_force, const MATHVECTOR <T, 3> & ext_torque, T dt)
{
	body.Integrate1(dt);

	UpdateWheelVelocity();
	UpdateWheelTransform();
	InterpolateWheelContacts();

	ApplyEngineTorqueToBody();

	// apply external force/torque
	body.ApplyForce(ext_force);
	body.ApplyTorque(ext_torque);

	// update suspension
	T normal_force[WHEEL_POSITION_SIZE];
	for(int i = 0; i < WHEEL_POSITION_SIZE; i++)
	{
		normal_force[i] = UpdateSuspension(i, dt);
	}
	
	// update powertrain
	T drive_torque[WHEEL_POSITION_SIZE];
	powertrain.IntegrateEngine(drive_torque, dt);
	for(int i = 0; i < WHEEL_POSITION_SIZE; ++i)
	{
		WHEEL_POSITION pos = WHEEL_POSITION(i);
		T tire_torque = ApplyTireForce(pos, normal_force[i], wheel_orientation[i]);
		T wheel_torque = powertrain.IntegrateWheel(pos, tire_torque, drive_torque[i], dt);
		
		powertrain.UpdateABS(i, normal_force[i]);
		powertrain.UpdateTCS(i, normal_force[i]);

		MATHVECTOR <T, 3> world_wheel_torque(0, -wheel_torque, 0);
		wheel_orientation[i].RotateVector(world_wheel_torque);
		ApplyTorque(world_wheel_torque);
	}

	body.Integrate2(dt);
}

void CARDYNAMICS::Tick(MATHVECTOR<T, 3> ext_force, MATHVECTOR<T, 3> ext_torque, T dt)
{
	AddAerodynamics(ext_force, ext_torque);
	powertrain.Update(dt);

	const int num_repeats = 10;
	const float internal_dt = dt / num_repeats;
	for(int i = 0; i < num_repeats; ++i)
	{
		UpdateBody(ext_force, ext_torque, internal_dt);
		feedback += 0.5 * (GetTire(FRONT_LEFT).GetFeedback() + GetTire(FRONT_RIGHT).GetFeedback());
	}
	feedback /= (num_repeats + 1);

	UpdateTelemetry(dt);
}

void CARDYNAMICS::UpdateWheelContacts()
{
	MATHVECTOR <float, 3> raydir = GetDownVector();
	for (int i = 0; i < WHEEL_POSITION_SIZE; i++)
	{
		COLLISION_CONTACT & wheelContact = wheel_contact[WHEEL_POSITION(i)];
		MATHVECTOR <float, 3> raystart = wheel_position[i];
		raystart = raystart - raydir * GetTire(WHEEL_POSITION(i)).GetRadius();
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
		raystart = raystart - raydir * GetTire(WHEEL_POSITION(i)).GetRadius();
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
	total_mass += powertrain.GetFuelTank().GetMass();
	center_of_mass = center_of_mass + powertrain.GetFuelTank().GetPosition() * powertrain.GetFuelTank().GetMass();

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

void CARDYNAMICS::AddMassParticle(T mass, MATHVECTOR <T, 3> pos)
{
	mass_particles.push_back(std::pair <T, MATHVECTOR <T, 3> > (mass, pos));
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

bool LoadAeroDevices(
	CONFIGFILE & c,
	std::vector< CARAERO <T> > & aerodynamics,
	std::ostream & error_output)
{
	int version(1);
	c.GetParam("version", version);

	for(int i = 0; i < 10; i++)
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
	if (!LoadAeroDevices(c, aerodynamics, error_output)) return false;
	if (!powertrain.Load(c, sharedpartspath, error_output)) return false;
	
	// suspension
	maxangle = 0.0;
	for (int i = 0; i < WHEEL_POSITION_SIZE; i++)
	{
		std::stringstream num;
		num << i;
		const std::string suspensionname("suspension-"+num.str());
		if (!LoadSuspension(c, suspensionname, suspension[i], error_output)) return false;
		if (suspension[i].GetMaxSteeringAngle() > maxangle) maxangle = suspension[i].GetMaxSteeringAngle();
	}
	
	// mass particles
	AddMassParticle(GetEngine().GetMass(), GetEngine().GetPosition());
	for (int i = 0; i < WHEEL_POSITION_SIZE; i++)
	{
		AddMassParticle(GetWheel(WHEEL_POSITION(i)).GetMass(), suspension[i].GetWheelPosition());
	}
	if (!LoadMassParticles(c, mass_particles, error_output)) return false;
	UpdateMass();

	return true;
}
