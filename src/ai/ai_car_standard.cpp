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
#include "physics/cardynamics.h"
#include "physics/dynamicsworld.h"
#include "minmax.h"
#include "tobullet.h"
#include "track.h"
#include "unittest.h"

#include <cassert>
#include <cmath>
#include <algorithm>
#include <iostream>

//used to calculate brake value
#define MAX_SPEED_DIFF 6.0f
#define MIN_SPEED_DIFF 1.0f

//used to find the point the car should steer towards to
#define LOOKAHEAD_FACTOR1 2.25f
#define LOOKAHEAD_FACTOR2 0.33f

//used to detect very sharp corners like Circuit de Pau
#define LOOKAHEAD_MIN_RADIUS 8.0f

//used to calculate friction coefficient
#define FRICTION_FACTOR_LONG 0.68f
#define FRICTION_FACTOR_LAT 0.62f

//maximum change in brake value per second
#define BRAKE_RATE_LIMIT 0.1f
#define THROTTLE_RATE_LIMIT 0.1f

static const float rad2deg = 180 / M_PI;

AiCar * AiCarStandardFactory::Create(unsigned carid, float difficulty)
{
	return new AiCarStandard(carid, difficulty);
}

AiCarStandard::AiCarStandard(unsigned new_carid, float new_difficulty) :
	AiCar(new_carid, new_difficulty),
	last_patch(NULL)
{
	// ctor
}

AiCarStandard::~AiCarStandard()
{
#ifdef VISUALIZE_AI_DEBUG
	SceneNode & topnode  = car.GetNode();
	if (brakedraw.valid())
	{
		topnode.GetDrawList().normal_noblend.erase(brakedraw);
	}
	if (steerdraw.valid())
	{
		topnode.GetDrawList().normal_noblend.erase(steerdraw);
	}
#endif
}

//note that rate_limit_neg should be positive, it gets inverted inside the function
float AiCarStandard::RateLimit(float old_value, float new_value, float rate_limit_pos, float rate_limit_neg)
{
	if (new_value - old_value > rate_limit_pos)
		return old_value + rate_limit_pos;
	else if (new_value - old_value < -rate_limit_neg)
		return old_value - rate_limit_neg;
	else
		return new_value;
}

void AiCarStandard::Update(float dt, const CarDynamics cars[], const unsigned cars_num)
{
	AnalyzeOthers(dt, cars, cars_num);
	UpdateGasBrake(cars[carid]);
	UpdateSteer(cars[carid]);
}

const RoadPatch * AiCarStandard::GetCurrentPatch(const CarDynamics & car)
{
	const RoadPatch *curr_patch = car.GetWheelContact(WheelPosition(0)).GetPatch();
	if (!curr_patch)
	{
		// let's try the other wheel
		curr_patch = car.GetWheelContact(WheelPosition(1)).GetPatch();
		if (!curr_patch) return NULL;
	}
	return curr_patch;
}

Vec3 AiCarStandard::GetPatchFrontCenter(const RoadPatch & patch)
{
	return (patch.GetPoint(0,0) + patch.GetPoint(0,3)) * 0.5;
}

Vec3 AiCarStandard::GetPatchBackCenter(const RoadPatch & patch)
{
	return (patch.GetPoint(3,0) + patch.GetPoint(3,3)) * 0.5;
}

Vec3 AiCarStandard::GetPatchDirection(const RoadPatch & patch)
{
	return (GetPatchFrontCenter(patch) - GetPatchBackCenter(patch)) * 0.5;
}

Vec3 AiCarStandard::GetPatchWidthVector(const RoadPatch & patch)
{
	return ((patch.GetPoint(0,0) + patch.GetPoint(3,0)) -
			(patch.GetPoint(0,3) + patch.GetPoint(3,3))) * 0.5;
}

float AiCarStandard::GetPatchRadius(const RoadPatch & patch)
{
	if (patch.GetNextPatch() && patch.GetNextPatch()->GetNextPatch())
	{
		Vec3 d1 = -(patch.GetNextPatch()->GetRacingLine() - patch.GetRacingLine());
		Vec3 d2 = patch.GetNextPatch()->GetNextPatch()->GetRacingLine() - patch.GetNextPatch()->GetRacingLine();
		d1[2] = 0;
		d2[2] = 0;
		float d1mag = d1.Magnitude();
		float d2mag = d2.Magnitude();
		float diff = d2mag - d1mag;
		float dd = ((d1mag < 1E-8f) || (d2mag < 1E-8f)) ? 0 : d1.Normalize().dot(d2.Normalize());
		float angle = std::acos((dd >= 1) ? 1 :(dd <= -1) ? -1 : dd);
		float d1d2mag = d1mag + d2mag;
		float alpha = (d1d2mag < 1E-8f) ? 0 : (float(M_PI) * diff + 2 * d1mag * angle) / d1d2mag * 0.5f;
		float track_radius = 0;
		if (std::abs(alpha - float(M_PI_2)) < 1E-3f) track_radius = 10000;
		else track_radius = 0.5f * d1mag / std::cos(alpha);
		return track_radius;
	}
	//fall back
	return 0;
}

///trim the patch's width in-place
void AiCarStandard::TrimPatch(RoadPatch & patch, float trimleft_front, float trimright_front, float trimleft_back, float trimright_back)
{
	Vec3 frontvector = (patch.GetPoint(0,3) - patch.GetPoint(0,0));
	Vec3 backvector = (patch.GetPoint(3,3) - patch.GetPoint(3,0));
	float frontwidth = frontvector.Magnitude();
	float backwidth = backvector.Magnitude();
	if (trimleft_front + trimright_front > frontwidth)
	{
		float scale = frontwidth/(trimleft_front + trimright_front);
		trimleft_front *= scale;
		trimright_front *= scale;
	}
	if (trimleft_back + trimright_back > backwidth)
	{
		float scale = backwidth/(trimleft_back + trimright_back);
		trimleft_back *= scale;
		trimright_back *= scale;
	}

	Vec3 newfl = patch.GetPoint(0,0);
	Vec3 newfr = patch.GetPoint(0,3);
	Vec3 newbl = patch.GetPoint(3,0);
	Vec3 newbr = patch.GetPoint(3,3);

	if (frontvector.MagnitudeSquared() > 1E-6f)
	{
		Vec3 trimdirection_front = frontvector.Normalize();
		newfl = patch.GetPoint(0,0) + trimdirection_front*trimleft_front;
		newfr = patch.GetPoint(0,3) - trimdirection_front*trimright_front;
	}

	if (backvector.MagnitudeSquared() > 1E-6f)
	{
		Vec3 trimdirection_back = backvector.Normalize();
		newbl = patch.GetPoint(3,0) + trimdirection_back*trimleft_back;
		newbr = patch.GetPoint(3,3) - trimdirection_back*trimright_back;
	}

	patch.SetFromCorners(newfl, newfr, newbl, newbr);
}

RoadPatch AiCarStandard::RevisePatch(const RoadPatch * origpatch)
{
	RoadPatch patch = *origpatch;

	//take into account the racing line
	if (patch.GetNextPatch() && patch.HasRacingline())
	{
		float widthfront = Min((patch.GetNextPatch()->GetRacingLine()-patch.GetPoint(0,0)).Magnitude(),
									 (patch.GetNextPatch()->GetRacingLine()-patch.GetPoint(0,3)).Magnitude());
		float widthback = Min((patch.GetRacingLine()-patch.GetPoint(3,0)).Magnitude(),
									(patch.GetRacingLine()-patch.GetPoint(3,3)).Magnitude());
		float trimleft_front = (patch.GetNextPatch()->GetRacingLine() - patch.GetPoint(0,0)).Magnitude()-widthfront;
		float trimright_front = (patch.GetNextPatch()->GetRacingLine() - patch.GetPoint(0,3)).Magnitude()-widthfront;
		float trimleft_back = (patch.GetRacingLine() - patch.GetPoint(3,0)).Magnitude()-widthback;
		float trimright_back = (patch.GetRacingLine() - patch.GetPoint(3,3)).Magnitude()-widthback;
		TrimPatch(patch, trimleft_front, trimright_front, trimleft_back, trimright_back);
	}

	//check for revisions due to other cars
	/*const float trim_falloff_distance = 100.0; //trim fallof distance in meters per (meters per second)
	const Vec3 throttle_axis(-1,0,0); //positive is in front of the car
	for (const auto & path_rev : path_revisions)
	{
		if (path_rev.first != car)
		{
			//compute relative info
			Vec3 myvel = car.GetVelocity();
			Vec3 othervel = path_rev.first->GetVelocity();
			(-car.GetOrientation()).RotateVector(myvel);
			(-path_rev.first->GetOrientation()).RotateVector(othervel);
			float speed_diff = myvel.dot(throttle_axis) - othervel.dot(throttle_axis); //positive if other car is faster //actually positive if my car is faster, right?

			float cardist_back = patch.dist_from_start - path_rev.second.car_pos_along_track; //positive if patch is ahead of car
			float patchlen = GetPatchDirection(patch).Magnitude();
			float cardist_front = (patch.dist_from_start+patchlen) - path_rev.second.car_pos_along_track;

			const float minfalloff = 10;
			const float maxfalloff = 60;
			float cur_trim_falloff_distance_fwd = minfalloff;
			float cur_trim_falloff_distance_rear = minfalloff;
			float falloff = Clamp(trim_falloff_distance*std::abs(speed_diff),minfalloff,maxfalloff);
			if (speed_diff > 0)
			{
				//cur_trim_falloff_distance_fwd = falloff;
			}
			else
				cur_trim_falloff_distance_rear = falloff;

			float scale_front = Clamp(1.0f-cardist_front/cur_trim_falloff_distance_fwd, 0.0f, 1.0f);
			if (cardist_front < 0)
				scale_front = Clamp(1.0f+cardist_front/cur_trim_falloff_distance_rear, 0.0f, 1.0f);
			float scale_back = Clamp(1.0f-cardist_back/cur_trim_falloff_distance_fwd, 0.0f, 1.0f);
			if (cardist_back < 0)
				scale_back = Clamp(1.0f+cardist_back/cur_trim_falloff_distance_rear, 0.0f, 1.0f);

			std::cout << speed_diff << ", " << cur_trim_falloff_distance_fwd << ", " << cur_trim_falloff_distance_rear << ", " << cardist_front << ", " << cardist_back << ", " << scale_front << ", " << scale_back << std::endl;

			float trimleft_front = path_rev.second.trimleft_front*scale_front;
			float trimright_front = path_rev.second.trimright_front*scale_front;
			float trimleft_back = path_rev.second.trimleft_back*scale_back;
			float trimright_back = path_rev.second.trimright_back*scale_back;

			TrimPatch(patch, trimleft_front, trimright_front, trimleft_back, trimright_back);
		}
	}*/

	return patch;
}

void AiCarStandard::UpdateGasBrake(const CarDynamics & car)
{
#ifdef VISUALIZE_AI_DEBUG
	brakelook.clear();
#endif

	float brake_value = 0.0;
	float gas_value = 0.5;

	if (car.GetEngine().GetRPM() < car.GetEngine().GetStallRPM())
		inputs[CarInput::START_ENGINE] = 1.0;
	else
		inputs[CarInput::START_ENGINE] = 0.0;

	const RoadPatch * curr_patch_ptr = GetCurrentPatch(car);
	if (!curr_patch_ptr)
	{
		// if car is not on track, just let it roll
		inputs[CarInput::THROTTLE] = 0.8;
		inputs[CarInput::BRAKE] = 0.0;
		return;
	}

	RoadPatch curr_patch = RevisePatch(curr_patch_ptr);

	const Vec3 patch_direction = GetPatchDirection(curr_patch).Normalize();
	const Vec3 car_velocity = ToMathVector<float>(car.GetVelocity());
	float currentspeed = car_velocity.dot(patch_direction);

	// check speed against speed limit of current patch
	float speed_limit = 0;
	if (!curr_patch.GetNextPatch())
	{
		speed_limit = CalcSpeedLimit(car, &curr_patch, 0, 0);
	}
	else
	{
		RoadPatch next_patch = RevisePatch(curr_patch.GetNextPatch());
		float width = GetPatchWidthVector(*curr_patch_ptr).Magnitude();
		speed_limit = CalcSpeedLimit(car, &curr_patch, &next_patch, width);
	}
	speed_limit *= difficulty;

	float speed_diff = speed_limit - currentspeed;
	if (speed_diff < 0)
	{
		if (-speed_diff < MIN_SPEED_DIFF) //no need to brake if diff is small
		{
			brake_value = 0;
		}
		else
		{
			brake_value = -speed_diff / MAX_SPEED_DIFF;
			if (brake_value > 1) brake_value = 1;
		}
		gas_value = 0;
	}
	else if (std::isnan(speed_diff) || speed_diff > MAX_SPEED_DIFF)
	{
		gas_value = 1;
		brake_value = 0;
	}
	else
	{
		gas_value = speed_diff / MAX_SPEED_DIFF;
		brake_value = 0.;
	}

	// check upto maxlookahead distance
	float maxlookahead = car.GetBrakeDistance(currentspeed, 0, FRICTION_FACTOR_LONG) + 10;
	float dist_checked = 0;
	float brake_dist = 0;
	RoadPatch patch_to_check = curr_patch;

#ifdef VISUALIZE_AI_DEBUG
	brakelook.push_back(patch_to_check);
#endif

	while (dist_checked < maxlookahead)
	{
		RoadPatch * unmodified_patch_to_check = patch_to_check.GetNextPatch();

		if (!patch_to_check.GetNextPatch())
		{
			// if there is no next patch(probably a non-closed track, just let it roll
			brake_value = 0;
			dist_checked = maxlookahead;
			break;
		}
		else
		{
			patch_to_check = RevisePatch(patch_to_check.GetNextPatch());
		}

#ifdef VISUALIZE_AI_DEBUG
		brakelook.push_back(patch_to_check);
#endif

		if (!patch_to_check.GetNextPatch())
		{
			speed_limit = CalcSpeedLimit(car, &patch_to_check, 0, 0);
		}
		else
		{
			RoadPatch next_patch = RevisePatch(patch_to_check.GetNextPatch());
			float width = GetPatchWidthVector(*unmodified_patch_to_check).Magnitude();
			speed_limit = CalcSpeedLimit(car, &patch_to_check, &next_patch, width);
		}

		dist_checked += GetPatchDirection(patch_to_check).Magnitude();
		brake_dist = car.GetBrakeDistance(currentspeed, speed_limit, FRICTION_FACTOR_LONG);
		if (brake_dist > dist_checked)
		{
			brake_value = 1;
			gas_value = 0;
			break;
		}
	}

	gas_value = RateLimit(inputs[CarInput::THROTTLE], gas_value, THROTTLE_RATE_LIMIT, THROTTLE_RATE_LIMIT);
	brake_value = RateLimit(inputs[CarInput::BRAKE], brake_value, BRAKE_RATE_LIMIT, BRAKE_RATE_LIMIT);

	inputs[CarInput::THROTTLE] = gas_value;
	inputs[CarInput::BRAKE] = brake_value;
}

float AiCarStandard::CalcSpeedLimit(
	const CarDynamics & car,
	const RoadPatch * patch,
	const RoadPatch * nextpatch,
	float extraradius)
{
	assert(patch);

	// adjust the radius at corner exit to allow a higher speed.
	// this will get the car to accelerate out of corner
	float radius = GetPatchRadius(*patch);
	if (nextpatch &&
		GetPatchRadius(*nextpatch) > radius &&
		radius > LOOKAHEAD_MIN_RADIUS)
	{
		radius += extraradius;
	}
	return car.GetMaxSpeed(radius, FRICTION_FACTOR_LAT);
}

void AiCarStandard::UpdateSteer(const CarDynamics & car)
{
#ifdef VISUALIZE_AI_DEBUG
	steerlook.clear();
#endif

	const RoadPatch *curr_patch_ptr = GetCurrentPatch(car);

	//if car has no contact with track, just let it roll
	if (!curr_patch_ptr)
	{
		if (!last_patch) return;
		//if car is off track, steer the car towards the last patch it was on
		//this should get the car back on track
		else curr_patch_ptr = last_patch;
	}

	last_patch = curr_patch_ptr; //store the last patch car was on

	RoadPatch curr_patch = RevisePatch(curr_patch_ptr);

#ifdef VISUALIZE_AI_DEBUG
	steerlook.push_back(curr_patch);
#endif

	// if there is no next patch (probably a non-closed track), let it roll
	if (!curr_patch.GetNextPatch()) return;

	RoadPatch next_patch = RevisePatch(curr_patch.GetNextPatch());

	// find the point to steer towards
	float lookahead = 1;
	float length = 0;
	Vec3 dest_point = GetPatchFrontCenter(next_patch);

	while (length < lookahead)
	{
#ifdef VISUALIZE_AI_DEBUG
		steerlook.push_back(next_patch);
#endif

		length += GetPatchDirection(next_patch).Magnitude()*2;
		dest_point = GetPatchFrontCenter(next_patch);

		// if there is no next patch for whatever reason, stop lookahead
		if (!next_patch.GetNextPatch())
		{
			length = lookahead;
			break;
		}

		next_patch = RevisePatch(next_patch.GetNextPatch());

		// if next patch is a very sharp corner, stop lookahead
		if (GetPatchRadius(next_patch) < LOOKAHEAD_MIN_RADIUS)
		{
			length = lookahead;
			break;
		}
	}

	btVector3 car_position = car.GetCenterOfMass();
	btVector3 car_orientation = quatRotate(car.GetOrientation(), Direction::forward);
	btVector3 desire_orientation = ToBulletVector(dest_point) - car_position;

	//car's direction on the horizontal plane
	car_orientation[2] = 0;
	//desired direction on the horizontal plane
	desire_orientation[2] = 0;

	car_orientation.normalize();
	desire_orientation.normalize();

	//the angle between car's direction and unit y vector (forward direction)
	float alpha = std::atan2(car_orientation[1], car_orientation[0]) * rad2deg;

	//the angle between desired direction and unit y vector (forward direction)
	float beta = std::atan2(desire_orientation[1], desire_orientation[0])  * rad2deg;

	//calculate steering angle and direction
	float angle = beta - alpha;

	//angle += steerAwayFromOthers(c, dt, othercars, angle); //sum in traffic avoidance bias

	if (angle > -360 && angle <= -180)
		angle = -(360 + angle);
	else if (angle > -180 && angle <= 0)
		angle = - angle;
	else if (angle > 0 && angle <= 180)
		angle = - angle;
	else if (angle > 180 && angle <= 360)
		angle = 360 - angle;

	float steer_value = Clamp(angle / car.GetMaxSteeringAngle(), -1.0f, 1.0f);

	inputs[CarInput::STEER_RIGHT] = steer_value;
}

float AiCarStandard::GetHorizontalDistanceAlongPatch(const RoadPatch & patch, Vec3 carposition)
{
	Vec3 leftside = (patch.GetPoint(0,0) + patch.GetPoint(3,0))*0.5f;
	Vec3 rightside = (patch.GetPoint(0,3) + patch.GetPoint(3,3))*0.5f;
	Vec3 patchwidthvector = rightside - leftside;
	return patchwidthvector.Normalize().dot(carposition-leftside);
}

float AiCarStandard::RampBetween(float val, float startat, float endat)
{
	assert(endat > startat);
	return (Clamp(val,startat,endat)-startat)/(endat-startat);
}

float AiCarStandard::BrakeFromOthers(float speed_diff)
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

	for (const auto & car : othercars)
	{
		if (car.active && std::abs(car.horizontal_distance) < horizontal_care)
		{
			if (car.fore_distance < mindistance)
			{
				mindistance = car.fore_distance;
				mineta = car.eta;
			}
		}
	}

	float bias = 0;

	float etafeedback = 0;
	float distancefeedback = 0;

	//correct for eta of impact
	if (mineta < 1000)
	{
		etafeedback += 1-RampBetween(mineta, fullbrakeeta, startateta);
	}

	//correct for absolute distance
	if (mindistance < 1000)
	{
		distancefeedback += 1-RampBetween(mineta,fullbrakedistance,startatdistance);
	}

	//correct for speed versus speed limit (don't bother to brake for slowpokes, just go around)
	float speedfeedback = 1-RampBetween(speed_diff, fullbiasdiff, nobiasdiff);

	//std::cout << mineta << ": " << etafeedback << ", " << mindistance << ": " << distancefeedback << ", " << speed_diff << ": " << speedfeedback << std::endl;

	//bias = Clamp((etafeedback+distancefeedback)*speedfeedback,0.0f,1.0f);
	bias = Clamp(etafeedback*distancefeedback*speedfeedback,0.0f,1.0f);

	return bias;
}

void AiCarStandard::AnalyzeOthers(float dt, const CarDynamics cars[], const unsigned cars_num)
{
	const float half_carlength = 1.25;
	const btVector3 throttle_axis = Direction::forward;
	const CarDynamics & car = cars[carid];

	if (othercars.size() < cars_num)
		othercars.resize(cars_num);

	for (unsigned i = 0; i != cars_num; ++i)
	{
		if (i == carid)
			continue;

		const CarDynamics & icar = cars[i];
		OtherCarInfo & info = othercars[i];

		// find direction of other cars in our frame
		btVector3 relative_position = icar.GetCenterOfMass() - car.GetCenterOfMass();
		relative_position = quatRotate(car.GetOrientation().inverse(), relative_position);

		// only make a move if the other car is within our distance limit
		float fore_position = relative_position.dot(throttle_axis);

		btVector3 myvel = quatRotate(car.GetOrientation().inverse(), car.GetVelocity());
		btVector3 othervel = quatRotate(icar.GetOrientation().inverse(), icar.GetVelocity());
		float speed_diff = othervel.dot(throttle_axis) - myvel.dot(throttle_axis);

		const float fore_position_offset = -half_carlength;
		if (fore_position > fore_position_offset)
		{
			const RoadPatch * othercarpatch = GetCurrentPatch(icar);
			const RoadPatch * mycarpatch = GetCurrentPatch(car);

			if (othercarpatch && mycarpatch)
			{
				Vec3 mypos = ToMathVector<float>(car.GetCenterOfMass());
				Vec3 otpos = ToMathVector<float>(icar.GetCenterOfMass());
				float my_track_placement = GetHorizontalDistanceAlongPatch(*mycarpatch, mypos);
				float their_track_placement = GetHorizontalDistanceAlongPatch(*othercarpatch, otpos);

				float speed_diff_denom = Clamp(speed_diff, -100.f, -0.01f);
				float eta = (fore_position - fore_position_offset) / -speed_diff_denom;

				if (!info.active)
					info.eta = eta;
				else
					info.eta = RateLimit(info.eta, eta, 10.f*dt, 10000.f*dt);

				info.horizontal_distance = their_track_placement - my_track_placement;
				info.fore_distance = fore_position;
				info.active = true;
			}
			else
			{
				info.active = false;
			}
		}
		else
		{
			info.active = false;
		}
	}
}

float AiCarStandard::SteerAwayFromOthers(float carspeed)
{
	const float spacingdistance = 3.5; //how far left and right we target for our spacing in meters (center of mass to center of mass)
	const float horizontal_meters_per_second = 5.0; //how fast we want to steer away in horizontal meters per second
	const float speed = Max(1.0f, carspeed);
	const float authority = Min(10.0f, std::atan(horizontal_meters_per_second / speed) * rad2deg); //steering bias authority limit magnitude in degrees
	const float gain = 4.0; //amplify steering command by this factor
	const float mineta = 1.0; //fastest reaction time in seconds
	const float etaexponent = 1.0;

	float eta = 1000;
	float min_horizontal_distance = 1000;

	for (const auto & car : othercars)
	{
		if (car.active && std::abs(car.horizontal_distance) < std::abs(min_horizontal_distance))
		{
			min_horizontal_distance = car.horizontal_distance;
			eta = car.eta;
		}
	}

	if (min_horizontal_distance == 1000)
		return 0.0;

	eta = Max(eta, mineta);

	float bias = Clamp(min_horizontal_distance, -spacingdistance, spacingdistance);
	if (bias < 0)
		bias = -bias - spacingdistance;
	else
		bias = spacingdistance - bias;

	bias *= std::pow(mineta,etaexponent)*gain/std::pow(eta,etaexponent);
	bias = Clamp(bias, -spacingdistance, spacingdistance);

	return (bias / spacingdistance) * authority;
}

#ifdef VISUALIZE_AI_DEBUG
void AiCarStandard::ConfigureDrawable(SceneNode::DrawableHandle & ref, SceneNode & topnode, float r, float g, float b)
{
	if (!ref.valid())
	{
		ref = topnode.GetDrawList().normal_noblend.insert(Drawable());
		Drawable & d = topnode.GetDrawList().normal_noblend.get(ref);
		d.SetColor(r,g,b,1);
		d.SetDecal(true);
	}
}

void AiCarStandard::AddLinePoint(VertexArray & va, const Vec3 & p)
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

void AiCarStandard::Visualize()
{
	SceneNode& topnode  = car.GetNode();
	ConfigureDrawable(brakedraw, topnode, 0,1,0);
	ConfigureDrawable(steerdraw, topnode, 0,0,1);
	//ConfigureDrawable(avoidancedraw, topnode, 1,0,0);

	Drawable & brakedrawable = topnode.GetDrawList().normal_noblend.get(brakedraw);
	Drawable & steerdrawable = topnode.GetDrawList().normal_noblend.get(steerdraw);

	brakedrawable.SetVertArray(&brakeshape);
	brakeshape.Clear();
	for (const auto & patch : brakelook)
	{
		AddLinePoint(brakeshape, patch.GetBL());
		AddLinePoint(brakeshape, patch.GetFL());
		AddLinePoint(brakeshape, patch.GetFR());
		AddLinePoint(brakeshape, patch.GetBR());
		AddLinePoint(brakeshape, patch.GetBL());
	}

	steerdrawable.SetVertArray(&steershape);
	steershape.Clear();
	for (const auto & patch : steerlook)
	{
		AddLinePoint(steershape, patch.GetBL());
		AddLinePoint(steershape, patch.GetFL());
		AddLinePoint(steershape, patch.GetBR());
		AddLinePoint(steershape, patch.GetFR());
		AddLinePoint(steershape, patch.GetBL());
	}
}
#endif
