#include <algorithm>
#include "datametric.h"

using std::string;
using std::map;
using std::vector;
using std::find;
using std::sort;
using std::set_intersection;

/** static class member declaration */
TYPEDMETRICFACTORY::metric_map_T * TYPEDMETRICFACTORY::metric_types;

METRICMANAGER::~METRICMANAGER()
{
	// clean up the metrics
	map< string, DATAMETRIC* >::iterator metric_iter;
	for (metric_iter = data_metrics.begin(); metric_iter != data_metrics.end(); ++metric_iter)
	{
		//cout << "cleaning up metric " << metric_iter->first << endl;
		delete metric_iter->second;
		metric_iter->second = NULL;
	}
}

void METRICMANAGER::Init(CONFIGFILE const& data_settings, DATALOG const& data_log)
{
	//data_log_column_names = data_log.GetColumnNames();

	vector<string> metric_names_to_load;
	data_settings.GetParam("datametrics.load_metrics", metric_names_to_load);
	vector<string>::const_iterator metric_name;
	for (metric_name = metric_names_to_load.begin(); metric_name != metric_names_to_load.end(); ++metric_name)
	{
		// collect up all the settings for this metric from the config file
		string metric_section_name("datametric." + *metric_name);
		vector<string> metric_required_columns;
		vector<string> metric_output_vars;
		vector<string> metric_options;
		string metric_type("undefined type");
		string metric_description("empty description");
		data_settings.GetParam(metric_section_name + ".required_columns", metric_required_columns);
		data_settings.GetParam(metric_section_name + ".output_vars", metric_output_vars);
		data_settings.GetParam(metric_section_name + ".options", metric_options);
		data_settings.GetParam(metric_section_name + ".type", metric_type);
		data_settings.GetParam(metric_section_name + ".description", metric_description);
		assert(metric_type != "undefined type");

		vector<string>::const_iterator col_name;
		map< string, vector<double> const* > input_data_columns;
		for (col_name = metric_required_columns.begin(); col_name != metric_required_columns.end(); ++col_name)
		{
			// the following assert makes sure the requested column name is actually in the data log
			assert (find(data_log.GetColumnNames().begin(), data_log.GetColumnNames().end(), *col_name) != data_log.GetColumnNames().end());
			vector< double > * column = NULL;
			string column_name = *col_name;
			if (data_log.GetColumn(column_name, column))
			{
				input_data_columns[*col_name] = column;
			}
		}

		// look up the type of the metric by name and get an instance of it
		DATAMETRIC * new_data_metric = TYPEDMETRICFACTORY::CreateInstance(metric_type, input_data_columns, metric_output_vars, metric_options, metric_description);
		assert (new_data_metric != 0);
		data_metrics[*metric_name] = new_data_metric;
	}
}

void METRICMANAGER::Update(float dt)
{
	map< string, DATAMETRIC* >::iterator metrics_iter;
	vector<string> metric_outvars;

	// iterate over all metrics in the map
	for (metrics_iter = data_metrics.begin(); metrics_iter != data_metrics.end(); ++metrics_iter)
	{
		// run the update function for the metric
		metrics_iter->second->Update(dt);

		/* we don't have a way to access data_log... where should this stuff go? GAME?
		// update the data log with the new values from the metric's output variables
		metric_outvars = metrics_iter->second->GetOutputVariableNames();
		vector<string>::const_iterator outvar_iter;
		// for each of the output variables
		for (outvar_iter = metric_outvars.begin(); outvar_iter != metric_outvars.end(); ++outvar_iter)
		{
			// add the output data to the datalog
			double val = metrics_iter->second->GetOutputVariable(*outvar_iter);
			data_log.ModifyLastEntry(*outvar_iter, val);
		}
		*/
	}
}

bool METRICMANAGER::PollEvents(METRICEVENT & event)
{
	if (events.empty())
	{
		return false;
	}
	event = events.front();
	events.pop();
	return true;
}
