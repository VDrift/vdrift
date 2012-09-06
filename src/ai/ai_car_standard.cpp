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

#include "ai_car_standard.h"
#include "car.h"
#include "bezier.h"
#include "track.h"
#include "carinput.h"
#include "mathvector.h"
#include "coordinatesystem.h"
#include "optional.h"
#include "unittest.h"

#include <cassert>
#include <cmath>
#include <algorithm>
#include <iostream>

AI_Car* AI_Car_Standard_Factory::create(CAR * car, float difficulty)
{
	return new AI_Car_Standard(car, difficulty);
}

AI_Car_Standard::AI_Car_Standard(CAR * new_car, float newdifficulty) :
	AI_Car(new_car, newdifficulty),
	last_patch(NULL),
	use_racingline(true)
{
	assert(car->GetTCSEnabled());
	assert(car->GetABSEnabled());
	car->SetAutoShift(true);
	car->SetAutoClutch(true);
}

AI_Car_Standard::~AI_Car_Standard ()
{
#ifdef VISUALIZE_AI_DEBUG
	SCENENODE& topnode  = car->GetNode();
	if (brakedraw.valid())
	{
		topnode.GetDrawlist().normal_noblend.erase(brakedraw);
	}
	if (steerdraw.valid())
	{
		topnode.GetDrawlist().normal_noblend.erase(steerdraw);
	}
#endif
}

template<class T> bool AI_Car_Standard::isnan(const T & x)
{
	return x != x;
}

//used to calculate brake value
#define MAX_SPEED_DIFF 6.0
#define MIN_SPEED_DIFF 1.0

//used to detect very sharp corners like Circuit de Pau
#define LOOKAHEAD_MIN_RADIUS 8.0

//maximum change in brake value per second
#define BRAKE_RATE_LIMIT 0.1
#define THROTTLE_RATE_LIMIT 0.1

float AI_Car_Standard::clamp(float val, float min, float max)
{
	assert(min <= max);
	return std::min(max,std::max(min,val));
}

//note that rate_limit_neg should be positive, it gets inverted inside the function
float AI_Car_Standard::RateLimit(float old_value, float new_value, float rate_limit_pos, float rate_limit_neg)
{
	if (new_value - old_value > rate_limit_pos)
		return old_value + rate_limit_pos;
	else if (new_value - old_value < -rate_limit_neg)
		return old_value - rate_limit_neg;
	else
		return new_value;
}

void AI_Car_Standard::Update(float dt, const std::vector<CAR> & checkcars)
{
	analyzeOthers(dt, checkcars);
	updateGasBrake();
	updateSteer();
}

MATHVECTOR <float, 3> AI_Car_Standard::TransformToWorldspace(const MATHVECTOR <float, 3> & bezierspace)
{
	return MATHVECTOR <float, 3> (bezierspace[2], bezierspace[0], bezierspace[1]);
}

MATHVECTOR <float, 3> AI_Car_Standard::TransformToPatchspace(const MATHVECTOR <float, 3> & bezierspace)
{
	return MATHVECTOR <float, 3> (bezierspace[1], bezierspace[2], bezierspace[0]);
}

const BEZIER * AI_Car_Standard::GetCurrentPatch(const CAR *c)
{
	const BEZIER *curr_patch = c->GetCurPatch(0);
	if (!curr_patch)
	{
		curr_patch = c->GetCurPatch(1); //let's try the other wheel
		if (!curr_patch) return NULL;
	}

	return curr_patch;
}

MATHVECTOR <float, 3> AI_Car_Standard::GetPatchFrontCenter(const BEZIER & patch)
{
	return (patch.GetPoint(0,0) + patch.GetPoint(0,3)) * 0.5;
}

MATHVECTOR <float, 3> AI_Car_Standard::GetPatchBackCenter(const BEZIER & patch)
{
	return (patch.GetPoint(3,0) + patch.GetPoint(3,3)) * 0.5;
}

MATHVECTOR <float, 3> AI_Car_Standard::GetPatchDirection(const BEZIER & patch)
{
	return (GetPatchFrontCenter(patch) - GetPatchBackCenter(patch)) * 0.5;
}

MATHVECTOR <float, 3> AI_Car_Standard::GetPatchWidthVector(const BEZIER & patch)
{
	return ((patch.GetPoint(0,0) + patch.GetPoint(3,0)) -
			(patch.GetPoint(0,3) + patch.GetPoint(3,3))) * 0.5;
}

double AI_Car_Standard::GetPatchRadius(const BEZIER & patch)
{
	if (patch.GetNextPatch() && patch.GetNextPatch()->GetNextPatch())
	{
		double track_radius = 0;

		/*MATHVECTOR <float, 3> d1 = -GetPatchDirection(patch);
		MATHVECTOR <float, 3> d2 = GetPatchDirection(*patch.GetNextPatch());*/
		MATHVECTOR <float, 3> d1 = -(patch.GetNextPatch()->GetRacingLine() - patch.GetRacingLine());
		MATHVECTOR <float, 3> d2 = patch.GetNextPatch()->GetNextPatch()->GetRacingLine() - patch.GetNextPatch()->GetRacingLine();
		d1[1] = 0;
		d2[1] = 0;
		float d1mag = d1.Magnitude();
		float d2mag = d2.Magnitude();
		float diff = d2mag - d1mag;
		double dd = ((d1mag < 0.0001) || (d2mag < 0.0001)) ? 0.0 : d1.Normalize().dot(d2.Normalize());
		float angle = acos((dd>=1.0L)?1.0L:(dd<=-1.0L)?-1.0L:dd);
		float d1d2mag = d1mag + d2mag;
		float alpha = (d1d2mag < 0.0001) ? 0.0f : (M_PI * diff + 2.0 * d1mag * angle) / d1d2mag / 2.0;
		if (fabs(alpha - M_PI/2.0) < 0.001) track_radius = 10000.0;
		else track_radius = d1mag / 2.0 / cos(alpha);

		return track_radius;
	}
	else //fall back
		return 0;
}

BEZIER AI_Car_Standard::RevisePatch(const BEZIER * origpatch, bool use_racingline)
{
	if (!use_racingline || !origpatch->GetNextPatch())
		return *origpatch;

	const BEZIER * nextpatch = origpatch->GetNextPatch();
	BEZIER patch = *origpatch;

	MATHVECTOR <float, 3> vfront = (patch.GetPoint(0,3) - patch.GetPoint(0,0));
	MATHVECTOR <float, 3> vback = (patch.GetPoint(3,3) - patch.GetPoint(3,0));
	MATHVECTOR <float, 3> vfrontleft = nextpatch->GetRacingLine() - patch.GetPoint(0,0);
	MATHVECTOR <float, 3> vbackleft = patch.GetRacingLine() - patch.GetPoint(3,0);

	// front/back width fractions
	float width_fl = vfront.dot(vfrontleft) / vfront.dot(vfront);
	float width_bl = vback.dot(vbackleft) / vback.dot(vback);
	float width_fm = std::min(width_fl, 1 - width_fl);
	float width_bm = std::min(width_bl, 1 - width_bl);

	// front/back trims
	float trim_fl = width_fl - width_fm;
	float trim_bl = width_bl - width_bm;
	float trim_fr = 1 - width_fl - width_fm;
	float trim_br = 1 - width_bl - width_bm;

	if (trim_fl + trim_fr > 1)
	{
		float scale = 1 / (trim_fl + trim_fr);
		trim_fl *= scale;
		trim_fr *= scale;
	}

	if (trim_bl + trim_br > 1)
	{
		float scale = 1 / (trim_bl + trim_br);
		trim_bl *= scale;
		trim_br *= scale;
	}

	MATHVECTOR <float, 3> fl = patch.GetPoint(0,0) + vfront * trim_fl;
	MATHVECTOR <float, 3> fr = patch.GetPoint(0,3) - vfront * trim_fr;
	MATHVECTOR <float, 3> bl = patch.GetPoint(3,0) + vback * trim_bl;
	MATHVECTOR <float, 3> br = patch.GetPoint(3,3) - vback * trim_br;
	patch.SetFromCorners(fl, fr, bl, br);

	return patch;
}

float AI_Car_Standard::calcSpeedLimit(
	const BEZIER * patch,
	const BEZIER * nextpatch,
	float extraradius) const
{
	assert(patch);

	//adjust the radius at corner exit to allow a higher speed.
	//this will get the car to accellerate out of corner
	float radius = GetPatchRadius(*patch);
	if (nextpatch && radius > LOOKAHEAD_MIN_RADIUS && GetPatchRadius(*nextpatch) > radius)
	{
		radius += extraradius;
	}
	return car->GetMaxVelocity(radius);
}

void AI_Car_Standard::updateGasBrake()
{
#ifdef VISUALIZE_AI_DEBUG
	brakelook.clear();
#endif

	float brake_value = 0.0;
	float gas_value = 0.5;

	if (car->GetEngineRPM() < car->GetEngineStallRPM())
		inputs[CARINPUT::START_ENGINE] = 1.0;
	else
		inputs[CARINPUT::START_ENGINE] = 0.0;

	const BEZIER *curr_patch_ptr = GetCurrentPatch(car);
	if (!curr_patch_ptr)
	{
		// if car is not on track, just let it roll
		inputs[CARINPUT::THROTTLE] = 0.8;
		inputs[CARINPUT::BRAKE] = 0.0;
		return;
	}

	// trimm patch if we have a racing line
	BEZIER curr_patch = RevisePatch(curr_patch_ptr, use_racingline);

	// get velocity along tangent vector, it should calculate a lower current speed
	MATHVECTOR<float, 3> patch_direction = TransformToWorldspace(GetPatchDirection(curr_patch));
	float currentspeed = car->GetVelocity().dot(patch_direction.Normalize());

	// check speed against speed limit of current patch
	float speed_limit = 0;
	float extraradius = GetPatchWidthVector(*curr_patch_ptr).Magnitude();
	if (!curr_patch.GetNextPatch())
	{
		speed_limit = calcSpeedLimit(&curr_patch, NULL, extraradius);
	}
	else
	{
		BEZIER next_patch = RevisePatch(curr_patch.GetNextPatch(), use_racingline);
		speed_limit = calcSpeedLimit(&curr_patch, &next_patch, extraradius);
	}
	speed_limit *= difficulty;

	float speed_diff = speed_limit - currentspeed;
	if (speed_diff < 0.0)
	{
		if (-speed_diff < MIN_SPEED_DIFF) //no need to brake if diff is small
		{
			brake_value = 0.0;
		}
		else
		{
			brake_value = -speed_diff / MAX_SPEED_DIFF;
			if (brake_value > 1.0) brake_value = 1.0;
		}
		gas_value = 0.0;
	}
	else if (isnan(speed_diff) || speed_diff > MAX_SPEED_DIFF)
	{
		gas_value = 1.0;
		brake_value = 0.0;
	}
	else
	{
		gas_value = speed_diff / MAX_SPEED_DIFF;
		brake_value = 0.0;
	}

	// check upto maxlookahead distance
	float maxlookahead = car->GetBrakingDistance(0) + 10;
	float dist_checked = 0.0;
	float brake_dist = 0.0;
	BEZIER patch_to_check = curr_patch;
#ifdef VISUALIZE_AI_DEBUG
	brakelook.push_back(patch_to_check);
#endif

	while (dist_checked < maxlookahead)
	{
		//if there is no next patch(probably a non-closed track, just let it roll
		if (!patch_to_check.GetNextPatch())
		{
			brake_value = 0.0;
			dist_checked = maxlookahead;
			break;
		}

		float extraradius = GetPatchWidthVector(*patch_to_check.GetNextPatch()).Magnitude();
		patch_to_check = RevisePatch(patch_to_check.GetNextPatch(), use_racingline);
#ifdef VISUALIZE_AI_DEBUG
		brakelook.push_back(patch_to_check);
#endif

		if (!patch_to_check.GetNextPatch())
		{
			speed_limit = calcSpeedLimit(&patch_to_check, NULL, extraradius);
		}
		else
		{
			BEZIER next_patch = RevisePatch(patch_to_check.GetNextPatch(), use_racingline);
			speed_limit = calcSpeedLimit(&patch_to_check, &next_patch, extraradius);
		}

		dist_checked += GetPatchDirection(patch_to_check).Magnitude();
		brake_dist = car->GetBrakingDistance(speed_limit);
		if (brake_dist > dist_checked)
		{
			brake_value = 1.0;
			gas_value = 0.0;
			break;
		}
	}

	if (car->GetGear() == 0)
	{
		inputs[CARINPUT::SHIFT_UP] = 1.0;
		gas_value = 0.2;
	}
	else
	{
		inputs[CARINPUT::SHIFT_UP] = 0.0;
	}

	// consider traffic avoidance bias
	/*float trafficbrake = brakeFromOthers(c, dt, othercars, speed_diff);
	if (trafficbrake > 0)
	{
		gas_value = 0.0;
		brake_value = std::max(trafficbrake, brake_value);
	}*/

	inputs[CARINPUT::THROTTLE] = RateLimit(
		inputs[CARINPUT::THROTTLE], gas_value, THROTTLE_RATE_LIMIT, THROTTLE_RATE_LIMIT);

	inputs[CARINPUT::BRAKE] =
		RateLimit(inputs[CARINPUT::BRAKE], brake_value, BRAKE_RATE_LIMIT, BRAKE_RATE_LIMIT);
}

void AI_Car_Standard::updateSteer()
{
#ifdef VISUALIZE_AI_DEBUG
	steerlook.clear();
#endif

	const BEZIER * curr_patch_ptr = GetCurrentPatch(car);
	if (!curr_patch_ptr)
	{
		// no contact with track
		if (!last_patch)
		{
			// no valid last patch, just let it roll
			return;
		}
		else
		{
			// use last patch to get the car back on track
			// get next closest patch 8 meter in front of the car
			MATHVECTOR <float, 3> offset = direction::Forward * 8;
			car->GetOrientation().RotateVector(offset);
			MATHVECTOR <float, 3> pos = car->GetPosition() + offset;
			pos = TransformToPatchspace(pos);
			curr_patch_ptr = last_patch->GetNextClosestPatch(pos);
		}
	}

	// store the last patch car was on
	last_patch = curr_patch_ptr;

	BEZIER curr_patch = RevisePatch(curr_patch_ptr, use_racingline);
#ifdef VISUALIZE_AI_DEBUG
	steerlook.push_back(curr_patch);
#endif

	if (!curr_patch.GetNextPatch())
	{
		// no next patch (probably a non-closed track), let it roll
		return;
	}

	// find the point to steer towards
	BEZIER next_patch = RevisePatch(curr_patch.GetNextPatch(), use_racingline);
	MATHVECTOR <float, 3> dest_point = GetPatchFrontCenter(next_patch);

	// lookahead if next patch is not too sharp
	while (GetPatchRadius(next_patch) < LOOKAHEAD_MIN_RADIUS)
	{
#ifdef VISUALIZE_AI_DEBUG
		steerlook.push_back(next_patch);
#endif
		// if there is no next patch for whatever reason, stop lookahead
		if (!next_patch.GetNextPatch())
		{
			break;
		}
		next_patch = RevisePatch(next_patch.GetNextPatch(), use_racingline);
		dest_point = GetPatchFrontCenter(next_patch);
	}

	MATHVECTOR <float, 3> next_position = TransformToWorldspace(dest_point);
	MATHVECTOR <float, 3> car_position = car->GetCenterOfMassPosition();
	MATHVECTOR <float, 3> car_orientation = direction::Forward;
	(car->GetOrientation()).RotateVector(car_orientation);

	MATHVECTOR <float, 3> desire_orientation = next_position - car_position;

	// car's direction on the horizontal plane
	car_orientation[2] = 0;

	// desired direction on the horizontal plane
	desire_orientation[2] = 0;

	car_orientation = car_orientation.Normalize();
	desire_orientation = desire_orientation.Normalize();

	// the angle between car's direction and unit y vector (forward direction)
	double alpha = Angle(car_orientation[0], car_orientation[1]);

	// the angle between desired direction and unit y vector (forward direction)
	double beta = Angle(desire_orientation[0], desire_orientation[1]);

	// calculate steering angle and direction
	double angle = beta - alpha;

	// sum in traffic avoidance bias
	//angle += steerAwayFromOthers(c, dt, othercars, angle);

	if (angle > -360.0 && angle <= -180.0)
		angle = -(360.0 + angle);
	else if (angle > -180.0 && angle <= 0.0)
		angle = - angle;
	else if (angle > 0.0 && angle <= 180.0)
		angle = - angle;
	else if (angle > 180.0 && angle <= 360.0)
		angle = 360.0 - angle;

	float optimum_range = car->GetIdealSteeringAngle();
	angle = clamp(angle, -optimum_range, optimum_range);

	float steer_value = angle / car->GetMaxSteeringAngle();
	if (steer_value > 1.0) steer_value = 1.0;
	else if (steer_value < -1.0) steer_value = -1.0;

	assert(!isnan(steer_value));
	inputs[CARINPUT::STEER_RIGHT] = steer_value;
}

///note that carposition must be in patch space
///returns distance from left side of the track
float AI_Car_Standard::GetHorizontalDistanceAlongPatch(const BEZIER & patch, MATHVECTOR <float, 3> carposition)
{
	MATHVECTOR <float, 3> leftside = (patch.GetPoint(0,0) + patch.GetPoint(3,0))*0.5;
	MATHVECTOR <float, 3> rightside = (patch.GetPoint(0,3) + patch.GetPoint(3,3))*0.5;
	MATHVECTOR <float, 3> patchwidthvector = rightside - leftside;
	return patchwidthvector.Normalize().dot(carposition-leftside);
}

float AI_Car_Standard::RampBetween(float val, float startat, float endat)
{
	assert(endat > startat);
	return (clamp(val,startat,endat)-startat)/(endat-startat);
}

float AI_Car_Standard::brakeFromOthers(float speed_diff)
{
	const float nobiasdiff = 30;
	const float fullbiasdiff = 0;
	const float horizontal_care = 2.5; //meters to left and right which we'll brake for
	const float startateta = 10.0;
	const float fullbrakeeta = 1.0;
	const float startatdistance = 10.0;
	const float fullbrakedistance = 4.0;

	float mineta = 1000;
	float mindistance = 1000;

	for (std::map <const CAR *, AI_Car_Standard::OTHERCARINFO>::iterator i = othercars.begin(); i != othercars.end(); ++i)
	{
		if (i->second.active && std::abs(i->second.horizontal_distance) < horizontal_care)
		{
			if (i->second.fore_distance < mindistance)
			{
				mindistance = i->second.fore_distance;
				mineta = i->second.eta;
			}
		}
	}

	float bias = 0;

	float etafeedback = 0;
	float distancefeedback = 0;

	//correct for eta of impact
	if (mineta < 1000)
	{
		etafeedback += 1.0-RampBetween(mineta, fullbrakeeta, startateta);
	}

	//correct for absolute distance
	if (mindistance < 1000)
	{
		distancefeedback += 1.0-RampBetween(mineta,fullbrakedistance,startatdistance);
	}

	//correct for speed versus speed limit (don't bother to brake for slowpokes, just go around)
	float speedfeedback = 1.0-RampBetween(speed_diff, fullbiasdiff, nobiasdiff);

	//std::cout << mineta << ": " << etafeedback << ", " << mindistance << ": " << distancefeedback << ", " << speed_diff << ": " << speedfeedback << std::endl;

	//bias = clamp((etafeedback+distancefeedback)*speedfeedback,0,1);
	bias = clamp(etafeedback*distancefeedback*speedfeedback,0,1);

	return bias;
}

void AI_Car_Standard::analyzeOthers(float dt, const std::vector<CAR> & checkcars)
{
	//const float speed = std::max(1.0f,car->GetVelocity().Magnitude());
	const float half_carlength = 1.25; //in meters

	//std::cout << speed << ": " << authority << std::endl;

	//const MATHVECTOR <float, 3> steer_right_axis = direction::Right;
	const MATHVECTOR <float, 3> throttle_axis = direction::Forward;

#ifdef VISUALIZE_AI_DEBUG
	//avoidancedraw->ClearLine();
#endif

	for (std::vector<CAR>::const_iterator i = checkcars.begin(); i != checkcars.end(); ++i)
	{
		if (&(*i) != car)
		{
			struct AI_Car_Standard::OTHERCARINFO & info = othercars[&(*i)];

			//find direction of other cars in our frame
			MATHVECTOR <float, 3> relative_position = i->GetCenterOfMassPosition() - car->GetCenterOfMassPosition();
			(-car->GetOrientation()).RotateVector(relative_position);

			//std::cout << relative_position.dot(throttle_axis) << ", " << relative_position.dot(steer_right_axis) << std::endl;

			//only make a move if the other car is within our distance limit
			float fore_position = relative_position.dot(throttle_axis);
			//float speed_diff = i->GetVelocity().dot(throttle_axis) - car->GetVelocity().dot(throttle_axis); //positive if other car is faster

			MATHVECTOR <float, 3> myvel = car->GetVelocity();
			MATHVECTOR <float, 3> othervel = i->GetVelocity();
			(-car->GetOrientation()).RotateVector(myvel);
			(-i->GetOrientation()).RotateVector(othervel);
			float speed_diff = othervel.dot(throttle_axis) - myvel.dot(throttle_axis); //positive if other car is faster

			//std::cout << speed_diff << std::endl;
			//float distancelimit = clamp(distancelimitcoeff*-speed_diff, distancelimitmin, distancelimitmax);
			const float fore_position_offset = -half_carlength;
			if (fore_position > fore_position_offset)// && fore_position < distancelimit) //only pay attention to cars roughly in front of us
			{
				//float horizontal_distance = relative_position.dot(steer_right_axis); //fallback method if not on a patch
				//float orig_horiz = horizontal_distance;

				const BEZIER * othercarpatch = GetCurrentPatch(&(*i));
				const BEZIER * mycarpatch = GetCurrentPatch(car);

				if (othercarpatch && mycarpatch)
				{
					float my_track_placement = GetHorizontalDistanceAlongPatch(*mycarpatch, TransformToPatchspace(car->GetCenterOfMassPosition()));
					float their_track_placement = GetHorizontalDistanceAlongPatch(*othercarpatch, TransformToPatchspace(i->GetCenterOfMassPosition()));

					float speed_diff_denom = clamp(speed_diff, -100, -0.01);
					float eta = (fore_position-fore_position_offset)/-speed_diff_denom;

					info.fore_distance = fore_position;

					if (!info.active)
						info.eta = eta;
					else
						info.eta = RateLimit(info.eta, eta, 10.f*dt, 10000.f*dt);

					float horizontal_distance = their_track_placement - my_track_placement;
					//if (!info.active)
						info.horizontal_distance = horizontal_distance;
					/*else
						info.horizontal_distance = RateLimit(info.horizontal_distance, horizontal_distance, spacingdistance*dt, spacingdistance*dt);*/

					//std::cout << info.horizontal_distance << ", " << info.eta << std::endl;

					info.active = true;
				}
				else
					info.active = false;

				//std::cout << orig_horiz << ", " << horizontal_distance << ",    " << fore_position << ", " << speed_diff << std::endl;

				/*if (!min_horizontal_distance)
					min_horizontal_distance = optional <float> (horizontal_distance);
				else if (std::abs(min_horizontal_distance.get()) > std::abs(horizontal_distance))
					min_horizontal_distance = optional <float> (horizontal_distance);*/
			}
			else
				info.active = false;

/*#ifdef VISUALIZE_AI_DEBUG
			if (info.active)
			{
				avoidancedraw->AddLinePoint(car->GetCenterOfMassPosition());
				MATHVECTOR <float, 3> feeler1(speed*info.eta,0,0);
				car->GetOrientation().RotateVector(feeler1);
				MATHVECTOR <float, 3> feeler2(0,-info.horizontal_distance,0);
				car->GetOrientation().RotateVector(feeler2);
				avoidancedraw->AddLinePoint(car->GetCenterOfMassPosition()+feeler1+feeler2);
				avoidancedraw->AddLinePoint(car->GetCenterOfMassPosition());
			}
#endif*/
		}
	}
}

/*float AI::steerAwayFromOthers(AI_Car *c, float dt, const std::list <CAR> & othercars, float cursteer)
{
	const float spacingdistance = 3.0; //how far left and right we target for our spacing in meters (center of mass to center of mass)
	const float ignoredistance = 6.0;

	float eta = 1000;
	float min_horizontal_distance = 1000;
	QUATERNION <float> otherorientation;

	for (std::map <const CAR *, AI_Car::OTHERCARINFO>::iterator i = othercars.begin(); i != othercars.end(); i++)
	{
		if (i->second.active && std::abs(i->second.horizontal_distance) < std::abs(min_horizontal_distance))
		{
			min_horizontal_distance = i->second.horizontal_distance;
			eta = i->second.eta;
			otherorientation = i->first->GetOrientation();
		}
	}

	if (min_horizontal_distance >= ignoredistance)
		return 0.0;

	float sidedist = min_horizontal_distance;
	float d = std::abs(sidedist) - spacingdistance*0.5;
	if (d < spacingdistance)
	{
		const MATHVECTOR <float, 3> forward(1,0,0);
		MATHVECTOR <float, 3> otherdir = forward;
		otherorientation.RotateVector(otherdir); //rotate to worldspace
		(-car->GetOrientation()).RotateVector(otherdir); //rotate to this car space
		otherdir[2] = 0; //remove vertical component
		otherdir = otherdir.Normalize(); //resize to unit size
		if (otherdir[0] > 0) //heading in the same direction
		{
			float diffangle = (180.0/3.141593)*asin(otherdir[1]); //positive when other car is pointing to our right

			if (diffangle * -sidedist < 0) //heading toward collision
			{
				const float c = spacingdistance * 0.5;
				d = std::max(0.0f, d - c);
				float psteer = diffangle;

				psteer = cursteer*(d/c)+1.5*psteer*(1.0f-d/c);

				std::cout << diffangle << ", " << d/c << ", " << psteer << std::endl;

				if (cursteer*psteer > 0 && std::abs(cursteer) > std::abs(psteer))
					return 0.0; //no bias, already more than correcting
				else
					return psteer - cursteer;
			}
		}
		else
			std::cout << "wrong quadrant: " << otherdir << std::endl;
	}
	else
	{
		std::cout << "out of range: " << d << ", " << spacingdistance << std::endl;
	}

	return 0.0;
}*/

float AI_Car_Standard::steerAwayFromOthers()
{
	const float spacingdistance = 3.5; //how far left and right we target for our spacing in meters (center of mass to center of mass)
	const float horizontal_meters_per_second = 5.0; //how fast we want to steer away in horizontal meters per second
	const float speed = std::max(1.0f,car->GetVelocity().Magnitude());
	const float authority = std::min(10.0,(180.0/3.141593)*atan(horizontal_meters_per_second/speed)); //steering bias authority limit magnitude in degrees
	const float gain = 4.0; //amplify steering command by this factor
	const float mineta = 1.0; //fastest reaction time in seconds
	const float etaexponent = 1.0;

	float eta = 1000;
	float min_horizontal_distance = 1000;

	for (std::map <const CAR *, AI_Car_Standard::OTHERCARINFO>::iterator i = othercars.begin(); i != othercars.end(); ++i)
	{
		if (i->second.active && std::abs(i->second.horizontal_distance) < std::abs(min_horizontal_distance))
		{
			min_horizontal_distance = i->second.horizontal_distance;
			eta = i->second.eta;
		}
	}

	if (min_horizontal_distance == 1000)
		return 0.0;

	eta = std::max(eta, mineta);

	float bias = clamp(min_horizontal_distance, -spacingdistance, spacingdistance);
	if (bias < 0)
		bias = -bias - spacingdistance;
	else
		bias = spacingdistance - bias;

	bias *= pow(mineta,etaexponent)*gain/pow(eta,etaexponent);
	clamp(bias, -spacingdistance, spacingdistance);

	//std::cout << "min horiz: " << min_horizontal_distance << ", eta: " << eta << ", " << bias << std::endl;

	return (bias/spacingdistance)*authority;
}

double AI_Car_Standard::Angle(double x1, double y1)
{
	return atan2(y1, x1) * 180.0 / M_PI;
}
#ifdef VISUALIZE_AI_DEBUG
void AI_Car_Standard::ConfigureDrawable(keyed_container <DRAWABLE>::handle & ref, SCENENODE & topnode, float r, float g, float b)
{
	if (!ref.valid())
	{
		ref = topnode.GetDrawlist().normal_noblend.insert(DRAWABLE());
		DRAWABLE & d = topnode.GetDrawlist().normal_noblend.get(ref);
		d.SetColor(r,g,b,1);
		d.SetDecal(true);
	}
}

void AI_Car_Standard::AddLinePoint(VERTEXARRAY & va, const MATHVECTOR<float, 3> & p)
{
	int vsize;
	const float* vbase;
	va.GetVertices(vbase, vsize);

	if (vsize == 0)
	{
		int vcount = 3;
		float verts[3] = {p[0], p[1], p[2]};
		va.SetVertices(verts, vcount, vsize);
	}
	else
	{
		int vcount = 6;
		float verts[6] = {p[0], p[1], p[2], p[0], p[1], p[2]};
		va.SetVertices(verts, vcount, vsize);
	}
}

void AI_Car_Standard::Visualize()
{
	SCENENODE& topnode  = car->GetNode();
	ConfigureDrawable(brakedraw, topnode, 0,1,0);
	ConfigureDrawable(steerdraw, topnode, 0,0,1);
	//ConfigureDrawable(avoidancedraw, topnode, 1,0,0);

	DRAWABLE & brakedrawable = topnode.GetDrawlist().normal_noblend.get(brakedraw);
	DRAWABLE & steerdrawable = topnode.GetDrawlist().normal_noblend.get(steerdraw);

	brakedrawable.SetLineSize(4);
	brakedrawable.SetVertArray(&brakeshape);
	brakeshape.Clear();
	for (std::vector <BEZIER>::iterator i = brakelook.begin(); i != brakelook.end(); ++i)
	{
		BEZIER & patch = *i;
		AddLinePoint(brakeshape, TransformToWorldspace(patch.GetBL()));
		AddLinePoint(brakeshape, TransformToWorldspace(patch.GetFL()));
		AddLinePoint(brakeshape, TransformToWorldspace(patch.GetFR()));
		AddLinePoint(brakeshape, TransformToWorldspace(patch.GetBR()));
		AddLinePoint(brakeshape, TransformToWorldspace(patch.GetBL()));
	}

	steerdrawable.SetLineSize(4);
	steerdrawable.SetVertArray(&steershape);
	steershape.Clear();
	for (std::vector <BEZIER>::iterator i = steerlook.begin(); i != steerlook.end(); ++i)
	{
		BEZIER & patch = *i;
		AddLinePoint(steershape, TransformToWorldspace(patch.GetBL()));
		AddLinePoint(steershape, TransformToWorldspace(patch.GetFL()));
		AddLinePoint(steershape, TransformToWorldspace(patch.GetBR()));
		AddLinePoint(steershape, TransformToWorldspace(patch.GetFR()));
		AddLinePoint(steershape, TransformToWorldspace(patch.GetBL()));
	}
}
#endif
