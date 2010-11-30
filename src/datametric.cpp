#include <assert.h>
#include <algorithm>
#include "datametric.h"

using std::string;
using std::map;
using std::vector;
using std::find;
using std::sort;
using std::set_intersection;

#include <iostream>
using std::cout;
using std::endl;

/** static class member declaration */
TYPEDMETRICFACTORY::metric_map_T * TYPEDMETRICFACTORY::metric_types;

void DATAMANAGER::Init(std::string const& data_config_filename, std::string const& log_path)
{
	CONFIG data_settings(data_config_filename);
	data_settings.GetParam("datalog", "enabled", enabled);
	if (!enabled)
		return;

	/// Set up the datalog
	float update_frequency_Hz = 60.0;
	vector<string> log_column_names;
	string log_name("unnamed_datalog");
	string log_format("csv");
	data_settings.GetParam("datalog", "frequency", update_frequency_Hz);
	data_settings.GetParam("datalog", "columns", log_column_names);
	data_settings.GetParam("datalog", "name", log_name);
	data_settings.GetParam("datalog", "format", log_format);
	assert(find(log_column_names.begin(), log_column_names.end(), "Time") != log_column_names.end());
	update_frequency = 1.0 / update_frequency_Hz;
	data_log.Init(log_path, log_name, log_column_names, log_format);

	vector<string> metric_names;
	data_settings.GetParam("datametrics", "load_order", metric_names);
	vector<string>::const_iterator metric_name;
	for (metric_name = metric_names.begin(); metric_name != metric_names.end(); ++metric_name)
	{
		// collect up all the settings for this metric from the config file
		string metric_section_name("datametric." + *metric_name);
		vector<string> metric_required_columns;
		vector<string> metric_output_vars;
		vector<string> metric_options;
		string metric_type("undefined type");
		string metric_description("empty description");
		data_settings.GetParam(metric_section_name, "required_columns", metric_required_columns);
		data_settings.GetParam(metric_section_name, "output_vars", metric_output_vars);
		data_settings.GetParam(metric_section_name, "options", metric_options);
		data_settings.GetParam(metric_section_name, "type", metric_type);
		data_settings.GetParam(metric_section_name, "description", metric_description);
		assert(metric_type != "undefined type");

		vector<string>::const_iterator col_name;
		map< string, vector<double> const* > input_data_columns;
		for (col_name = metric_required_columns.begin(); col_name != metric_required_columns.end(); ++col_name)
		{
			// the following assert makes sure the requested column name is actually in the data log
			assert (find(log_column_names.begin(), log_column_names.end(), *col_name) != log_column_names.end());
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

void DATAMANAGER::Clear()
{
	// write out the data log, if necessary
	data_log.Write();

	// clean up the metrics
	map< string, DATAMETRIC* >::iterator metric_iter;
	for (metric_iter = data_metrics.begin(); metric_iter != data_metrics.end(); ++metric_iter)
	{
		assert(metric_iter->second != NULL);
		delete metric_iter->second;
		metric_iter->second = NULL;
	}

	// turn it off until next Init()
	enabled = false;
}

void DATAMANAGER::Update(float dt)
{
	bool must_return = false;
	if (!NeedsUpdate())
		must_return = true;

	time_since_update += dt;

	if (must_return)
		return;

	cout << "time since update " << time_since_update << endl;

	cout << "add next log entry" << endl;

	// the next log entry had better be valid, so it can be added.
	assert(next_log_entry != NULL);
	data_log.AddEntry(next_log_entry);
	next_log_entry = NULL;

	cout << "visit all metrics" << endl;

	// iterate over all metrics in the map
	for (metric_map_T::iterator metric = data_metrics.begin(); metric != data_metrics.end(); ++metric)
	{
		cout << "  metric " << metric->first << " update" << endl;
		// run the update function for the metric
		metric->second->Update(dt);

		cout << "  metric " << metric->first << " datalog feedback" << endl;
		// update the data log with the new values from the metric's output variables
		for (vector<string>::const_iterator output_var = metric->second->GetOutputVariableNames().begin();
		     output_var != metric->second->GetOutputVariableNames().end(); ++output_var)
		{
			cout << "    output var " << *output_var << " value " << metric->second->GetOutputVariable(*output_var) << endl;
			// add the output data to the datalog
			data_log.ModifyLastEntry(*output_var, metric->second->GetOutputVariable(*output_var));
		}
	}

	// consume the rest of the time difference
	while (NeedsUpdate())
	{
		time_since_update -= update_frequency;
	}
}

bool DATAMANAGER::PollEvents(METRICEVENT & event)
{
	if (events.empty())
	{
		return false;
	}
	event = events.front();
	events.pop();
	return true;
}
