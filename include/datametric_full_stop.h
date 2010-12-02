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

		void DetermineState();

		state_T state;
		state_T last_state;
		DATALOG::log_data_T braking_stimulus_time;
		DATALOG::log_data_T braking_start_time;
		static METRICTYPEREGISTER<FULLSTOPMETRIC> reg; //!< object registering this type for creation by name
};

#endif // FULLSTOPMETRIC_H

