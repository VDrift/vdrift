#ifndef DATAMETRIC_H
#define DATAMETRIC_H

#include <string>
#include <vector>
#include <map>
#include <queue>
#include "datalog.h"
#include "configfile.h"

#define DATAMETRIC_CTOR_PARAMS_TYPES std::map< std::string, std::vector< double > const* > &, std::vector<std::string> &, std::vector< std::string > &, std::string &
#define DATAMETRIC_CTOR_PARAMS_DEF std::map< std::string, std::vector< double > const* > & input_columns, std::vector<std::string> & outvar_names, std::vector< std::string > & opts, std::string & desc
#define DATAMETRIC_CTOR_PARAMS input_columns, outvar_names, opts, desc

/** Base class for metrics, defines the interface for the derived classes and
 * contains common data members. Derived classes implement their calculations
 * in Update. */
class DATAMETRIC
{
	public:
		/** Default constructor */
		DATAMETRIC() : run(false) {}
		/** Default destructor */
		virtual ~DATAMETRIC() {}
		/** Copy constructor */
		DATAMETRIC(DATAMETRIC const& other)
		{
			*this = other;
		}
		/** Copy operator */
		DATAMETRIC& operator=(DATAMETRIC const& other)
		{
			input_data_columns = other.input_data_columns;
			output_variable_names = other.output_variable_names;
			output_data = other.output_data;
			return *this;
		}
		/** Access names of keys for output_data
		 * \return a vector of the key values naming the output data variables
		 */
		std::vector< std::string > const& GetOutputVariableNames() const { return output_variable_names; };
		/** Access output_data by key
		 * \param var_name Key name of the variable to retrieve
		 * \return the current value of output_data for the given var_name
		 */
		double const& GetOutputVariable(std::string const& var_name) { return output_data[var_name]; }
		/** Update the metric's calculations
		 * pure virtual function - derived classes must override this
		 */
		virtual void Update(float dt) = 0;
	protected:
		bool run; //!< whether to perform calculations or not
		std::map< std::string, std::vector< double > const* > input_data_columns; //!< pointers to input data columns
		std::vector< std::string > output_variable_names; //!< names of the variables in the output data
		std::map< std::string, double > output_data; //!< output values (output_variable_names contains keys)
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
 * a pointer to a metric_map_T.
 */
struct TYPEDMETRICFACTORY
{
	public:
		typedef std::map< std::string, DATAMETRIC*(*)(DATAMETRIC_CTOR_PARAMS_TYPES) > metric_map_T;

		/** create a new DATAMETRIC-derived object
		 * \param type_name the identifier-name of the type of object to create
		 * \param DATAMETRIC_CTOR_PARAMS parameters for object construction
		 * \return new instance of named metric-type as DATAMETRIC* pointer
		 */
		static DATAMETRIC* CreateInstance(std::string & type_name, DATAMETRIC_CTOR_PARAMS_DEF)
		{
			metric_map_T::iterator metric_type = GetMetricTypes()->find(type_name);
			if (metric_type == GetMetricTypes()->end())
			{
				return 0;
			}
			/// this calls CreateTypedMetric for the type specified at registration
			return metric_type->second(DATAMETRIC_CTOR_PARAMS);
		}
	protected:
		/** access the types map - make sure only one instance of metric_types
		 * exists. */
		static metric_map_T * GetMetricTypes()
		{
			if (!metric_types)
			{
				metric_types = new metric_map_T;
			}
			return metric_types;
		}
	private:
		static metric_map_T * metric_types; //!< map of metric-type names to class types
};

/** register DATAMETRIC-derived classes as named metric types. this class
 * handles type registration for metric types made available through
 * TYPEDMETRICFACTORY. inherits from the same for direct access to its map
 * of metric types.
 */
template<typename T>
struct METRICTYPEREGISTER : TYPEDMETRICFACTORY
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
 * of type METRICEVENT go into METRICMANAGER's queue and can subsequently be
 * handled by an outside entity (i.e. the owner of the METRICMANAGER).
 */
class METRICEVENT
{
	public:
		/** Default constructor */
		METRICEVENT(std::string t, std::string d) : event_type(t), event_data(d) {}
		/** Default destructor */
		~METRICEVENT() {}
		/** Copy constructor */
		METRICEVENT(METRICEVENT const& other)
		{
			*this = other;
		}
		/** Copy operator */
		METRICEVENT& operator=(METRICEVENT const& other)
		{
			event_type = other.event_type;
			event_data = other.event_data;
			return *this;
		}
		/** Access event type
		 * \return event_type
		 */
		std::string const& GetType() const { return event_type; }
		/** Access event data
		 * \return event_data
		 */
		std::string const& GetData() const { return event_data; }
	protected:
	private:
		std::string event_type;
		std::string event_data;
};

/** This class handles all the DATAMETRIC objects. It creates them, updates
 * them, and destroys them. It also manages communications between DATAMETRIC
 * objects and outside entities by providing an event queue.
 */
class METRICMANAGER
{
	public:
		/** Default constructor */
		METRICMANAGER() { }
		/** Default destructor */
		~METRICMANAGER();
		/** Copy constructor */
		METRICMANAGER(METRICMANAGER const& other)
		{
			*this = other;
		}
		/** Copy operator */
		METRICMANAGER& operator=(METRICMANAGER const& other)
		{
			data_metrics = other.data_metrics;
			events = other.events;
			return *this;
		}
		/** Create the metrics and populate the map
		 * \param filename name of file to load metric definitions from
		 * \param data_log a reference to the datalog object which holds the columns of data
		 */
		void Init(CONFIGFILE const& data_settings, DATALOG const& data_log);
		/** Update all the managed metrics
		 * \param dt the amount of seconds passed since last update
		 */
		void Update(float dt);
		/** Check for queued events and return one if there are any.
		 * \return next event in the queue, if any
		 */
		bool PollEvents(METRICEVENT & event);
	protected:
	private:
		std::map< std::string, DATAMETRIC* > data_metrics;
		std::queue< METRICEVENT > events;
};

#endif // DATAMETRIC_H
