#ifndef /*$metric_class*/_H
#define /*$metric_class*/_H

#include "datamanager.h"

/** This is the place where you should describe your new DATAMETRIC-
 * derived class.
 */
class /*$metric_class*/ : public DATAMETRIC
{
	public:
		/** ctor */
		/*$metric_class*/(DATAMETRIC_CTOR_PARAMS_DEF);
		/** dtor */
		~/*$metric_class*/();
		/** Update the calculations, add to log, generate events */
		void Update(float dt);
	protected:
	private:
		static METRICTYPEREGISTER</*$metric_class*/> reg; //!< object registering this type for creation by name
};

#endif // /*$metric_class*/_H

