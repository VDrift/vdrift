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
/*
DATALOG::~DATALOG()
{
	Write();
}
*/
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

void DATALOG::AddEntry(std::map< std::string, double > & values)
{
	std::vector< std::string >::const_iterator column_name;

	for (column_name = column_names.begin(); column_name != column_names.end(); ++column_name)
	{
		data[*column_name].push_back(values[*column_name]);
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
	std::map< std::string, std::vector< double >::const_iterator > column_iters;
	std::string sep;

	if (!log_file)
	{
		// TODO: throw exception: couldn't open file. is directory writable?
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
			column_idx++;
			if (*column_name == "Time")
				continue;

			if (column_idx == column_names.size() - 1)
				sep = "";

			plt_file << "\\" << std::endl << "\"" << filename << "\" using 1:" << column_idx + 1 << " title '" << *column_name << "' with lines" << sep << " ";

			// this will populate a map of iterators for each column, to be used below when writing data rows
			column_iters[*column_name] = data[*column_name].begin();
		}
		plt_file << std::endl;

		// write rows of space-separated data
		bool finished = false;

		while (!finished)
		{
			sep = "";
			for (column_name = column_names.begin(); column_name != column_names.end(); ++column_name)
			{
				if (column_iters[*column_name] == data[*column_name].end())
				{
					finished = true;
					break;
				}

				log_file << sep << *(column_iters[*column_name]);

				if (sep == "")
					sep = " ";

				++column_iters[*column_name];
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

			// this will populate a map of iterators for each column, to be used below when writing data rows
			column_iters[*column_name] = data[*column_name].begin();
		}
		log_file << std::endl;

		// write the rows of comma-separated data
		bool finished = false;

		while (!finished)
		{
			sep = "";
			for (column_name = column_names.begin(); column_name != column_names.end(); ++column_name)
			{
				if (column_iters[*column_name] == data[*column_name].end())
				{
					finished = true;
					break;
				}

				log_file << sep << *(column_iters[*column_name]);

				if (sep == "")
					sep = ",";

				++column_iters[*column_name];
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
