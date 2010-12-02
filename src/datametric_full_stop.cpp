#include <iostream>
#include <sstream>
#include <assert.h>
using std::ostringstream;
using std::cout;
using std::endl;

#include "datametric_full_stop.h"

/// static class member declaration, register metric type name
METRICTYPEREGISTER<FULLSTOPMETRIC> FULLSTOPMETRIC::reg("FullStop");

FULLSTOPMETRIC::FULLSTOPMETRIC(DATAMETRIC_CTOR_PARAMS_DEF)
 : DATAMETRIC(DATAMETRIC_CTOR_PARAMS),
   state(stopped),
   last_state(stopped),
   braking_start_time(-1.0)
{
	// Enabled by default
	DATAMETRIC::run = true;
}

FULLSTOPMETRIC::~FULLSTOPMETRIC()
{
	// Clean up any class data
}

void FULLSTOPMETRIC::DetermineState()
{
	if (state == not_braking)
	{
		if (DATAMETRIC::GetLastInColumn("Velocity") * 2.2369 > 60.0)
			state = brake_now;
	}
	if (state == brake_now)
	{
		if (DATAMETRIC::GetLastInColumn("Brake") > 0.0)
			state = braking;
	}
	else if (state == braking)
	{
		if (DATAMETRIC::GetLastInColumn("Velocity") <= 0.5)
			state = stopped;
		else if (DATAMETRIC::GetLastInColumn("Brake") == 0.0)
			state = not_braking;
	}
	else if (state == stopped)
	{
		if (DATAMETRIC::GetLastInColumn("Velocity") > 0.5)
		{
			if (DATAMETRIC::GetLastInColumn("Brake") > 0.0)
			{
				state = braking;
			}
			else
			{
				state = not_braking;
			}
		}
	}
}

void FULLSTOPMETRIC::Update(float dt)
{
	// Do nothing when disabled
	if (!run)
		return;

	DetermineState();

	if (state != last_state)
	{
		//cout << "state change" << endl;

		if (state == brake_now)
		{
			braking_stimulus_time = DATAMETRIC::GetLastInColumn("Time");
			//cout << "braking stimulus at " << braking_stimulus_time << endl;
			METRICEVENT::event_data_T event_data;
			event_data["Message"] = "STOP";
			DATAMETRIC::SetEvent("DriverFeedbackMessage", event_data);
		}
		else if (state == braking)
		{
			braking_start_time = DATAMETRIC::GetLastInColumn("Time");
			//cout << "started braking at " << braking_start_time << endl;
		}
		else if (state == stopped)
		{
			DATALOG::log_data_T current_time = DATAMETRIC::GetLastInColumn("Time");
			DATALOG::log_data_T stopping_time = current_time - braking_start_time;
			DATALOG::log_data_T reaction_time = braking_start_time - braking_stimulus_time;
			//cout << "stopped braking at " << current_time << ", stopping time: " << stopping_time << ", delay: " << reaction_time << endl;
			ostringstream os;
			os << "You stopped in " << stopping_time << " seconds" << endl;
			os << "after a " << reaction_time << " second delay." << endl;
			METRICEVENT::event_data_T event_data;
			event_data["Message"] = os.str();
			DATAMETRIC::SetEvent("DriverFeedbackMessage", event_data);
		}
		else if (state == not_braking)
		{
			//cout << "not braking" << endl;
			braking_start_time = -1.0;
		}
	}

	last_state = state;
}
