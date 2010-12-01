#ifndef DATAMETRIC_H
#define DATAMETRIC_H

#include <string>
#include <vector>
#include <map>
#include <queue>
#include "datalog.h"
#include "config.h"

#define DATAMETRIC_CTOR_PARAMS_TYPES std::string const&, std::string const&, std::map< std::string, std::vector< DATALOG::log_data_T > const* > const&, std::vector<std::string> const&, std::vector< std::string > const&
#define DATAMETRIC_CTOR_PARAMS_DEF std::string const& nom, std::string const& desc, std::map< std::string, std::vector< DATALOG::log_data_T > const* > const& input_columns, std::vector<std::string> const& outvar_names, std::vector< std::string > const& opts
#define DATAMETRIC_CTOR_PARAMS nom, desc, input_columns, outvar_names, opts

/** Base class for metrics, defines the interface for the derived classes and
 * contains common data members. Derived classes implement their calculations
 * in Update. */
class DATAMETRIC
{
	public:
		typedef std::map< std::string, DATALOG::log_column_T const* > metric_input_map_T;
		typedef std::map< std::string, DATALOG::log_data_T > metric_output_map_T;

		/** ctor */
		DATAMETRIC(DATAMETRIC_CTOR_PARAMS_DEF);
		/** virtual dtor */
		virtual ~DATAMETRIC() {}
		/** Access names of keys for output_data
		 * \return a vector of the key values naming the output data variables
		 */
		std::vector< std::string > const& GetOutputVariableNames() const { return output_variable_names; }
		/** Access output_data by key
		 * \param var_name key name of the variable to retrieve
		 * \param output_variable a reference to a spot to put the current value of output_data for the given var_name
		 * \return true if var_name was found, false otherwise
		 */
		bool GetOutputVariable(std::string const& var_name, DATALOG::log_data_T & output_variable) const;
		/** Update the metric's calculations (must be overridden) */
		virtual void Update(float dt) = 0;
	protected:
		/** Get a pointer to a named data column
		 * \param column_name the name of the column to get
		 * \return a pointer to the data column with the given name
		 */
		DATALOG::log_column_T const* GetColumn(std::string const& column_name) const;

		bool run; //!< whether to perform calculations or not
		std::string name; //!< user-defined name for the metric
		std::string description; //!< user-defined text for describing metric
		metric_input_map_T input_data_columns; //!< pointers to input data columns
		std::vector< std::string > output_variable_names; //!< names of the variables in the output data
		metric_output_map_T output_data; //!< output values (output_variable_names contains keys)
		std::vector< std::string > options; //!< the options set for this metric
	private:
};

/** CreateTypedMetric - create a new DATAMETRIC-derived instance on the heap
 * and return a DATAMETRIC* pointer to it. references to this function are
 * stored in the metric_types map of TYPEDMETRICFACTORY by the constructor of
 * METRICTYPEREGISTER. they are later called when instances of the type are
 * created by calling CreateInstance in TYPEDMETRICFACTORY.
 */
template<typename T> DATAMETRIC* CreateTypedMetric(DATAMETRIC_CTOR_PARAMS_DEF)
{
	return new T(DATAMETRIC_CTOR_PARAMS);
}

/** Create DATAMETRIC-derived instances by a named metric-type. this class
 * uses the singleton and factory patterns to maintain a map from metric type
 * name to metric type class. this is the static member metric_types, which is
 * a pointer to a metric_ctor_map_T.
 */
class TYPEDMETRICFACTORY
{
	public:
		typedef std::map< std::string, DATAMETRIC*(*)(DATAMETRIC_CTOR_PARAMS_TYPES) > metric_ctor_map_T;

		/** create a new DATAMETRIC-derived object
		 * \param type_name the identifier-name of the type of object to create
		 * \param DATAMETRIC_CTOR_PARAMS parameters for object construction
		 * \return new instance of named metric-type as DATAMETRIC* pointer
		 */
		static DATAMETRIC* CreateInstance(std::string const& type_name, DATAMETRIC_CTOR_PARAMS_DEF)
		{
			metric_ctor_map_T::iterator metric_type = GetMetricTypes()->find(type_name);
			if (metric_type == GetMetricTypes()->end())
			{
				return 0;
			}
			/// this calls CreateTypedMetric for the type specified at registration
			return metric_type->second(DATAMETRIC_CTOR_PARAMS);
		}
		/** Check to see if a type name is a valid registered type
		 * \param type_name name of the type to check for
		 * \return true if named type is valid, false otherwise
		 */
		static bool IsValidType(std::string const& type_name)
		{
			return (GetMetricTypes()->find(type_name) != GetMetricTypes()->end());
		}
	protected:
		/** access the types map - make sure only one instance of metric_types
		 * exists. */
		static metric_ctor_map_T * GetMetricTypes()
		{
			if (!metric_types)
			{
				metric_types = new metric_ctor_map_T;
			}
			return metric_types;
		}
	private:
		static metric_ctor_map_T * metric_types; //!< map of metric-type names to functors for creating a metric object of named type
};

/** register DATAMETRIC-derived classes as named metric types. this class
 * handles type registration for metric types made available through
 * TYPEDMETRICFACTORY. inherits from the same for direct access to its map
 * of metric types.
 */
template<typename T>
class METRICTYPEREGISTER : TYPEDMETRICFACTORY
{
	public:
		/** constructor registers type T as a given metric-type name
		 * \param tname name for metric type T being registered
		 */
		METRICTYPEREGISTER(std::string const& tname) : type_name(tname)
		{
			GetMetricTypes()->insert(std::make_pair(tname, &CreateTypedMetric<T>));
		}
		/** access metric type name
		 * \return name of the metric type used to register type T
		 */
		std::string const& GetMetricTypeName()
		{
			return type_name;
		}
	private:
		std::string type_name; //!< name of the metric type used to register type T
};

/** This class defines an event which can be generated by a metric. Objects
 * of type METRICEVENT go into DATAMANAGER's queue and can subsequently be
 * handled by an outside entity (i.e. the owner of the DATAMANAGER).
 */
class METRICEVENT
{
	public:
		/** constructor */
		METRICEVENT(std::string t="", std::string d="") : event_type(t), event_data(d) {}
		/** Access event type */
		std::string const& GetType() const { return event_type; }
		/** Access event data */
		std::string const& GetData() const { return event_data; }
	protected:
	private:
		std::string event_type; //!< a name for the kind of event this is
		std::string event_data; //!< some data about the event
};

/** This class handles all the DATAMETRIC objects. It creates them, updates
 * them, and destroys them. It also manages communications between DATAMETRIC
 * objects and outside entities by providing an event queue. It also holds the
 * DATALOG object and handles updating it.
 */
class DATAMANAGER
{
	public:
		typedef std::map< std::string, DATAMETRIC* > metric_map_T;
		typedef std::queue< METRICEVENT > event_queue_T;

		/** ctor: Disable until Init is run */
		DATAMANAGER() : enabled(false), update_frequency(0.01), time_since_update(0.0), next_log_entry(NULL) {}
		/** dtor */
		~DATAMANAGER() { if (enabled) Clear(); }
		/** Create the metrics and populate the map
		 * \param data_config_filename name of file to load data log/metric settings from
		 */
		void Init(std::string const& data_config_filename, std::string const& log_path);
		/** Write the data log */
		void WriteLog() { data_log.Write(); }
		/** Delete all the metrics */
		void Clear();
		/** This function must be run to prepare for running Update.
		 * This is a hack until a better solution is found to get rid of the
		 * function GAME::QueryLogData.
		 * \param new_log_entry a pointer to a new entry for the log.
		 */
		void SetNextLogEntry(DATALOG::log_entry_T * new_log_entry) { next_log_entry = new_log_entry; }
		/** Update the log, metrics, and event queue
		 * \param dt the amount of seconds passed since last update
		 */
		void Update(float dt);
		/** Check for queued events, pop and return one if there are any.
		 * \return next event in the queue, if any
		 */
		bool PollEvents(METRICEVENT & event);
		/** Is the data manager enabled? */
		bool IsEnabled() { return enabled; }
		/** Has enough time passed since the last update? */
		bool NeedsUpdate() { return time_since_update >= update_frequency; }
		/** Access to the column names in data_log. Only needed to support
		 * GAME::QueryLogData, can be removed with that function.
		 * \return column names
		 */
		std::vector< std::string > const& GetLogColumnNames() { return data_log.GetColumnNames(); }
	protected:
	private:
		DATALOG data_log; //!< store the data to be logged
		metric_map_T data_metrics; //!< map of pointers to DATAMETRIC-derived objects
		event_queue_T events; //!< event queue for METRICEVENT
		bool enabled; //!< only true when it's ok to update (post-Init())
		float update_frequency; //!< how long (in seconds) to wait between each update
		float time_since_update; //!< how long (in seconds) it has been since the last update
		DATALOG::log_entry_T * next_log_entry; //!< a pointer to the log data for the next entry
};

#endif // DATAMETRIC_H
