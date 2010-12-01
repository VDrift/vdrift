#include <iostream>
#include <assert.h>
using std::cout;
using std::endl;

#include "datametric_test.h"

/// static class member declaration, register metric type name
METRICTYPEREGISTER<TESTMETRIC> TESTMETRIC::reg("Test");

TESTMETRIC::TESTMETRIC(DATAMETRIC_CTOR_PARAMS_DEF)
 : DATAMETRIC(DATAMETRIC_CTOR_PARAMS)
{
	DATAMETRIC::run = true;
}

TESTMETRIC::~TESTMETRIC()
{
	//dtor
}

void TESTMETRIC::Update(float dt)
{
	//cout << "Updating metric dt=" << dt << endl;
	if (!run)
		return;

	//cout << "Getting Velocity column" << endl;
	DATALOG::log_column_T const* velocity_col = DATAMETRIC::GetColumn("Velocity");


	//cout << "Velocity column size: " << velocity_col->size() << endl;
	//cout << "Getting Velocity value" << endl;
	DATALOG::log_data_T velocity = velocity_col->back();

	//cout << "Got Velocity=" << velocity << ", setting output Test to Velocity*2" << endl;
	output_data["Test"] = velocity * 2.0;
	//cout << "The result is " << output_data["Test"] << ". All done, g'bye" << endl;
}
