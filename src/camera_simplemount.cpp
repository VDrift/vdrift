#include "camera_simplemount.h"

CAMERA_SIMPLEMOUNT::CAMERA_SIMPLEMOUNT(const std::string & name) : CAMERA(name)
{
	randgen.ReSeed(0);
}

float CAMERA_SIMPLEMOUNT::Random()
{
	return randgen.Get();
}

float CAMERA_SIMPLEMOUNT::Distribution(float input)
{
	float sign = 1.0;
	if (input < 0)
		sign = -1.0;
	float power = pow(std::abs(input), 64);
	power -= 0.2;
	if (power < 0)
		power = 0;
    assert(power <= 1.0f);
	return power * sign * 8.0;
}

void CAMERA_SIMPLEMOUNT::Update(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newdir, const MATHVECTOR <float, 3> &, float dt)
{
	MATHVECTOR <float, 3> vel = newpos - position;

	MATHVECTOR <float, 3> randvec;
	randvec[1] = Distribution(Random())*0.5;
	randvec[2] = Distribution(Random());

	offset_randomwalk = offset_randomwalk + randvec*dt;

	MATHVECTOR <float, 3> controlleroutput;
	for (int i = 0; i < 3; i++)
	{
		float error = -offsetdelay[i].Process(offsetdelay[i].Process(offset_filtered[i]));
		controlleroutput[i] = offsetpid[i].Process(error, 0);
	}

	//std::cout << offset_filtered << " || " << controlleroutput << std::endl;

	for (int i = 0; i < 3; i++)
		offset_filtered[i] = offsetlowpass[i].Process(offset_randomwalk[i]+controlleroutput[i]);

	float effect = vel.Magnitude();
	effect -= 0.04;
	effect /= 0.2;
	if (effect < 0)
		effect = 0;
	if (effect > 1.0)
		effect = 1.0;
	effect *= effect;
	//std::cout << effect << std::endl;

	offset_final = offset_filtered*effect;

	position = newpos + offset_final;
	orientation = orientation.QuatSlerp(newdir, dt*40.0);
}

void CAMERA_SIMPLEMOUNT::Reset(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newquat)
{
	offsetdelay.clear();
	offsetlowpass.clear();
	offsetpid.clear();
	for (int i = 0; i < 3; i++)
	{
		offsetdelay.push_back(signalprocessing::DELAY(3-2*stiffness));
		offsetlowpass.push_back(signalprocessing::LOWPASS(.1*(stiffness+1.0)));
		offsetpid.push_back(signalprocessing::PID(0.2*stiffness,0.25*(stiffness+1.0),0,false));
	}

	position = newpos;
	orientation = newquat;
	offset_randomwalk.Set(0,0,0);
}
