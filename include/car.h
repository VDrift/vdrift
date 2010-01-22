#ifndef _CAR_H
#define _CAR_H

#include <string>
#include <ostream>
#include <list>
#include <map>

#include "cardynamics.h"
#include "scenegraph.h"
#include "model_joe03.h"
#include "texture.h"
#include "quaternion.h"
#include "mathvector.h"
#include "configfile.h"
#include "carinput.h"
#include "sound.h"
#include "camera.h"
#include "aabb.h"
#include "joeserialize.h"
#include "macros.h"
#include "collision_detection.h"

class BEZIER;
class PERFORMANCE_TESTING;

class CAR
{
friend class PERFORMANCE_TESTING;
friend class joeserialize::Serializer;
private:
	CARDYNAMICS dynamics;
	
	std::vector <float> feedbackbuffer;
	unsigned int feedbackbufferpos;

	SCENENODE * topnode;

	DRAWABLE * bodydraw;
	DRAWABLE * interiordraw;
	DRAWABLE * glassdraw;
	SCENENODE * bodynode;
	MODEL_JOE03 bodymodel;
	MODEL_JOE03 interiormodel;
	MODEL_JOE03 glassmodel;
	TEXTURE_GL bodytexture;
	TEXTURE_GL bodytexture_misc1;
	TEXTURE_GL interiortexture;
	TEXTURE_GL interiortexture_misc1;
	TEXTURE_GL glasstexture;
	TEXTURE_GL glasstexture_misc1;
	AABB <float> collisiondimensions;
	COLLISION_OBJECT collisionobject;
	DRAWABLE * driverdraw;
	SCENENODE * drivernode;
	MODEL_JOE03 drivermodel;
	TEXTURE_GL drivertexture;
	TEXTURE_GL drivertexture_misc1;
	
	class SUSPENSIONBUMPDETECTION
	{
		private:
			friend class CAR;
			enum
			{
				DISPLACING,
				DISPLACED,
				SETTLING,
				SETTLED
			} state, laststate;
			
			const float displacetime; ///< how long the suspension has to be displacing a high velocity, uninterrupted
			const float displacevelocitythreshold; ///< the threshold for high velocity
			const float settletime; ///< how long the suspension has to be settled, uninterrupted
			const float settlevelocitythreshold;
			
			float displacetimer;
			float settletimer;
			
			float dpstart, dpend;
			
		public:
			SUSPENSIONBUMPDETECTION();
			
			void Update(float vel, float displacementpercent, float dt)
			{
				laststate = state;
				
				//switch states based on velocity
				if (state == SETTLED)
				{
					if (vel >= displacevelocitythreshold)
					{
						state = DISPLACING;
						displacetimer = displacetime;
						dpstart = displacementpercent;
					}
				}
				else if (state == DISPLACING)
				{
					if (vel < displacevelocitythreshold)
					{
						state = SETTLED;
					}
				}
				else if (state == DISPLACED)
				{
					if (vel <= settlevelocitythreshold)
					{
						state = SETTLING;
					}
				}
				else if (state == SETTLING)
				{
					//if (std::abs(vel) > settlevelocitythreshold)
					if (vel > settlevelocitythreshold)
					{
						state = DISPLACED;
					}
				}
				
				//switch states based on time
				if (state == DISPLACING)
				{
					displacetimer -= dt;
					if (displacetimer <= 0)
					{
						state = DISPLACED;
						settletimer = settletime;
					}
				}
				else if (state == SETTLING)
				{
					settletimer -= dt;
					if (settletimer <= 0)
					{
						state = SETTLED;
						dpend = displacementpercent;
					}
				}
			}
			
			bool JustDisplaced() const {return (state == DISPLACED && laststate != DISPLACED);}
			bool JustSettled() const {return (state == SETTLED && laststate != SETTLED);}
			float GetTotalBumpSize() const {return dpend - dpstart;}
	};
	
	SUSPENSIONBUMPDETECTION suspensionbumpdetection[4];
	
	class CRASHDETECTION
	{
		private:
			float lastvel;
			float curmaxdecel;
			float maxdecel;
			
			const float deceltrigger;
		
		public:
			CRASHDETECTION();
			
			void Update(float vel, float dt)
			{
				maxdecel = 0;
				
				float decel = (lastvel - vel)/dt;
				
				//std::cout << "Decel: " << decel << std::endl;
				
				if (decel > deceltrigger && curmaxdecel == 0)
				{
					//idle, start capturing decel
					curmaxdecel = decel;
				}
				else if (curmaxdecel > 0)
				{
					//currently capturing, check for max
					if (decel > curmaxdecel)
						curmaxdecel = decel;
					else
					{
						maxdecel = curmaxdecel;
						assert(maxdecel > deceltrigger);
						curmaxdecel = 0;
					}
				}
				
				lastvel = vel;
			}

			float GetMaxDecel() const
			{
				return maxdecel;
			}
	};
	
	CRASHDETECTION crashdetection;

	std::map <std::string, SOUNDBUFFER> soundbuffers;
	class ENGINESOUNDINFO
	{
		public:
			float minrpm, maxrpm, naturalrpm, fullgainrpmstart, fullgainrpmend;
			enum
			{
				POWERON,
    				POWEROFF,
				BOTH
			} power;

			ENGINESOUNDINFO() : minrpm(1.0), maxrpm(100000.0), naturalrpm(7000.0),fullgainrpmstart(minrpm),fullgainrpmend(maxrpm),power(BOTH) {}

			bool operator < (const ENGINESOUNDINFO & other) const {return minrpm < other.minrpm;}
	};
	std::list <std::pair <ENGINESOUNDINFO, SOUNDSOURCE> > enginesounds;

	DRAWABLE * wheeldraw[4];
	SCENENODE * wheelnode[4];
	DRAWABLE * floatingdraw[4];
	SCENENODE * floatingnode[4];
	DRAWABLE * debugwheeldraw[40]; //10 debug wheels per wheel
	SCENENODE * debugwheelnode[40]; //10 debug wheels per wheel
	MODEL_JOE03 wheelmodelfront;
	MODEL_JOE03 wheelmodelrear;
	MODEL_JOE03 floatingmodelfront;
	MODEL_JOE03 floatingmodelrear;
	TEXTURE_GL wheeltexturefront;
	TEXTURE_GL wheeltexturerear;
	TEXTURE_GL wheeltexturefront_misc1;
	TEXTURE_GL wheeltexturerear_misc1;
	TEXTURE_GL braketexture;

	SOUNDSOURCE tiresqueal[4];
	SOUNDSOURCE tirebump[4];
	SOUNDSOURCE grasssound[4]; //added grass & gravel
	SOUNDSOURCE gravelsound[4];
	SOUNDSOURCE crashsound;
	SOUNDSOURCE roadnoise;

	MATHVECTOR <float, 3> view_position;
	MATHVECTOR <float, 3> hood_position;

	//CAMERA_FIXED driver_cam;
	CAMERA_MOUNT driver_cam;
	//CAMERA_FIXED hood_cam;
	CAMERA_MOUNT hood_cam;
	CAMERA_CHASE rigidchase_cam;
	CAMERA_CHASE chase_cam;
	CAMERA_ORBIT orbit_cam;

	//internal variables that might change during driving (so, they need to be serialized)
	bool auto_clutch;
	float last_auto_clutch;
	float remaining_shift_time;
	float shift_clutch_start;
	int shift_gear;
	bool shifted;
	float last_steer;
	bool autoshift_enabled;
	float shift_time;
	bool lookbehind;

	bool debug_wheel_draw;

	std::string cartype;
	int sector; //the last lap timing sector that the car hit
	const BEZIER * curpatch[4]; //the last bezier patch that each wheel hit
	
	float mz_nominalmax; //the nominal maximum Mz force, used to scale force feedback

	///take the parentnode, add a scenenode (only if output_scenenodeptr is NULL), add a drawable to the scenenode, load a model, load a texture, and set up the drawable with the model and texture.
	/// the given TEXTURE_GL textures will not be reloaded if they are already loaded
	/// returns true if successful
	bool LoadInto(SCENENODE * parentnode, SCENENODE * & output_scenenodeptr, DRAWABLE * & output_drawableptr, const std::string & joefile,
		      MODEL_JOE03 & output_model, const std::string & texfile, TEXTURE_GL & output_texture_diffuse,
		const std::string & misc1texfile, TEXTURE_GL & output_texture_misc1,
  		int anisotropy, const std::string & texsize, std::ostream & error_output);

	template <typename T>
	bool GetConfigfileParam(std::ostream & error_output, CONFIGFILE & c, const std::string & paramname, T & output) const
	{
		if (!c.GetParam(paramname, output))
		{
			error_output << "Couldn't get car parameter: " << paramname << std::endl;
			return false;
		}
		else
			return true;
	}

	bool LoadDynamics(CONFIGFILE & c, std::ostream & error_output);

	float AutoClutch(float last_clutch, float dt) const;
	float ShiftAutoClutch() const;
	float ShiftAutoClutchThrottle(float throttle, float dt);
	int AutoShift() const;

	float DownshiftPoint(int gear) const;

	void ShiftGears(int new_gear, bool immediate=false)
	{
		if (immediate)
			remaining_shift_time = 0.001;
		else
			remaining_shift_time = shift_time;
		
		shift_gear = new_gear;
		shifted = false;
		shift_clutch_start = dynamics.GetClutch().GetClutch();
	}

	void UpdateSounds(float dt);
	void UpdateCameras(float dt);
	
	void CopyPhysicsResultsIntoDisplay();

	bool LoadSounds(const std::string & carpath, const std::string & carname, const SOUNDINFO & sound_device_info, const SOUNDBUFFERLIBRARY & soundbufferlibrary, std::ostream & info_output, std::ostream & error_output);
	
	void GenerateCollisionData();

public:
	CAR();
	~CAR();
	bool Load(CONFIGFILE & carconf, const std::string & carpath, const std::string & driverpath, const std::string & carname, const std::string & carpaint,
		  const MATHVECTOR <float, 3> & initial_position, const QUATERNION <float> & initial_orientation,
    		  SCENENODE & sceneroot, bool soundenabled, const SOUNDINFO & sound_device_info, const SOUNDBUFFERLIBRARY & soundbufferlibrary,
		  int anisotropy, bool defaultabs, bool defaulttcs, const std::string & texsize, float camerabounce,
    		  bool debugmode, std::ostream & info_output, std::ostream & error_output);

	void TickPhysics(double dt)
	{
		remaining_shift_time-=dt;
		if (remaining_shift_time < 0)
			remaining_shift_time = 0;
		dynamics.Tick(dt);
		
		assert (feedbackbufferpos < feedbackbuffer.size());
		feedbackbuffer[feedbackbufferpos] = 0.5*(dynamics.GetTire(FRONT_LEFT).GetFeedback()+dynamics.GetTire(FRONT_RIGHT).GetFeedback());
		feedbackbufferpos++;
		if (feedbackbufferpos >= feedbackbuffer.size())
			feedbackbufferpos = 0;
	}
	
	void Update(double dt)
	{
		CopyPhysicsResultsIntoDisplay();
		UpdateSounds(dt);
		UpdateCameras(dt);
	}

	bool Shifting() const
	{
		return (remaining_shift_time > 0);
	}

	void GetSoundList(std::list <SOUNDSOURCE *> & outputlist)
	{
		for (std::list <std::pair <ENGINESOUNDINFO, SOUNDSOURCE> >::iterator i =
			enginesounds.begin(); i != enginesounds.end(); ++i)
		{
			outputlist.push_back(&i->second);
		}

		for (int i = 0; i < 4; i++)
			outputlist.push_back(&tiresqueal[i]);
		
		for (int i = 0; i < 4; i++)
			outputlist.push_back(&grasssound[i]);
		
		for (int i = 0; i < 4; i++)
			outputlist.push_back(&gravelsound[i]);
		
		for (int i = 0; i < 4; i++)
			outputlist.push_back(&tirebump[i]);
		
		outputlist.push_back(&crashsound);

		outputlist.push_back(&roadnoise);
	}

	void GetEngineSoundList(std::list <SOUNDSOURCE *> & outputlist)
	{
		for (std::list <std::pair <ENGINESOUNDINFO, SOUNDSOURCE> >::iterator i =
			enginesounds.begin(); i != enginesounds.end(); ++i)
		{
			outputlist.push_back(&i->second);
		}
	}

	///returns the position of the center of the wheel
	const MATHVECTOR <float, 3> GetWheelPosition(const WHEEL_POSITION wpos) const
	{
		MATHVECTOR <float, 3> v;
		v=dynamics.GetWheelPosition(wpos);
		return v;
	}

	///returns the position of the center of the wheel at the given suspension displacement
	const MATHVECTOR <float, 3> GetWheelPositionAtDisplacement(const WHEEL_POSITION wpos, const float displacement) const
	{
		MATHVECTOR <float, 3> v;
		v=dynamics.GetWheelPositionAtDisplacement(wpos, displacement);
		return v;
	}

	float GetTireRadius(const WHEEL_POSITION wpos) const
	{
		return dynamics.GetTire(wpos).GetRadius();
	}

	const MATHVECTOR <float, 3> GetDownVector() const
	{
		MATHVECTOR <float, 3> v;
		v.Set(0,0,-1);
		dynamics.GetBody().GetOrientation().RotateVector(v);
		return v;
	}

	void SetWheelContactProperties(WHEEL_POSITION wheel_index, float wheelheight, const MATHVECTOR <float, 3> & position, const MATHVECTOR <float, 3> & normal, float bumpwave, float bumpamp, float frict_no, float frict_tread, float roll_res, float roll_drag, SURFACE::CARSURFACETYPE surface)
	{
		//std::cout << wheel_index << ", " << wheelheight << std::endl;
		MATHVECTOR <double, 3> dblpos;
		dblpos = position;
		MATHVECTOR <double, 3> dblnorm;
		dblnorm = normal;
		dynamics.SetWheelContactProperties(wheel_index, wheelheight, dblpos, dblnorm,
						  bumpwave, bumpamp, frict_no, frict_tread,
						  roll_res, roll_drag, surface);
	}

	void HandleInputs(const std::vector <float> & inputs, float dt);

	int GetEngineRPM() const
	{
		if (dynamics.GetEngine().GetCombustion())
			return dynamics.GetTachoRPM();
		else
			return 0;
	}

	int GetEngineStallRPM() const
	{
		return dynamics.GetEngine().GetStallRPM();
	}

	int GetEngineRedline() const
	{
		return dynamics.GetEngine().GetRedline();
	}

	int GetEngineRPMLimit() const
	{
		return dynamics.GetEngine().GetRPMLimit();
	}

	int GetGear() const
	{
		return dynamics.GetTransmission().GetGear();
	}

	float GetClutch() const
	{
		return dynamics.GetClutch().GetClutch();
	}

	void DebugPrint(std::ostream & out, bool p1, bool p2, bool p3, bool p4)
	{
		dynamics.DebugPrint(out, p1, p2, p3, p4);
	}
	
	MATHVECTOR <float, 3> GetTotalAero() const
	{
		return dynamics.GetTotalAero();
	}

	void SetAutoClutch(bool new_auto_clutch)
	{
		auto_clutch = new_auto_clutch;
	}

	///set the position of the center of mass of the car
	void SetPosition(const MATHVECTOR <float,3> & new_position)
	{
		MATHVECTOR <double,3> newpos;
		newpos = new_position;
		dynamics.GetBody().SetPosition(newpos);

		QUATERNION <float> floatq;
		floatq = dynamics.GetBody().GetOrientation();
		chase_cam.Reset(GetCenterOfMassPosition(), floatq);

		MATHVECTOR <double, 3> doublevec;
		doublevec = view_position;
		MATHVECTOR <float, 3> floatvec;
		floatvec = dynamics.CarLocalToWorld(doublevec);
		driver_cam.Reset(floatvec, floatq);
		doublevec = hood_position;
		floatvec = dynamics.CarLocalToWorld(doublevec);
		hood_cam.Reset(floatvec, floatq);
	}

	MATHVECTOR <float, 3> GetCenterOfMassPosition() const
	{
		MATHVECTOR <float,3> pos;
		pos = dynamics.GetBody().GetPosition();
		return pos;
	}

	MATHVECTOR <float, 3> GetOriginPosition() const
	{
		MATHVECTOR <float,3> pos;
		pos = dynamics.GetOriginPosition();
		return pos;
	}

	QUATERNION <float> GetOrientation() const
	{
		QUATERNION <float> q;
		q = dynamics.GetBody().GetOrientation();
		return q;
	}
	
	MATHVECTOR <float, 3> CarLocalToWorld(const MATHVECTOR <float, 3> & carlocal) const
	{
		MATHVECTOR <float,3> pos;
		MATHVECTOR <double,3> local;
		local = carlocal;
		pos = dynamics.CarLocalToWorld(local);
		return pos;
	}
	
	MATHVECTOR <float, 3> WorldToRigidBodyLocal(const MATHVECTOR <float, 3> & world) const
	{
		return dynamics.WorldToRigidBodyLocal(world);
	}

	const CAMERA * GetDriverCamera() const {return &driver_cam;}
	const CAMERA * GetHoodCamera() const {return &hood_cam;}
	const CAMERA * GetOrbitCamera() const {return &orbit_cam;}
	const CAMERA * GetChaseCamera() const {return &chase_cam;}
	const CAMERA * GetRigidChaseCamera() const {return &rigidchase_cam;}

	const AABB <float> GetCollisionDimensions()
	{
		return collisiondimensions;
	}

	void SetChassisContact(const MATHVECTOR <float, 3> & pos, const MATHVECTOR <float, 3> & normal, float depth, float dt)
	{
		if (depth > 0.0)
		{
			MATHVECTOR <double, 3> dpos;
			dpos = pos;
			MATHVECTOR <double, 3> dnormal;
			dnormal = normal;
			dynamics.ProcessContact(dpos, dnormal, dynamics.GetBody().GetVelocity(), depth, dt);
		}
	}
	
	void SetDynamicChassisContact(const MATHVECTOR <float, 3> & pos, const MATHVECTOR <float, 3> & normal, const MATHVECTOR <float, 3> & relativevelocity, float depth, float dt)
	{
		if (depth > 0.0)
		{
			MATHVECTOR <double, 3> dpos;
			dpos = pos;
			MATHVECTOR <double, 3> dnormal;
			dnormal = normal;
			MATHVECTOR <double, 3> dvel;
			dvel = relativevelocity;
			dynamics.ProcessContact(dpos, dnormal, dvel, depth, dt);
		}
	}

	///return the speedometer reading (based on the driveshaft speed) in m/s
	float GetSpeedometer()
	{
		return dynamics.GetSpeedo();
	}

	std::string GetCarType() const
	{
		return cartype;
	}

	void SetSector ( int value )
	{
		sector = value;
	}

	int GetSector() const
	{
		return sector;
	}

	void SetCurPatch (unsigned int wheel, const BEZIER* value )
	{
		assert (wheel < 4);
		curpatch[wheel] = value;
	}

	const BEZIER* GetCurPatch(unsigned int wheel) const
	{
		assert (wheel < 4);
		return curpatch[wheel];
	}

	bool GetABSEnabled() const {return dynamics.GetABSEnabled();}
	bool GetABSActive() const {return dynamics.GetABSActive();}
	bool GetTCSEnabled() const {return dynamics.GetTCSEnabled();}
	bool GetTCSActive() const {return dynamics.GetTCSActive();}

	float GetLastSteer() const
	{
		return last_steer;
	}

	///return the magnitude of the car's velocity in m/s
	float GetSpeed()
	{
		return dynamics.GetSpeed();
	}

	void SetAutoShift ( bool value )
	{
		autoshift_enabled = value;
		if(value) ShiftGears(1);
	}

	bool Serialize(joeserialize::Serializer & s)
	{
		_SERIALIZE_(s,dynamics);
		_SERIALIZE_(s,last_auto_clutch);
		_SERIALIZE_(s,remaining_shift_time);
		_SERIALIZE_(s,shift_clutch_start);
		_SERIALIZE_(s,shift_gear);
		_SERIALIZE_(s,shifted);
		_SERIALIZE_(s,last_steer);
		_SERIALIZE_(s,autoshift_enabled);
		return true;
	}

	float GetFeedback();

	///returns a float from 0.0 to 1.0 with the amount of tire squealing going on
	float GetTireSquealAmount(WHEEL_POSITION i) const;
	
	MATHVECTOR <float, 3> GetWheelVelocity(WHEEL_POSITION i) const
	{
		MATHVECTOR <float, 3> groundvel;
		groundvel = dynamics.GetWheelVelocity(WHEEL_POSITION(i));
		return groundvel;
	}

	///used by the AI
	float GetAerodynamicDownforceCoefficient() const
	{
		return dynamics.GetAerodynamicDownforceCoefficient();
	}

	///used by the AI
	float GetAeordynamicDragCoefficient() const
	{
		return dynamics.GetAeordynamicDragCoefficient();
	}

	float GetMass() const
	{
		return dynamics.GetBody().GetMass();
	}

	MATHVECTOR <float, 3> GetVelocity() const
	{
		MATHVECTOR <float, 3> vel;
		vel = dynamics.GetBody().GetVelocity();
		return vel;
	}

	///used by AI
	float GetTireMaxFx(WHEEL_POSITION tire_index) const
	{
		return dynamics.GetTire(tire_index).GetMaximumFx(GetMass()*0.25*9.81);
	}

	///used by AI
	float GetTireMaxFy(WHEEL_POSITION tire_index) const
	{
		return dynamics.GetTire(tire_index).GetMaximumFy(GetMass()*0.25*9.81, 0.0);
	}
	
	float GetTireMaxMz(WHEEL_POSITION tire_index) const
	{
		return dynamics.GetTire(tire_index).GetMaximumMz(GetMass()*0.25*9.81, 0.0);
	}
	
	///get the optimum steering angle in degrees.  used by the AI
	float GetOptimumSteeringAngle() const
	{
		return dynamics.GetTire(FRONT_LEFT).GetOptimumSteeringAngle(GetMass()*0.25*9.81);
	}

	///Get the maximum steering angle in degrees.  used by the AI
	float GetMaxSteeringAngle() const
	{
		return dynamics.GetMaxSteeringAngle();
	}
	
	void EnableGlass(bool enable)
	{
		if (glassdraw)
			glassdraw->SetDrawEnable(enable);
	}

	COLLISION_OBJECT & GetCollisionObject()
	{
		return collisionobject;
	}
	
};

#endif
