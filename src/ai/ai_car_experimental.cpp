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

#include "ai_car_experimental.h"
#include "physics/cardynamics.h"
#include "physics/dynamicsworld.h"
#include "tobullet.h"
#include "track.h"
#include "unittest.h"

#include <cassert>
#include <cmath>
#include <ctime>
#include <algorithm>
#include <iostream>

AiCar * AiCarExperimentalFactory::Create(const CarDynamics * car, float difficulty)
{
	return new AiCarExperimental(car, difficulty);
}

AiCarExperimental::AiCarExperimental(const CarDynamics * new_car, float new_difficulty) :
	AiCar(new_car, new_difficulty),
	longitude_mu(0.9),
	lateral_mu(0.9),
	last_patch(NULL),
	use_racingline(true),
	isRecovering(false)
{
	assert(car);
}

AiCarExperimental::~AiCarExperimental()
{
#ifdef VISUALIZE_AI_DEBUG
	SceneNode & topnode  = car->GetNode();
	if (brakedraw.valid())
	{
		topnode.GetDrawList().normal_noblend.erase(brakedraw);
	}
	if (steerdraw.valid())
	{
		topnode.GetDrawList().normal_noblend.erase(steerdraw);
	}
	if (raycastdraw.valid())
	{
		topnode.GetDrawList().normal_noblend.erase(raycastdraw);
	}
#endif
}

#define GRAVITY 9.8

//used to calculate brake value
#define MAX_SPEED_DIFF 6.0
#define MIN_SPEED_DIFF 1.0

//used to find the point the car should steer towards to
#define LOOKAHEAD_FACTOR1 2.25
#define LOOKAHEAD_FACTOR2 0.33

//used to detect very sharp corners like Circuit de Pau
#define LOOKAHEAD_MIN_RADIUS 8.0

//used to calculate friction coefficient
#define FRICTION_FACTOR_LONG 0.68
#define FRICTION_FACTOR_LAT 0.62

//maximum change in brake value per second
#define BRAKE_RATE_LIMIT 2.0 // 500 milisec
#define THROTTLE_RATE_LIMIT 2.0

float AiCarExperimental::clamp(float val, float min, float max)
{
	assert(min <= max);
	return std::min(max,std::max(min,val));
}

//note that rate_limit_neg should be positive, it gets inverted inside the function
float AiCarExperimental::RateLimit(float old_value, float new_value, float rate_limit_pos, float rate_limit_neg)
{
	if (new_value - old_value > rate_limit_pos)
		return old_value + rate_limit_pos;
	else if (new_value - old_value < -rate_limit_neg)
		return old_value - rate_limit_neg;
	else
		return new_value;
}

void AiCarExperimental::Update(float dt, const CarDynamics cars[], const int cars_num)
{
	float lastThrottle = inputs[CarInput::THROTTLE];
	float lastBreak = inputs[CarInput::BRAKE];
	fill(inputs.begin(), inputs.end(), 0);

	AnalyzeOthers(dt, cars, cars_num);
	UpdateGasBrake();
	UpdateSteer();
	float rateLimit = THROTTLE_RATE_LIMIT * dt;
	inputs[CarInput::THROTTLE] = RateLimit(lastThrottle, inputs[CarInput::THROTTLE],
		rateLimit, rateLimit);
	rateLimit = BRAKE_RATE_LIMIT * dt;
	inputs[CarInput::BRAKE] = RateLimit(lastBreak, inputs[CarInput::BRAKE],
		rateLimit, rateLimit);
}

const Bezier * AiCarExperimental::GetCurrentPatch(const CarDynamics * c)
{
	const Bezier * curr_patch = c->GetWheelContact(WheelPosition(0)).GetPatch();
	if (!curr_patch)
	{
		// let's try the other wheel
		curr_patch = c->GetWheelContact(WheelPosition(1)).GetPatch();
		if (!curr_patch) return NULL;
	}
	return curr_patch;
}

Vec3 AiCarExperimental::GetPatchFrontCenter(const Bezier & patch)
{
	return (patch.GetPoint(0,0) + patch.GetPoint(0,3)) * 0.5;
}

Vec3 AiCarExperimental::GetPatchBackCenter(const Bezier & patch)
{
	return (patch.GetPoint(3,0) + patch.GetPoint(3,3)) * 0.5;
}

Vec3 AiCarExperimental::GetPatchDirection(const Bezier & patch)
{
	return GetPatchFrontCenter(patch) - GetPatchBackCenter(patch);
}

Vec3 AiCarExperimental::GetPatchWidthVector(const Bezier & patch)
{
	return ((patch.GetPoint(0,0) + patch.GetPoint(3,0)) -
			(patch.GetPoint(0,3) + patch.GetPoint(3,3))) * 0.5;
}

double AiCarExperimental::GetPatchRadius(const Bezier & patch)
{
	if (patch.GetNextPatch() && patch.GetNextPatch()->GetNextPatch())
	{
		double track_radius = 0;

		Vec3 d1 = -(patch.GetNextPatch()->GetRacingLine() - patch.GetRacingLine());
		Vec3 d2 = patch.GetNextPatch()->GetNextPatch()->GetRacingLine() - patch.GetNextPatch()->GetRacingLine();
		d1[2] = 0;
		d2[2] = 0;
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

///trim the patch's width in-place
void AiCarExperimental::TrimPatch(Bezier & patch, float trimleft_front, float trimright_front, float trimleft_back, float trimright_back)
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

	if (frontvector.Magnitude() > 0.001)
	{
		Vec3 trimdirection_front = frontvector.Normalize();
		newfl = patch.GetPoint(0,0) + trimdirection_front*trimleft_front;
		newfr = patch.GetPoint(0,3) - trimdirection_front*trimright_front;
	}

	if (backvector.Magnitude() > 0.001)
	{
		Vec3 trimdirection_back = backvector.Normalize();
		newbl = patch.GetPoint(3,0) + trimdirection_back*trimleft_back;
		newbr = patch.GetPoint(3,3) - trimdirection_back*trimright_back;
	}

	patch.SetFromCorners(newfl, newfr, newbl, newbr);
}

Bezier AiCarExperimental::RevisePatch(const Bezier * origpatch, bool use_racingline)
{
	Bezier patch = *origpatch;

	//take into account the racing line
	//use_racingline = false;
	if (use_racingline && patch.GetNextPatch() && patch.HasRacingline())
	{
		float widthfront = std::min((patch.GetNextPatch()->GetRacingLine()-patch.GetPoint(0,0)).Magnitude(),
									 (patch.GetNextPatch()->GetRacingLine()-patch.GetPoint(0,3)).Magnitude());
		float widthback = std::min((patch.GetRacingLine()-patch.GetPoint(3,0)).Magnitude(),
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
	std::map <const Car *, PathRevision> & revmap = path_revisions;
	for (std::map <const Car *, PathRevision>::iterator i = revmap.begin(); i != revmap.end(); i++)
	{
		if (i->first != car)
		{
			//compute relative info
			Vec3 myvel = car->GetVelocity();
			Vec3 othervel = i->first->GetVelocity();
			(-car->GetOrientation()).RotateVector(myvel);
			(-i->first->GetOrientation()).RotateVector(othervel);
			float speed_diff = myvel.dot(throttle_axis) - othervel.dot(throttle_axis); //positive if other car is faster //actually positive if my car is faster, right?

			float cardist_back = patch.dist_from_start - i->second.car_pos_along_track; //positive if patch is ahead of car
			float patchlen = GetPatchDirection(patch).Magnitude();
			float cardist_front = (patch.dist_from_start+patchlen) - i->second.car_pos_along_track;

			const float minfalloff = 10;
			const float maxfalloff = 60;
			float cur_trim_falloff_distance_fwd = minfalloff;
			float cur_trim_falloff_distance_rear = minfalloff;
			float falloff = clamp(trim_falloff_distance*std::abs(speed_diff),minfalloff,maxfalloff);
			if (speed_diff > 0)
			{
				//cur_trim_falloff_distance_fwd = falloff;
			}
			else
				cur_trim_falloff_distance_rear = falloff;

			float scale_front = clamp(1.0f-cardist_front/cur_trim_falloff_distance_fwd, 0, 1);
			if (cardist_front < 0)
				scale_front = clamp(1.0f+cardist_front/cur_trim_falloff_distance_rear, 0, 1);
			float scale_back = clamp(1.0f-cardist_back/cur_trim_falloff_distance_fwd, 0, 1);
			if (cardist_back < 0)
				scale_back = clamp(1.0f+cardist_back/cur_trim_falloff_distance_rear, 0, 1);

			std::cout << speed_diff << ", " << cur_trim_falloff_distance_fwd << ", " << cur_trim_falloff_distance_rear << ", " << cardist_front << ", " << cardist_back << ", " << scale_front << ", " << scale_back << std::endl;

			float trimleft_front = i->second.trimleft_front*scale_front;
			float trimright_front = i->second.trimright_front*scale_front;
			float trimleft_back = i->second.trimleft_back*scale_back;
			float trimright_back = i->second.trimright_back*scale_back;

			TrimPatch(patch, trimleft_front, trimright_front, trimleft_back, trimright_back);
		}
	}*/

	return patch;
}

void AiCarExperimental::UpdateGasBrake()
{
#ifdef VISUALIZE_AI_DEBUG
	brakelook.clear();
	raycastshape.Clear();
#endif

	float brake_value = 0.0;
	float gas_value = 0.5;

	if (car->GetEngine().GetRPM() < car->GetEngine().GetStallRPM())
		inputs[CarInput::START_ENGINE] = 1.0;
	else
		inputs[CarInput::START_ENGINE] = 0.0;

	CalcMu();

	const Bezier * curr_patch_ptr = GetCurrentPatch(car);
	if (!curr_patch_ptr)
	{
		// if car is not on track, just let it roll
		inputs[CarInput::THROTTLE] = 0.8;
		inputs[CarInput::BRAKE] = 0.0;
		return;
	}

	Bezier curr_patch = RevisePatch(curr_patch_ptr, use_racingline);

	const Vec3 patch_direction = GetPatchDirection(curr_patch).Normalize();
	const Vec3 car_velocity = ToMathVector<float>(car->GetVelocity());
	float currentspeed = car_velocity.dot(patch_direction);

	// check speed against speed limit of current patch
	float speed_limit = 0;
	if (!curr_patch.GetNextPatch())
	{
		speed_limit = CalcSpeedLimit(&curr_patch, NULL, lateral_mu, GetPatchWidthVector(*curr_patch_ptr).Magnitude());
	}
	else
	{
		Bezier next_patch = RevisePatch(curr_patch.GetNextPatch(), use_racingline);
		speed_limit = CalcSpeedLimit(&curr_patch, &next_patch, lateral_mu, GetPatchWidthVector(*curr_patch_ptr).Magnitude());
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
	else if (std::isnan(speed_diff) || speed_diff > MAX_SPEED_DIFF)
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
	float maxlookahead = CalcBrakeDist(currentspeed, 0.0, longitude_mu)+10;
	float dist_checked = 0.0;
	float brake_dist = 0.0;
	Bezier patch_to_check = curr_patch;

#ifdef VISUALIZE_AI_DEBUG
	brakelook.push_back(patch_to_check);
#endif

	while (dist_checked < maxlookahead)
	{
		Bezier * unmodified_patch_to_check = patch_to_check.GetNextPatch();

		if (!patch_to_check.GetNextPatch())
		{
			// if there is no next patch(probably a non-closed track, just let it roll
			brake_value = 0.0;
			dist_checked = maxlookahead;
			break;
		}
		else
		{
			patch_to_check = RevisePatch(patch_to_check.GetNextPatch(), use_racingline);
		}

#ifdef VISUALIZE_AI_DEBUG
		brakelook.push_back(patch_to_check);
#endif

		if (!patch_to_check.GetNextPatch())
		{
			speed_limit = CalcSpeedLimit(&patch_to_check, NULL, lateral_mu, GetPatchWidthVector(*unmodified_patch_to_check).Magnitude());
		}
		else
		{
			Bezier next_patch = RevisePatch(patch_to_check.GetNextPatch(), use_racingline);
			speed_limit = CalcSpeedLimit(&patch_to_check, &next_patch, lateral_mu, GetPatchWidthVector(*unmodified_patch_to_check).Magnitude());
		}

		dist_checked += GetPatchDirection(patch_to_check).Magnitude();
		brake_dist = CalcBrakeDist(currentspeed, speed_limit, longitude_mu) * 1.4;
		if (brake_dist > dist_checked)
		{
			brake_value = 1.0;
			gas_value = 0.0;
			break;
		}
	}

	inputs[CarInput::THROTTLE] = gas_value;
	inputs[CarInput::BRAKE] = brake_value;
}

void AiCarExperimental::CalcMu()
{
	const float tire_load = 0.25 * GRAVITY / car->GetInvMass();
	float long_friction = 0.0;
	float lat_friction = 0.0;
	for (int i = 0; i < 4; i++)
	{
		long_friction += car->GetTire(WheelPosition(i)).getMaxFx(tire_load);
		lat_friction += car->GetTire(WheelPosition(i)).getMaxFy(tire_load, 0.0);
	}
	float long_mu = FRICTION_FACTOR_LONG * long_friction * car->GetInvMass() / GRAVITY;
	float lat_mu = FRICTION_FACTOR_LAT * lat_friction * car->GetInvMass() / GRAVITY;
	if (!std::isnan(long_mu)) longitude_mu = long_mu;
	if (!std::isnan(lat_mu)) lateral_mu = lat_mu;
}

float AiCarExperimental::CalcSpeedLimit(const Bezier  * patch, const Bezier * nextpatch, float friction, float extraradius=0)
{
	assert(patch);

	//adjust the radius at corner exit to allow a higher speed.
	//this will get the car to accelerate out of corner
	//double track_width = GetPatchWidthVector(*patch).Magnitude();
	double adjusted_radius = GetPatchRadius(*patch);
	if (nextpatch)
	{
		if (GetPatchRadius(*nextpatch) > adjusted_radius &&
			GetPatchRadius(*patch) > LOOKAHEAD_MIN_RADIUS)
		{
			adjusted_radius += extraradius;
		}
	}

	//no downforce
	//float v1 = sqrt(friction * GRAVITY * adjusted_radius);

	//take into account downforce
	double denom = (1.0 - std::min(1.01, adjusted_radius * -(car->GetAerodynamicDownforceCoefficient()) * friction * car->GetInvMass()));
	double real = (friction * GRAVITY * adjusted_radius) / denom;
	double v2 = 1000.0; //some really big number
	if (real > 0)
		v2 = sqrt(real);

	//std::cout << v2 << ", " << sqrt(friction * GRAVITY * adjusted_radius) << ", " << GetPatchRadius(*patch) << ", " << acos((-GetPatchDirection(*patch)).Normalize().dot(GetPatchDirection(*patch->GetNextPatch()).Normalize()))*180.0/3.141593 << " --- " << -GetPatchDirection(*patch) << " --- " << GetPatchDirection(*patch->GetNextPatch()) << std::endl;

	return v2;
}

float AiCarExperimental::CalcBrakeDist(float current_speed, float allowed_speed, float friction)
{
	if (allowed_speed < current_speed)
	{
		// equations used:
		// mu * mass * gravity * distance = 0.5 * mass * (Initial_velocity^2 - Final_velocity^2)
		// where distance is:
		//  distance = (Initial_velocity^2 - Final_velocity^2) / (2 * mu * gravity)
		return (current_speed * current_speed - allowed_speed * allowed_speed) / ( 2.0 * friction * GRAVITY);
	}

	// if allowed speed  is bigger then the current speed then break distance is 0
	return 0;
}

float AiCarExperimental::RayCastDistance(Vec3 direction, float max_length)
{
	btVector3 pos = car->GetPosition();
	btVector3 dir = car->LocalToWorld(ToBulletVector(direction)) - pos;

	CollisionContact contact;
	car->getDynamicsWorld()->castRay(
		pos, dir, max_length,
		&car->getCollisionObject(),
		contact);

	float depth = contact.GetDepth();
	float dist = std::min(max_length, depth);

#ifdef VISUALIZE_AI_DEBUG
	Vec3 pos_start(ToMathVector<float>(pos));
	Vec3 pos_end = pos_start + (ToMathVector<float>(dir) * dist);
	AddLinePoint(raycastshape, pos_start);
	AddLinePoint(raycastshape, pos_end);
#endif

	return dist;
}

const Bezier * AiCarExperimental::GetNearestPatch(const Bezier * /*helper*/)
{
	// At the moment this is very slow!
	// TODO: Implement backward chaining for BEZIER, to just look around the helper if passed.
 	const btVector3 c = car->GetPosition();
	const Bezier * b_end = car->getDynamicsWorld()->GetSectorPatch(0);
	const Bezier * b = b_end->GetNextPatch();
	const Bezier * b_nearest = 0;
	float v_nearDist = 1000000.0f;
	while(b != 0 && b != b_end)
	{
		Vec3 v(b->GetPoint(2,2));
		v[0] -= c[1];
		v[2] -= c[0];
		float dist = v[0]*v[0]+v[2]*v[2];
		if(dist < v_nearDist){
			v_nearDist = dist;
			b_nearest = b;

		}
		b = b->GetNextPatch();
	}
	assert(b_nearest);
	return b_nearest;
}

bool AiCarExperimental::Recover(const Bezier * /*patch*/)
{
	// Recover mode will basically detect walls on the front
	// of the car, then go reverse if needed for 3 secs,
	// then recheck if there is still a wall on the front. If there is no wall,
	// break to zero speed and then switch to first gear back and leave recover mode.
	const float maxRayDistant = 3;


	if(car->GetSpeed() < 1)
	{
		//If the car is not moving, there may be a wall in the front.

		//Cast ray towards front-middle
		float dist = RayCastDistance(Vec3(0, 1, 0), maxRayDistant);

		if(dist < maxRayDistant * 0.99)
		{
			// Collision detected: we are probably trying to cross a wall.
			// We need to drive backwards.
			inputs[CarInput::REVERSE] = 1;
			time (&recoverStartTime);
			isRecovering = true;
			return true;
		} else {
			// If there are no walls in front, just leave the recover mode.
			inputs[CarInput::FIRST_GEAR] = 1;
			isRecovering = false;
			return false;
		}
	}
	else if(isRecovering)
	{
		// If car is driving and it is in recover mode, count the time since start of recover mode.
		time_t curr;
		time (&curr);
		float diff = difftime(curr, recoverStartTime);
		if (diff > 3)
		{
			// Break to 0 after 3 secs of driving backwards.
			// After breaking, it will trigger the "isRecovering = false" above and leave recover mode.
			inputs[CarInput::THROTTLE] = 0;
			inputs[CarInput::BRAKE] = 1;
			return true;
		}
	}
	// If car is still driving and it is not in recover mode, just do the usual stuff.
	return false;
}
void AiCarExperimental::UpdateSteer()
{
#ifdef VISUALIZE_AI_DEBUG
	steerlook.clear();
#endif

	const Bezier * curr_patch_ptr = GetCurrentPatch(car);

	// if car has no contact with track, just let it roll
	if (!curr_patch_ptr || isRecovering)
	{
		last_patch = GetNearestPatch(last_patch);

		// if car is off track, steer the car towards the last patch it was on
		// this should get the car back on track
		curr_patch_ptr = last_patch;

		// recover to the road.
		if(Recover(curr_patch_ptr))
			return;
	}

	last_patch = curr_patch_ptr;

	Bezier curr_patch = RevisePatch(curr_patch_ptr, use_racingline);
#ifdef VISUALIZE_AI_DEBUG
	steerlook.push_back(curr_patch);
#endif

	// if there is no next patch (probably a non-closed track), let it roll
	if (!curr_patch.GetNextPatch())
		return;

	Bezier next_patch = RevisePatch(curr_patch.GetNextPatch(), use_racingline);

	// find the point to steer towards
	float lookahead = 1.0;
	float length = 0.0;
	Vec3 dest_point = GetPatchFrontCenter(next_patch);

	while (length < lookahead)
	{
#ifdef VISUALIZE_AI_DEBUG
		steerlook.push_back(next_patch);
#endif

		length += GetPatchDirection(next_patch).Magnitude();
		dest_point = GetPatchFrontCenter(next_patch);

		// if there is no next patch for whatever reason, stop lookahead
		if (!next_patch.GetNextPatch())
		{
			length = lookahead;
			break;
		}

		next_patch = RevisePatch(next_patch.GetNextPatch(), use_racingline);

		// if next patch is a very sharp corner, stop lookahead
		if (GetPatchRadius(next_patch) < LOOKAHEAD_MIN_RADIUS)
		{
			length = lookahead;
			break;
		}
	}

	btVector3 car_position = car->GetCenterOfMass();
	btVector3 car_orientation = quatRotate(car->GetOrientation(), Direction::forward);
	btVector3 desire_orientation = ToBulletVector(dest_point) - car_position;

	//car's direction on the horizontal plane
	car_orientation[2] = 0;
	//desired direction on the horizontal plane
	desire_orientation[2] = 0;

	car_orientation.normalize();
	desire_orientation.normalize();

	//the angle between car's direction and unit y vector (forward direction)
	double alpha = Angle(car_orientation[0], car_orientation[1]);

	//the angle between desired direction and unit y vector (forward direction)
	double beta = Angle(desire_orientation[0], desire_orientation[1]);

	//calculate steering angle and direction
	double angle = beta - alpha;

	//angle += steerAwayFromOthers(c, dt, othercars, angle); //sum in traffic avoidance bias

	if (angle > -360.0 && angle <= -180.0)
		angle = -(360.0 + angle);
	else if (angle > -180.0 && angle <= 0.0)
		angle = - angle;
	else if (angle > 0.0 && angle <= 180.0)
		angle = - angle;
	else if (angle > 180.0 && angle <= 360.0)
		angle = 360.0 - angle;

	float optimum_range = car->GetTire(FRONT_LEFT).getIdealSlipAngle() * SIMD_DEGS_PER_RAD;
	angle = clamp(angle, -optimum_range, optimum_range);

	float steer_value = angle / car->GetMaxSteeringAngle();
	if (steer_value > 1.0) steer_value = 1.0;
	else if (steer_value < -1.0) steer_value = -1.0;

	assert(!std::isnan(steer_value));
	if(isRecovering){
		// If we are driving backwards, we need to invert steer direction.
		steer_value = steer_value > 0.0 ? -1.0 : 1.0;
	}
	inputs[CarInput::STEER_RIGHT] = steer_value;
}

float AiCarExperimental::GetHorizontalDistanceAlongPatch(const Bezier & patch, Vec3 carposition)
{
	Vec3 leftside = (patch.GetPoint(0,0) + patch.GetPoint(3,0))*0.5;
	Vec3 rightside = (patch.GetPoint(0,3) + patch.GetPoint(3,3))*0.5;
	Vec3 patchwidthvector = rightside - leftside;
	return patchwidthvector.Normalize().dot(carposition-leftside);
}

float AiCarExperimental::RampBetween(float val, float startat, float endat)
{
	assert(endat > startat);
	return (clamp(val,startat,endat)-startat)/(endat-startat);
}

float AiCarExperimental::BrakeFromOthers(float speed_diff)
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

	for (std::map<const CarDynamics *, OtherCarInfo>::iterator i = othercars.begin(); i != othercars.end(); ++i)
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

void AiCarExperimental::AnalyzeOthers(float dt, const CarDynamics cars[], const int cars_num)
{
	const float half_carlength = 1.25;
	const btVector3 throttle_axis = Direction::forward;

	for (int i = 0; i != cars_num; ++i)
	{
		const CarDynamics * icar = &cars[i];
		if (icar != car)
		{
			OtherCarInfo & info = othercars[icar];

			// find direction of other cars in our frame
			btVector3 relative_position = icar->GetCenterOfMass() - car->GetCenterOfMass();
			relative_position = quatRotate(-car->GetOrientation(), relative_position);

			// only make a move if the other car is within our distance limit
			float fore_position = relative_position.dot(throttle_axis);

			btVector3 myvel = quatRotate(-car->GetOrientation(), car->GetVelocity());
			btVector3 othervel = quatRotate(-icar->GetOrientation(), icar->GetVelocity());
			float speed_diff = othervel.dot(throttle_axis) - myvel.dot(throttle_axis);

			const float fore_position_offset = -half_carlength;
			if (fore_position > fore_position_offset)
			{
				const Bezier * othercarpatch = GetCurrentPatch(icar);
				const Bezier * mycarpatch = GetCurrentPatch(car);

				if (othercarpatch && mycarpatch)
				{
					Vec3 mypos = ToMathVector<float>(car->GetCenterOfMass());
					Vec3 otpos = ToMathVector<float>(icar->GetCenterOfMass());
					float my_track_placement = GetHorizontalDistanceAlongPatch(*mycarpatch, mypos);
					float their_track_placement = GetHorizontalDistanceAlongPatch(*othercarpatch, otpos);

					float speed_diff_denom = clamp(speed_diff, -100, -0.01);
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
}

float AiCarExperimental::SteerAwayFromOthers()
{
	const float spacingdistance = 3.5; //how far left and right we target for our spacing in meters (center of mass to center of mass)
	const float horizontal_meters_per_second = 5.0; //how fast we want to steer away in horizontal meters per second
	const float speed = std::max(1.0f, car->GetVelocity().length());
	const float authority = std::min(10.0, (180.0 / 3.141593) * atan(horizontal_meters_per_second / speed)); //steering bias authority limit magnitude in degrees
	const float gain = 4.0; //amplify steering command by this factor
	const float mineta = 1.0; //fastest reaction time in seconds
	const float etaexponent = 1.0;

	float eta = 1000;
	float min_horizontal_distance = 1000;

	for (std::map <const CarDynamics *, OtherCarInfo>::iterator i = othercars.begin(); i != othercars.end(); ++i)
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

double AiCarExperimental::Angle(double x1, double y1)
{
	return atan2(y1, x1) * 180.0 / M_PI;
}

#ifdef VISUALIZE_AI_DEBUG
void AiCarExperimental::ConfigureDrawable(SceneNode::DrawableHandle & ref, SceneNode & topnode, float r, float g, float b)
{
	if (!ref.valid())
	{
		ref = topnode.GetDrawList().normal_noblend.insert(Drawable());
		Drawable & d = topnode.GetDrawList().normal_noblend.get(ref);
		d.SetColor(r,g,b,1);
		d.SetDecal(true);
	}
}

void AiCarExperimental::AddLinePoint(VertexArray & va, const Vec3 & p)
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

void AiCarExperimental::Visualize()
{
	SceneNode& topnode  = car->GetNode();
	ConfigureDrawable(brakedraw, topnode, 0,1,0);
	ConfigureDrawable(steerdraw, topnode, 0,0,1);
	ConfigureDrawable(raycastdraw, topnode, 1,0,0);
	//ConfigureDrawable(avoidancedraw, topnode, 1,0,0);

	Drawable & brakedrawable = topnode.GetDrawList().normal_noblend.get(brakedraw);
	Drawable & steerdrawable = topnode.GetDrawList().normal_noblend.get(steerdraw);
	Drawable & raycastdrawable = topnode.GetDrawList().normal_noblend.get(raycastdraw);

	brakedrawable.SetVertArray(&brakeshape);
	brakeshape.Clear();
	for (std::vector <Bezier>::iterator i = brakelook.begin(); i != brakelook.end(); ++i)
	{
		Bezier & patch = *i;
		AddLinePoint(brakeshape, patch.GetBL());
		AddLinePoint(brakeshape, patch.GetFL());
		AddLinePoint(brakeshape, patch.GetFR());
		AddLinePoint(brakeshape, patch.GetBR());
		AddLinePoint(brakeshape, patch.GetBL());
	}

	steerdrawable.SetVertArray(&steershape);
	steershape.Clear();
	for (std::vector <Bezier>::iterator i = steerlook.begin(); i != steerlook.end(); ++i)
	{
		Bezier & patch = *i;
		AddLinePoint(steershape, patch.GetBL());
		AddLinePoint(steershape, patch.GetFL());
		AddLinePoint(steershape, patch.GetBR());
		AddLinePoint(steershape, patch.GetFR());
		AddLinePoint(steershape, patch.GetBL());
	}

	raycastdrawable.SetVertArray(&raycastshape);
}
#endif
