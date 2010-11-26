#ifndef DATAMETRIC_H
#define DATAMETRIC_H

#include <string>
#include <vector>
#include <map>


class DATAMETRIC
{
	public:
		/** Default constructor */
		DATAMETRIC() : run(false) {}
		/** Default destructor */
		~DATAMETRIC() {}
		/** copy constructor */
		DATAMETRIC(DATAMETRIC const& rhs)
		 : input_data_columns(rhs.input_data_columns), output_variable_names(rhs.output_variable_names), output_data(rhs.output_data)
		{ }
		/** copy operator */
		DATAMETRIC& operator=(DATAMETRIC const& rhs)
		{
			input_data_columns = rhs.input_data_columns;
			output_variable_names = rhs.output_variable_names;
			output_data = rhs.output_data;
			return *this;
		}
		/** Access names of keys for output_data
		 * \return a vector of the key values naming the output data variables
		 * invariant:
		 */
		std::vector< std::string > const& GetOutputVariableNames() const { return output_variable_names; };
		/** Access output_data by key
		 * \param var_name Key name of the variable to retrieve
		 * \return the current value of output_data for the given var_name
		 */
		double const& GetOutputVariable(std::string const& var_name) { return output_data[var_name]; }
		/** Update the metric's calculations
		 * derived classes should override this
		 */
		virtual void Update(float dt);
	protected:
		bool run; //!< whether to perform calculations or not
		std::map< std::string, std::vector< double > const* > input_data_columns; //!< pointers to input data columns
		std::vector< std::string > output_variable_names; //!< names of the variables in the output data
		std::map< std::string, double > output_data; //!< output values (output_variable_names contains keys)
	private:
};

#endif // DATAMETRIC_H
