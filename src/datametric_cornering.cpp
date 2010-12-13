#include <iostream>
#include <cmath>
#include <assert.h>
#include <sstream>
using std::cout;
using std::endl;
using std::abs;
using std::ostringstream;

#include "datametric_cornering.h"

/// static class member declaration, register metric type name
METRICTYPEREGISTER<CORNERINGMETRIC> CORNERINGMETRIC::reg("Cornering");

CORNERINGMETRIC::CORNERINGMETRIC(DATAMETRIC_CTOR_PARAMS_DEF)
 : DATAMETRIC(DATAMETRIC_CTOR_PARAMS),
   state(not_turning),
   last_state(not_turning),
   turn_start_time(0.0),
   turn_max_lateral_accel(0.0)
{
	// Enabled by default
	DATAMETRIC::run = true;
}

CORNERINGMETRIC::~CORNERINGMETRIC()
{
	// Clean up any class data
}

void CORNERINGMETRIC::DetermineState()
{
	double steering = DATAMETRIC::GetLastInColumn("Steering");
	if (state == not_turning)
	{
		// check if the driver has started turning and note the time
		if (abs(steering) >= abs(DATAMETRIC::GetNextLastInColumn("Steering")))
		{
			state = turning_under_threshold;
		}
	}
	else if (state == turning_under_threshold)
	{
		if (abs(steering) >= 0.06 /*DATAMETRIC::GetOption("start_turn_steering_angle")*/ )
		{
			state = turning_over_threshold;
		}
		else if (abs(steering) < abs(DATAMETRIC::GetNextLastInColumn("Steering")))
		{
			state = not_turning;
		}
	}
	else if (state == turning_over_threshold)
	{
		if (abs(steering) < 0.02 /*DATAMETRIC::GetOption("end_turn_steering_angle")*/ )
		{
			state = turning_under_threshold;
		}
	}
}

void CORNERINGMETRIC::StateChangeReaction()
{
	// if the state hasn't changed, do nothing more.
	if (state == last_state)
		return;

	//cout << "Changed state: " << state << endl;

	// the state has changed, see what the present state now is
	if (state == turning_under_threshold)
	{
		if (last_state == not_turning)
		{
			turn_start_time = DATAMETRIC::GetNextLastInColumn("Time");
			//turn_start_lateral_velocity = DATAMETRIC::GetNextLastInColumn("LateralVelocity");
		}
		else if (last_state == turning_over_threshold)
		{
			// a turn was completed, report on it to driver
			double current_time = DATAMETRIC::GetLastInColumn("Time");
			double total_turn_time = current_time - turn_start_time;
			if (total_turn_time >= 1.0 /*DATAMETRIC::GetOption("min_turn_time")*/ )
			{
				ostringstream os;
				os << "Turn completed, took " << total_turn_time << " seconds" << endl;
				os << "Maximum lateral accel: " << (turn_max_lateral_accel / 9.81) << " Gs" << endl;
				DATAMETRIC::SetFeedbackMessageEvent("Cornering Report", os.str(), 10.0, 1.0, 2.0);
			}
			turn_max_lateral_accel = 0.0;
		}
	}
	else if (state == turning_over_threshold)
	{
		assert (last_state == turning_under_threshold);
		/*double current_time = DATAMETRIC::GetLastInColumn("Time");
		ostringstream os;
		os << "Started turning " << (current_time - turn_start_time) << " seconds ago" << endl;
		//cout << "started braking at " << braking_start_time << endl;
		DATAMETRIC::SetFeedbackMessageEvent("Start Turn Message", os.str(), 5.0, 1.0, 1.0);*/
	}
	else if (state == not_turning)
	{
		assert (last_state == turning_under_threshold);
		//cout << "not braking" << endl;
		turn_start_time = -1.0;
	}

	last_state = state;
}

void CORNERINGMETRIC::Update(float dt)
{
	// Do nothing when disabled
	if (!run)
		return;

	DetermineState();

	StateChangeReaction();

	// Calculate lateral acceleration, add to output.
	double lateral_velocity = DATAMETRIC::GetLastInColumn("LateralVelocity");
	double last_lateral_velocity = DATAMETRIC::GetNextLastInColumn("LateralVelocity");
	double lateral_acceleration = (lateral_velocity - last_lateral_velocity) / dt;
	output_data["LateralAcceleration"] = lateral_acceleration;

	// if turning, update maximum lateral acceleration
	if (state == turning_over_threshold && lateral_acceleration > turn_max_lateral_accel)
	{
		turn_max_lateral_accel = lateral_acceleration;
	}
}
