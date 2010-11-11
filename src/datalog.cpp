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

std::vector< double > const& DATALOG::GetColumn(std::string const& column_name) const
{
	if (HasColumn(column_name))
	{
		return data[column_name];
	}
	/*else
	{
		// TODO: throw exception: no such column
	}
	*/
}

void DATALOG::AddEntry(std::map< std::string, double > & values)
{
	std::vector< std::string >::const_iterator column_name;

	for (column_name = column_names.begin(); column_name != column_names.end(); ++column_name)
	{
		//std::cout << "Adding value: " << values[*column_name] << " to column: " << *column_name << std::endl;
		data[*column_name].push_back(values[*column_name]);
		//std::cout << "Added value: " << data[*column_name].back() << " to column: " << *column_name << std::endl;
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
	//std::map< std::string, std::vector< double >::const_iterator > column_iters;
	std::vector< double >::const_iterator data_point;
	std::string sep;

	if (!log_file)
	{
		// TODO: throw exception: couldn't open file. is directory writable?
		return;
	}

	if (data.size() == 0)
	{
		// no columns, bail out.
		return;
	}

	if (data.find("Time") == data.end())
	{
		// code below depends on existence of the time column. if it's missing, bail out.
		return;
	}

	if (file_format == "gnuplot")
	{
		std::ofstream plt_file((log_directory + "/" + log_name + ".plt").c_str());

		if (!plt_file)
		{
			// TODO: throw exception: couldn't open file. is directory writable?
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
	}
	else if (file_format != "none")
	{
		// TODO: throw exception: unrecognized output file format
	}
}
