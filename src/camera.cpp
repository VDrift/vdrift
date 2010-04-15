#include "camera.h"
#include "signalprocessing.h"

#include <cmath>

using std::endl;

// reset position, orientation
void CAMERA::Reset(const MATHVECTOR <float, 3> &, const QUATERNION <float> &)
{
	
}

// update position, orientation
void CAMERA::Update(const MATHVECTOR <float, 3> &, const QUATERNION <float> &, const MATHVECTOR <float, 3> &, float)
{
	
}

// move relative to current position, orientation
void CAMERA::Move(float, float, float)
{

}

// rotate relative to current position, orientation
void CAMERA::Rotate(float, float)
{

}
