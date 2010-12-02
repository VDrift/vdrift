#include <iostream>
#include <assert.h>
using std::cout;
using std::endl;

#include "datametric_acceleration_detector.h"

/// static class member declaration, register metric type name
METRICTYPEREGISTER<ACCELERATIONDETECTORMETRIC> ACCELERATIONDETECTORMETRIC::reg("AccelerationDetector");

ACCELERATIONDETECTORMETRIC::ACCELERATIONDETECTORMETRIC(DATAMETRIC_CTOR_PARAMS_DEF)
 : DATAMETRIC(DATAMETRIC_CTOR_PARAMS)
{
	// Enabled by default
	DATAMETRIC::run = true;
}

ACCELERATIONDETECTORMETRIC::~ACCELERATIONDETECTORMETRIC()
{
	// Clean up any class data
}

void ACCELERATIONDETECTORMETRIC::Update(float dt)
{
	// Do nothing when disabled
	if (!run)
		return;

	// Get columns out
	DATALOG::log_column_T const* time_col = DATAMETRIC::GetColumn("Time");

	// Get the most recent time
	DATALOG::log_data_T t = time_col->back();

	// Output variables go here
	output_data["Test"] = t * 2.0;
}
