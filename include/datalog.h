#ifndef _DATALOG_H
#define _DATALOG_H

#include <fstream>
#include <list>
#include <map>
#include <vector>
#include "boost/any.hpp"

class DATALOG
{
	private:
		// the data columns could be int, float, string, or empty, so use boost::any
		// TODO: associate columns with types and enforce type checking on columns when data is added
		std::list< std::map< std::string, boost::any > > data;
		float time;
		std::string log_directory;
		std::string log_name;
		std::vector< std::string > column_names;
		std::string file_format;

		bool AnyTypeOK(boost::any const& val);
		std::string AnyToString(boost::any const& val);

		void Write();

	public:
		DATALOG();
		DATALOG(DATALOG const& other);
		~DATALOG();

		void Init(std::string const& directory, std::string const& name, std::vector< std::string > const& columns, std::string const& format="none");

		bool HasColumn(std::string const& column_name);

		std::vector< std::string > const& GetColumns() const
		{
			return column_names;
		}

		std::map< std::string, boost::any > const& GetLastEntry() const
		{
			return data.back();
		}

		void AddEntry(float dt, std::vector< std::pair< std::string, boost::any > > const& records);
};

#endif //_CARTELEMETRY_H
