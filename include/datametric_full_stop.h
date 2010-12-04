#ifndef FULLSTOPMETRIC_H
#define FULLSTOPMETRIC_H

#include "datamanager.h"

/** This is the place where you should describe your new DATAMETRIC-
 * derived class.
 */
class FULLSTOPMETRIC : public DATAMETRIC
{
	public:
		enum state_T {not_braking, brake_now, braking, stopped};

		/** ctor */
		FULLSTOPMETRIC(DATAMETRIC_CTOR_PARAMS_DEF);
		/** dtor */
		~FULLSTOPMETRIC();
		/** Update the calculations, add to log, generate events */
		void Update(float dt);
	protected:
	private:
		/** Figure out what state the metric is in by Brake & Velocity values */
		void DetermineState();

		state_T state; //!< current car state w.r.t. braking
		state_T last_state; //!< the state from the last update (to check for state changes)
		double braking_start_velocity; //!< the velocity of the car when it began to brake
		double braking_stimulus_time; //!< the time at which the braking stimulus was given
		double braking_start_time; //!< the time at which braking began
		static METRICTYPEREGISTER<FULLSTOPMETRIC> reg; //!< object registering this type for creation by name
};

#endif // FULLSTOPMETRIC_H

