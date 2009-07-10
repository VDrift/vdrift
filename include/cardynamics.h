#ifndef _CARDYNAMICS_H
#define _CARDYNAMICS_H

#include "rigidbody.h"
#include "mathvector.h"
#include "quaternion.h"
#include "carengine.h"
#include "carclutch.h"
#include "cartransmission.h"
#include "cardifferential.h"
#include "carfueltank.h"
#include "carsuspension.h"
#include "carwheel.h"
#include "cartire.h"
#include "carwheelposition.h"
#include "carbrake.h"
#include "caraerodynamicdevice.h"
#include "joeserialize.h"
#include "macros.h"
#include "carsurfacetype.h"

#include <vector>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <fstream>
#include <sstream>
#include <cmath>
#include <deque>
#include <cmath>

class CARCONTACTPROPERTIES
{
private:
	typedef double T;
	
	T wheelheight;
	MATHVECTOR <T, 3> position;
	MATHVECTOR <T, 3> normal;
	T bump_wavelength;
	T bump_amplitude;
	T friction_notread;
	T friction_tread;
	T rolling_resistance;
	T rolling_drag;
	SURFACE::CARSURFACETYPE surface;
	
public:
	CARCONTACTPROPERTIES() : wheelheight(100.0), position(0), normal(MATHVECTOR <T, 3> (0,1,0)), surface(SURFACE::ASPHALT) {}
	CARCONTACTPROPERTIES(T newwh, const MATHVECTOR <T, 3> & newpos, const MATHVECTOR <T, 3> & newnorm,
		T newbw, T newba, T fn, T ft, T rr, T rd, SURFACE::CARSURFACETYPE newsurface) :
		wheelheight(newwh),position(newpos),normal(newnorm),bump_wavelength(newbw),
		bump_amplitude(newba), friction_notread(fn), friction_tread(ft), rolling_resistance(rr),
		rolling_drag(rd),surface(newsurface) {}

	T GetWheelheight() const
	{
		return wheelheight;
	}

	MATHVECTOR< T, 3 > GetNormal() const
	{
		return normal;
	}
	
	T GetBumpWavelength() const
	{
		return bump_wavelength;
	}

	T GetBumpAmplitude() const
	{
		return bump_amplitude;
	}

	T GetFrictionTread() const
	{
		return friction_tread;
	}

	T GetRollingResistanceCoefficient() const
	{
		return rolling_resistance;
	}

	T GetRollingDrag() const
	{
		return rolling_drag;
	}

	T GetFrictionNoTread() const
	{
		return friction_notread;
	}

	MATHVECTOR< T, 3 > GetPosition() const
	{
		return position;
	}

	void SetSurface ( const SURFACE::CARSURFACETYPE& value )
	{
		surface = value;
	}

	SURFACE::CARSURFACETYPE GetSurface() const
	{
		return surface;
	}
};

class CARTELEMETRY
{
	private:
		typedef double T;
	
		std::vector <std::pair <std::string, T> > variable_names;
		T time;
		bool wroteheader;
		const std::string telemetryname;
		std::ofstream file;
		
		void WriteHeader(const std::string & filename)
		{
			std::ofstream f((filename+".plt").c_str());
			assert(f);
			f << "plot ";
			unsigned int count = 0;
			for (std::vector <std::pair <std::string, T> >::iterator i =
				variable_names.begin(); i != variable_names.end(); ++i)
			{
				f << "\\" << std::endl << "\"" << filename+".dat" << "\" u 1:" << count+2 << " t '" << i->first << "' w lines";
				if (count < variable_names.size()-1)
					f << ",";
				f << " ";
				count++;
			}
			f << std::endl;
			wroteheader = true;
		}
		
	public:
		CARTELEMETRY(const std::string & name) : time(0), wroteheader(false), telemetryname(name), file((name+".dat").c_str()) {}
		CARTELEMETRY(const CARTELEMETRY & other) : variable_names(other.variable_names), time(other.time), wroteheader(other.wroteheader), telemetryname(other.telemetryname), file((telemetryname+".dat").c_str()) {}
		
		void AddRecord(const std::string & name, T value)
		{
			bool found = false;
			for (std::vector <std::pair <std::string, T> >::iterator i =
				variable_names.begin(); i != variable_names.end(); ++i)
			{
				if (name == i->first)
				{
					i->second = value;
					found = true;
					break;
				}
			}
			if (!found)
				variable_names.push_back(std::make_pair(name, value));
		}
		
		void Update(T dt)
		{
			if (time != 0 && !wroteheader)
				WriteHeader(telemetryname);
			time += dt;
			
			assert(file);
			file << time << " ";
			for (std::vector <std::pair <std::string, T> >::iterator i =
				variable_names.begin(); i != variable_names.end(); ++i)
				file << i->second << " ";
			file << "\n";
		}
};

class CARDYNAMICS
{
friend class PERFORMANCE_TESTING;
friend class joeserialize::Serializer;
public:
	typedef double T;
private:
	//-----------Data------------
	
	//sub-components
	RIGIDBODY <T> body; ///< the rigid body is centered at the center of mass
	CARENGINE <T> engine;
	CARCLUTCH <T> clutch;
	CARTRANSMISSION <T> transmission;
	CARDIFFERENTIAL <T> front_differential;
	CARDIFFERENTIAL <T> rear_differential;
	CARDIFFERENTIAL <T> center_differential;
	CARFUELTANK <T> fuel_tank;
	std::vector <CARSUSPENSION <T> > suspension;
	std::vector <CARWHEEL <T> > wheel;
	std::vector <CARBRAKE <T> > brake;
	std::vector <CARTIRE <T> > tire;
	std::vector <CARAERO <T> > aerodynamics;
	
	//set at load time
	std::list <std::pair <T, MATHVECTOR <T, 3> > > mass_only_particles;
	enum
	{
		FWD,
		RWD,
		AWD
	} drive;
	MATHVECTOR <T, 3> center_of_mass; ///< the center of mass of the rigid body in local coordinates
	T maxangle;
	
	//set every tick
	std::vector <MATHVECTOR <T, 3> > wheel_velocity; ///< wheel (hub) velocity
	bool abs; ///<true if ABS is enabled on this car
	std::vector <int> abs_active; ///< for each wheel, whether or not the abs system is currently activating (releasing the brake)
	bool tcs; ///<true if TCS is enabled on this car
	std::vector <int> tcs_active; ///< for each wheel, whether or not the tcs system is currently activating (releasing the throttle)
	std::vector <MATHVECTOR <T, 3> > last_suspension_force; ///< needs to be held through an iteration so TCS can use it to find the optimal slip ratio
	MATHVECTOR <T, 3> lastbodyforce; ///< held so external classes can extract it for things such as applying physics to camera mounts
	//mutable std::vector <MATHVECTOR <T, 3> > wheel_position; ///< computed once per frame and cached for performance reasons
	//mutable std::vector <bool> wheel_position_valid; ///< true if wheel_position has been computed yet for this frame
	
	//set by external systems
	std::vector <CARCONTACTPROPERTIES> wheel_contacts;
	MATHVECTOR <T, 3> contact_force;
	MATHVECTOR <T, 3> contact_torque;
	
	//debug systems
	CARTELEMETRY telemetry;
	
	
	//--------Functions------------
	
	MATHVECTOR <T, 3> GetGravityForce();
	
	///use the wheel speed to calculate the driveshaft speed
	T CalculateDriveshaftSpeed();
	
	///apply forces on the engine due to drag from the clutch
	void ApplyClutchTorque(T engine_drag, T clutch_speed);
	
	///calculate the drive torque that the engine applies to each wheel, and put the output into the supplied 4-element array
	void CalculateDriveTorque(T * wheel_drive_torque, T clutch_torque);
	
	void ApplyEngineTorqueToBody(MATHVECTOR <T, 3> & total_force, MATHVECTOR <T, 3> & total_torque);
	
	void ApplyAerodynamicsToBody(MATHVECTOR <T, 3> & total_force, MATHVECTOR <T, 3> & total_torque);
	
	void ComputeSuspensionDisplacement(int i, T dt);
	
	///returns the suspension force (so it can be applied to the tires)
	MATHVECTOR <T, 3> ApplySuspensionForceToBody(int i, T dt, MATHVECTOR <T, 3> & total_force, MATHVECTOR <T, 3> & total_torque);
	
	MATHVECTOR <T, 3> ComputeTireFrictionForce(int i, T dt, const MATHVECTOR <T, 3> & suspension_force,
			bool frictionlimiting, T wheelspeed, MATHVECTOR <T, 3> & groundvel,
			const QUATERNION <T> & wheel_orientation);
	
	///do traction control system (wheelspin prevention) calculations and modify the throttle position if necessary
	void DoTCS(int i, T suspension_force);
	
	///do anti-lock brake system calculations and modify the brake force if necessary
	void DoABS(int i, T suspension_force);
	
	void ApplyWheelForces(T dt, T wheel_drive_torque, int i, const MATHVECTOR <T, 3> & suspension_force, MATHVECTOR <T, 3> & total_force, MATHVECTOR <T, 3> & total_torque);
	
	///the core function of the car dynamics simulation:  find and apply all forces on the car and components.
	void ApplyForces(T dt);
	
	///set up our data structures for storing previous wheel positions
	void InitializeWheelVelocity();
	
public:
	CARDYNAMICS();
	
	template <typename T2>
	void SetInitialConditions(const MATHVECTOR <T2, 3> & initial_position, const QUATERNION <T2> & initial_orientation)
	{
		MATHVECTOR <T, 3> initvec;
		initvec = initial_position;
		QUATERNION <T> initquat;
		initquat = initial_orientation;
		body.SetPosition(initvec);
		body.SetOrientation(initquat);
		
		body.SetInitialForce(GetGravityForce());
		initvec.Set(0,0,0);
		body.SetInitialTorque(initvec);
		
		engine.SetInitialConditions();
		
		for (int i = 0; i < WHEEL_POSITION_SIZE; i++)
			wheel[WHEEL_POSITION(i)].SetInitialConditions();
		
		InitializeWheelVelocity();
	}
	
	void SetABS(const bool newabs);
	bool GetABSEnabled() const;
	bool GetABSActive() const;
	
	void SetTCS(const bool newtcs);
	bool GetTCSEnabled() const;
	bool GetTCSActive() const;
	
	void UpdateTelemetry(float dt);
	
	const MATHVECTOR <T, 3> & GetCenterOfMassPosition() const {return body.GetPosition();}
	MATHVECTOR <T, 3> GetOriginPosition() const {MATHVECTOR <T, 3> zero;return CarLocalToWorld(zero);}
	const QUATERNION <T> & GetOrientation() const {return body.GetOrientation();}
	
	void Tick(T dt);
	
	///set the height between the center of the wheel and the ground
	void SetWheelContactProperties(WHEEL_POSITION wheel_index, T wheelheight, const MATHVECTOR <T, 3> & position, const MATHVECTOR <T, 3> & normal, T bumpwave, T bumpamp, T frict_no, T frict_tread, T roll_res, T roll_drag, SURFACE::CARSURFACETYPE surface);
	
	const CARCONTACTPROPERTIES & GetWheelContactProperties(WHEEL_POSITION wheel_index) const;
	
	///returns the worldspace position of the center of the wheel
	MATHVECTOR< T, 3 > GetWheelPosition(WHEEL_POSITION wp) const;
	
	///returns the worldspace linear velocity of the center of the wheel relative to the ground
	MATHVECTOR <T, 3> GetWheelVelocity(WHEEL_POSITION wp) const;
	
	///returns the orientation of the wheel
	QUATERNION <T> GetWheelOrientation(WHEEL_POSITION wp);
	
	///returns the orientation of the wheel, accounting for a supplied orientation adjustment for the model
	QUATERNION <T> GetWheelOrientation(WHEEL_POSITION wp, QUATERNION <T> modelorient);
	
	///returns the orientation of the wheel due only to steering and suspension
	QUATERNION <T> GetWheelSteeringAndSuspensionOrientation(WHEEL_POSITION wp) const;
	
	///returns the worldspace position of the center of the wheel when the suspension is compressed by the displacement_percent where 1.0 is fully compressed
	MATHVECTOR< T, 3 > GetWheelPositionAtDisplacement(WHEEL_POSITION wp, T displacement_percent) const;
	
	RIGIDBODY <T> & GetBody() {return body;}
	const RIGIDBODY <T> & GetBody() const {return body;}
	
	CARENGINE <T> & GetEngine() {return engine;}
	const CARENGINE <T> & GetEngine() const {return engine;}
	
	CARCLUTCH <T> & GetClutch() {return clutch;}
	const CARCLUTCH <T> & GetClutch() const {return clutch;}
	
	CARTRANSMISSION <T> & GetTransmission() {return transmission;}
	const CARTRANSMISSION <T> & GetTransmission() const {return transmission;}
	
	CARDIFFERENTIAL <T> & GetRearDifferential() {return rear_differential;}
	const CARDIFFERENTIAL <T> & GetRearDifferential() const {return rear_differential;}
	CARDIFFERENTIAL <T> & GetFrontDifferential() {return front_differential;}
	const CARDIFFERENTIAL <T> & GetFrontDifferential() const {return front_differential;}
	CARDIFFERENTIAL <T> & GetCenterDifferential() {return center_differential;}
	const CARDIFFERENTIAL <T> & GetCenterDifferential() const {return center_differential;}	
	
	CARFUELTANK <T> & GetFuelTank() {return fuel_tank;}
	const CARFUELTANK <T> & GetFuelTank() const {return fuel_tank;}
	
	CARSUSPENSION <T> & GetSuspension(WHEEL_POSITION pos) {return suspension[pos];}
	const CARSUSPENSION <T> & GetSuspension(WHEEL_POSITION pos) const {return suspension[pos];}
	
	CARWHEEL <T> & GetWheel(WHEEL_POSITION pos) {return wheel[pos];}
	const CARWHEEL <T> & GetWheel(WHEEL_POSITION pos) const {return wheel[pos];}
	
	CARTIRE <T> & GetTire(WHEEL_POSITION pos) {return tire[pos];}
	const CARTIRE <T> & GetTire(WHEEL_POSITION pos) const {return tire[pos];}
	
	CARBRAKE <T> & GetBrake(WHEEL_POSITION pos) {return brake[pos];}
	const CARBRAKE <T> & GetBrake(WHEEL_POSITION pos) const {return brake[pos];}
	
	void SetDrive(const std::string & newdrive);
	
	/// print debug info to the given ostream.  set p1, p2, etc if debug info part 1, and/or part 2, etc is desired
	void DebugPrint(std::ostream & out, bool p1, bool p2, bool p3, bool p4);
	
	MATHVECTOR <T, 3> GetTotalAero() const;
	
	void AddMassParticle(T newmass, MATHVECTOR <T, 3> newpos);
	
	///calculate the center of mass, calculate the total mass of the body, calculate the inertia tensor
	/// then store this information in the rigid body
	void UpdateMass();
	
	MATHVECTOR <T, 3> RigidBodyLocalToCarLocal(const MATHVECTOR <T, 3> & rigidbodylocal) const;
	
	MATHVECTOR <T, 3> CarLocalToRigidBodyLocal(const MATHVECTOR <T, 3> & carlocal) const;
	
	MATHVECTOR <T, 3> CarLocalToWorld(const MATHVECTOR <T, 3> & carlocal) const;
	
	MATHVECTOR <T, 3> RigidBodyLocalToWorld(const MATHVECTOR <T, 3> rigidbodylocal) const;
	
	MATHVECTOR <T, 3> WorldToRigidBodyLocal(const MATHVECTOR <T, 3> & world) const;
	
	MATHVECTOR <T, 3> WorldToCarLocal(const MATHVECTOR <T, 3> & world) const;
	
	//TODO: adjustable ackermann-like parameters
	///set the steering angle to "value", where 1.0 is maximum right lock and -1.0 is maximum left lock.
	void SetSteering(const T value);
	
	///Get the maximum steering angle in degrees
	T GetMaxSteeringAngle() const;
	
	///Set the maximum steering angle in degrees
	void SetMaxSteeringAngle(T newangle);
	
	///get the worldspace engine position
	MATHVECTOR <T, 3> GetEnginePosition();
	
	///translate by the given translation in worldspace
	void Translate(const MATHVECTOR <T, 3> & translation);
	
	///apply force from a contact to the body.  units are worldspace
	void ProcessContact(const MATHVECTOR <T, 3> & pos, const MATHVECTOR <T, 3> & normal, const MATHVECTOR <T, 3> & relative_velocity, T depth, T dt);
	
	///return the spedometer reading in m/s based on the driveshaft speed
	T GetSpeedo();
	
	///return the magnitude of the car's velocity in m/s
	T GetSpeed();
	
	void AddAerodynamicDevice(const MATHVECTOR <T, 3> & newpos, T drag_frontal_area, T drag_coefficient, T lift_surface_area,
				 T lift_coefficient, T lift_efficiency);
	
	T GetDriveshaftRPM() const;

	MATHVECTOR< T, 3 > GetCenterOfMass() const;
	
	///used by the AI
	T GetAerodynamicDownforceCoefficient() const;
	
	///used by the AI
	T GetAeordynamicDragCoefficient() const;
	
	bool Serialize(joeserialize::Serializer & s);

	MATHVECTOR< T, 3 > GetLastBodyForce() const;
	
	bool WheelDriven(WHEEL_POSITION pos) const;
};

#endif
