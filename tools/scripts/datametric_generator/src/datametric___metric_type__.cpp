#include <iostream>
#include <assert.h>
using std::cout;
using std::endl;

#include /*$metric_include*/

/// static class member declaration, register metric type name
METRICTYPEREGISTER</*$metric_class*/> /*$metric_class*/::reg(/*$metric_type_name*/);

/*$metric_class*/::/*$metric_class*/(DATAMETRIC_CTOR_PARAMS_DEF)
 : DATAMETRIC(DATAMETRIC_CTOR_PARAMS)
{
	// Enabled by default
	DATAMETRIC::run = true;
}

/*$metric_class*/::~/*$metric_class*/()
{
	// Clean up any class data
}

void /*$metric_class*/::Update(float dt)
{
	// Do nothing when disabled
	if (!run)
		return;

	// Get columns out
	DATALOG::log_column_T const* time_col = DATAMETRIC::GetColumn("Time");

	// Get the most recent time
	DATALOG::log_data_T t = time_col->back();

	// Output variables go here
	output_data[/*$metric_output_var*/] = t * 2.0;
}
