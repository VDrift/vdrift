#ifndef TESTMETRIC_H
#define TESTMETRIC_H

#include "datamanager.h"


class TESTMETRIC : public DATAMETRIC
{
	public:
		/** Default constructor */
		TESTMETRIC(DATAMETRIC_CTOR_PARAMS_DEF);
		/** Default destructor */
		~TESTMETRIC();
		void Update(float dt);
	protected:
	private:
		static METRICTYPEREGISTER<TESTMETRIC> reg; //!< Member variable "reg"
};

#endif // TESTMETRIC_H
