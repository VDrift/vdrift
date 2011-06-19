#ifndef _CAMERA_SIMPLEMOUNT_H
#define _CAMERA_SIMPLEMOUNT_H

#include "camera.h"
#include "random.h"
#include "signalprocessing.h"

class CAMERA_SIMPLEMOUNT : public CAMERA
{
public:
	CAMERA_SIMPLEMOUNT(const std::string & name);
	
	const MATHVECTOR <float, 3> & GetPosition() const {return position;}
	
	const QUATERNION <float> & GetOrientation() const {return rotation;}
	
	void Reset(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newquat);
	
	void Update(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newdir, float dt);
	
	MATHVECTOR <float, 3> GetOffset() const {return offset_final;}
	
	void SetStiffness(float value) {stiffness = value;}
	
private:
	MATHVECTOR <float, 3> position;
	QUATERNION <float> rotation;
	

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
};

#endif // _CAMERA_SIMPLEMOUNT_H
