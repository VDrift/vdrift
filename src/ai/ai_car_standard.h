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

#ifndef _AI_CAR_STANDARD_H
#define _AI_CAR_STANDARD_H

#include "ai_car.h"
#include "ai_factory.h"
#include "carinput.h"
#include "reseatable_reference.h"
#include "graphics/scenenode.h"
#include "bezier.h"

#include <vector>
#include <list>
#include <map>

class Car;
class Track;

class AiCarStandardFactory :
	public AiFactory
{
	AiCar* create(Car * car, float difficulty);
};

class AiCarStandard :
	public AiCar
{
private:

	void updateGasBrake();
	void calcMu();
	float calcSpeedLimit(const Bezier* patch, const Bezier* nextpatch, float friction, float extraradius);
	float calcBrakeDist(float current_speed, float allowed_speed, float friction);
	void updateSteer();
	void analyzeOthers(float dt, const std::list <Car> & othercars);
	float steerAwayFromOthers(); ///< returns a float that should be added into the steering wheel command
	float brakeFromOthers(float speed_diff); ///< returns a float that should be added into the brake command. speed_diff is the difference between the desired speed and speed limit of this area of the track
	double Angle(double x1, double y1); ///< returns the angle in degrees of the normalized 2-vector
	Bezier RevisePatch(const Bezier * origpatch, bool use_racingline);

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

	std::map <const Car *, OTHERCARINFO> othercars;

	float shift_time;
	float longitude_mu; ///<friction coefficient of the tire - longitude direction
	float lateral_mu; ///<friction coefficient of the tire - lateral direction
	const Bezier * last_patch; ///<last patch the car was on, used in case car is off track
	bool use_racingline; ///<true allows the AI to take a proper racing line

	template<class T> static bool isnan(const T & x);
	static float clamp(float val, float min, float max);
	static float RateLimit(float old_value, float new_value, float rate_limit_pos, float rate_limit_neg);
	static const Bezier * GetCurrentPatch(const Car *c);
	static Vec3 GetPatchFrontCenter(const Bezier & patch);
	static Vec3 GetPatchBackCenter(const Bezier & patch);
	static Vec3 GetPatchDirection(const Bezier & patch);
	static Vec3 GetPatchWidthVector(const Bezier & patch);
	static double GetPatchRadius(const Bezier & patch);
	static void TrimPatch(Bezier & patch, float trimleft_front, float trimright_front, float trimleft_back, float trimright_back);
	static float GetHorizontalDistanceAlongPatch(const Bezier & patch, Vec3 carposition);
	static float RampBetween(float val, float startat, float endat);

#ifdef VISUALIZE_AI_DEBUG
	VERTEXARRAY brakeshape;
	VERTEXARRAY steershape;
	VERTEXARRAY avoidanceshape;
	std::vector <BEZIER> brakelook;
	std::vector <BEZIER> steerlook;
	keyed_container <DRAWABLE>::handle brakedraw;
	keyed_container <DRAWABLE>::handle steerdraw;
	keyed_container <DRAWABLE>::handle avoidancedraw;

	static void ConfigureDrawable(keyed_container <DRAWABLE>::handle & ref, SCENENODE & topnode, float r, float g, float b);
	static void AddLinePoint(VERTEXARRAY & va, const MATHVECTOR<float, 3> & p);
#endif

public:
	AiCarStandard (Car * new_car, float newdifficulty);
	~AiCarStandard();
	void Update(float dt, const std::list <Car> & checkcars);

#ifdef VISUALIZE_AI_DEBUG
	void Visualize();
#endif
};

#endif // _AI_CAR_STANDARD_H
