#ifndef _CARDATALOG_H
#define _CARDATALOG_H

#include <fstream>
#include <list>
#include <map>
#include <vector>
#include "boost/any.hpp"

class CARDATALOG
{
	private:
		// the data columns could be int, double, string, or empty, so use boost::any
		// TODO: associate columns with types and enforce type checking on columns when data is added
		std::list< std::map< std::string, boost::any > > data;
		double time;
		std::string log_directory;
		std::string log_name;
		std::vector< std::string > column_names;
		std::string file_format;

		bool AnyTypeOK(boost::any const& val);
		std::string AnyToString(boost::any const& val);

		void Write();

	public:
		CARDATALOG();
		CARDATALOG(CARDATALOG const& other);
		~CARDATALOG();

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

		void AddEntry(double dt, std::vector< std::pair< std::string, boost::any > > const& records);
};

#endif //_CARTELEMETRY_H
