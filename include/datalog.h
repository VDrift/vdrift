#ifndef _DATALOG_H
#define _DATALOG_H

#include <fstream>
#include <map>
#include <vector>

class DATALOG
{
	private:
		std::map< std::string, std::vector< double > > data;
		float time;
		std::string log_directory;
		std::string log_name;
		std::vector< std::string > column_names;
		std::string file_format;

	public:
		DATALOG();
		DATALOG(DATALOG const& other);
		//~DATALOG();

		void Init(std::string const& directory, std::string const& name, std::vector< std::string > const& columns, std::string const& format="none");

		void Write();

		void AddEntry(std::map< std::string, double > & values);

		bool HasColumn(std::string const& column_name);

		std::vector< std::string > const& GetColumnNames() const
		{
			return column_names;
		}
/*
		std::map< std::string, boost::any > const& GetLastEntry() const
		{
			return data.back();
		}*/
};

#endif //_CARTELEMETRY_H
