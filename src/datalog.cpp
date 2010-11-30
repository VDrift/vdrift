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


void DATALOG::Init(std::string const& directory, std::string const& name, std::vector< std::string > const& columns, std::string const& format)
{
	log_directory = directory;
	log_name = name;
	column_names = columns;
	file_format = format;
}

bool DATALOG::HasColumn(std::string const& column_name) const
{
	vector< string >::const_iterator result;
	result = find(column_names.begin(), column_names.end(), column_name);
	return result != column_names.end();
}

bool DATALOG::GetColumn(std::string column_name, log_column_T const* column_ref) const
{
	log_map_T::const_iterator column;
	column = data.find(column_name);
	if (column != data.end())
	{
		column_ref = &column->second;
		return true;
	}
	return false;
}

void DATALOG::AddEntry(log_entry_T const* values)
{
	assert(values != NULL);
	vector< string > missing_values(column_names.size());
	vector< string >::iterator mv_iter;
	vector< string > sorted_values;

	for (log_entry_T::const_iterator value = values->begin(); value != values->end(); ++value)
	{
		sorted_values.push_back(value->first);
	}
	vector< string > sorted_column_names = column_names;
	sort(sorted_values.begin(), sorted_values.end());
	sort(sorted_column_names.begin(), sorted_column_names.end());
	mv_iter = std::set_difference(sorted_column_names.begin(), sorted_column_names.end(), sorted_values.begin(), sorted_values.end(), missing_values.begin());

	for (vector< string >::const_iterator column_name = sorted_values.begin(); column_name != sorted_values.end(); ++column_name)
	{
		log_entry_T::const_iterator value = values->find(*column_name);
		assert(value != values->end());
		data[*column_name].push_back(value->second);
	}
	for (vector< string >::const_iterator column_name = missing_values.begin(); column_name != missing_values.end(); ++column_name)
	{
		if (*column_name == "")
			break;

		// for missing columns, add NULL values (i would prefer NaN...how?) to the data stream
		data[*column_name].push_back(NULL);
	}
	cout << "Added entry" << endl;
}

void DATALOG::Write()
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
	vector< string >::const_iterator column_name;
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
		for (column_name = column_names.begin(); column_name != column_names.end(); ++column_name)
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
		for (unsigned int row_idx = 0; row_idx < data["Time"].size(); row_idx++)
		{
			sep = "";
			for (column_name = column_names.begin(); column_name != column_names.end(); ++column_name)
			{
				log_file << sep << data[*column_name][row_idx];

				if (sep == "")
					sep = " ";
			}
			log_file << std::endl;
		}
	}
	else if (file_format == "csv")
	{
		// write a comma-separated column header
		sep = "";
		for (column_name = column_names.begin(); column_name != column_names.end(); ++column_name)
		{
			log_file << sep << *column_name;

			if (sep == "")
				sep = ",";
		}
		log_file << std::endl;

		// write the rows of comma-separated data
		for (unsigned int row_idx = 0; row_idx < data["Time"].size(); row_idx++)
		{
			sep = "";
			for (column_name = column_names.begin(); column_name != column_names.end(); ++column_name)
			{
				log_file << sep << data[*column_name][row_idx];

				if (sep == "")
					sep = ",";
			}
			log_file << std::endl;
		}
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

