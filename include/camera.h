#ifndef _CAMERA_H
#define _CAMERA_H

#include <ostream>
#include <cmath>

#include "mathvector.h"
#include "quaternion.h"
#include "signalprocessing.h"
#include "rigidbody.h"
#include "random.h"

///abstract base class for a camera
class CAMERA
{
	public:
		virtual MATHVECTOR <float, 3> GetPosition() const = 0;
		virtual QUATERNION <float> GetOrientation() const = 0;
};

class CAMERA_FIXED : public CAMERA
{
	private:
		MATHVECTOR <float, 3> position;
		QUATERNION <float> orientation;

	public:
		virtual MATHVECTOR <float, 3> GetPosition() const {return position;}
		virtual QUATERNION <float> GetOrientation() const {return orientation;}
		void Set(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newquat)
		{
			position = newpos;
			orientation = newquat;
		}
};

class CAMERA_FREE : public CAMERA
{
	private:
		MATHVECTOR <float, 3> position;
		float leftright_rotation;
		float updown_rotation;

	public:
		CAMERA_FREE() : leftright_rotation(0), updown_rotation(0) {}

		virtual MATHVECTOR <float, 3> GetPosition() const {return position;}
		virtual QUATERNION <float> GetOrientation() const
		{
			QUATERNION <float> rot;
			rot.Rotate(updown_rotation, 0,1,0);
			rot.Rotate(leftright_rotation, 0,0,1);
			return rot;
		}
		void Reset(const MATHVECTOR <float, 3> & newpos)
		{
			position = newpos;
			leftright_rotation = 0;
			updown_rotation = 0;
		}
		void ResetRotation()
		{
			updown_rotation = 0;
			leftright_rotation = 0;
		}
		///rotate by the given amount in radians.  a negative value will result in left rotation
		void RotateRight(float deltarot)
		{
			leftright_rotation += -deltarot;
			//TODO: account for wrap
		}
		///rotate by the given amount in radians
		void RotateDown(float deltarot)
		{
			updown_rotation += deltarot;
			if (updown_rotation > 1.0)
				updown_rotation = 1.0;
			if (updown_rotation <-1.0)
				updown_rotation =-1.0;
		}
		void MoveForward(float meters)
		{
			MATHVECTOR <float, 3> forward(8,0,0);
			forward = forward * meters;
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
		CAMERA_ORBIT() : leftright_rotation(0), updown_rotation(0), orbit_distance(4.0) {}

		virtual MATHVECTOR <float, 3> GetPosition() const
		{
			MATHVECTOR <float, 3> position(-orbit_distance,0,0);
			GetOrientation().RotateVector(position);
			return focus + position;
		}
		virtual QUATERNION <float> GetOrientation() const
		{
			QUATERNION <float> rot;
			rot.Rotate(updown_rotation, 0,1,0);
			rot.Rotate(leftright_rotation, 0,0,1);
			return rot;
		}
		void Reset(const MATHVECTOR <float, 3> & newfocus)
		{
			SetFocus(newfocus);
			leftright_rotation = 0;
			updown_rotation = 0;
			orbit_distance = 4.0;
		}
		void SetFocus(const MATHVECTOR <float, 3> & newfocus)
		{
			focus = newfocus;
		}
		///rotate by the given amount in radians.  a negative value will result in left rotation
		void RotateRight(float deltarot)
		{
			leftright_rotation += -deltarot;
			//TODO: account for wrap
		}
		///rotate by the given amount in radians
		void RotateDown(float deltarot)
		{
			float maxupdown = 1.5;
			updown_rotation += deltarot;
			if (updown_rotation > maxupdown)
				updown_rotation = maxupdown;
			if (updown_rotation <-maxupdown)
				updown_rotation =-maxupdown;
		}
		void ZoomIn(float meters)
		{
			orbit_distance -= meters;
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
			//std::cout << dotprod << std::endl;
			//assert(angle == angle);
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
		CAMERA_CHASE() : chase_distance(6),chase_height(1.5),posblend_on(true) {}
		virtual MATHVECTOR <float, 3> GetPosition() const {return position;}
		virtual QUATERNION <float> GetOrientation() const {return orientation;}
		void Reset(const MATHVECTOR <float, 3> & newfocus, const QUATERNION <float> & focus_facing)
		{
			focus = newfocus;
			orientation = focus_facing;
			MATHVECTOR <float, 3> view_offset(-chase_distance,0,chase_height);
			orientation.RotateVector(view_offset);
			position = focus + view_offset;
		}

		void SetChaseDistance ( float value )
		{
			chase_distance = value;
		}

		void Set(const MATHVECTOR <float, 3> & newfocus, const QUATERNION <float> & focus_facing, float dt, bool lookbehind);

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

		float Random();
		float Distribution(float input);

	public:
        CAMERA_SIMPLEMOUNT() {randgen.ReSeed(0);}
		virtual MATHVECTOR <float, 3> GetPosition() const {return position;}
		virtual QUATERNION <float> GetOrientation() const {return orientation;}
		void Reset(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newquat, float stiffness); ///< where 0.0 is nominal stiffness for a sports car and 1.0 is a formula 1 car
		void Set(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newdir, float dt);
		MATHVECTOR <float, 3> GetOffset() const {return offset_final;}
};

class CAMERA_MOUNT : public CAMERA
{
	private:
		RIGIDBODY <float> body;
		MATHVECTOR <float, 3> anchor;
		float effect;

		//CAMERA_SIMPLEMOUNT bouncesim;
		
		DETERMINISTICRANDOM randgen;

		float stiffness; ///< where 0.0 is nominal stiffness for a sports car and 1.0 is a formula 1 car
		float offset_effect_strength; ///< where 1.0 is normal effect strength

	public:
		CAMERA_MOUNT() : effect(0.0), stiffness(0.0), offset_effect_strength(1.0) {}

		virtual MATHVECTOR <float, 3> GetPosition() const;
		virtual QUATERNION <float> GetOrientation() const {return body.GetOrientation();}

		void Reset(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newquat);
		void Set(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newdir, const MATHVECTOR <float, 3> & accel, float dt);

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
