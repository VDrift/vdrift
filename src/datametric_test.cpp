#include <iostream>
#include <assert.h>
using std::cout;
using std::endl;

#include "datametric_test.h"

/// static class member declaration, register metric type name
METRICTYPEREGISTER<TESTMETRIC> TESTMETRIC::reg("Test");

TESTMETRIC::TESTMETRIC(DATAMETRIC_CTOR_PARAMS_DEF)
{
	DATAMETRIC::run = true;
	DATAMETRIC::input_data_columns = input_columns;
	DATAMETRIC::output_variable_names = outvar_names;
	DATAMETRIC::options = opts;
	DATAMETRIC::description = desc;
}

TESTMETRIC::~TESTMETRIC()
{
	//dtor
}

void TESTMETRIC::Update(float dt)
{
	cout << "Updating metric dt=" << dt << endl;
	if (!run)
		return;

	cout << "Getting Velocity column" << endl;
	assert(DATAMETRIC::input_data_columns.find("Velocity") != DATAMETRIC::input_data_columns.end());
	DATALOG::log_column_T const* velocity_col = DATAMETRIC::input_data_columns["Velocity"];
	cout << "velocity column size: " << velocity_col->size() << endl;
	cout << "Getting Velocity value" << endl;
	double velocity = velocity_col->back();

	cout << "Got velocity=" << velocity << ", setting output Test to velocity*2" << endl;
	output_data["Test"] = velocity * 2.0;
}
