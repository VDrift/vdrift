/************************************************************************/
/*                                                                      */
/* This file is part of VDrift.                                         */
/*                                                                      */
/* VDrift is free software: you can redistribute it and/or modify       */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation, either version 3 of the License, or    */
/* (at your option) any later version.                                  */
/*                                                                      */
/* VDrift is distributed in the hope that it will be useful,            */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */
/*                                                                      */
/* You should have received a copy of the GNU General Public License    */
/* along with VDrift.  If not, see <http://www.gnu.org/licenses/>.      */
/*                                                                      */
/************************************************************************/

#ifndef _AI_Car_Experimental_H
#define _AI_Car_Experimental_H

#include "ai_car.h"
#include "ai_factory.h"
#include "carinput.h"
#include "reseatable_reference.h"
#include "scenenode.h"
#include "bezier.h"

#include <vector>
#include <list>
#include <map>

class CAR;
class TRACK;

class AI_Car_Experimental_Factory :
	public AI_Factory
{
	AI_Car* create(CAR * car, float difficulty);
};

class AI_Car_Experimental :
	public AI_Car
{
private:

	void updateGasBrake();
	void calcMu();
	float calcSpeedLimit(const BEZIER* patch, const BEZIER* nextpatch, float friction, float extraradius);
	float calcBrakeDist(float current_speed, float allowed_speed, float friction);
	void updateSteer();
	void analyzeOthers(float dt, const std::list <CAR> & othercars);
	float steerAwayFromOthers(); ///< returns a float that should be added into the steering wheel command
	float brakeFromOthers(float speed_diff); ///< returns a float that should be added into the brake command. speed_diff is the difference between the desired speed and speed limit of this area of the track
	double Angle(double x1, double y1); ///< returns the angle in degrees of the normalized 2-vector
	BEZIER RevisePatch(const BEZIER * origpatch, bool use_racingline);

	/*
	/// for replanning the path
	struct PATH_REVISION
	{
		PATH_REVISION() : trimleft_front(0), trimright_front(0), trimleft_back(0), trimright_back(0), car_pos_along_track(0) {}

		float trimleft_front;
		float trimright_front;
		float trimleft_back;
		float trimright_back;
		float car_pos_along_track;
	};

	std::map <const CAR *, PATH_REVISION> path_revisions;
	*/

	struct OTHERCARINFO
	{
		OTHERCARINFO() : active(false) {}

		float horizontal_distance;
		float fore_distance;
		float eta;
		bool active;
	};

	std::map <const CAR *, OTHERCARINFO> othercars;

	float shift_time;
	float longitude_mu; ///<friction coefficient of the tire - longitude direction
	float lateral_mu; ///<friction coefficient of the tire - lateral direction
	const BEZIER * last_patch; ///<last patch the car was on, used in case car is off track
	bool use_racingline; ///<true allows the AI to take a proper racing line
	bool isRecovering; ///< tries to get back to the road.
	time_t recoverStartTime;

	template<class T> static bool isnan(const T & x);
	static float clamp(float val, float min, float max);
	static float RateLimit(float old_value, float new_value, float rate_limit_pos, float rate_limit_neg);
	static MATHVECTOR <float, 3> TransformToWorldspace(const MATHVECTOR <float, 3> & bezierspace);
	static MATHVECTOR <float, 3> TransformToPatchspace(const MATHVECTOR <float, 3> & bezierspace);
	static const BEZIER * GetCurrentPatch(const CAR *c);
	static MATHVECTOR <float, 3> GetPatchFrontCenter(const BEZIER & patch);
	static MATHVECTOR <float, 3> GetPatchBackCenter(const BEZIER & patch);
	static MATHVECTOR <float, 3> GetPatchDirection(const BEZIER & patch);
	static MATHVECTOR <float, 3> GetPatchWidthVector(const BEZIER & patch);
	static double GetPatchRadius(const BEZIER & patch);
	static void TrimPatch(BEZIER & patch, float trimleft_front, float trimright_front, float trimleft_back, float trimright_back);
	static float GetHorizontalDistanceAlongPatch(const BEZIER & patch, MATHVECTOR <float, 3> carposition);
	static float RampBetween(float val, float startat, float endat);

	/// This will return the nearest patch to the car.
	/// This is only useful if the car is outside of the road.
	/// Optionally, you can pass a helper bezier to improve performance, which should be near to the car, but maybe not the nearest.
	const BEZIER* getNearestPatch(const BEZIER* helper = 0);

	bool recover(const BEZIER* patch);
	/// Creates a ray from the middle of the car. Returns the distance to the first colliding object or max_length.
	float RayCastDistance( MATHVECTOR <float, 3> direction, float max_length);

#ifdef VISUALIZE_AI_DEBUG
	VERTEXARRAY brakeshape;
	VERTEXARRAY steershape;
	VERTEXARRAY avoidanceshape;
	VERTEXARRAY raycastshape;
	std::vector <BEZIER> brakelook;
	std::vector <BEZIER> steerlook;
	keyed_container <DRAWABLE>::handle brakedraw;
	keyed_container <DRAWABLE>::handle steerdraw;
	keyed_container <DRAWABLE>::handle avoidancedraw;
	keyed_container <DRAWABLE>::handle raycastdraw;

	static void ConfigureDrawable(keyed_container <DRAWABLE>::handle & ref, SCENENODE & topnode, float r, float g, float b);
	static void AddLinePoint(VERTEXARRAY & va, const MATHVECTOR<float, 3> & p);
#endif

public:
	AI_Car_Experimental (CAR * new_car, float newdifficulty);
	~AI_Car_Experimental();
	void Update(float dt, const std::list <CAR> & checkcars);

#ifdef VISUALIZE_AI_DEBUG
	void Visualize();
#endif
};

#endif // _AI_Car_Experimental_H
