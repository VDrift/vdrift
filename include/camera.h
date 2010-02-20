#ifndef _CAMERA_H
#define _CAMERA_H

#include <ostream>
#include <cmath>

#include "mathvector.h"
#include "quaternion.h"
#include "signalprocessing.h"
#include "rigidbody.h"
#include "random.h"

///base class for a camera
class CAMERA
{
	protected:
		const std::string name;
	
	public:
		CAMERA(const std::string camera_name) : name(camera_name) {}
		const std::string & GetName() const {return name;}
		
		virtual MATHVECTOR <float, 3> GetPosition() const = 0;
		virtual QUATERNION <float> GetOrientation() const = 0;
		
		// reset position, orientation
		virtual void Reset(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newquat)
		{
		}
		// update position, orientation
		virtual void Update(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newquat, const MATHVECTOR <float, 3> & accel, float dt)
		{
		}
		// move relative to current position, orientation
		virtual void Move(float dx, float dy, float dz) {}
		// rotate relative to current position, orientation
		virtual void Rotate(float up, float left) {}
};

///camera manager class
class CAMERA_SYSTEM
{
	protected:
		std::vector<CAMERA*> camera;			//camera vector used to preserve order
		unsigned int active;					//active camera index
		std::map<std::string, int> camera_map;	//name => index
	
	public:
		~CAMERA_SYSTEM()
		{
			for(unsigned int i = 0; i < camera.size(); i++)
			{
				if (camera[i]) delete camera[i];
			}
		}
		CAMERA * Active() const
		{
			return camera[active];
		}
		CAMERA * Select(const std::string & name)
		{
			active = 0;
			std::map<std::string, int>::iterator it = camera_map.find(name);
			if (it != camera_map.end()) active = it->second;
			return camera[active];
		}
		CAMERA * Next()
		{
			active++;
			if (active == camera.size()) active = 0;
			return camera[active];
		}
		CAMERA * Prev()
		{
			if (active == 0) active = camera.size();
			active--;
			return camera[active];
		}
		void Add(CAMERA * newcam)
		{
			active = camera.size();
			camera_map[newcam->GetName()] = active;
			camera.push_back(newcam);
			//std::cout << "camera " << active << ": "<< newcam->GetName() << std::endl;
		}
};

class CAMERA_FIXED : public CAMERA
{
	private:
		MATHVECTOR <float, 3> position;
		MATHVECTOR <float, 3> offset;
		QUATERNION <float> orientation;

	public:
		CAMERA_FIXED(const std::string name) : CAMERA(name) {}
		virtual MATHVECTOR <float, 3> GetPosition() const {return position;}
		virtual QUATERNION <float> GetOrientation() const {return orientation;}
		virtual void Reset(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newquat)
		{
			MATHVECTOR <float, 3> newoffset = offset;
			newquat.RotateVector(newoffset);
			position = newpos + newoffset;
			orientation = newquat;
		}
		virtual void Update(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newquat, const MATHVECTOR <float, 3> & accel, float dt)
		{
			Reset(newpos, newquat);
		}
		void SetOffset(float x, float y, float z)
		{
			offset.Set(x, y, z);
		}
};

class CAMERA_FREE : public CAMERA
{
	private:
		MATHVECTOR <float, 3> position;
		MATHVECTOR <float, 3> offset;
		float leftright_rotation;
		float updown_rotation;

	public:
		CAMERA_FREE(const std::string name) : CAMERA(name), offset(0, 0, 2), leftright_rotation(0), updown_rotation(0) {}

		virtual MATHVECTOR <float, 3> GetPosition() const {return position;}
		virtual QUATERNION <float> GetOrientation() const
		{
			QUATERNION <float> rot;
			rot.Rotate(updown_rotation, 0, 1, 0);
			rot.Rotate(leftright_rotation, 0, 0, 1);
			return rot;
		}
		virtual void Reset(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newquat)
		{
			position = newpos + offset;
			leftright_rotation = 0;
			updown_rotation = 0;
			Move(-8.0, 0, 0);
		}
		virtual void Rotate(float up, float left)
		{
			updown_rotation += up;
			if (updown_rotation > 1.0)
				updown_rotation = 1.0;
			if (updown_rotation <-1.0)
				updown_rotation =-1.0;
			
			leftright_rotation += left;
		}
		virtual void Move(float dx, float dy, float dz)
		{
			MATHVECTOR <float, 3> forward(dx, 0, 0);
			GetOrientation().RotateVector(forward);
			position = position + forward;
		}
};

class CAMERA_ORBIT : public CAMERA
{
	private:
		MATHVECTOR <float, 3> focus;
		float leftright_rotation;
		float updown_rotation;
		float orbit_distance;

	public:
		CAMERA_ORBIT(const std::string name) : CAMERA(name), leftright_rotation(0), updown_rotation(0), orbit_distance(4.0) {}

		virtual MATHVECTOR <float, 3> GetPosition() const
		{
			MATHVECTOR <float, 3> position(-orbit_distance,0,0);
			GetOrientation().RotateVector(position);
			return focus + position;
		}
		virtual QUATERNION <float> GetOrientation() const
		{
			QUATERNION <float> rot;
			rot.Rotate(updown_rotation, 0, 1, 0);
			rot.Rotate(leftright_rotation, 0, 0, 1);
			return rot;
		}
		virtual void Reset(const MATHVECTOR <float, 3> & newfocus, const QUATERNION <float> & newquat)
		{
			focus = newfocus;
			leftright_rotation = 0;
			updown_rotation = 0;
			orbit_distance = 4.0;
		}
		virtual void Update(const MATHVECTOR <float, 3> & newfocus, const QUATERNION <float> & newquat, const MATHVECTOR <float, 3> & accel, float dt)
		{
			focus = newfocus;
		}
		virtual void Rotate(float up, float left)
		{
			const float maxupdown = 1.5;
			updown_rotation += up;
			if (updown_rotation > maxupdown)
				updown_rotation = maxupdown;
			if (updown_rotation <-maxupdown)
				updown_rotation =-maxupdown;
			
			leftright_rotation -= left;
		}
		virtual void Move(float dx, float dy, float dz)
		{
			orbit_distance -= dx;
			if (orbit_distance < 3.0)
				orbit_distance = 3.0;
			if (orbit_distance > 10.0)
				orbit_distance = 10.0;
		}
};

class CAMERA_CHASE : public CAMERA
{
	private:
		MATHVECTOR <float, 3> position;
		MATHVECTOR <float, 3> focus;
		QUATERNION <float> orientation;

		float chase_distance;
		float chase_height;
		bool posblend_on;

		float AngleBetween(MATHVECTOR <float, 3> vec1, MATHVECTOR <float, 3> vec2)
		{
			float dotprod = vec1.Normalize().dot(vec2.Normalize());
			float angle = acos(dotprod);
			float epsilon = 1e-6;
			if (fabs(dotprod) <= epsilon)
				angle = 3.141593*0.5;
			if (dotprod >= 1.0-epsilon)
				angle = 0.0;
			if (dotprod <= -1.0+epsilon)
				angle = 3.141593;
			return angle;
		}

		void LookAt(MATHVECTOR <float, 3> eye, MATHVECTOR <float, 3> center, MATHVECTOR <float, 3> up);

	public:
		CAMERA_CHASE(const std::string name) : CAMERA(name), chase_distance(6), chase_height(1.5), posblend_on(true) {}
		virtual MATHVECTOR <float, 3> GetPosition() const {return position;}
		virtual QUATERNION <float> GetOrientation() const {return orientation;}
		virtual void Reset(const MATHVECTOR <float, 3> & newfocus, const QUATERNION <float> & focus_facing)
		{
			focus = newfocus;
			orientation = focus_facing;
			MATHVECTOR <float, 3> view_offset(-chase_distance, 0, chase_height);
			orientation.RotateVector(view_offset);
			position = focus + view_offset;
		}
		virtual void Update(const MATHVECTOR <float, 3> & newfocus, const QUATERNION <float> & focus_facing, const MATHVECTOR <float, 3> & accel, float dt);

		void SetChaseDistance ( float value )
		{
			chase_distance = value;
		}
		
		void SetChaseHeight ( float value )
		{
			chase_height = value;
		}

		void SetPositionBlending ( bool value )
		{
			posblend_on = value;
		}

};

class CAMERA_SIMPLEMOUNT : public CAMERA
{
	private:
		MATHVECTOR <float, 3> position;
		QUATERNION <float> orientation;

		MATHVECTOR <float, 3> offset_randomwalk;
		MATHVECTOR <float, 3> offset_filtered;
		MATHVECTOR <float, 3> offset_final;

		std::vector<signalprocessing::DELAY> offsetdelay;
		std::vector<signalprocessing::LOWPASS> offsetlowpass;
		std::vector<signalprocessing::PID> offsetpid;

		DETERMINISTICRANDOM randgen;
		
		float stiffness; ///< where 0.0 is nominal stiffness for a sports car and 1.0 is a formula 1 car

		float Random();
		float Distribution(float input);

	public:
        CAMERA_SIMPLEMOUNT(const std::string name) : CAMERA(name) {randgen.ReSeed(0);}
		virtual MATHVECTOR <float, 3> GetPosition() const {return position;}
		virtual QUATERNION <float> GetOrientation() const {return orientation;}
		virtual void Reset(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newquat); 
		virtual void Update(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newdir, const MATHVECTOR <float, 3> & accel, float dt);
		MATHVECTOR <float, 3> GetOffset() const {return offset_final;}
		void SetStiffness ( float value ) {stiffness = value;}
};

class CAMERA_MOUNT : public CAMERA
{
	private:
		RIGIDBODY <float> body;
		MATHVECTOR <float, 3> anchor;
		MATHVECTOR <float, 3> offset;	// offset relative car(center of mass)
		QUATERNION<float> rotation; 	// camera rotation relative to car
		float effect;
		
		DETERMINISTICRANDOM randgen;

		float stiffness; ///< where 0.0 is nominal stiffness for a sports car and 1.0 is a formula 1 car
		float offset_effect_strength; ///< where 1.0 is normal effect strength

	public:
		CAMERA_MOUNT(const std::string name) : CAMERA(name), effect(0.0), stiffness(0.0), offset_effect_strength(1.0) {}

		virtual MATHVECTOR <float, 3> GetPosition() const;
		virtual QUATERNION <float> GetOrientation() const {return body.GetOrientation();}
		virtual void Reset(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newquat);
		virtual void Update(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newdir, const MATHVECTOR <float, 3> & accel, float dt);

		void SetOffset(MATHVECTOR <float, 3> value)
		{
			offset = value;
		}
		
		void SetRotation(float up, float left)
		{
			rotation.Rotate(up, 0, 1, 0);
			rotation.Rotate(left, 0, 0, 1);
		}
		
		void SetStiffness ( float value )
		{
			stiffness = value;
		}

		void SetEffectStrength ( float value )
		{
			offset_effect_strength = value;
		}
};

#endif
