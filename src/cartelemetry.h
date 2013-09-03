/************************************************************************/
/*                                                                      */
/* This file is part of VDrift.                                         */
/*                                                                      */
/* VDrift is free software: you can redistribute it and/or modify       */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation, either version 3 of the License, or    */
/* (at your option) any later version.                                  */
/*                                                                      */
/* VDrift is distributed in the hope that it will be useful,            */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */
/*                                                                      */
/* You should have received a copy of the GNU General Public License    */
/* along with VDrift.  If not, see <http://www.gnu.org/licenses/>.      */
/*                                                                      */
/************************************************************************/

#ifndef _CARTELEMETRY_H
#define _CARTELEMETRY_H

#include <fstream>

class CarTelemetry
{
	private:
		typedef double T;

		std::vector <std::pair <std::string, T> > variable_names;
		T time;
		bool wroteheader;
		const std::string telemetryname;
		std::ofstream file;

		void WriteHeader(const std::string & filename)
		{
			std::ofstream f((filename+".plt").c_str());
			if (f)
			{
				f << "plot ";
				unsigned int count = 0;
				for (std::vector <std::pair <std::string, T> >::iterator i =
					variable_names.begin(); i != variable_names.end(); ++i)
				{
					f << "\\" << std::endl << "\"" << filename+".dat" << "\" u 1:" << count+2 << " t '" << i->first << "' w lines";
					if (count < variable_names.size()-1)
						f << ",";
					f << " ";
					count++;
				}
				f << std::endl;
			}
			wroteheader = true;
		}

	public:
		CarTelemetry(const std::string & name) : time(0), wroteheader(false), telemetryname(name), file((name+".dat").c_str()) {}
		CarTelemetry(const CarTelemetry & other) : variable_names(other.variable_names), time(other.time), wroteheader(other.wroteheader), telemetryname(other.telemetryname), file((telemetryname+".dat").c_str()) {}

		void AddRecord(const std::string & name, T value)
		{
			bool found = false;
			for (std::vector <std::pair <std::string, T> >::iterator i =
				variable_names.begin(); i != variable_names.end(); ++i)
			{
				if (name == i->first)
				{
					i->second = value;
					found = true;
					break;
				}
			}
			if (!found)
				variable_names.push_back(std::make_pair(name, value));
		}

		void Update(T dt)
		{
			if (time != 0 && !wroteheader)
				WriteHeader(telemetryname);
			time += dt;

			if (file)
			{
				file << time << " ";
				for (std::vector <std::pair <std::string, T> >::iterator i =
					variable_names.begin(); i != variable_names.end(); ++i)
					file << i->second << " ";
				file << "\n";
			}
		}
};

#endif //_CARTELEMETRY_H
