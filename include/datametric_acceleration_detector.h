#ifndef ACCELERATIONDETECTORMETRIC_H
#define ACCELERATIONDETECTORMETRIC_H

#include "datamanager.h"

/** This is the place where you should describe your new DATAMETRIC-
 * derived class.
 */
class ACCELERATIONDETECTORMETRIC : public DATAMETRIC
{
	public:
		/** ctor */
		ACCELERATIONDETECTORMETRIC(DATAMETRIC_CTOR_PARAMS_DEF);
		/** dtor */
		~ACCELERATIONDETECTORMETRIC();
		/** Update the calculations, add to log, generate events */
		void Update(float dt);
	protected:
	private:
		static METRICTYPEREGISTER<ACCELERATIONDETECTORMETRIC> reg; //!< object registering this type for creation by name
};

#endif // ACCELERATIONDETECTORMETRIC_H

