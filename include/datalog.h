#ifndef _DATALOG_H
#define _DATALOG_H

#include <map>
#include <vector>

/** Log data in memory and then write it to a file. This class helps gather
 * data from different modules and put it all in one place, so that it can
 * be used as input for other functionality, such as DATAMETRICs. The output
 * file can be written in different formats; currently csv and gnuplot are
 * supported.
 *
 * TODO: Replace double as log_data_T with a new class that can flexibly
 * handle double, string, long, bool, and supports NaN values well.
 */
class DATALOG
{
	public:
		typedef double log_data_T;
		typedef std::map< std::string, log_data_T > log_entry_T;
		typedef std::vector< log_data_T > log_column_T;
		typedef std::map< std::string, log_column_T * > log_map_T;

		/** ctor: default values */
		DATALOG() : log_directory("."), log_name("uninitialized_datalog"), file_format("none") {}
		/** dtor: clean up columns of data after write */
		~DATALOG();
		/** Setup the datalog, storage information and column names
		 * \param directory the path to write the data log file to
		 * \param name the log name (used as file basename)
		 * \param columns the names of the columns in the data log
		 * \param format the kind of output file to write (none, csv, gnuplot)
		 */
		void Init(std::vector< std::string > const& columns, std::string const& directory=".", std::string const& name="unnamed", std::string const& format="none");
		/** Write the data log to its output file */
		void Write() const;
		/** Add a new entry to the log
		 * \param values a map of values indexed by column name
		 */
		void AddEntry(log_entry_T const* values);
		/** Modify the last entry made in the log. This provides a way to put
		 * values calculated from the current entry of log data back into the
		 * log.
		 * \param column_name the name of the column to modify
		 * \param value the new value to put in the column
		 */
		void ModifyLastEntry(std::string const& column_name, log_data_T value) { AppendToColumn(column_name, value); }
		/** Check to see if a named column is in the log
		 * \param column_name the name of the column to check for
		 * \return true if the named column is in the log, false otherwise
		 */
		bool HasColumn(std::string const& column_name) const;
		/** Access all the names of the columns in the log. */
		std::vector< std::string > const& GetColumnNames() const { return column_names; }
		/** Get a pointer to a named column
		 * \param column_name the column to get
		 * \return the pointer to the column
		 */
		log_column_T const* GetColumn(std::string const& column_name) const;
	private:
		/** Add a new value to the end of the named column.
		 * \param column_name the name fo the column to append to
		 * \param value the data to append to the named column
		 */
		void AppendToColumn(std::string const& column_name, log_data_T value);
		/** Write rows of data to a file stream separated by a delimiter
		 * \param os the output file stream
		 * \param delim the delimiter to separate fields with
		 */
		void WriteDelimitedRows(std::ofstream & os, std::string const& delim) const;

		log_map_T data; //!< the map of columns that stores the logged data
		std::string log_directory; //!< the directory to write the log file to
		std::string log_name; //!< the name of the log
		std::string file_format; //!< the kind of log file to write
		std::vector< std::string > column_names; //!< the names of each column, in order
		std::vector< std::string > sorted_column_names; //!< same as column_names but sorted, used to speed up AddEntry
};

#endif //_DATALOG_H
