#ifndef TESTMETRIC_H
#define TESTMETRIC_H

#include "datamanager.h"


class TESTMETRIC : public DATAMETRIC
{
	public:
		/** ctor */
		TESTMETRIC(DATAMETRIC_CTOR_PARAMS_DEF);
		/** dtor */
		~TESTMETRIC();
		/** Update the calculations, add to log, generate events */
		void Update(float dt);
	protected:
	private:
		static METRICTYPEREGISTER<TESTMETRIC> reg; //!< object registering this type for creation by name
};

#endif // TESTMETRIC_H
