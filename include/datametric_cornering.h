#ifndef CORNERINGMETRIC_H
#define CORNERINGMETRIC_H

#include "datamanager.h"

/** This is the place where you should describe your new DATAMETRIC-
 * derived class.
 */
class CORNERINGMETRIC : public DATAMETRIC
{
	public:
		enum state_T {not_turning, turning_under_threshold, turning_over_threshold};

		/** ctor */
		CORNERINGMETRIC(DATAMETRIC_CTOR_PARAMS_DEF);
		/** dtor */
		~CORNERINGMETRIC();
		/** Update the calculations, add to log, generate events */
		void Update(float dt);
	protected:
	private:
		/** Figure out what state the metric is in by Steering values */
		void DetermineState();
		/** React to a new state when the state changes */
		void StateChangeReaction();

		state_T state; //!< current car state w.r.t. cornering
		state_T last_state; //!< the state from the last update (to check for state changes)
		double turn_start_time; //!< time when the driver began turning
		double turn_max_lateral_accel; //!< the maximum sideways force on the car during a turn
		static METRICTYPEREGISTER<CORNERINGMETRIC> reg; //!< object registering this type for creation by name
};

#endif // CORNERINGMETRIC_H

