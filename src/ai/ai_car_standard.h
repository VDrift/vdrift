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
#include "graphics/scenenode.h"
#include "bezier.h"

#include <vector>
#include <map>

class AI_Car_Standard_Factory : public AI_Factory
{
	AI_Car* create(CAR * car, float difficulty);
};

class AI_Car_Standard : public AI_Car
{
public:
	AI_Car_Standard(CAR * new_car, float newdifficulty);

	~AI_Car_Standard();

	void Update(float dt, const std::vector<CAR> & cars);

#ifdef VISUALIZE_AI_DEBUG
	void Visualize();
#endif

private:
	struct OTHERCARINFO
	{
		OTHERCARINFO() : active(false) {}

		float horizontal_distance;
		float fore_distance;
		float eta;
		bool active;
	};
	std::map <const CAR *, OTHERCARINFO> othercars;
	const BEZIER * last_patch; ///< last patch the car was on, used in case car is off track

	float GetSpeedLimit(const BEZIER * patch, const BEZIER * nextpatch, float extraradius) const;

	void UpdateGasBrake();

	void UpdateSteer();

	void AnalyzeOthers(float dt, const std::vector<CAR> & othercars);

	/// returns a float that should be added into the steering wheel command
	float SteerAwayFromOthers();

	/// returns a float that should be added into the brake command
	/// speed_diff is the difference between the desired speed and speed limit of this area of the track
	float BrakeFromOthers(float speed_diff);

	template <class T> static bool isnan(const T & x);

	static float clamp(float val, float min, float max);

	static float RateLimit(float old_value, float new_value, float rate_limit_pos, float rate_limit_neg);

	static MATHVECTOR <float, 3> TransformToWorldspace(const MATHVECTOR <float, 3> & bezierspace);

	static MATHVECTOR <float, 3> TransformToPatchspace(const MATHVECTOR <float, 3> & bezierspace);

	static const BEZIER * GetCurrentPatch(const CAR *c);

	static double GetPatchRadius(const BEZIER & patch);

	static BEZIER RevisePatch(const BEZIER * origpatch);

	static float GetHorizontalDistanceAlongPatch(const BEZIER & patch, MATHVECTOR <float, 3> carposition);

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
};

#endif // _AI_CAR_STANDARD_H
