#include <algorithm>
#include <sstream>
#include <iostream>
#include <fstream>
#include <assert.h>

#include "datalog.h"

using std::vector;
using std::string;
using std::ofstream;
using std::sort;

using std::cout;
using std::endl;

DATALOG::~DATALOG()
{
	for (log_map_T::iterator column = data.begin(); column != data.end(); ++column)
	{
		assert(column->second != NULL);
		delete column->second;
		column->second = NULL;
	}
}

void DATALOG::Init(std::vector< std::string > const& columns, std::string const& directory, std::string const& name, std::string const& format)
{
	//cout << "Initializing datalog" << endl;
	log_directory = directory;
	log_name = name;
	file_format = format;
	column_names = columns;

	//cout << "Sorting column names" << endl;
	// store column names sorted to improve AddEntry performance
	sorted_column_names = column_names;
	sort(sorted_column_names.begin(), sorted_column_names.end());

	//cout << "Creating columns" << endl;
	// initialize the map with the column names and create the columns
	for (vector<string>::const_iterator column_name = column_names.begin(); column_name != column_names.end(); ++column_name)
	{
		//cout << "creating new log column '" << *column_name << "'" << endl;
		data[*column_name] = new log_column_T;
		//cout << "created new column '" << *column_name << "' at " << data[*column_name] << endl;
	}
}

bool DATALOG::HasColumn(std::string const& column_name) const
{
	vector< string >::const_iterator result;
	result = find(column_names.begin(), column_names.end(), column_name);
	return result != column_names.end();
}

DATALOG::log_column_T const* DATALOG::GetColumn(std::string const& column_name) const
{
	log_map_T::const_iterator column;
	column = data.find(column_name);
	assert(column != data.end());
	return column->second;
}

void DATALOG::AppendToColumn(std::string const& column_name, log_data_T value)
{
	log_map_T::iterator column;
	column = data.find(column_name);
	assert(column != data.end());
	column->second->push_back(value);
}

void DATALOG::AddEntry(log_entry_T const* values)
{
	assert(values != NULL);

	// append the values to respective columns, storing the column names encountered
	vector< string > input_colnames;
	for (log_entry_T::const_iterator value = values->begin(); value != values->end(); ++value)
	{
		AppendToColumn(value->first, value->second);
		input_colnames.push_back(value->first);
	}

	// get the difference between the data log's column names and the input
	// column names, in missing_values, padded to column_names.size() with ""s
	sort(input_colnames.begin(), input_colnames.end());
	vector< string > missing_values(column_names.size());
	std::set_difference(sorted_column_names.begin(), sorted_column_names.end(), input_colnames.begin(), input_colnames.end(), missing_values.begin());

	// for missing columns, add an empty value to the column, until padding
	for (vector< string >::const_iterator column_name = missing_values.begin(); (column_name != missing_values.end()) && (*column_name != ""); ++column_name)
	{
		AppendToColumn(*column_name, NULL);
	}

	//cout << "Added entry" << endl;
}

void DATALOG::WriteDelimitedRows(std::ofstream & os, std::string const& delim) const
{
	string sep;
	log_column_T const* time_col = GetColumn("Time");
	for (unsigned int row_idx = 0; row_idx < time_col->size(); row_idx++)
	{
		sep = "";
		for (vector< string >::const_iterator column_name = column_names.begin(); column_name != column_names.end(); ++column_name)
		{
			log_column_T const* column = GetColumn(*column_name);
			os << sep << column->at(row_idx);

			if (sep == "")
				sep = delim;
		}
		os << std::endl;
	}
	os << std::endl;
}


void DATALOG::Write() const
{
	if (file_format == "none")
		return;

	string file_extension = file_format;
	if (file_format == "gnuplot")
		file_extension = "dat";
	else if (file_format == "XML")
		file_extension = "xml";

	string filename(log_directory + "/" + log_name + "." + file_extension);
	ofstream log_file(filename.c_str());
	log_column_T::const_iterator data_point;
	string sep;

	if (!log_file)
	{
		// TODO: throw exception: couldn't open file. is directory writable?
		std::cout << "Couldn't open log file " << filename << " for writing." << std::endl;
		return;
	}

	if (data.size() == 0)
	{
		// no columns, bail out.
		std::cout << "No data!" << std::endl;
		return;
	}

	if (data.find("Time") == data.end())
	{
		// code below depends on existence of the time column. if it's missing, bail out.
		std::cout << "Couldn't find the Time column" << std::endl;
		return;
	}

	if (file_format == "gnuplot")
	{
		string log_filename = log_directory + "/" + log_name + ".plt";
		ofstream plt_file(log_filename.c_str());

		if (!plt_file)
		{
			// TODO: throw exception: couldn't open file. is directory writable?
			std::cout << "Couldn't open plt file " << log_filename << " for writing" << std::endl;
			return;
		}

		// put "header" information in a .plt file containing gnuplot commands
		sep = ",";
		unsigned int column_idx = 0;

		plt_file << "set term png size 1280, 960" << std::endl;
		plt_file << "set output \"" << log_directory << "/" << log_name << ".png\"" << std::endl;
		plt_file << "plot ";
		for (vector< string >::const_iterator column_name = column_names.begin(); column_name != column_names.end(); ++column_name)
		{
			// skip the time column - that is the y-axis, so no need to plot it.
			if (*column_name == "Time")
				continue;

			column_idx++;

			if (column_idx == column_names.size() - 1)
				sep = "";

			plt_file << "\\" << std::endl << "\"" << filename << "\" using 1:" << column_idx + 1 << " title '" << *column_name << "' with lines" << sep << " ";
		}
		plt_file << std::endl;

		// close the plot file
		plt_file.close();

		// write rows of space-separated data
		WriteDelimitedRows(log_file, " ");
	}
	else if (file_format == "csv")
	{
		// write a comma-separated column header
		sep = "";
		for (vector< string >::const_iterator column_name = column_names.begin(); column_name != column_names.end(); ++column_name)
		{
			log_file << sep << *column_name;

			if (sep == "")
				sep = ",";
		}
		log_file << std::endl;

		// write the rows of comma-separated data
		WriteDelimitedRows(log_file, ",");
	}
	else if (file_format == "XML")
	{
		// TODO
		// write XML doctype, root node, etc.
		// write each row of data in a row element containing elements named for column names
		std::cout << "XML writer is unfinished, no data written to log file" << std::endl;
	}
	else if (file_format != "none")
	{
		// TODO: throw exception: unrecognized output file format
		std::cout << "Unrecognized file format" << std::endl;
		return;
	}

	// close the log file
	log_file.close();
}

