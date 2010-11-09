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

DATALOG::~DATALOG()
{
	Write();
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

void DATALOG::AddEntry(float dt, std::vector< std::pair< std::string, boost::any > > const& records)
{
	time += dt;

	std::vector< std::pair< std::string, boost::any > >::const_iterator i;
	std::map< std::string, boost::any > new_entry;

	new_entry["Time"] = time;

	for (i = records.begin(); i != records.end(); ++i)
	{
		if (HasColumn(i->first) && AnyTypeOK(i->second))
		{
			new_entry[i->first] = i->second;
		}
		/*
		else
		{
			// TODO: throw exception: does not have this column or bad type
		}
		*/
	}

	data.push_back(new_entry);
}

/* TODO: The next two functions are a kludge. Replace with some better fitting
 * data structure than boost::any or write an any-like class that has this
 * kind of capability built-in. Perhaps even a class derived from any could
 * meet the requirements.
 */
bool DATALOG::AnyTypeOK(boost::any const& val)
{
	if (val.empty())
		return true;
	else if (val.type() == typeid(int))
		return true;
	else if (val.type() == typeid(double))
		return true;
	else if (val.type() == typeid(float))
		return true;
	else if (val.type() == typeid(std::string))
		return true;
	else
		return false;
}

std::string DATALOG::AnyToString(boost::any const& val)
{
	std::string result;

	if (val.empty())
		result = "";
	else if (val.type() == typeid(std::string))
		result = boost::any_cast< std::string >(val);
	else if (val.type() == typeid(int))
	{
		int temp = boost::any_cast< int >(val);
		std::stringstream ss;
		ss << temp;
		result = ss.str();
	}
	else if (val.type() == typeid(double))
	{
		double temp = boost::any_cast< double >(val);
		std::stringstream ss;
		ss << temp;
		result = ss.str();
	}
	else if (val.type() == typeid(float))
	{
		float temp = boost::any_cast< float >(val);
		std::stringstream ss;
		ss << temp;
		result = ss.str();
	}
	/*
	else
	{
		// TODO: throw exception: bad type
	}
	*/

	return result;
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
	std::list< std::map< std::string, boost::any > >::iterator data_row;

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
		std::string sep = ",";
		unsigned int column_idx = -1;

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

		}
		plt_file << std::endl;

		// write the rows of data to a space-separated .dat file
		for (data_row = data.begin(); data_row != data.end(); ++data_row)
		{
			sep = "";
			for (column_name = column_names.begin(); column_name != column_names.end(); ++column_name)
			{
				log_file << sep << AnyToString((*data_row)[(*column_name)]);

				if (sep == "")
					sep = " ";
			}
			log_file << std::endl;
		}
	}
	else if (file_format == "csv")
	{
		// write a comma-separated column header
		std::string sep = "";
		for (column_name = column_names.begin(); column_name != column_names.end(); ++column_name)
		{
			log_file << sep << *column_name;

			if (sep == "")
				sep = ",";
		}
		log_file << std::endl;

		// write the rows of comma-separated data
		for (data_row = data.begin(); data_row != data.end(); ++data_row)
		{
			sep = "";
			for (column_name = column_names.begin(); column_name != column_names.end(); ++column_name)
			{
				log_file << sep << AnyToString((*data_row)[(*column_name)]);

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
