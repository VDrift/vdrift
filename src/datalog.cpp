#include "datalog.h"
#include <algorithm>
#include <sstream>
#include <iostream>

DATALOG::DATALOG() :
	time(0.0),
	log_directory("."),
	log_name("uninitialized_datalog"),
	file_format("none")
{

}

DATALOG::DATALOG(DATALOG const& other) :
	data(other.data),
	time(other.time),
	log_directory(other.log_directory),
	log_name(other.log_name),
	column_names(other.column_names),
	file_format(other.file_format)
{

}

void DATALOG::Init(std::string const& directory, std::string const& name, std::vector< std::string > const& columns, std::string const& format)
{
	log_directory = directory;
	log_name = name;
	column_names = columns;
	file_format = format;

}

bool DATALOG::HasColumn(std::string const& column_name)
{
	std::vector< std::string >::const_iterator result;
	result = std::find(column_names.begin(), column_names.end(), column_name);
	return result != column_names.end();
}

std::vector< double > const& DATALOG::GetColumn(std::string const& column_name)
{
	//if (HasColumn(column_name))
	//{
		return data[column_name];
	//}
	/*else
	{
		// TODO: throw exception: no such column
	}
	*/
}

void DATALOG::AddEntry(std::map< std::string, double > & values)
{
	std::vector< std::string >::const_iterator column_name;
	std::vector< std::string > missing_values(column_names.size());
	std::vector< std::string >::iterator mv_iter;
	std::vector< std::string > sorted_values;
	std::map< std::string, double >::const_iterator value_iter;
	for (value_iter = values.begin(); value_iter != values.end(); ++value_iter)
	{
		//std::cout << "value named " << value_iter->first << " = " << value_iter->second << std::endl;
		sorted_values.push_back(value_iter->first);
	}
	std::vector< std::string > sorted_column_names = column_names;
	std::sort(sorted_values.begin(), sorted_values.end());
	/*std::vector< std::string >::const_iterator sv_iter;
	for (sv_iter = sorted_values.begin(); sv_iter != sorted_values.end(); ++sv_iter)
	{
		std::cout << "sorted value " << *sv_iter << std::endl;
	}*/
	std::sort(sorted_column_names.begin(), sorted_column_names.end());
	/*std::vector< std::string >::const_iterator scn_iter;
	for (scn_iter = sorted_column_names.begin(); scn_iter != sorted_column_names.end(); ++scn_iter)
	{
		std::cout << "sorted column named " << *scn_iter << std::endl;
	}*/
	mv_iter = std::set_difference(sorted_column_names.begin(), sorted_column_names.end(), sorted_values.begin(), sorted_values.end(), missing_values.begin());

	for (column_name = sorted_values.begin(); column_name != sorted_values.end(); ++column_name)
	{
		//std::cout << "Adding value " << *column_name << " = " << values[*column_name] << std::endl;
		data[*column_name].push_back(values[*column_name]);
		//std::cout << "Added value " << *column_name << " = " << data[*column_name].back() << std::endl;
	}
	for (column_name = missing_values.begin(); column_name != missing_values.end(); ++column_name)
	{
		// if the column name is empty, we have run out of missing values, bail out
		if (*column_name == "")
			break;

		// for missing columns, add NULL values (i would prefer NaN...how?) to the data stream
		data[*column_name].push_back(NULL);
	}
}

void DATALOG::Write()
{
	if (file_format == "none")
		return;

	std::string file_extension = file_format;
	if (file_format == "gnuplot")
		file_extension = "dat";
	else if (file_format == "XML")
		file_extension = "xml";

	std::string filename(log_directory + "/" + log_name + "." + file_extension);
	std::ofstream log_file(filename.c_str());
	std::vector< std::string >::const_iterator column_name;
	std::vector< double >::const_iterator data_point;
	std::string sep;

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
		std::string log_filename = log_directory + "/" + log_name + ".plt";
		std::ofstream plt_file(log_filename.c_str());

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
