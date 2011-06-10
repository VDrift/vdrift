#include <assert.h>
#include <algorithm>
#include "datamanager.h"

using std::string;
using std::vector;
using std::find;
using std::sort;
using std::set_intersection;

#include <iostream>
using std::cout;
using std::endl;

/** static class member declaration */
TYPEDMETRICFACTORY::metric_ctor_map_T * TYPEDMETRICFACTORY::metric_types;

std::string METRICEVENT::GetType() const
{
	string type;
	assert (event_config.GetParam("event", "type", type));
	return type;
}

std::string METRICEVENT::GetName() const
{
	string name;
	assert (event_config.GetParam("event", "name", name));
	return name;
}

DATAMETRIC::DATAMETRIC(DATAMETRIC_CTOR_PARAMS_DEF)
 : run(false),
   name(nom),
   description(desc),
   input_data_columns(input_columns),
   output_variable_names(outvar_names),
   options(opts),
   has_event(false)
{

}

bool DATAMETRIC::GetOutputVariable(std::string const& var_name, DATALOG::log_data_T & output_variable) const
{
	metric_output_map_T::const_iterator outvar(output_data.find(var_name));
	if (outvar != output_data.end())
	{
		output_variable = outvar->second;
		return true;
	}
	return false;
}

void DATAMETRIC::SetEvent(METRICEVENT::event_data_T new_event_config)
{
	has_event = true;
	event_config = new_event_config;
}

void DATAMETRIC::SetFeedbackMessageEvent(std::string const& name, std::string const& message, float duration, float fadein, float fadeout)
{
	METRICEVENT::event_data_T event_config;
	event_config.SetParam("event", "name", name);
	event_config.SetParam("event", "type", "DriverFeedbackMessage");
	event_config.SetParam("feedback", "message", message);
	event_config.SetParam("feedback", "duration", duration);
	event_config.SetParam("feedback", "fade-in", fadein);
	event_config.SetParam("feedback", "fade-out", fadeout);
	SetEvent(event_config);
}

bool DATAMETRIC::GetEvent(METRICEVENT::event_data_T & config)
{
	if (has_event)
	{
		config = event_config;
		has_event = false;
		event_config.Clear();
		return true;
	}
	return false;
}

DATALOG::log_column_T const* DATAMETRIC::GetColumn(std::string const& column_name) const
{
	metric_input_map_T::const_iterator column;
	column = input_data_columns.find(column_name);
	assert(column != input_data_columns.end());
	assert(column->second != NULL);
	return column->second;
}

DATALOG::log_data_T DATAMETRIC::GetLastInColumn(std::string const& column_name) const
{
	return GetColumn(column_name)->back();
}

DATALOG::log_data_T DATAMETRIC::GetNextLastInColumn(std::string const& column_name) const
{
	DATALOG::log_column_T const* column = GetColumn(column_name);
	DATALOG::log_column_T::const_reverse_iterator next_last = column->rbegin();
	++next_last;
	if (next_last != column->rend())
	{
		return *next_last;
	}
	return -1.0;
}

void DATAMANAGER::Init(std::string const& data_config_filename, std::string const& log_path)
{
	CONFIG data_settings(data_config_filename);
	data_settings.GetParam("datalog", "enabled", enabled);
	if (!enabled)
		return;

	//cout << "DataManager enabled" << endl;
	/// Set up the datalog
	float update_frequency_Hz = 60.0;
	vector<string> log_column_names;
	string log_name("unnamed_datalog");
	string log_format("csv");

	data_settings.GetParam("datalog", "frequency", update_frequency_Hz);
	update_frequency = 1.0 / update_frequency_Hz;
	//cout << "Update frequency: " << update_frequency << endl;

	bool required_settings = true;
	required_settings &= data_settings.GetParam("datalog", "columns", log_column_names);
	required_settings &= data_settings.GetParam("datalog", "name", log_name);
	assert(required_settings);
	//cout << "Got required settings" << endl;

	data_settings.GetParam("datalog", "format", log_format);
	assert(find(log_column_names.begin(), log_column_names.end(), "Time") != log_column_names.end());
	//cout << "Time column is present" << endl;

	//cout << "init data log" << endl;
	data_log.Init(log_column_names, log_path, log_name, log_format);
	//cout << "data log init complete" << endl;

	vector<string> metric_names;
	assert(data_settings.GetParam("datametrics", "load_order", metric_names));

	for (vector<string>::const_iterator metric_name = metric_names.begin(); metric_name != metric_names.end(); ++metric_name)
	{
		// collect up all the settings for this metric from the config file
		string metric_section_name("datametric." + *metric_name);
		string metric_type("undefined type");
		string metric_description("empty description");
		vector<string> metric_required_columns;
		vector<string> metric_output_vars;
		vector<string> metric_options;

		//cout << "Setting up metric '" << *metric_name << "'" << endl;
		required_settings = true;
		required_settings &= data_settings.GetParam(metric_section_name, "required_columns", metric_required_columns);
		required_settings &= data_settings.GetParam(metric_section_name, "type", metric_type);
		assert(required_settings);
		assert(TYPEDMETRICFACTORY::IsValidType(metric_type));
		//cout << "got required settings and metric type '" << metric_type << "' is valid" << endl;

		data_settings.GetParam(metric_section_name, "options", metric_options);
		data_settings.GetParam(metric_section_name, "description", metric_description);
		data_settings.GetParam(metric_section_name, "output_vars", metric_output_vars);

		//cout << "Creating " << metric_required_columns.size() << " input_data_columns" << endl;
		// create map of pointers to log columns.
		vector<string>::const_iterator col_name;
		DATAMETRIC::metric_input_map_T input_data_columns;
		for (col_name = metric_required_columns.begin(); col_name != metric_required_columns.end(); ++col_name)
		{
			//cout << "making sure we have a log column named " << *col_name << endl;
			// the following assert makes sure the requested column name is actually in the data log
			assert (find(log_column_names.begin(), log_column_names.end(), *col_name) != log_column_names.end());
			//cout << "trying to get column named " << *col_name << endl;
			DATALOG::log_column_T const* column = data_log.GetColumn(*col_name);
			//cout << "Found column pointer value " << column << endl;
			input_data_columns[*col_name] = column;
			//cout << "Set input data column '" << *col_name << "' to " << input_data_columns[*col_name] << endl;
		}

		//cout << "Done creating input data columns, creating new instance" << endl;
		// look up the type of the metric by name and get an instance of it
		DATAMETRIC * new_data_metric = TYPEDMETRICFACTORY::CreateInstance(metric_type, *metric_name, metric_description, input_data_columns, metric_output_vars, metric_options);
		assert (new_data_metric != NULL);
		//cout << "Created new data metric at " << new_data_metric << endl;
		data_metrics[*metric_name] = new_data_metric;
		//cout << "Mapped new data metric named '" << *metric_name << "', value " << data_metrics[*metric_name] << endl;
	}
}

void DATAMANAGER::Clear()
{
	//cout << "Clearing Datamanager" << endl;
	// clean up the metrics
	for (metric_map_T::iterator metric_iter = data_metrics.begin(); metric_iter != data_metrics.end(); ++metric_iter)
	{
		//cout << "Metric '" << metric_iter->first << "' at " << metric_iter->second << endl;
		assert(metric_iter->second != NULL);
		delete metric_iter->second;
		metric_iter->second = NULL;
	}
	data_metrics.clear();

	// turn it off until next Init()
	enabled = false;
	//cout << "All clear" << endl;
}

using std::stringstream;

void DATAMANAGER::HandleNewEvent(METRICEVENT::event_data_T config)
{
	// handle special events that do not need to be added to the queue for
	// external processing
	string type;
	assert (config.GetParam("event", "type", type));
	if (type == "DisableMetrics")
	{
		// this event type calls Disable() on each metric in a list of metric names
		vector<string> metric_names;
		assert (config.GetParam("disable", "metrics", metric_names));
		for (vector<string>::const_iterator metric_name = metric_names.begin(); metric_name != metric_names.end(); ++metric_name)
		{
			metric_map_T::iterator metric = data_metrics.find(*metric_name);
			assert (metric != data_metrics.end());
			metric->second->Disable();
		}
	}
	if (type == "EnableMetrics")
	{
		// this event type calls Enable() on each metric in a list of metric names
		vector<string> metric_names;
		assert (config.GetParam("enable", "metrics", metric_names));
		for (vector<string>::const_iterator metric_name = metric_names.begin(); metric_name != metric_names.end(); ++metric_name)
		{
			metric_map_T::iterator metric = data_metrics.find(*metric_name);
			assert (metric != data_metrics.end());
			metric->second->Enable();
		}
	}
	else
	{
		// all other events are enqueued for external processing
		events.push(METRICEVENT(config));
	}
}

void DATAMANAGER::Update(float dt)
{
	bool must_return = false;
	if (!NeedsUpdate())
		must_return = true;
	if (dt == 0.0)
		must_return = true;

	time_since_update += dt;

	if (must_return)
		return;

	//cout << "time since update " << time_since_update << endl;

	//cout << "add next log entry" << endl;

	// the next log entry had better be valid, so it can be added.
	assert(next_log_entry != NULL);
	data_log.AddEntry(next_log_entry);
	next_log_entry = NULL;

	//cout << "visit all metrics" << endl;

	// iterate over all metrics in the map
	for (metric_map_T::iterator metric = data_metrics.begin(); metric != data_metrics.end(); ++metric)
	{
		//cout << "  metric " << metric->first << " update" << endl;
		// run the update function for the metric
		metric->second->Update(dt);

		//cout << "  metric " << metric->first << " datalog feedback" << endl;
		// update the data log with the new values from the metric's output variables
		for (vector<string>::const_iterator output_var = metric->second->GetOutputVariableNames().begin();
		     output_var != metric->second->GetOutputVariableNames().end(); ++output_var)
		{
			DATALOG::log_data_T val;
			bool get_output_value_success = metric->second->GetOutputVariable(*output_var, val);
			assert(get_output_value_success);
			//cout << "    output var " << *output_var << " value " << val << endl;
			// add the output data to the datalog
			data_log.ModifyLastEntry(*output_var, val);
		}

		// check metric for a new event and process it
		METRICEVENT::event_data_T event_config;
		if (metric->second->GetEvent(event_config))
		{
			HandleNewEvent(event_config);
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
